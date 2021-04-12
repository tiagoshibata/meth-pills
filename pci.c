#include "pci.h"

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

static const uint16_t safe_devices[] = {0x1b80};
static int pci_devices_fd;

void init_pci() {
    pci_devices_fd = open("/sys/bus/pci/devices", O_DIRECTORY | O_RDONLY);
    if (pci_devices_fd == -1) {
        perror("Failed to open /sys/bus/pci/devices\nopen");
        exit(-1);
    }
}

static ssize_t read_at(int at_fd, const char *pathname, char *buffer, size_t count) {
    int fd = openat(at_fd, pathname, O_RDONLY);
    if (fd == -1) {
        fprintf(stderr,"Failed to open %s\nopenat: %s", pathname, strerrordesc_np(errno));
        return -1;
    }
    ssize_t bytes_read = read(fd, buffer, count);
    if (bytes_read == -1)
        perror("read");
    else
        buffer[bytes_read] = 0;
    if (close(fd))
        perror("close");
    return bytes_read;
}

static int validate_device(int fd) {
    char buffer[16];
    ssize_t bytes_read = read_at(fd, "vendor", buffer, 6);
    if (bytes_read < 6 || strncmp(buffer, "0x10de", 6)) {
        printf("Vendor mismatch\n");
        return -1;
    }

    bytes_read = read_at(fd, "class", buffer, 8);
    if (bytes_read == -1 || (strtoul(buffer, NULL, 16) & 0xff0000) != 0x30000) {
        printf("Device isn't a display controller\n");
        return -1;
    }

    // bytes_read = read_at(fd, "device", buffer, 6);
    // if (bytes_read < 6 || strncmp(buffer, "0x1b80", 6)) {
    //     printf("Unsupported device\n");
    //     return -1;
    // }
    return 0;
}

struct linux_dirent64 {
    ino64_t        d_ino;
    off64_t        d_off;
    unsigned short d_reclen;
    unsigned char  d_type;
    char           d_name[];
};

void list_devices() {
    char buffer[1024];
    for (;;) {
        ssize_t size = getdents64(pci_devices_fd, buffer, sizeof(buffer));
        if (size == -1) {
            perror("getdents64");
            exit(-1);
        }
        if (!size)
            break;
        for (ssize_t pos = 0; pos < size; ) {
            struct linux_dirent64 *dir = (struct linux_dirent64 *) (buffer + pos);
            if (*dir->d_name != '.')
                printf("Device %s\n", dir->d_name);
            pos += dir->d_reclen;
        }
    }
}

void run_on_safe_pci_devices(/* callback */) {

}

void run_on_pci_devices(/* callback */ char **devices) {
    for (; *devices; ++devices) {
        printf("Injecting on %s...\n", *devices);
        int fd = openat(pci_devices_fd, *devices, O_RDONLY);
        if (fd == -1) {
            perror("openat");
            continue;
        }
        if (validate_device(fd) != -1) {
            printf("Show\n");
        }
        if (close(fd) == -1)
            perror("close");
    }
}
