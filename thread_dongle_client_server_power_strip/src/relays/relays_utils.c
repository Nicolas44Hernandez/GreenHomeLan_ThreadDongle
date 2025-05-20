#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/device.h>
#include <zephyr/sys/printk.h>
#include <thread_dongle_interface.h>


// Variable for storing relays status ressources */
struct relays_ressources {
     bool r1_status;
     bool r2_status;
     bool r3_status;
     bool r4_status;
};

static struct relays_ressources relays_ressources = {
    .r1_status = NULL,
    .r2_status = NULL,
    .r3_status = NULL,
    .r4_status = NULL,
};

// Setup for L1 and L2
static const struct gpio_dt_spec relay_1 = GPIO_DT_SPEC_GET(DT_ALIAS(relay1), gpios);
static const struct gpio_dt_spec relay_2 = GPIO_DT_SPEC_GET(DT_ALIAS(relay2), gpios);
static const struct gpio_dt_spec relay_3 = GPIO_DT_SPEC_GET(DT_ALIAS(relay3), gpios);
static const struct gpio_dt_spec relay_4 = GPIO_DT_SPEC_GET(DT_ALIAS(relay4), gpios);


void set_relays_status(bool r1_status, bool r2_status, bool r3_status, bool r4_status){
    /*Set relays status*/

    gpio_pin_set_dt(&relay_1, r1_status? 1:0);
    relays_ressources.r1_status = r1_status;
    gpio_pin_set_dt(&relay_2, r2_status? 1:0);
    relays_ressources.r2_status = r2_status;
    gpio_pin_set_dt(&relay_3, r3_status? 1:0);
    relays_ressources.r3_status = r3_status;
    gpio_pin_set_dt(&relay_4, r4_status? 1:0);
    relays_ressources.r4_status = r4_status;
    printk("RELAYS [DEBBUG]: Setting relays statuses  R1:%d  R2:%d  R3:%d  R4:%d \r\n",relays_ressources.r1_status, relays_ressources.r2_status, relays_ressources.r3_status, relays_ressources.r4_status);
}

const char *get_relays_status_string(void)
{
    static char status_str[5]; // 4 relays + null terminator

    status_str[0] = relays_ressources.r1_status ? '1' : '0';
    status_str[1] = relays_ressources.r2_status ? '1' : '0';
    status_str[2] = relays_ressources.r3_status ? '1' : '0';
    status_str[3] = relays_ressources.r4_status ? '1' : '0';
    status_str[4] = '\0';

    return status_str;
}

int init_relays(void)
{
    int ret1;
    int ret2;
    int ret3;
    int ret4;
    
    //Check if R1, R2, R3 and R4 are ready
    if ((!device_is_ready(relay_1.port)) && (!device_is_ready(relay_2.port)) && (!device_is_ready(relay_3.port)) && (!device_is_ready(relay_4.port))){
        printk("GPIO [ERROR]: Cannot configure devices\r\n");
        return 0;
    }

    //Configure R1, R2, R3 and R4 as outputs
    ret1 = gpio_pin_configure_dt(&relay_1, GPIO_OUTPUT_ACTIVE);
    ret2 = gpio_pin_configure_dt(&relay_2, GPIO_OUTPUT_ACTIVE);
    ret3 = gpio_pin_configure_dt(&relay_3, GPIO_OUTPUT_ACTIVE);
    ret4 = gpio_pin_configure_dt(&relay_4, GPIO_OUTPUT_ACTIVE);
    if ((ret1 < 0) && (ret2 < 0) && (ret3 < 0) && (ret4 < 0)) {
        printk("GPIO [ERROR]: Cannot configure pins\r\n");
        return 1;
    } 

    //Set R1(on), R2(on), R3(on) and R4(on) initial values
    set_relays_status(true, true, true, true);

    return 0;
}