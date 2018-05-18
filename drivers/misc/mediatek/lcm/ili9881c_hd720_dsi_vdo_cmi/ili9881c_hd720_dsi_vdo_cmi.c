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
/* LCM id voltage is 1.5V */
#define LCM_ID_MIN_VOLTAGE   1350

// ---------------------------------------------------------------------------
//  Local Variables
// ---------------------------------------------------------------------------
struct NT5038_SETTING_TABLE {
	unsigned char cmd;
	unsigned char data;
};
static struct NT5038_SETTING_TABLE nt5038_cmd_data[3] = {
	{ 0x00, 0x0a },
	{ 0x01, 0x0a },
	{ 0x03, 0x30 }
};

static LCM_UTIL_FUNCS lcm_util;
extern int IMM_GetOneChannelValue(int dwChannel, int data[4], int* rawdata);
/* LCM inversion, for *#87# lcm flicker test */
extern unsigned int g_lcm_inversion;
extern void lcm_set_reset_pin(uint32_t value);
extern long lcd_enp_bias_setting(unsigned int value);
extern long lcd_enn_bias_setting(unsigned int value);
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
#define SET_RESET_PIN(v)	  								lcm_util.set_reset_pin(v)
#define high_set_gpio_lcd_enp_enn()						do { \
	lcd_enp_bias_setting(1); \
	lcm_util.mdelay(10); \
	lcd_enn_bias_setting(1); \
} while(0)
#define low_set_gpio_lcd_enp_enn()						do { \
	lcd_enn_bias_setting(0); \
	lcm_util.mdelay(10); \
	lcd_enp_bias_setting(0); \
} while(0)

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
	{0xFF,3,{0x98,0x81,0x03}},

	{0x01,1,{0x00}},
	{0x02,1,{0x00}},
	{0x03,1,{0x53}},
	{0x04,1,{0x13}},
	{0x05,1,{0x13}},
	{0x06,1,{0x06}},
	{0x07,1,{0x00}},
	{0x08,1,{0x04}},
	{0x09,1,{0x00}},      //04
	{0x0a,1,{0x00}},     //03
	{0x0b,1,{0x00}},     //03
	{0x0c,1,{0x00}},
	{0x0d,1,{0x00}},
	{0x0e,1,{0x00}},
	{0x0f,1,{0x00}},    //04
		                
	{0x10,1,{0x00}},    //04
	{0x11,1,{0x00}},
	{0x12,1,{0x00}},
	{0x13,1,{0x00}},
	{0x14,1,{0x00}},
	{0x15,1,{0x00}},
	{0x16,1,{0x00}}, 
	{0x17,1,{0x00}},
	{0x18,1,{0x00}},
	{0x19,1,{0x00}},
	{0x1a,1,{0x00}},
	{0x1b,1,{0x00}},
	{0x1c,1,{0x00}},
	{0x1d,1,{0x00}},
	{0x1e,1,{0xc0}},
	{0x1f,1,{0x80}},
		                
	{0x20,1,{0x04}},
	{0x21,1,{0x0b}},
	{0x22,1,{0x00}},
	{0x23,1,{0x00}},
	{0x24,1,{0x00}},
	{0x25,1,{0x00}},
	{0x26,1,{0x00}},
	{0x27,1,{0x00}},
	{0x28,1,{0x55}},
	{0x29,1,{0x03}},
	{0x2a,1,{0x00}},
	{0x2b,1,{0x00}},
	{0x2c,1,{0x00}},
	{0x2d,1,{0x00}},
	{0x2e,1,{0x00}},
	{0x2f,1,{0x00}},
		                
	{0x30,1,{0x00}},
	{0x31,1,{0x00}},
	{0x32,1,{0x00}},
	{0x33,1,{0x00}},
	{0x34,1,{0x04}},
	{0x35,1,{0x05}},    //00
	{0x36,1,{0x05}},
	{0x37,1,{0x00}},
	{0x38,1,{0x3c}},
	{0x39,1,{0x00}},
	{0x3a,1,{0x40}}, 
	{0x3b,1,{0x40}},
	{0x3c,1,{0x00}},
	{0x3d,1,{0x00}},
	{0x3e,1,{0x00}},
	{0x3f,1,{0x00}},
		               
	{0x40,1,{0x00}},
	{0x41,1,{0x00}},
	{0x42,1,{0x00}},
	{0x43,1,{0x00}},
	{0x44,1,{0x00}},
		               	               
	//GIP_2                
	{0x50,1,{0x01}},
	{0x51,1,{0x23}},
	{0x52,1,{0x45}},
	{0x53,1,{0x67}},
	{0x54,1,{0x89}},
	{0x55,1,{0xab}},
	{0x56,1,{0x01}},
	{0x57,1,{0x23}},
	{0x58,1,{0x45}},
	{0x59,1,{0x67}},
	{0x5a,1,{0x89}},
	{0x5b,1,{0xab}},
	{0x5c,1,{0xcd}},
	{0x5d,1,{0xef}},
		                
	//GIP_3                 
	{0x5e,1,{0x01}},
	{0x5f,1,{0x14}},
	{0x60,1,{0x15}},
	{0x61,1,{0x0c}},
	{0x62,1,{0x0d}},
	{0x63,1,{0x0e}},
	{0x64,1,{0x0f}},
	{0x65,1,{0x10}},
	{0x66,1,{0x11}},
	{0x67,1,{0x08}},
	{0x68,1,{0x02}},
	{0x69,1,{0x0a}},
	{0x6a,1,{0x02}},
	{0x6b,1,{0x02}},
	{0x6c,1,{0x02}},
	{0x6d,1,{0x02}},
	{0x6e,1,{0x02}},
	{0x6f,1,{0x02}},
		              
	{0x70,1,{0x02}},
	{0x71,1,{0x02}},
	{0x72,1,{0x06}},
	{0x73,1,{0x02}},
	{0x74,1,{0x02}},
	{0x75,1,{0x14}},
	{0x76,1,{0x15}},
	{0x77,1,{0x11}}, 
	{0x78,1,{0x10}},
	{0x79,1,{0x0f}},
	{0x7a,1,{0x0e}},
	{0x7b,1,{0x0d}},
	{0x7c,1,{0x0c}},
	{0x7d,1,{0x06}},
	{0x7e,1,{0x02}},
	{0x7f,1,{0x0a}},
		                
	{0x80,1,{0x02}},
	{0x81,1,{0x02}},
	{0x82,1,{0x02}},
	{0x83,1,{0x02}},
	{0x84,1,{0x02}},
	{0x85,1,{0x02}},
	{0x86,1,{0x02}},
	{0x87,1,{0x02}},
	{0x88,1,{0x08}},
	{0x89,1,{0x02}},
	{0x8A,1,{0x02}},
	          
	//CMD_Page 4      
	{0xFF,3,{0x98,0x81,0x04}},
	{0x00,1,{0x00}}, 
	{0x6C,1,{0x15}},                //Set VCORE voltage =1.5V
	{0x6E,1,{0x3b}},                //di_pwr_reg=0 for power mode 2A //VGH clamp 15V
	{0x6F,1,{0x53}},                // reg vcl + pumping ratio VGH=3x VGL=-2.5x
	{0x3A,1,{0xA4}},                //POWER SAVING
	{0x8D,1,{0x15}},                //VGL clamp -10
	{0x87,1,{0xBA}},                              
	{0x26,1,{0x76}},                            
	{0xB2,1,{0xD1}},               
	{0x88,1,{0x0b}},
		          
	///BIST MODE 
	//Generic_Short_Write_1P(0x2D,1,{0xFF}},               //VN0
	//Generic_Short_Write_1P(0x2F,1,{0x01}},               //VN0
	    		       
	//CMD_Page 1       
	{0xFF,3,{0x98,0x81,0x01}},
	{0x22,1,{0x0A}},		//09BGR,1,{ GS
	{0x31,1,{0x00}},		//column inversion (Zig-zag type3)
	{0x53,1,{0x6c}},		//7bVCOM1
	{0x55,1,{0x99}},		//VCOM2
        
        /*modify for gamma pass test by cheng.wang start 20160622*/
	{0x50,1,{0x95}},		//a7VREG1OUT=4.5V a6
	{0x51,1,{0x90}},		//9eVREG2OUT=-4.5V a6
	/*modify for gamma pass test by cheng.wang end 20160622*/
	{0x60,1,{0x14}},    //van+
	
	{0xA0,1,{0x08}},		//VP255	Gamma P
	{0xA1,1,{0x29}},              //VP251        
	{0xA2,1,{0x39}},               //VP247        
	{0xA3,1,{0x16}},               //VP243        
	{0xA4,1,{0x19}},               //VP239        
	{0xA5,1,{0x2c}},               //VP231        
	{0xA6,1,{0x1f}},               //VP219        
	{0xA7,1,{0x21}},               //VP203        
	{0xA8,1,{0xa5}},               //VP175        
	{0xA9,1,{0x1d}},               //VP144        
	{0xAA,1,{0x29}},               //VP111        
	{0xAB,1,{0x8a}},               //VP80         
	{0xAC,1,{0x1b}},               //VP52         
	{0xAD,1,{0x1A}},               //VP36         
	{0xAE,1,{0x4E}},               //VP24         
	{0xAF,1,{0x23}},               //VP16         
	{0xB0,1,{0x28}},               //VP12         
	{0xB1,1,{0x52}},              //VP8          
	{0xB2,1,{0x60}},               //VP4          
	{0xB3,1,{0x2c}},               //VP0          
		                                           
	{0xC0,1,{0x08}},		//VN255 GAMMA N1
	{0xC1,1,{0x29}},               //VN251        
	{0xC2,1,{0x39}},               //VN247        
	{0xC3,1,{0x16}},               //VN243        
	{0xC4,1,{0x19}},               //VN239        
	{0xC5,1,{0x2d}},               //VN231        
	{0xC6,1,{0x20}},               //VN219        
	{0xC7,1,{0x21}},               //VN203        
	{0xC8,1,{0xa6}},               //VN175        
	{0xC9,1,{0x1d}},               //VN144        
	{0xCA,1,{0x2a}},               //VN111        
	{0xCB,1,{0x89}},               //VN80         
	{0xCC,1,{0x1a}},               //VN52         
	{0xCD,1,{0x19}},              //VN36         
	{0xCE,1,{0x4d}},              //VN24         
	{0xCF,1,{0x21}},               //VN16         
	{0xD0,1,{0x28}},               //VN12         
	{0xD1,1,{0x52}},               //VN8          
	{0xD2,1,{0x60}},               //VN4          
	{0xD3,1,{0x2c}},               //VN0

	//CMD_Page 0
	{0xFF,3,{0x98,0x81,0x00}},
	//{0x36,1,{0x03}},
	//REGISTER,35,00
	{0x11,0,{}},
	{REGFLAG_DELAY, 120, {}},
	{0x29,0,{}},
	{REGFLAG_DELAY, 40, {}},
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
    params->dsi.LANE_NUM = LCM_THREE_LANE;
    //The following defined the fomat for data coming from LCD engine.
    params->dsi.data_format.color_order = LCM_COLOR_ORDER_RGB;
    params->dsi.data_format.trans_seq   = LCM_DSI_TRANS_SEQ_MSB_FIRST;
    params->dsi.data_format.padding     = LCM_DSI_PADDING_ON_LSB;
    params->dsi.data_format.format      = LCM_DSI_FORMAT_RGB888;
    
    params->dsi.intermediat_buffer_num = 0;//because DSI/DPI HW design change, this parameters should be 0 when video mode in MT658X; or memory leakage
    
    params->dsi.PS=LCM_PACKED_PS_24BIT_RGB888;
    
    params->dsi.word_count=FRAME_WIDTH*3; //DSI CMD mode need set these two bellow params, different to 6577
    params->dsi.vertical_active_line=FRAME_HEIGHT;
    
    /*modified for gamma pass test by cheng.wang start 20160622*/		          
    params->dsi.vertical_sync_active = 8;   //4//4
    params->dsi.vertical_backporch = 24,//10 20;
    params->dsi.vertical_frontporch = 24,//10  20; 
    /*modified for gamma pass test by cheng.wang end 20160622*/		          
    params->dsi.vertical_active_line = FRAME_HEIGHT;
    
    params->dsi.horizontal_sync_active =20;
    /*modified for gamma pass test by cheng.wang start 20160622*/		          
    params->dsi.horizontal_backporch = 40,//120;//50;
    params->dsi.horizontal_frontporch = 50,// 120;//50;
    /*modified for gamma pass test by cheng.wang end 20160622*/		          
    params->dsi.horizontal_active_pixel = FRAME_WIDTH;
//    params->dsi.compatibility_for_nvk = 0; // this parameter would be set to 1 if DriverIC is NTK's and when force match DSI clock for NTK's
    
	params->dsi.PLL_CLOCK = 261;
	params->dsi.ssc_range = 1;
	params->dsi.ssc_disable = 0;
	params->dsi.HS_TRAIL = 15;
    params->dsi.vertical_vfp_lp = 1000;
	
	params->dsi.esd_check_enable = 0;
	params->dsi.customization_esd_check_enable = 0;
	params->dsi.lcm_esd_check_table[0].cmd          = 0x0A;
	params->dsi.lcm_esd_check_table[0].count        = 1;
	params->dsi.lcm_esd_check_table[0].para_list[0] = 0x9C;	
}


static void lcm_init_power(void)
{
	MDELAY(10);
#ifdef BUILD_LK
	pmic_set_register_value(PMIC_RG_VEFUSE_VOSEL,3);
	MDELAY(5);
	pmic_set_register_value(PMIC_RG_VEFUSE_EN,1);
	MDELAY(5);
#endif
	low_set_gpio_lcd_enp_enn();
	MDELAY(5);
	nt5038_i2c_write_byte(nt5038_cmd_data[0].cmd, nt5038_cmd_data[0].data);
	MDELAY(1);
	nt5038_i2c_write_byte(nt5038_cmd_data[1].cmd, nt5038_cmd_data[1].data);
	MDELAY(1);
	nt5038_i2c_write_byte(nt5038_cmd_data[2].cmd, nt5038_cmd_data[2].data);
	MDELAY(1);
	high_set_gpio_lcd_enp_enn();
	MDELAY(10);
}

static void lcm_resume_power(void)
{
#if 0
	high_set_gpio_lcd_enp_enn();
	MDELAY(2);
	nt5038_i2c_write_byte(nt5038_cmd_data[0].cmd, nt5038_cmd_data[1].data);
	MDELAY(2);
	nt5038_i2c_write_byte(nt5038_cmd_data[1].cmd, nt5038_cmd_data[1].data);
	MDELAY(2);
#endif
	pmic_set_register_value(PMIC_RG_VEFUSE_EN, 1);
	MDELAY(5);
	high_set_gpio_lcd_enp_enn();
	MDELAY(5);
}

static void lcm_suspend_power(void)
{
	SET_RESET_PIN(0);
	MDELAY(5);
	low_set_gpio_lcd_enp_enn();
	MDELAY(5);
	pmic_set_register_value(PMIC_RG_VEFUSE_EN, 0);
	MDELAY(5);
}


static void lcm_init(void)
{
#if defined(BUILD_LK) || defined(CONFIG_MTK_LEGACY)
	/* set lcm reset pin mode */
	mt_set_gpio_mode(GPIO_LCM_RST, GPIO_MODE_GPIO);
	MDELAY(2);
	mt_set_gpio_dir(GPIO_LCM_RST, GPIO_DIR_OUT);
	MDELAY(2);
	mt_set_gpio_pull_enable(GPIO_LCM_RST, GPIO_PULL_DISABLE);
	MDELAY(10);
#endif
	SET_RESET_PIN(1);
	MDELAY(10);
	SET_RESET_PIN(0);
	MDELAY(10);
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
	MDELAY(2);
	mt_set_gpio_dir(GPIO_LCM_RST, GPIO_DIR_OUT);
	MDELAY(2);
	mt_set_gpio_pull_enable(GPIO_LCM_RST, GPIO_PULL_DISABLE);
	MDELAY(10);
#endif
	SET_RESET_PIN(1);
	MDELAY(10);
	SET_RESET_PIN(0);
	MDELAY(10);
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
		LCM_ERROR("(%s) ili9881c_cmi_fhd_dsi_vdo_tcl  get lcm chip id vol fail\n", __func__);
		return 0;
	}
#endif
	lcm_vol = data[0]*1000+data[1]*10;

	LCM_DEBUG("(%s) ili9881c_cmi_fhd_dsi_vdo_tcl lcm chip id adc raw data:%d, lcm_vol:%d\n", __func__, rawdata, lcm_vol);

	if (lcm_vol > LCM_ID_MIN_VOLTAGE)
		return 1;
	else
		return 0;

}


LCM_DRIVER ili9881c_hd720_dsi_vdo_cmi_lcm_drv = 
{
	.name		= "ili9881c_hd720_dsi_vdo_cmi",
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
