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

typedef struct {
    uint16_t id;
    const char *name;
} pci_device_id;

static const pci_device_id safe_devices[] = {
    {0x1b80, "GeForce GTX 1080"}
};

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

static int validate_vendor(int fd) {
    char buffer[7];
    ssize_t bytes_read = read_at(fd, "vendor", buffer, 6);
    return bytes_read > 0 && !strncmp(buffer, "0x10de", 6);
}

static int validate_class(int fd) {
    char buffer[9];
    ssize_t bytes_read = read_at(fd, "class", buffer, 8);
    return bytes_read > 0 && (strtoul(buffer, NULL, 16) & 0xff0000) == 0x30000;
}

static uint16_t get_device_id(int fd) {
    char buffer[7];
    ssize_t bytes_read = read_at(fd, "device", buffer, 6);
    if (bytes_read == -1) {
        perror("read");
        return 0;
    }
    return strtoul(buffer, NULL, 16);
}

static const char *get_supported_device_name(uint16_t device_id) {
    // TODO
    if (device_id == safe_devices[0].id)
        return safe_devices[0].name;
    return NULL;
}

struct linux_dirent64 {
    ino64_t        d_ino;
    off64_t        d_off;
    unsigned short d_reclen;
    unsigned char  d_type;
    char           d_name[];
};

void run_on_pci_nvidia_gpus(void(*callback)(int), int safe_only) {
    char buffer[1024];
    if (lseek(pci_devices_fd, 0, SEEK_SET) == (off_t)-1)
        perror("lseek");
    for (;;) {
        ssize_t size = getdents64(pci_devices_fd, buffer, sizeof(buffer));
        if (size == -1) {
            perror("getdents64");
            return;;
        }
        if (!size)
            break;
        for (ssize_t pos = 0; pos < size; ) {
            struct linux_dirent64 *dir = (struct linux_dirent64 *) (buffer + pos);
            if (*dir->d_name != '.') {
                int fd = openat(pci_devices_fd, dir->d_name, O_RDONLY);
                if (validate_vendor(fd) && validate_class(fd)) {
                    printf("Device %s: ", dir->d_name);
                    uint16_t device_id = get_device_id(fd);
                    if (device_id < 0x1b80) {
                        puts("Unsupported; only Pascal and newer GPUs are supported");
                    } else {
                        const char *name = get_supported_device_name(device_id);
                        if (name)
                            printf("%s\n", name);
                        else
                            printf("not officially supported and is being skipped by default; to run on it anyways, pass %s as a command line parameter. If pills work for you, report your success at https://github.com/tiagoshibata/meth-pills/issues\n", dir->d_name);
                        if (!safe_only || name)
                            callback(fd);
                    }
                }
            }
            pos += dir->d_reclen;
        }
    }
}

int open_pci_device(const char *device) {
    int fd = openat(pci_devices_fd, device, O_RDONLY);
    if (fd == -1) {
        perror("openat");
        return -1;
    }
    if (!validate_vendor(fd)) {
        puts("Vendor mismatch");
        goto fail;
    }

    if (!validate_class(fd)) {
        puts("Device isn't a display controller");
        goto fail;
    }

    return fd;

fail:
    if (close(fd))
        perror("close");
    return -1;
}
