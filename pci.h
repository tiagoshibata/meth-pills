#pragma once

void init_pci();
void list_devices();
void run_on_pci_devices(/* callback */ char **devices);
