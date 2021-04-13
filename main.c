#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include "pci.h"

_Noreturn static void help() {
    puts("Usage: meth-pills [-h] [-l] [-s] [-d device] [pci_device[=t0,t1] ...]\n"
         "  -h    show this help\n"
         "  -l    list devices and their current timings, then quit\n"
         "  -s    use \"safer\"/slower timings. Use if your GPU isn't stable with the default timings\n"
         "  pci_device=t0,t1    if no devices are given, run on all safe devices using optimized timings. If devices are given, run on specified devices only. You can also set custom timings for specified devices. Devices must be identified by their full PCI path, as given in /sys/bus/pci/devices (domain:bus:slot:function, e.g. 0000:08:00.0)\n\n"
         "Examples:\n"
         "  $ meth-pills -l\nList devices and timings of safe devices\n\n"
         "  $ meth-pills 0000:08:00.0=15,4\nRun only on 0000:08:00.0 with timings of 15 and 4\n\n"
         "To help identify devices and their addresses, run \"lspci\" or \"lspci -v\". lspci has a database of devices and manufacturers and can help you tell your cards apart\n"
         "If meth-pills works on your card, please report it at https://github.com/tiagoshibata/meth-pills/issues so it can be added to the safe list");
    exit(0);
}

static uint32_t *map_resource0(int pci_fd) {
    int fd = openat(pci_fd, "resource0", O_RDWR);
    if (fd == -1) {
        perror("Failed to open resource0");
        return MAP_FAILED;
    }

    uint32_t *pfb_fbpa = mmap(NULL, 0x4000, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0x9A0000);
    if (pfb_fbpa == MAP_FAILED)
        perror("Failed to map device to memory. This can happen if it's locked by the kernel; boot your kernel with the iomem=relaxed parameter, which allows userspace access to devices in use by kernel drivers\nmmap");
    if (close(fd))
        perror("close");
    return pfb_fbpa;
}

#define T0_REG (0x29c / 4)
#define T1_REG (0x2a0 / 4)

static uint32_t get_t0_from_register(uint32_t value) {
    // Filter out bits 9-16
    return (value >> 9) & 0xff;
}

static uint32_t get_t1_from_register(uint32_t value) {
    // Filter out bits 15-20
    return (value >> 15) & 0x3f;
}

static uint32_t write_t0_to_register(uint32_t value, uint32_t t0) {
    return (value & 0xfffe01ff) | t0 << 9;
}

static uint32_t write_t1_to_register(uint32_t value, uint32_t t1) {
    return (value & 0xffe07fff) | t1 << 15;
}

static void list_callback(int fd) {
    volatile uint32_t *pfb_fbpa = map_resource0(fd);
    if (pfb_fbpa == MAP_FAILED)
        return;
    printf("\tt0 = %" PRIu16 ", t1 = %" PRIu16 "\n", get_t0_from_register(pfb_fbpa[T0_REG]), get_t1_from_register(pfb_fbpa[T1_REG]));
}

static void set_timings(int fd, uint32_t t0, uint32_t t1) {
    volatile uint32_t *pfb_fbpa = map_resource0(fd);
    if (pfb_fbpa == MAP_FAILED)
        return;

    uint32_t t0_register = pfb_fbpa[T0_REG], t1_register = pfb_fbpa[T1_REG];
    uint32_t current_t0 = get_t0_from_register(t0_register), current_t1 = get_t1_from_register(t1_register);
    printf("\tt0 = %" PRIu16 " (desired: %" PRIu16 "), t1 = %" PRIu16 " (desired: %" PRIu16 ")\n", current_t0, t0, current_t1, t1);
    if (current_t0 > t0) {
        puts("Updating t0");
        pfb_fbpa[T0_REG] = write_t0_to_register(t0_register, t0);
    }
    if (current_t1 > t1) {
        puts("Updating t1");
        pfb_fbpa[T1_REG] = write_t1_to_register(t1_register, t1);
    }
}

static uint32_t desired_t0 = 16, desired_t1 = 4;

static void run_callback(int fd) {
    set_timings(fd, desired_t0, desired_t1);
}

int main(int argc, char **argv) {
    init_pci();

    int opt;
    while ((opt = getopt(argc, argv, "hls")) != -1) {
        switch (opt) {
        case 'l':
            run_on_pci_nvidia_gpus(list_callback, 0);
            return 0;
        case 's':
            // Safer timings
            desired_t0 = 20;
            desired_t1 = 5;
            break;
        case 'h':
        default:
            help();
        }
    }

    for (;;) {
        if (optind >= argc) {
            run_on_pci_nvidia_gpus(run_callback, 1);
        } else {
            for (char **device = argv + optind; *device; ++device) {
                char *timing_separator = strchr(*device, '=');
                uint32_t t0 = desired_t0, t1 = desired_t1;
                if (timing_separator) {
                    // Use custom timings specified on the command line
                    errno = 0;
                    char *end_of_t0;
                    t0 = strtoul(timing_separator + 1, &end_of_t0, 10);
                    if (*end_of_t0 != ',' || errno) {
                        fprintf(stderr, "Failed to parse %s\n", *device);
                        exit(-1);
                    }
                    char *end_of_t1;
                    t1 = strtoul(end_of_t0 + 1, &end_of_t1, 10);
                    if (*end_of_t1 || errno) {
                        fprintf(stderr, "Failed to parse %s\n", *device);
                        exit(-1);
                    }
                    *timing_separator = 0;
                }
                int fd = open_pci_device(*device);
                if (timing_separator)
                    *timing_separator = '=';
                set_timings(fd, t0, t1);
            }
        }
        sleep(5);
    }

    return 0;
}
