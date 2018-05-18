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
#include <mach/gpio_const.h>
#include "mt-plat/mt_gpio.h"
#ifdef CONFIG_MTK_LEGACY
#include <mach/mt_pm_ldo.h>
#include <mach/mt_gpio.h>
#ifndef CONFIG_FPGA_EARLY_PORTING
#include <cust_gpio_usage.h>
#include <cust_i2c.h>
#endif
#endif
#endif
#include "mt_gpio.h" 
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
#define UDELAY(n) 											(lcm_util.udelay(n))
#define REGFLAG_PORT_SWAP									0xFFFA
//#define REGFLAG_DELAY             							0xFC
#define REGFLAG_UDELAY             							0xFB
//#define REGFLAG_END_OF_TABLE      							0xFD
#define REGFLAG_DELAY             							0xFFFC
#define REGFLAG_END_OF_TABLE      							0xFFFD
#define AUXADC_LCM_VOLTAGE_CHANNEL     12
#define LCM_ID_R61350		0x96
/* LCM id voltage is 0V */
#define LCM_ID_MAX_VOLTAGE   150

// ---------------------------------------------------------------------------
//  Local Variables
// ---------------------------------------------------------------------------
struct NT5038_SETTING_TABLE {
	unsigned char cmd;
	unsigned char data;
};
static struct NT5038_SETTING_TABLE nt5038_cmd_data[3] = {
	{ 0x00, 0x0f },
	{ 0x01, 0x0f },
	{ 0x03, 0x03 }
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
#define MDELAY(n)                          (lcm_util.mdelay(n))


#define LCM_RESET_PIN                                       (GPIO146 | 0x80000000)
#define SET_RESET_PIN(v)                                  (mt_set_gpio_out(LCM_RESET_PIN,(v)))	

#ifndef GPIO_LCD_BIAS_ENP_PIN
#define GPIO_LCD_BIAS_ENP_PIN                               (GPIO82 | 0x80000000)
#endif

#ifndef GPIO_LCD_BIAS_ENN_PIN
#define GPIO_LCD_BIAS_ENN_PIN                               (GPIO42 | 0x80000000)
#endif

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
#define DCDC_I2C_BUSNUM  I2C_I2C_LCD_BIAS_CHANNEL//for I2C channel 0
#define DCDC_I2C_ID_NAME "nt5038"
#define DCDC_I2C_ADDR 0x3E

// ---------------------------------------------------------------------------
// GLobal Variable
// ---------------------------------------------------------------------------
#if defined(CONFIG_MTK_LEGACY)
static struct i2c_board_info __initdata nt5038_board_info = {I2C_BOARD_INFO(DCDC_I2C_ID_NAME, DCDC_I2C_ADDR)};
#else
static const struct of_device_id lcm_of_match[] = {
	{.compatible = "mediatek,I2C_LCD_BIAS"},
	{},
};
#endif
static struct i2c_client *nt5038_i2c_client = NULL;


// ---------------------------------------------------------------------------
// Function Prototype
// ---------------------------------------------------------------------------
static int nt5038_probe(struct i2c_client *client, const struct i2c_device_id *id);
static int nt5038_remove(struct i2c_client *client);

// ---------------------------------------------------------------------------
// Data Structure
// ---------------------------------------------------------------------------
struct nt5038_dev	{
	struct i2c_client	*client;

};

static const struct i2c_device_id nt5038_id[] = {
	{ DCDC_I2C_ID_NAME, 0 },
	{ }
};

/* DC-DC nt5038 i2c driver */
static struct i2c_driver nt5038_iic_driver = {
	.id_table	= nt5038_id,
	.probe		= nt5038_probe,
	.remove		= nt5038_remove,
	.driver		= {
		.owner	= THIS_MODULE,
		.name	= "nt5038",
#if !defined(CONFIG_MTK_LEGACY)
		.of_match_table = lcm_of_match,
#endif
	},
};

// ---------------------------------------------------------------------------
// Function
// ---------------------------------------------------------------------------
static int nt5038_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	nt5038_i2c_client  = client;
	return 0;
}


static int nt5038_remove(struct i2c_client *client)
{
	nt5038_i2c_client = NULL;
	i2c_unregister_device(client);
	return 0;
}


static int nt5038_i2c_write_byte(unsigned char addr, unsigned char value)
{
	int ret = 0;
	struct i2c_client *client = nt5038_i2c_client;
	char write_data[2]={0};

	if(client == NULL)
	{
		LCM_ERROR("ERROR!!nt5038_i2c_client is null\n");
		return 0;
	}

	write_data[0]= addr;
	write_data[1] = value;
	ret=i2c_master_send(client, write_data, 2);
	if(ret<0)
		LCM_DEBUG("nt5038 write data fail !!\n");
	return ret ;
}

static int __init nt5038_iic_init(void)
{
#if defined(CONFIG_MTK_LEGACY)
	i2c_register_board_info(DCDC_I2C_BUSNUM, &nt5038_board_info, 1);
#endif
	i2c_add_driver(&nt5038_iic_driver);
	return 0;
}

static void __exit nt5038_iic_exit(void)
{
	i2c_del_driver(&nt5038_iic_driver);
}

module_init(nt5038_iic_init);
module_exit(nt5038_iic_exit);

MODULE_AUTHOR("Xiaokuan Shi");
MODULE_DESCRIPTION("MTK NT5038 I2C Driver");
MODULE_LICENSE("GPL");
#else
#define NT5038_SLAVE_ADDR_WRITE  0x7C
static struct mt_i2c_t NT5038_i2c;

static int nt5038_i2c_write_byte(kal_uint8 addr, kal_uint8 value)
{
	kal_uint32 ret_code = I2C_OK;
	kal_uint8 write_data[2];
	kal_uint16 len;

	write_data[0]= addr;
	write_data[1] = value;

	NT5038_i2c.id = I2C_I2C_LCD_BIAS_CHANNEL;// I2C2;
	/* Since i2c will left shift 1 bit, we need to set FAN5405 I2C address to >>1 */
	NT5038_i2c.addr = (NT5038_SLAVE_ADDR_WRITE >> 1);
	NT5038_i2c.mode = ST_MODE;
	NT5038_i2c.speed = 100;
	len = 2;

	ret_code = i2c_write(&NT5038_i2c, write_data, len);
	printf("%s: i2c_write: addr:0x%x, value:0x%x ret_code: %d\n", __func__, addr, value, ret_code);

	return ret_code;
}
#endif

struct LCM_setting_table {
    unsigned int cmd;
    unsigned char count;
    unsigned char para_list[120];
};

/*
static struct LCM_setting_table lcm_initialization_setting[] = {
	
	
	{0xB0 , 1 , {0x04}},
		
	{0xE3 , 1 , {0x01}},
	
	{0xB6 , 2 , {0x61,0x2C}},

	{0xC0,6,{0x23,0xB2,0x10,0x10,0x02,0x7F}},

	{0xC1,22,{0x0B,0x8A,0xA0,0x8A,0x00,0x00,
			0x00,0x00,0x00,0x03,0x00,0x00,
			0x00,0x00,0x00,0x00,0x00,0x00,
			0x00,0x00,0x00,0x00}},

	{0xC5,28,{0x07,0x04,0x40,0x04,0x00,0x00,
			0x03,0x01,0x80,0x07,0x00,0x00,
			0x00,0x00,0x00,0x00,0x00,0x00,
			0x00,0x00,0x00,0x00,0x00,0x00,
			0x00,0x00,0x00,0x0E}},
			
	{0xC6,2,{0x01,0x02}},
	
	{0xC8,58,{0x01,0x20,0x22,0x24,0x26,
			0x00,0x00,0x1D,0x1D,0x00,
			0x00,0x00,0x1D,0x1D,0x1D,0x00,
			0x00,0x00,0x00,0x11,0x13,0x09,
			0x02,0x21,0x23,0x25,0x27,0x00,
			0x00,0x1D,0x1D,0x00,0x00,0x00,
			0x1D,0x1D,0x1D,0x00,0x00,0x00,
			0x00,0x11,0x13,0x0A,0x89,0x00,
			0x00,0x00,0x01,0x00,0x00,0x00,
			0x02,0x00,0x00,0x00,0x03,0x00}},
			
	{0xCA,38,{0x1B,0x29,0x34,0x42,0x4E,0x59,
			0x70,0x82,0x90,0x9D,0x50,0x5C,
			0x6B,0x7F,0x89,0x97,0xA7,0xB3,
			0xBF,0x1B,0x29,0x34,0x42,0x4E,
			0x59,0x70,0x82,0x90,0x9D,0x50,
			0x5C,0x6B,0x7F,0x89,0x97,0xA7,
			0xB3,0xBF}},
			
	{0xD0,6,{0x02,0x4B,0x5F,0x00,0x2B,0x14}},
	
	{0xD1,1,{0x03}},
	
	{0xD2,2,{0x8E,0x0B}},
	
	{0xE5,1,{0x02}},
	
	{0xD4,2,{0x00,0x9E}},
	
	{0xD5,2,{0x28,0x28}},
	{0x36,1,{0x03}},	
	// exit sleep out
	{0x11,0,{}},	
	{REGFLAG_DELAY, 120, {}},
	
	//set display on
	{0x29,0,{}},
	{REGFLAG_DELAY, 20, {}},
	
};
*/

static void lcm_register_init(void)
{
unsigned int data_array[20];
data_array[0] = 0x00022902;
data_array[1] = 0x000004b0;
dsi_set_cmdq(data_array, 2, 1);

data_array[0] = 0x00022902;
data_array[1] = 0x000001e3;
dsi_set_cmdq(data_array, 2, 1);

data_array[0] = 0x00032902;
data_array[1] = 0x002c61b6;
dsi_set_cmdq(data_array, 2, 1);

data_array[0] = 0x00072902;
data_array[1] = 0x10b223c0;
data_array[2] = 0x007f0210;
dsi_set_cmdq(data_array, 3, 1);

data_array[0] = 0x00172902;
data_array[1] = 0xa08a0bc1;
data_array[2] = 0x0000008a;
data_array[3] = 0x00030000;
data_array[4] = 0x00000000;
data_array[5] = 0x00000000;
data_array[6] = 0x00000000;
dsi_set_cmdq(data_array, 7, 1);

data_array[0] = 0x001d2902;
data_array[1] = 0x400407c5;
data_array[2] = 0x03000004;
data_array[3] = 0x00078001;
data_array[4] = 0x00000000;
data_array[5] = 0x00000000;
data_array[6] = 0x00000000;
data_array[7] = 0x00000000;
data_array[8] = 0x0000000e;
dsi_set_cmdq(data_array, 9, 1);

data_array[0] = 0x00032902;
data_array[1] = 0x000201c6;
dsi_set_cmdq(data_array, 2, 1);

data_array[0] = 0x003b2902;
data_array[1] = 0x222001c8;
data_array[2] = 0x00002624;
data_array[3] = 0x00001d1d;
data_array[4] = 0x1d1d1d00;
data_array[5] = 0x00000000;
data_array[6] = 0x02091311;
data_array[7] = 0x27252321;
data_array[8] = 0x1d1d0000;
data_array[9] = 0x1d000000;
data_array[10] = 0x00001d1d;
data_array[11] = 0x13110000;
data_array[12] = 0x0000890a;
data_array[13] = 0x00000100;
data_array[14] = 0x00000200;
data_array[15] = 0x00000300;
dsi_set_cmdq(data_array, 16, 1);

data_array[0] = 0x00272902;
data_array[1] = 0x34291bca;
data_array[2] = 0x70594e42;
data_array[3] = 0x509d9082;
data_array[4] = 0x897f6b5c;
data_array[5] = 0xbfb3a797;
data_array[6] = 0x4234291b;
data_array[7] = 0x8270594e;
data_array[8] = 0x5c509d90;
data_array[9] = 0x97897f6b;
data_array[10] = 0x00bfb3a7;
dsi_set_cmdq(data_array, 11, 1);

data_array[0] = 0x00072902;
data_array[1] = 0x5f4b02d0;
data_array[2] = 0x00142b00;
dsi_set_cmdq(data_array, 3, 1);

data_array[0] = 0x00022902;
data_array[1] = 0x000003d1;
dsi_set_cmdq(data_array, 2, 1);

data_array[0] = 0x00032902;
data_array[1] = 0x000b8ed2;
dsi_set_cmdq(data_array, 2, 1);

data_array[0] = 0x00022902;
data_array[1] = 0x000002e5;
dsi_set_cmdq(data_array, 2, 1);

data_array[0] = 0x00023902;
data_array[1] = 0x00000336;
dsi_set_cmdq(data_array, 2, 1);

data_array[0] = 0x00032902;
data_array[1] = 0x002828d5;
dsi_set_cmdq(data_array, 2, 1);

data_array[0] = 0x00110500;
dsi_set_cmdq(data_array, 1, 1);

MDELAY(150);
data_array[0] = 0x00290500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(30);

}


/*
static struct LCM_setting_table lcm_deep_sleep_mode_in_setting[] = {
	// Display off sequence
	{0x28, 0, {0x00}},
    {REGFLAG_DELAY, 20, {}},
    // Sleep Mode On
	{0x10, 0, {0x00}},
    {REGFLAG_DELAY, 120, {}},
	{REGFLAG_END_OF_TABLE, 0x00, {}}
};
*/
/*
static void push_table(struct LCM_setting_table *table, unsigned int count, unsigned char force_update)
{
	unsigned int i;
    for(i = 0; i < count; i++) {
        unsigned int cmd;
        cmd = table[i].cmd;
        switch (cmd) {
            case REGFLAG_DELAY :
                MDELAY(table[i].count);
                break;
            case REGFLAG_END_OF_TABLE :
                break;
            default:
				dsi_set_cmdq_V2(cmd, table[i].count, table[i].para_list, force_update);
       	}
    }
}
*/
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
	params->lcm_if = LCM_INTERFACE_DSI0;
	params->lcm_cmd_if = LCM_INTERFACE_DSI0;

	/* add for *#88* flicker test */
	params->inversion = LCM_INVERSIONE_COLUMN;
	g_lcm_inversion = LCM_INVERSIONE_COLUMN;

#if (LCM_DSI_CMD_MODE)
	params->dsi.mode   = CMD_MODE;
#else
	params->dsi.mode   = SYNC_PULSE_VDO_MODE;
#endif

	/* Command mode setting */
	params->dsi.LANE_NUM				= LCM_FOUR_LANE;

	/* The following defined the fomat for data coming from LCD engine. */
	params->dsi.data_format.color_order 	= LCM_COLOR_ORDER_RGB;
	params->dsi.data_format.trans_seq   	= LCM_DSI_TRANS_SEQ_MSB_FIRST;
	params->dsi.data_format.padding     	= LCM_DSI_PADDING_ON_LSB;
	params->dsi.data_format.format      	= LCM_DSI_FORMAT_RGB888;

	/* Highly depends on LCD driver capability */

	/* video mode timing */
	params->dsi.PS=LCM_PACKED_PS_24BIT_RGB888;
	params->dsi.vertical_sync_active				= 3;
	params->dsi.vertical_backporch					= 12;
	params->dsi.vertical_frontporch					= 16;
	params->dsi.vertical_active_line				= FRAME_HEIGHT;
	params->dsi.horizontal_sync_active				= 24;
	params->dsi.horizontal_backporch				= 100;
	params->dsi.horizontal_frontporch				= 50;
	params->dsi.horizontal_active_pixel				= FRAME_WIDTH;

	/* this value must be in MTK suggested table */	
	params->dsi.PLL_CLOCK = 220;
		params->dsi.ssc_disable							= 1;
	    params->dsi.ssc_range							= 4;
	     params->dsi.HS_TRAIL							= 15;
		 
	params->dsi.noncont_clock = 1;
	params->dsi.noncont_clock_period = 1;

#if 0 // Unused params start
	params->dsi.dual_dsi_type = DUAL_DSI_CMD;
	params->dsi.ufoe_enable  = 1;
	params->dsi.ufoe_params.lr_mode_en = 1;

	params->dsi.esd_check_enable = 1;
	params->dsi.customization_esd_check_enable      = 1;
	params->dsi.clk_lp_per_line_enable = 0;
	params->dsi.lcm_esd_check_table[0].cmd          = 0x09;
	params->dsi.lcm_esd_check_table[0].count        = 3;
	params->dsi.lcm_esd_check_table[0].para_list[0] = 0x80;
	params->dsi.lcm_esd_check_table[0].para_list[1] = 0x73;
	params->dsi.lcm_esd_check_table[0].para_list[2] = 0x06;

	params->dsi.lcm_esd_check_table[1].cmd          = 0xB1;
	params->dsi.lcm_esd_check_table[1].count        = 10;
	params->dsi.lcm_esd_check_table[1].para_list[0] = 0x00;
	params->dsi.lcm_esd_check_table[1].para_list[1] = 0x7C;
	params->dsi.lcm_esd_check_table[1].para_list[2] = 0x31;
	params->dsi.lcm_esd_check_table[1].para_list[3] = 0x30;
	params->dsi.lcm_esd_check_table[1].para_list[4] = 0x44;
	params->dsi.lcm_esd_check_table[1].para_list[5] = 0x09;
	params->dsi.lcm_esd_check_table[1].para_list[6] = 0x22;
	params->dsi.lcm_esd_check_table[1].para_list[7] = 0x22;
	params->dsi.lcm_esd_check_table[1].para_list[8] = 0x71;
	params->dsi.lcm_esd_check_table[1].para_list[9] = 0xF1;
#endif // Unsed params end
}


static void lcm_init_power(void)
{
#if 0
	MDELAY(10);
#ifdef BUILD_LK
	pmic_set_register_value(PMIC_RG_VEFUSE_VOSEL,3);
	MDELAY(5);
	pmic_set_register_value(PMIC_RG_VEFUSE_EN,1);
	MDELAY(5);
#endif
#endif
}

static void lcm_init(void)
{
	pmic_set_register_value(PMIC_RG_VEFUSE_VOSEL,3);
	MDELAY(2);
	pmic_set_register_value(PMIC_RG_VEFUSE_EN,1);
	MDELAY(2);

	mt_set_gpio_mode(GPIO_LCD_BIAS_ENP_PIN, GPIO_MODE_00);
	mt_set_gpio_dir(GPIO_LCD_BIAS_ENP_PIN, GPIO_DIR_OUT);
	mt_set_gpio_out(GPIO_LCD_BIAS_ENP_PIN, GPIO_OUT_ONE);

	MDELAY(5);
	mt_set_gpio_mode(GPIO_LCD_BIAS_ENN_PIN, GPIO_MODE_00);
	mt_set_gpio_dir(GPIO_LCD_BIAS_ENN_PIN, GPIO_DIR_OUT);
	mt_set_gpio_out(GPIO_LCD_BIAS_ENN_PIN, GPIO_OUT_ONE);

	MDELAY(1);
	nt5038_i2c_write_byte(nt5038_cmd_data[0].cmd, nt5038_cmd_data[0].data);
	MDELAY(1);
	nt5038_i2c_write_byte(nt5038_cmd_data[1].cmd, nt5038_cmd_data[1].data);
	MDELAY(1);
	nt5038_i2c_write_byte(nt5038_cmd_data[2].cmd, nt5038_cmd_data[2].data);
	
	//mt_set_gpio_mode(LCM_RESET_PIN,GPIO_MODE_GPIO);
	//mt_set_gpio_dir(LCM_RESET_PIN, GPIO_DIR_OUT);
	//mt_set_gpio_pull_enable(LCM_RESET_PIN, GPIO_PULL_DISABLE);
    
	MDELAY(5);
	SET_RESET_PIN(1);
	MDELAY(5);
	SET_RESET_PIN(0);
	MDELAY(5);
	SET_RESET_PIN(1);
	MDELAY(50);
	lcm_register_init();
	/* when phone initial , config output high, enable backlight drv chip */
	//push_table(lcm_initialization_setting, sizeof(lcm_initialization_setting) / sizeof(struct LCM_setting_table), 1);

}

static void lcm_suspend(void)
{
//	push_table(lcm_deep_sleep_mode_in_setting, sizeof(lcm_deep_sleep_mode_in_setting) / sizeof(struct LCM_setting_table), 1);
   unsigned int data_array[20];
	SET_RESET_PIN(1);
	MDELAY(5);
	SET_RESET_PIN(0);
	MDELAY(5);
	SET_RESET_PIN(1);
	MDELAY(30);
	data_array[0] = 0x00022902;
	data_array[1] = 0x000004b0;
	dsi_set_cmdq(data_array, 2, 1);

	data_array[0] = 0x00022902;
	data_array[1] = 0x000001b1;
	dsi_set_cmdq(data_array, 2, 1);
	MDELAY(5);
	
	mt_set_gpio_mode(GPIO_LCD_BIAS_ENN_PIN, GPIO_MODE_00);
	mt_set_gpio_dir(GPIO_LCD_BIAS_ENN_PIN, GPIO_DIR_OUT);
	mt_set_gpio_out(GPIO_LCD_BIAS_ENN_PIN, GPIO_OUT_ZERO);
	MDELAY(5);

	mt_set_gpio_mode(GPIO_LCD_BIAS_ENP_PIN, GPIO_MODE_00);
	mt_set_gpio_dir(GPIO_LCD_BIAS_ENP_PIN, GPIO_DIR_OUT);
	mt_set_gpio_out(GPIO_LCD_BIAS_ENP_PIN, GPIO_OUT_ZERO);
	MDELAY(2);
	pmic_set_register_value(PMIC_RG_VEFUSE_EN, 0);
}

static void lcm_resume(void)
{	
	lcm_init();
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
		LCM_ERROR("(%s) r61350_hd720_dsi_vdo_tcl_auo  get lcm chip id vol fail\n", __func__);
		return 0;
	} 
#endif
	lcm_vol = data[0]*1000+data[1]*10;

	LCM_DEBUG("(%s) r61350_hd720_dsi_vdo_tcl_auo lcm chip id adc raw data:%d, lcm_vol:%d\n", __func__, rawdata, lcm_vol);

	if (lcm_vol <= LCM_ID_MAX_VOLTAGE)
		return 1;
	else
		return 0;

}

#if 0
static unsigned int lcm_compare_id(void)
{
	unsigned int id = 0;
	unsigned char buffer[2];
	unsigned int array[16];
			    
	SET_RESET_PIN(1);   
	MDELAY(5);
	SET_RESET_PIN(0);
	MDELAY(5);
	SET_RESET_PIN(1);
	MDELAY(20);
    
    array[0] = 0x00023700;  /* read id return two byte,version and id */
	dsi_set_cmdq(array, 1, 1);

	read_reg_v2(0x3B, buffer, 2);
	id = buffer[0];     /* we only need ID */

	printk("%s,r61350_hd720_dsi_vdo_tcl_auo debug: r61350_hd720_dsi_vdo_tcl_auo id = 0x%x\n", __func__, id);

	if (id == LCM_ID_R61350)
		return 1;
	else
		return 0;
}
#endif

LCM_DRIVER r61350_hd720_dsi_vdo_truly_lcm_drv = 
{
	.name		= "r61350_hd720_dsi_vdo_truly",
	.set_util_funcs = lcm_set_util_funcs,
	.get_params     = lcm_get_params,
	.init           = lcm_init,
	.suspend        = lcm_suspend,
	.resume         = lcm_resume,
	.compare_id     = lcm_compare_id,
	.init_power		= lcm_init_power,
	//.resume_power   = lcm_resume_power,
	//.suspend_power  = lcm_suspend_power,

};
