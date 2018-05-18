/*Transsion Top Secret*/
#include "finger.h"
#include <linux/gpio.h>
#include <linux/interrupt.h>
#include <linux/of_irq.h>
#include <linux/completion.h>
#include <linux/delay.h>

#if 1//#ifdef CONFIG_OF//CONFIG_OF=y
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#endif
#include <mt_spi.h>

//------------------------------------------------------------------------------


/**
 * this function olny use thread ex.open
 *
 */

typedef enum DTS_STATE {
    STATE_RST_CLR,
    STATE_RST_SET,
    STATE_EINT,
    STATE_POWER_CLR,
    STATE_POWER_SET,
    STATE_MAX,  /* array size */
} dts_status_t;
static struct pinctrl* this_pctrl; /* static pinctrl instance */
/* DTS state mapping name */
static const char* dts_state_name[STATE_MAX] = {
    "fp_rst_low",
    "fp_rst_high",
    "eint_as_int",
    "fp_enable_low",
    "fp_enable_high",
};

/* pinctrl implementation */
inline static long dts_set_state(const char* name)
{
    long ret = 0;
    struct pinctrl_state* pin_state = 0;
    BUG_ON(!this_pctrl);
    pin_state = pinctrl_lookup_state(this_pctrl, name);

    if (IS_ERR(pin_state)) {
        pr_err("sunwave ----finger set state '%s' failed\n", name);
        ret = PTR_ERR(pin_state);
        return ret;
    }

    //sw_dbg("sunwave ---- dts_set_state = %s \n", name);
    /* select state! */
    ret = pinctrl_select_state(this_pctrl, pin_state);

    if ( ret < 0) {
        sw_err("sunwave --------pinctrl_select_state %s failed\n", name);
    }

    return ret;
}


inline static long dts_select_state(dts_status_t s)
{
    BUG_ON(!((unsigned int)(s) < (unsigned int)(STATE_MAX)));
    return dts_set_state(dts_state_name[s]);
}

int mt_dts_reset(struct spi_device**  spi)
{
    dts_select_state(STATE_RST_SET);
    msleep(10);
    dts_select_state(STATE_RST_CLR);
    msleep(20);
    dts_select_state(STATE_RST_SET);
    return 0;
}

int mt_dts_power(struct spi_device**  spi)
{
    dts_select_state(STATE_POWER_CLR);
    msleep(20);
    dts_select_state(STATE_POWER_SET);
    return 0;
}


static int  mt_dts_gpio_init(struct spi_device**   spidev)
{
    sunwave_sensor_t*  sunwave = spi_to_sunwave(spidev);
   // struct device_node* finger_irq_node = NULL;
    struct spi_device*   spi = *spidev;
	u32 ints[2] = {0};
    /* set power pin to high */
#if 1//#ifdef CONFIG_OF//CONFIG_OF=y
    spi->dev.of_node = of_find_compatible_node(NULL, NULL, "mediatek,fingerprint");
    this_pctrl = devm_pinctrl_get(&spi->dev);

    if (IS_ERR(this_pctrl)) {
        //int ret = PTR_ERR(this_pctrl);
        dev_err(&spi->dev, "fwq Cannot find fp pctrl!\n");
        return -ENODEV;
    }

	dts_select_state(STATE_EINT);
	of_property_read_u32_array(spi->dev.of_node, "debounce", ints, ARRAY_SIZE(ints));
	gpio_set_debounce(ints[0], ints[1]);
	sunwave->standby_irq = irq_of_parse_and_map(spi->dev.of_node, 0);
	sw_info("irq_num=%d",sunwave->standby_irq);

#endif
    return 0;
}


static int  mt_dts_irq_hander(struct spi_device** spi)
{
    spi_to_sunwave(spi);
    return 0;
}


static int mt_dts_speed(struct spi_device**    spi, unsigned int speed)
{
#define SPI_MODULE_CLOCK   100000000//    134300
    struct mt_chip_conf* config;
    unsigned int    time = SPI_MODULE_CLOCK / speed;
    config = (struct mt_chip_conf*)(*spi)->controller_data;
    config->low_time = time / 2;
    config->high_time = time / 2;

    if (time % 2) {
        config->high_time = config->high_time + 1;
    }

    //(chip_config->low_time + chip_config->high_time);
    return 0;
}

static finger_t  finger_sensor = {
    .ver                    	= 0,
    .attribute              = DEVICE_ATTRIBUTE_NONE,//DEVICE_ATTRIBUTE_SPI_FIFO,
    .write_then_read	= 0,
    .irq_hander		= mt_dts_irq_hander,
    .irq_request		= 0,
    .irq_free		= 0,
    .init                   	= mt_dts_gpio_init,
    .reset                  	= mt_dts_reset,
    .power			= mt_dts_power,
    .speed          		= mt_dts_speed,
};


void mt6797_dts_register_finger(sunwave_sensor_t*    sunwave)
{
    sunwave->finger = &finger_sensor;
}
EXPORT_SYMBOL_GPL(mt6797_dts_register_finger);







