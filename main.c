#include <fcntl.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>

_Noreturn void fatal(const char *message) {
    perror(message);
    exit(-1);
}

int main(/* int argc, char **argv */) {
#if 0
    const char *resource = "/sys/bus/pci/devices/0000:01:00.0/resource0";
#else
    const char *resource = "/sys/bus/pci/devices/0000:08:00.0/resource0";
#endif
    int fd = open(resource, O_RDWR);
    if (fd == -1)
        fatal("Failed to open device");

    volatile uint32_t *pfb_fbpa = mmap(NULL, 0x4000, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0x9A0000);
    if (pfb_fbpa == MAP_FAILED)
        fatal("Failed to map device to memory. This can happen if it's locked by the kernel; to run meth-pills, boot your kernel with the iomem=relaxed parameter, which allows userspace access to devices in use by the kernel\nmmap");

#define NV_PFB_FBPA_MAGIC_1 (0x29c / 4)
#define NV_PFB_FBPA_MAGIC_2 (0x2a0 / 4)

    // for (;;) {
        struct timespec ts = {
            .tv_sec = 0,
            .tv_nsec = 200e6,
        };
        nanosleep(&ts, NULL);
        printf("NV_PFB_FBPA[0x29c] = 0x%" PRIx32 " - expected 0x2200294a before injection, 0x2200194a? after?\n", pfb_fbpa[NV_PFB_FBPA_MAGIC_1]);
        printf("NV_PFB_FBPA[0x2a0] = 0x%" PRIx32 " - expected 0x6d830032 before injection, 0xd3618032? after?\n", pfb_fbpa[NV_PFB_FBPA_MAGIC_2]);
    // }

    uint32_t new_magic_1 = (pfb_fbpa[NV_PFB_FBPA_MAGIC_1] & 0xfffe01ff) | 0x2000;  // 0x2200214a
    pfb_fbpa[NV_PFB_FBPA_MAGIC_1] = new_magic_1;

    uint32_t new_magic_2 = (pfb_fbpa[NV_PFB_FBPA_MAGIC_2] & 0xffe07fff) | 0x20000;  // 0x6d820032
    pfb_fbpa[NV_PFB_FBPA_MAGIC_2] = new_magic_2;

    printf("NV_PFB_FBPA[0x29c] = 0x%" PRIx32 " - expected 0x2200294a before injection, 0x2200194a? after?\n", pfb_fbpa[NV_PFB_FBPA_MAGIC_1]);
    printf("NV_PFB_FBPA[0x2a0] = 0x%" PRIx32 " - expected 0x6d830032 before injection, 0xd3618032? after?\n", pfb_fbpa[NV_PFB_FBPA_MAGIC_2]);

    return 0;
}
