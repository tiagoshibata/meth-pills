#include <fcntl.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>

_Noreturn void fatal(const char *message) {
    perror(message);
    exit(-1);
}

int main(/* int argc, char **argv */) {
    const char *resource = "/sys/bus/pci/devices/0000:00:01.0/0000:01:00.0/resource0";
    int fd = open(resource, O_RDWR);
    if (fd == -1)
        fatal("open");

    volatile uint32_t *pfb_fbpa = mmap(NULL, 0x4000, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0x9A0000);
    if (pfb_fbpa == NULL)
        fatal("mmap");
}
