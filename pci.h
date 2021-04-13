#pragma once

void init_pci();
void run_on_pci_nvidia_gpus(void(*callback)(int), int safe_only);
int open_pci_device(const char *device);
