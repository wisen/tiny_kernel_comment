/*Transsion Top Secret*/
#include "lcm_drv.h"
#ifdef BUILD_LK
#include <platform/upmu_common.h>
#include <platform/upmu_hw.h>
#include <platform/mt_gpio.h>
#include <platform/mt_i2c.h> 
#include <platform/mt_pmic.h>
#include <string.h>
#include <cust_gpio_usage.h>
#include <cust_i2c.h>
#else
#include <linux/string.h>
#include <linux/kernel.h>
#include "mt-plat/upmu_common.h"
#include "mt-plat/mt_gpio.h"
 #include <mach/gpio_const.h>
#ifdef CONFIG_MTK_LEGACY
#include <mach/mt_pm_ldo.h>
#include <mach/mt_gpio.h>
#ifndef CONFIG_FPGA_EARLY_PORTING
#include <cust_gpio_usage.h>
#include <cust_i2c.h>
#endif
#endif
#endif

// ---------------------------------------------------------------------------
//  Define Log print
// ---------------------------------------------------------------------------
#define DEBUG

#ifdef BUILD_LK
#ifdef DEBUG
#define LCM_DEBUG(fmt, args...)  _dprintf(fmt, ##args)
#else 
#define LCM_DEBUG(fmt, args...) do {} while(0)
#endif
#define LCM_ERROR(fmt, args...)  _dprintf(fmt, ##args)
#else
#ifdef DEBUG
#define LCM_DEBUG(fmt, args...)  printk(fmt, ##args)
#else
#define LCM_DEBUG(fmt, args...) do {} while(0)
#endif
#define LCM_ERROR(fmt, args...)  printk(fmt, ##args)
#endif

// ---------------------------------------------------------------------------
//  Local Constants
// ---------------------------------------------------------------------------
#define LCM_DSI_CMD_MODE									0
#define FRAME_WIDTH  										(720)
#define FRAME_HEIGHT 										(1280)

#define REGFLAG_PORT_SWAP									0xFFFA
#define REGFLAG_DELAY             							0xFFFC
#define REGFLAG_END_OF_TABLE      							0xFFFD
#define AUXADC_LCM_VOLTAGE_CHANNEL     12

#define LCM_RESET_PIN                                       (GPIO146 | 0x80000000)
#define SET_RESET_PIN(v)                                  (mt_set_gpio_out(LCM_RESET_PIN,(v)))
#ifndef GPIO_LCD_BIAS_ENP_PIN
#define GPIO_LCD_BIAS_ENP_PIN                               (GPIO82 | 0x80000000)
#endif
#ifndef GPIO_LCD_BIAS_ENN_PIN
#define GPIO_LCD_BIAS_ENN_PIN                               (GPIO42 | 0x80000000)
#endif

/* LCM id voltage is 0V */
#define LCM_ID_MAX_VOLTAGE   1350

// ---------------------------------------------------------------------------
//  Local Variables
// ---------------------------------------------------------------------------
struct ktd2151_SETTING_TABLE {
	unsigned char cmd;
	unsigned char data;
};
static struct ktd2151_SETTING_TABLE ktd2151_cmd_data[3] = {
	{ 0x00, 0x0a },
	{ 0x01, 0x0a },
	{ 0x03, 0x30 }
};

static LCM_UTIL_FUNCS lcm_util;
extern int IMM_GetOneChannelValue(int dwChannel, int data[4], int* rawdata);
/* LCM inversion, for *#87# lcm flicker test */
extern unsigned int g_lcm_inversion;
// ---------------------------------------------------------------------------
//  Local Functions
// ---------------------------------------------------------------------------
#define dsi_set_cmdq_V2(cmd, count, ppara, force_update)	lcm_util.dsi_set_cmdq_V2(cmd, count, ppara, force_update)
#define dsi_set_cmdq(pdata, queue_size, force_update)		lcm_util.dsi_set_cmdq((unsigned int *)pdata, queue_size, force_update)
#define wrtie_cmd(cmd)										lcm_util.dsi_write_cmd(cmd)
#define write_regs(addr, pdata, byte_nums)					lcm_util.dsi_write_regs(addr, pdata, byte_nums)
#define read_reg(cmd)										lcm_util.dsi_dcs_read_lcm_reg(cmd)
#define read_reg_v2(cmd, buffer, buffer_size)   			lcm_util.dsi_dcs_read_lcm_reg_v2(cmd, buffer, buffer_size)
#define dsi_swap_port(swap)   								lcm_util.dsi_swap_port(swap)
#define MDELAY(n) 											lcm_util.mdelay(n)

#ifndef BUILD_LK
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/list.h>
#include <linux/i2c.h>
#include <linux/irq.h>
#include <linux/uaccess.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/platform_device.h>

// ---------------------------------------------------------------------------
// Define
// ---------------------------------------------------------------------------
#define I2C_I2C_LCD_BIAS_CHANNEL 1
#define DCDC_I2C_BUSNUM  I2C_I2C_LCD_BIAS_CHANNEL//for I2C channel 0
#define DCDC_I2C_ID_NAME "ktd2151"
#define DCDC_I2C_ADDR 0x3E

// ---------------------------------------------------------------------------
// GLobal Variable
// ---------------------------------------------------------------------------
#if defined(CONFIG_MTK_LEGACY)
static struct i2c_board_info __initdata ktd2151_board_info = {I2C_BOARD_INFO(DCDC_I2C_ID_NAME, DCDC_I2C_ADDR)};
#else
static const struct of_device_id lcm_of_match[] = {
	{.compatible = "mediatek,I2C_LCD_BIAS"},
	{},
};
#endif
static struct i2c_client *ktd2151_i2c_client = NULL;


// ---------------------------------------------------------------------------
// Function Prototype
// ---------------------------------------------------------------------------
static int ktd2151_probe(struct i2c_client *client, const struct i2c_device_id *id);
static int ktd2151_remove(struct i2c_client *client);

// ---------------------------------------------------------------------------
// Data Structure
// ---------------------------------------------------------------------------
struct ktd2151_dev	{
	struct i2c_client	*client;

};

static const struct i2c_device_id ktd2151_id[] = {
	{ DCDC_I2C_ID_NAME, 0 },
	{ }
};

/* DC-DC ktd2151 i2c driver */
static struct i2c_driver ktd2151_iic_driver = {
	.id_table	= ktd2151_id,
	.probe		= ktd2151_probe,
	.remove		= ktd2151_remove,
	.driver		= {
		.owner	= THIS_MODULE,
		.name	= "ktd2151",
#if !defined(CONFIG_MTK_LEGACY)
		.of_match_table = lcm_of_match,
#endif
	},
};

// ---------------------------------------------------------------------------
// Function
// ---------------------------------------------------------------------------
static int ktd2151_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	ktd2151_i2c_client  = client;
	return 0;
}


static int ktd2151_remove(struct i2c_client *client)
{
	ktd2151_i2c_client = NULL;
	i2c_unregister_device(client);
	return 0;
}


static int ktd2151_i2c_write_byte(unsigned char addr, unsigned char value)
{
	int ret = 0;
	struct i2c_client *client = ktd2151_i2c_client;
	char write_data[2]={0};

	if(client == NULL)
	{
		LCM_ERROR("ERROR!!ktd2151_i2c_client is null\n");
		return 0;
	}

	write_data[0]= addr;
	write_data[1] = value;
	ret=i2c_master_send(client, write_data, 2);
	if(ret<0)
		LCM_DEBUG("ktd2151 write data fail !!\n");
	return ret ;
}

static int __init ktd2151_iic_init(void)
{
#if defined(CONFIG_MTK_LEGACY)
	i2c_register_board_info(DCDC_I2C_BUSNUM, &ktd2151_board_info, 1);
#endif
	i2c_add_driver(&ktd2151_iic_driver);
	return 0;
}

static void __exit ktd2151_iic_exit(void)
{
	i2c_del_driver(&ktd2151_iic_driver);
}

module_init(ktd2151_iic_init);
module_exit(ktd2151_iic_exit);

MODULE_AUTHOR("Xiaokuan Shi");
MODULE_DESCRIPTION("MTK ktd2151 I2C Driver");
MODULE_LICENSE("GPL");
#else
#define ktd2151_SLAVE_ADDR_WRITE  0x7C
static struct mt_i2c_t ktd2151_i2c;

static int ktd2151_i2c_write_byte(kal_uint8 addr, kal_uint8 value)
{
	kal_uint32 ret_code = I2C_OK;
	kal_uint8 write_data[2];
	kal_uint16 len;

	write_data[0]= addr;
	write_data[1] = value;

	ktd2151_i2c.id = I2C_I2C_LCD_BIAS_CHANNEL;// I2C2;
	/* Since i2c will left shift 1 bit, we need to set FAN5405 I2C address to >>1 */
	ktd2151_i2c.addr = (ktd2151_SLAVE_ADDR_WRITE >> 1);
	ktd2151_i2c.mode = ST_MODE;
	ktd2151_i2c.speed = 100;
	len = 2;

	ret_code = i2c_write(&ktd2151_i2c, write_data, len);
	printf("%s: i2c_write: addr:0x%x, value:0x%x ret_code: %d\n", __func__, addr, value, ret_code);

	return ret_code;
}
#endif

struct LCM_setting_table {
    unsigned cmd;
    unsigned char count;
    unsigned char para_list[120];
};


static struct LCM_setting_table lcm_initialization_setting[] = {

    /*
    Note :

    Data ID will depends on the following rule.

        count of parameters > 1 => Data ID = 0x39
        count of parameters = 1 => Data ID = 0x15
        count of parameters = 0 => Data ID = 0x05

    Structure Format :

    {DCS command, count of parameters, {parameter list}}
    {REGFLAG_DELAY, milliseconds of time, {}},

    ...

    Setting ending by predefined flag

    {REGFLAG_END_OF_TABLE, 0x00, {}}
    */
	{0xFF,03,{0x98,0x81,0x03}},

	{0x01,01,{0x00}},

	{0x02,01,{0x00}},

	{0x03,01,{0x73}},

	{0x04,01,{0x00}},

	{0x05,01,{0x00}},

	{0x06,01,{0x0A}},

	{0x07,01,{0x00}},

	{0x08,01,{0x00}},

	{0x09,01,{0x01}},

	{0x0a,01,{0x00}},

	{0x0b,01,{0x00}},

	{0x0c,01,{0x01}},

	{0x0d,01,{0x00}},

	{0x0e,01,{0x00}},

	{0x0f,01,{0x1D}},

	{0x10,01,{0x1D}},

	{0x11,01,{0x00}},

	{0x12,01,{0x00}},

	{0x13,01,{0x00}},

	{0x14,01,{0x00}},

	{0x15,01,{0x00}},

	{0x16,01,{0x00}},

	{0x17,01,{0x00}},

	{0x18,01,{0x00}},

	{0x19,01,{0x00}},

	{0x1a,01,{0x00}},

	{0x1b,01,{0x00}},

	{0x1c,01,{0x00}},

	{0x1d,01,{0x00}},

	{0x1e,01,{0x40}},

	{0x1f,01,{0x80}},

	{0x20,01,{0x06}},

	{0x21,01,{0x02}},

	{0x22,01,{0x00}},

	{0x23,01,{0x00}},

	{0x24,01,{0x00}},

	{0x25,01,{0x00}},

	{0x26,01,{0x00}},

	{0x27,01,{0x00}},

	{0x28,01,{0x33}},

	{0x29,01,{0x03}},

	{0x2a,01,{0x00}},

	{0x2b,01,{0x00}},

	{0x2c,01,{0x00}},

	{0x2d,01,{0x00}},

	{0x2e,01,{0x00}},

	{0x2f,01,{0x00}},

	{0x30,01,{0x00}},

	{0x31,01,{0x00}},

	{0x32,01,{0x00}},

	{0x33,01,{0x00}},

	{0x34,01,{0x04}},

	{0x35,01,{0x00}},

	{0x36,01,{0x00}},

	{0x37,01,{0x00}},

	{0x38,01,{0x3C}},

	{0x39,01,{0x00}},

	{0x3a,01,{0x40}},

	{0x3b,01,{0x40}},

	{0x3c,01,{0x00}},

	{0x3d,01,{0x00}},

	{0x3e,01,{0x00}},

	{0x3f,01,{0x00}},

	{0x40,01,{0x00}},

	{0x41,01,{0x00}},

	{0x42,01,{0x00}},

	{0x43,01,{0x00}},

	{0x44,01,{0x00}},

	{0x50,01,{0x01}},

	{0x51,01,{0x23}},

	{0x52,01,{0x45}},

	{0x53,01,{0x67}},

	{0x54,01,{0x89}},

	{0x55,01,{0xab}},

	{0x56,01,{0x01}},

	{0x57,01,{0x23}},

	{0x58,01,{0x45}},

	{0x59,01,{0x67}},

	{0x5a,01,{0x89}},

	{0x5b,01,{0xab}},

	{0x5c,01,{0xcd}},

	{0x5d,01,{0xef}},

	{0x5e,01,{0x11}},

	{0x5f,01,{0x01}},

	{0x60,01,{0x00}},

	{0x61,01,{0x15}},

	{0x62,01,{0x14}},

	{0x63,01,{0x0E}},

	{0x64,01,{0x0F}},

	{0x65,01,{0x0C}},

	{0x66,01,{0x0D}},

	{0x67,01,{0x06}},

	{0x68,01,{0x02}},

	{0x69,01,{0x07}},

	{0x6a,01,{0x02}},

	{0x6b,01,{0x02}},

	{0x6c,01,{0x02}},

	{0x6d,01,{0x02}},

	{0x6e,01,{0x02}},

	{0x6f,01,{0x02}},

	{0x70,01,{0x02}},

	{0x71,01,{0x02}},

	{0x72,01,{0x02}},

	{0x73,01,{0x02}},

	{0x74,01,{0x02}},

	{0x75,01,{0x01}},

	{0x76,01,{0x00}},

	{0x77,01,{0x14}},

	{0x78,01,{0x15}},

	{0x79,01,{0x0E}},

	{0x7a,01,{0x0F}},

	{0x7b,01,{0x0C}},

	{0x7c,01,{0x0D}},

	{0x7d,01,{0x06}},

	{0x7e,01,{0x02}},

	{0x7f,01,{0x07}},

	{0x80,01,{0x02}},

	{0x81,01,{0x02}},

	{0x82,01,{0x02}},

	{0x83,01,{0x02}},

	{0x84,01,{0x02}},

	{0x85,01,{0x02}},

	{0x86,01,{0x02}},

	{0x87,01,{0x02}},

	{0x88,01,{0x02}},

	{0x89,01,{0x02}},

	{0x8A,01,{0x02}},

	{0xFF,03,{0x98,0x81,0x04}},

	{0x6C,01,{0x15}},

	{0x6E,01,{0x2B}},

	{0x6F,01,{0x33}},

	{0x8D,01,{0x14}},

	{0x87,01,{0xBA}},

	{0x26,01,{0x76}},

	{0xB2,01,{0xD1}},

	{0xB5,01,{0x06}},

	{0x3A,01,{0x24}},

	{0x35,01,{0x1F}},

	{0xFF,03,{0x98,0x81,0x01}},

	{0x22,01,{0x09}},

	{0x31,01,{0x00}},

	{0x40,01,{0x33}},

	{0x53,01,{0x87}},

	{0x55,01,{0xA7}},

	{0x50,01,{0xA5}},

	{0x51,01,{0xA0}},

	{0x60,01,{0x22}},

	{0x61,01,{0x00}},

	{0x62,01,{0x19}},

	{0x63,01,{0x00}},

	{0xA0,01,{0x08}},

	{0xA1,01,{0x13}},

	{0xA2,01,{0x1D}},

	{0xA3,01,{0x0F}},

	{0xA4,01,{0x11}},

	{0xA5,01,{0x22}},

	{0xA6,01,{0x18}},

	{0xA7,01,{0x1A}},

	{0xA8,01,{0x62}},

	{0xA9,01,{0x1B}},

	{0xAA,01,{0x27}},

	{0xAB,01,{0x59}},

	{0xAC,01,{0x1A}},

	{0xAD,01,{0x19}},

	{0xAE,01,{0x4D}},

	{0xAF,01,{0x21}},

	{0xB0,01,{0x28}},

	{0xB1,01,{0x4E}},

	{0xB2,01,{0x61}},

	{0xB3,01,{0x39}},

	{0xC0,01,{0x08}},

	{0xC1,01,{0x13}},

	{0xC2,01,{0x1D}},

	{0xC3,01,{0x0F}},

	{0xC4,01,{0x11}},

	{0xC5,01,{0x22}},

	{0xC6,01,{0x17}},

	{0xC7,01,{0x1A}},

	{0xC8,01,{0x62}},

	{0xC9,01,{0x1B}},

	{0xCA,01,{0x27}},

	{0xCB,01,{0x59}},

	{0xCC,01,{0x1A}},

	{0xCD,01,{0x19}},

	{0xCE,01,{0x4D}},

	{0xCF,01,{0x21}},

	{0xD0,01,{0x28}},

	{0xD1,01,{0x4D}},

	{0xD2,01,{0x61}},

	{0xD3,01,{0x39}},

	{0xFF,03,{0x98,0x81,0x00}},

	{0x11,01,{0x00}},

	{0xfffc ,120, {}},

	{0x29,01,{0x00}},

	{0x35,01,{0x00}},
};

static struct LCM_setting_table lcm_deep_sleep_mode_in_setting[] = {
	// Display off sequence
	{0x28, 0, {0x00}},
    	{REGFLAG_DELAY, 20, {}},
    	// Sleep Mode On
	{0x10, 0, {0x00}},
    	{REGFLAG_DELAY, 120, {}},
	{REGFLAG_END_OF_TABLE, 0x00, {}}
};

static void push_table(struct LCM_setting_table *table, unsigned int count, unsigned char force_update)
{
	unsigned int i;

    for(i = 0; i < count; i++) {
		
        unsigned cmd;
        cmd = table[i].cmd;
		
        switch (cmd) {
			
            case REGFLAG_DELAY :
                MDELAY(table[i].count);
                break;
				
            case REGFLAG_END_OF_TABLE :
                break;
				
            default:
				dsi_set_cmdq_V2(cmd, table[i].count, table[i].para_list, force_update);
				//MDELAY(10);//soso add or it will fail to send register
       	}
    }
	
}




// ---------------------------------------------------------------------------
//  LCM Driver Implementations
// ---------------------------------------------------------------------------

static void lcm_set_util_funcs(const LCM_UTIL_FUNCS *util)
{
	memcpy(&lcm_util, util, sizeof(LCM_UTIL_FUNCS));
}


static void lcm_get_params(LCM_PARAMS *params)
{
	memset(params, 0, sizeof(LCM_PARAMS));
    
    	params->type   = LCM_TYPE_DSI;
    
    	params->width  = FRAME_WIDTH;
    	params->height = FRAME_HEIGHT;
	/* add for *#87* flicker test */
	params->inversion = LCM_INVERSIONE_COLUMN;
	g_lcm_inversion = LCM_INVERSIONE_COLUMN;
	/* add end */
#ifndef BUILD_LK
    params->inversion  = LCM_INVERSIONE_COLUMN;
#endif
    // enable tearing-free
    params->dbi.te_mode  = LCM_DBI_TE_MODE_DISABLED;
    params->dbi.te_edge_polarity = LCM_POLARITY_RISING;
    
    params->dsi.mode   = SYNC_PULSE_VDO_MODE;//CMD_MODE;
    
    // DSI
    /* Command mode setting */
    params->dsi.LANE_NUM = LCM_FOUR_LANE;
    //The following defined the fomat for data coming from LCD engine.
    params->dsi.data_format.color_order = LCM_COLOR_ORDER_RGB;
    params->dsi.data_format.trans_seq   = LCM_DSI_TRANS_SEQ_MSB_FIRST;
    params->dsi.data_format.padding     = LCM_DSI_PADDING_ON_LSB;
    params->dsi.data_format.format      = LCM_DSI_FORMAT_RGB888;
    
    params->dsi.intermediat_buffer_num = 0;//because DSI/DPI HW design change, this parameters should be 0 when video mode in MT658X; or memory leakage
    
    params->dsi.PS=LCM_PACKED_PS_24BIT_RGB888;
    
    params->dsi.word_count=FRAME_WIDTH*3; //DSI CMD mode need set these two bellow params, different to 6577
    params->dsi.vertical_active_line=FRAME_HEIGHT;
    
    params->dsi.vertical_sync_active = 8;   //4//4
    params->dsi.vertical_backporch = 24,//10 20;
    params->dsi.vertical_frontporch = 24,//10  20; 
    params->dsi.vertical_active_line = FRAME_HEIGHT;
    
    params->dsi.horizontal_sync_active =50;
    params->dsi.horizontal_backporch = 60,//120;//50;
		params->dsi.horizontal_frontporch = 80,// 120;//50;
    params->dsi.horizontal_active_pixel = FRAME_WIDTH;
    
	params->dsi.PLL_CLOCK = 220;
	params->dsi.ssc_range = 1;
	params->dsi.ssc_disable = 0;
	params->dsi.HS_TRAIL = 15;
	
	params->dsi.esd_check_enable = 0;
	params->dsi.customization_esd_check_enable = 0;
	params->dsi.lcm_esd_check_table[0].cmd          = 0x0A;
	params->dsi.lcm_esd_check_table[0].count        = 1;
	params->dsi.lcm_esd_check_table[0].para_list[0] = 0x9C;	
}


static void lcm_init_power(void)
{

	ktd2151_i2c_write_byte(ktd2151_cmd_data[0].cmd, ktd2151_cmd_data[0].data);
	MDELAY(1);
	ktd2151_i2c_write_byte(ktd2151_cmd_data[1].cmd, ktd2151_cmd_data[1].data);
	MDELAY(1);
	ktd2151_i2c_write_byte(ktd2151_cmd_data[2].cmd, ktd2151_cmd_data[2].data);
	MDELAY(1);

	pmic_set_register_value(PMIC_RG_VEFUSE_VOSEL,3);
	MDELAY(1);
	pmic_set_register_value(PMIC_RG_VEFUSE_EN,1);
	MDELAY(1);
	mt_set_gpio_mode(GPIO_LCD_BIAS_ENP_PIN, GPIO_MODE_00);
    mt_set_gpio_dir(GPIO_LCD_BIAS_ENP_PIN, GPIO_DIR_OUT);
    mt_set_gpio_out(GPIO_LCD_BIAS_ENP_PIN, GPIO_OUT_ONE);
	MDELAY(5);
	mt_set_gpio_mode(GPIO_LCD_BIAS_ENN_PIN, GPIO_MODE_00);
	mt_set_gpio_dir(GPIO_LCD_BIAS_ENN_PIN, GPIO_DIR_OUT);
    mt_set_gpio_out(GPIO_LCD_BIAS_ENN_PIN, GPIO_OUT_ONE);
	MDELAY(5);

}

static void lcm_resume_power(void)
{

	ktd2151_i2c_write_byte(ktd2151_cmd_data[0].cmd, ktd2151_cmd_data[0].data);
	MDELAY(1);
	ktd2151_i2c_write_byte(ktd2151_cmd_data[1].cmd, ktd2151_cmd_data[1].data);
	MDELAY(1);
	ktd2151_i2c_write_byte(ktd2151_cmd_data[2].cmd, ktd2151_cmd_data[2].data);
	MDELAY(1);
	
	pmic_set_register_value(PMIC_RG_VEFUSE_EN,1);
	MDELAY(1);
	mt_set_gpio_mode(GPIO_LCD_BIAS_ENP_PIN, GPIO_MODE_00);
    mt_set_gpio_dir(GPIO_LCD_BIAS_ENP_PIN, GPIO_DIR_OUT);
    mt_set_gpio_out(GPIO_LCD_BIAS_ENP_PIN, GPIO_OUT_ONE);
	MDELAY(5);
	mt_set_gpio_mode(GPIO_LCD_BIAS_ENN_PIN, GPIO_MODE_00);
	mt_set_gpio_dir(GPIO_LCD_BIAS_ENN_PIN, GPIO_DIR_OUT);
    mt_set_gpio_out(GPIO_LCD_BIAS_ENN_PIN, GPIO_OUT_ONE);
	MDELAY(5);
	
}

static void lcm_suspend_power(void)
{
	SET_RESET_PIN(0);
	MDELAY(5);
	mt_set_gpio_out(GPIO_LCD_BIAS_ENN_PIN, GPIO_OUT_ZERO);
	MDELAY(5);
	mt_set_gpio_out(GPIO_LCD_BIAS_ENP_PIN, GPIO_OUT_ZERO);
	MDELAY(5);
	pmic_set_register_value(PMIC_RG_VEFUSE_EN, 0);
	MDELAY(5);
}


static void lcm_init(void)
{
#if defined(BUILD_LK) || defined(CONFIG_MTK_LEGACY)
	/* set lcm reset pin mode */
	mt_set_gpio_mode(GPIO_LCM_RST, GPIO_MODE_GPIO);
	mt_set_gpio_dir(GPIO_LCM_RST, GPIO_DIR_OUT);
	mt_set_gpio_pull_enable(GPIO_LCM_RST, GPIO_PULL_DISABLE);
	MDELAY(5);
#endif
	SET_RESET_PIN(1);
	MDELAY(5);
	SET_RESET_PIN(0);
	MDELAY(5);
	SET_RESET_PIN(1);
	MDELAY(120);

	/* when phone initial , config output high, enable backlight drv chip */
	push_table(lcm_initialization_setting, sizeof(lcm_initialization_setting) / sizeof(struct LCM_setting_table), 1);
}

static void lcm_suspend(void)
{
	push_table(lcm_deep_sleep_mode_in_setting, sizeof(lcm_deep_sleep_mode_in_setting) / sizeof(struct LCM_setting_table), 1);
	
    	SET_RESET_PIN(1);
		MDELAY(5);
    	SET_RESET_PIN(0);
    	MDELAY(10);//Must > 10ms
    	SET_RESET_PIN(1);
    	MDELAY(20);//Must > 120m
}

static void lcm_resume(void)
{
#if defined(BUILD_LK) || defined(CONFIG_MTK_LEGACY)
	/* set lcm reset pin mode */
	mt_set_gpio_mode(GPIO_LCM_RST, GPIO_MODE_GPIO);
	mt_set_gpio_dir(GPIO_LCM_RST, GPIO_DIR_OUT);
	mt_set_gpio_pull_enable(GPIO_LCM_RST, GPIO_PULL_DISABLE);
	MDELAY(5);
#endif
	SET_RESET_PIN(1);
	MDELAY(5);
	SET_RESET_PIN(0);
	MDELAY(5);
	SET_RESET_PIN(1);
	MDELAY(10);

	/* when phone initial , config output high, enable backlight drv chip */
	push_table(lcm_initialization_setting, sizeof(lcm_initialization_setting) / sizeof(struct LCM_setting_table), 1);
}

static unsigned int lcm_compare_id(void)
{
	int data[4] = {0,0,0,0};
	int res = 0;
	int rawdata = 0;
	int lcm_vol = 0;

#ifdef AUXADC_LCM_VOLTAGE_CHANNEL
	res = IMM_GetOneChannelValue(AUXADC_LCM_VOLTAGE_CHANNEL,data,&rawdata);
	if(res < 0)
	{
		LCM_ERROR("(%s) ili9881c_auo_fhd_dsi_vdo_tcl  get lcm chip id vol fail\n", __func__);
		return 0;
	}
#endif
	lcm_vol = data[0]*1000+data[1]*10;

	LCM_DEBUG("(%s) ili9881c_auo_fhd_dsi_vdo_tcl lcm chip id adc raw data:%d, lcm_vol:%d\n", __func__, rawdata, lcm_vol);

	if (lcm_vol >= LCM_ID_MAX_VOLTAGE)
		return 1;
	else
		return 0;

}


LCM_DRIVER ili9881c_hd720_dsi_vdo_truly_lcm_drv = 
{
	.name		= "ili9881c_hd720_dsi_vdo_truly",
	.set_util_funcs = lcm_set_util_funcs,
	.get_params     = lcm_get_params,
	.init           = lcm_init,
	.suspend        = lcm_suspend,
	.resume         = lcm_resume,
	.compare_id     = lcm_compare_id,
	.init_power	= lcm_init_power,
	.resume_power   = lcm_resume_power,
	.suspend_power  = lcm_suspend_power,

};
