#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>

int main(void)
{
    // printk is Zephyr's lightweight, interrupt-safe version of printf
    printk("Hello, Edge DevOps Pipeline!\n");
    printk("Booting on board: %s\n", CONFIG_BOARD);

    // Main control loop
    while (1) {
        printk("Motor Supervisory Controller is Idle...\n");
        k_msleep(1000); // Sleep for 1000 milliseconds (1 second)
    }
    
    return 0;
}