/**
 * Ticos stores some state across reboots. These regions must be placed in a region of RAM that
 * does not get initialized on bootup
 */
KEEP(*(*.tcs_coredump));
KEEP(*(*.tcs_reboot_info));
