/*Transsion Top Secret*/
/**************************************************************************
*  AW9136_ts_3button.c
* 
*  Create Date :
* 
*  Modify Date : 
*
*  Create by   : AWINIC Technology CO., LTD
*
*  Version     : 1.0 , 2016/03/22
* 
**************************************************************************/
//////////////////////////////////////////////////////////////
//  
//  APPLICATION DEFINE :
//
//                   Mobile -    MENU      HOME      BACK
//                   AW9136 -     S3        S2        S1
//
//////////////////////////////////////////////////////////////
#include <linux/device.h>
#include <linux/kernel.h>
#include <linux/i2c.h>
#include <linux/input.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/gpio.h>
#include <linux/miscdevice.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/irq.h>
#include <linux/firmware.h>
#include <linux/platform_device.h>
#include <linux/fb.h>
#include <linux/notifier.h>
#include <linux/workqueue.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <asm/io.h>
#include <linux/init.h>
#include <linux/pci.h>
#include <linux/dma-mapping.h>
#include <linux/gameport.h>
#include <linux/moduleparam.h>
#include <linux/mutex.h>
#include <linux/module.h>
//#include <mach/mt_gpio.h>
#include "AW9136_reg.h"
#include "AW9136_para.h"
//#include "cust_gpio_usage.h"
//#include <cust_eint.h>
#include <mach/mt_gpt.h>
#include <linux/wakelock.h>
#include <linux/atomic.h>

//////////////////////////////////////
// IO PIN DEFINE
//////////////////////////////////////
#define AW9136_PDN          (GPIO4|0x80000000)
#define AW9136_EINT_NUM     57
#define AW9136_EINT_PIN     (GPIO57|0x80000000)

//#define AW9136_ts_NAME		"AW9136_ts"
#define AW9136_ts_NAME		"aw9136_ts"
//#define AW9136_ts_NAME		"mediatek,aw9136"
#define AW9136_ts_I2C_ADDR	0x2C
#define AW9136_ts_I2C_BUS	2

#define ABS(x)				(x>0? (x):-(x))

////////////////////////////////////////////////////////////////////
#ifdef AW_AUTO_CALI
#define CALI_NUM	4
#define CALI_RAW_MIN	1000
#define CALI_RAW_MAX	3000	

unsigned char cali_flag = 0;
unsigned char cali_num = 0;
unsigned char cali_cnt = 0;
unsigned char cali_used = 0;
unsigned char old_cali_dir[6];	//	0: no cali		1: ofr pos cali		2: ofr neg cali
unsigned int old_ofr_cfg[6];
long Ini_sum[6];
#endif

////////////////////////////////////////////////////////////////////
//struct aw9136_i2c_setup_data {
//	unsigned i2c_bus;  //the same number as i2c->adap.nr in adapter probe function
//	unsigned short i2c_address;
//	int irq;
//	char type[I2C_NAME_SIZE];
//};

struct AW9136_ts_data {
	struct input_dev	*input_dev;
	struct work_struct 	eint_work;
	struct device_node *irq_node;
	int irq;
};

struct AW9136_ts_data *AW9136_ts;
static struct i2c_client *aw9136_i2c_client;
static struct work_struct aw9136_resume_work;
static struct workqueue_struct *aw9136_resume_workqueue;
static struct notifier_block aw9136_fb_notifier;
static void aw9136_resume_workqueue_callback(struct work_struct *work);
static int aw9136_suspend_flag;
pm_message_t aw9136_mesg;
static int AW9136_create_sysfs(struct i2c_client *client);
static int aw9136_fb_notifier_callback(struct notifier_block *self, unsigned long event, void *data);
//////////////////////////////////////////////////////////////////////////////////////
// Touch process variable
//////////////////////////////////////////////////////////////////////////////////////
static unsigned char suspend_flag = 0 ; //0: normal; 1: sleep
static int debug_level=0;
static int WorkMode = 1 ; //1: sleep, 2: normal

unsigned int arraylen=59;
unsigned int romcode[59] = 
{
0x9f0a,0x800,0x900,0x3600,0x3,0x3700,0x190b,0x180a,0x1c00,0x1800,0x1008,0x520,0x1800,0x1004,0x513,0x1800,
0x1010,0x52d,0x3,0xa4ff,0xa37f,0xa27f,0x3cff,0x3cff,0x3cff,0xc4ff,0xc37f,0xc27f,0x38ff,0x387f,0xbf00,0x3,
0xa3ff,0xa47f,0xa27f,0x3cff,0x3cff,0x3cff,0xc3ff,0xc47f,0xc27f,0x38ff,0x387f,0xbf00,0x3,0xa2ff,0xa37f,0xa47f,
0x3cff,0x3cff,0x3cff,0xc2ff,0xc37f,0xc47f,0x38ff,0x387f,0xbf00,0x3,0x3402,
};


//////////////////////////////////////////////////////////////////////////////////////////
// PDN power control
//////////////////////////////////////////////////////////////////////////////////////////
struct pinctrl *aw9136ctrl = NULL;
struct pinctrl_state *aw9136_int_pin = NULL;
struct pinctrl_state *aw9136_pdn_high = NULL;
struct pinctrl_state *aw9136_pdn_low = NULL;

int AW9136_gpio_init(struct platform_device *pdev)
{
	int ret = 0;

	aw9136ctrl = devm_pinctrl_get(&pdev->dev);
	if (IS_ERR(aw9136ctrl)) {
		dev_err(&pdev->dev, "Cannot find camera pinctrl!");
		ret = PTR_ERR(aw9136ctrl);
		printk("%s devm_pinctrl_get fail!\n", __func__);
	}
	aw9136_pdn_high = pinctrl_lookup_state(aw9136ctrl, "aw9136_pdn_high");
	if (IS_ERR(aw9136_pdn_high)) {
		ret = PTR_ERR(aw9136_pdn_high);
		printk("%s : pinctrl err, aw9136_pdn_high\n", __func__);
	}

	aw9136_pdn_low = pinctrl_lookup_state(aw9136ctrl, "aw9136_pdn_low");
	if (IS_ERR(aw9136_pdn_low)) {
		ret = PTR_ERR(aw9136_pdn_low);
		printk("%s : pinctrl err, aw9136_pdn_low\n", __func__);
	}

	printk("%s success\n", __func__);
	return ret;
}

static void AW9136_ts_pwron(void)
{
    printk("%s enter\n", __func__);
	pinctrl_select_state(aw9136ctrl, aw9136_pdn_low);
    msleep(5);
    pinctrl_select_state(aw9136ctrl, aw9136_pdn_high);
    msleep(10);
    printk("%s out\n", __func__);
}
/*
static void AW9136_ts_pwroff(void)
{
	pinctrl_select_state(aw9136ctrl, aw9136_pnd_low);
    msleep(5);
}
*/
static void AW9136_ts_config_pins(void)
{
	printk("%s enter\n", __func__);
	AW9136_ts_pwron();
    printk("%s out\n", __func__);
}

//////////////////////////////////////////////////////////////////////////////////////////
// i2c write and read
//////////////////////////////////////////////////////////////////////////////////////////
unsigned int I2C_write_reg(unsigned char addr, unsigned int reg_data)
{
	//int i;
	int ret;
	u8 wdbuf[512] = {0};

	struct i2c_msg msgs[] = {
		{
			.addr	= aw9136_i2c_client->addr,
			.flags	= 0,
			.len	= 3,
			.buf	= wdbuf,
		},
	};

	wdbuf[0] = addr;
	wdbuf[2] = (unsigned char)(reg_data & 0x00ff);
	wdbuf[1] = (unsigned char)((reg_data & 0xff00)>>8);

	ret = i2c_transfer(aw9136_i2c_client->adapter, msgs, 1);
	if (ret < 0)
		pr_err("msg %s i2c read error: %d\n", __func__, ret);

    return ret;

}


unsigned int I2C_read_reg(unsigned char addr)
{
	//int ret,i;
	int ret;
	u8 rdbuf[512] = {0};
	unsigned int getdata;

	struct i2c_msg msgs[] = {
		{
			.addr	= aw9136_i2c_client->addr,
			.flags	= 0,
			.len	= 1,
			.buf	= rdbuf,
		},
		{
			.addr	= aw9136_i2c_client->addr,
			.flags	= I2C_M_RD,
			.len	= 2,
			.buf	= rdbuf,
		},
	};

	rdbuf[0] = addr;

	ret = i2c_transfer(aw9136_i2c_client->adapter, msgs, 2);
	if (ret < 0)
		pr_err("msg %s i2c read error: %d\n", __func__, ret);

	getdata=rdbuf[0] & 0x00ff;
	getdata<<= 8;
	getdata |=rdbuf[1];

    return getdata;
}


//////////////////////////////////////////////////////////////////////
// AW9136 initial register @ mobile active
//////////////////////////////////////////////////////////////////////
void AW_NormalMode(void)
{
	//unsigned int i;
	I2C_write_reg(GCR,0x0000);  // disable chip

	///////////////////////////////////////
	// LED config
	//I2C_write_reg(LER1,N_LER1);			//LED enable
	//I2C_write_reg(LER2,N_LER2);			//LED enable

	//I2C_write_reg(CTRS1,0x0000);	//LED control RAM or I2C
	//I2C_write_reg(CTRS2,0x0000);	//LED control RAM or I2C
	I2C_write_reg(IMAX1,N_IMAX1);		//LED MAX light setting
	I2C_write_reg(IMAX2,N_IMAX2);		//LED MAX light setting
	I2C_write_reg(IMAX3,N_IMAX3);		//LED MAX light setting
	I2C_write_reg(IMAX4,N_IMAX4);		//LED MAX light setting
	I2C_write_reg(IMAX5,N_IMAX5);		//LED MAX light setting

	I2C_write_reg(LCR,N_LCR);				//LED effect control
	I2C_write_reg(IDLECR,N_IDLECR);		//IDLE time setting
        ///////////////////////////////////////
        // cap-touch config
	I2C_write_reg(SLPR,N_SLPR);   // touch key enable
	I2C_write_reg(SCFG1,N_SCFG1);  // scan time setting
	I2C_write_reg(SCFG2,N_SCFG2);		// bit0~3 is sense seting
	
	I2C_write_reg(OFR2,N_OFR2);   // offset
	I2C_write_reg(OFR3,N_OFR3);		 // offset

	I2C_write_reg(THR2, N_THR2);		//S1 press thred setting
	I2C_write_reg(THR3, N_THR3);		//S2 press thred setting
	I2C_write_reg(THR4, N_THR4);		//S3 press thred setting
	
	I2C_write_reg(SETCNT,N_SETCNT);  // debounce
	I2C_write_reg(BLCTH,N_BLCTH);   // base trace rate 
	
	I2C_write_reg(AKSR,N_AKSR);    // AKS 

#ifndef AW_AUTO_CALI
	I2C_write_reg(INTER,N_INTER);	 	// signel click interrupt 
#else
	I2C_write_reg(INTER,0x0080);	 	// signel click interrupt 
#endif

	I2C_write_reg(MPTR,N_MPTR); 
	I2C_write_reg(GDTR,N_GDTR);    // gesture time setting
	I2C_write_reg(GDCFGR,N_GDCFGR);	 //gesture  key select
	I2C_write_reg(TAPR1,N_TAPR1);		//double click 1
	I2C_write_reg(TAPR2,N_TAPR2);		//double click 2
	I2C_write_reg(TDTR,N_TDTR);			//double click time
#ifndef AW_AUTO_CALI
	I2C_write_reg(GIER,N_GIER);			//gesture and double click enable
#else
	I2C_write_reg(GIER,0x0000);			//gesture and double click disable
#endif	

        ///////////////////////////////////////
	I2C_write_reg(GCR,N_GCR);    // LED enable and touch scan enable

	///////////////////////////////////////
	// LED RAM program
#if 0
	I2C_write_reg(INTVEC,0x5);	
	I2C_write_reg(TIER,0x1c);  
	I2C_write_reg(PMR,0x0000);
	I2C_write_reg(RMR,0x0000);
	I2C_write_reg(WADDR,0x0000);

	for(i=0;i<59;i++){
		I2C_write_reg(WDATA,N_romcode[i]);
	}

	I2C_write_reg(SADDR,0x0000);
	I2C_write_reg(PMR,0x0001);
	I2C_write_reg(RMR,0x0002);
#endif
	WorkMode = 2;
	printk("%s Finish\n", __func__);

}


//////////////////////////////////////////////////////////////////////
// AW9136 initial register @ mobile sleep
//////////////////////////////////////////////////////////////////////
void AW_SleepMode(void)
{
	//unsigned int i;

	I2C_write_reg(GCR,0x0000);   // disable chip

    ///////////////////////////////////////
    // LED config
	I2C_write_reg(LER1,S_LER1);		//LED enable
	I2C_write_reg(LER2,S_LER2);		//LED enable
	//I2C_write_reg(CTRS1,0x0000);
	//I2C_write_reg(CTRS2,0x0000);

	I2C_write_reg(LCR,S_LCR);			//LED effect control
	I2C_write_reg(IDLECR,S_IDLECR);		//IDLE time setting

	I2C_write_reg(IMAX1,S_IMAX1);		//LED MAX light setting
	I2C_write_reg(IMAX2,S_IMAX2);		//LED MAX light setting
	I2C_write_reg(IMAX3,S_IMAX3);		//LED MAX light setting
	I2C_write_reg(IMAX4,S_IMAX4);		//LED MAX light setting
	I2C_write_reg(IMAX5,S_IMAX5);		//LED MAX light setting

    ///////////////////////////////////////
    // cap-touch config
	I2C_write_reg(SLPR,S_SLPR);   // touch key enable

	I2C_write_reg(SCFG1,S_SCFG1);  // scan time setting
	I2C_write_reg(SCFG2,S_SCFG2);		// bit0~3 is sense seting

	I2C_write_reg(OFR2,S_OFR2);   // offset
	I2C_write_reg(OFR3,S_OFR3);   // offset

	I2C_write_reg(THR2, S_THR2);		//S1 press thred setting
	I2C_write_reg(THR3, S_THR3);		//S2 press thred setting
	I2C_write_reg(THR4, S_THR4);		//S3 press thred setting
	
	I2C_write_reg(SETCNT,S_SETCNT);		// debounce
	I2C_write_reg(IDLECR, S_IDLECR);
	I2C_write_reg(BLCTH,S_BLCTH);		//base speed setting

	I2C_write_reg(AKSR,S_AKSR);			//AKS

#ifndef AW_AUTO_CALI
	I2C_write_reg(INTER,S_INTER);	 	// signel click interrupt 
#else
	I2C_write_reg(INTER,0x0080);	 	// signel click interrupt 
#endif


	I2C_write_reg(GDCFGR,S_GDCFGR); //gesture  key select
	I2C_write_reg(TAPR1,S_TAPR1);		//double click 1
	I2C_write_reg(TAPR2,S_TAPR2);		//double click 2

	I2C_write_reg(TDTR,S_TDTR);	//double click time
#ifndef AW_AUTO_CALI
	I2C_write_reg(GIER,S_GIER);			//gesture and double click enable
#else
	I2C_write_reg(GIER,0x0000);			//gesture and double click disable
#endif	

	///////////////////////////////////////
	I2C_write_reg(GCR, S_GCR);   // enable chip sensor function

	///////////////////////////////////////
	// LED RAM program
#if 0	
	I2C_write_reg(INTVEC,0x5);	
	I2C_write_reg(TIER,0x1c);  
	I2C_write_reg(PMR,0x0000);
	I2C_write_reg(RMR,0x0000);
	I2C_write_reg(WADDR,0x0000);

	for(i=0;i<59;i++){
		I2C_write_reg(WDATA,S_romcode[i]);
	}

	I2C_write_reg(SADDR,0x0000);
	I2C_write_reg(PMR,0x0001);
	I2C_write_reg(RMR,0x0002);
#endif
	WorkMode = 1;
	printk("%s Finish\n", __func__);
}


void AW9136_LED_ON(void)
{
	I2C_write_reg(IMAX1,0x3300);
	I2C_write_reg(IMAX2,0x0003);
	I2C_write_reg(LER1,0x001c);
	I2C_write_reg(CTRS1,0x00fc);
	I2C_write_reg(CMDR,0xa2ff);  //LED1 ON
	I2C_write_reg(CMDR,0xa3ff);  //LED2 ON
	I2C_write_reg(CMDR,0xa4ff);  //LED3 ON
	I2C_write_reg(GCR,0x0003);
}

void AW9136_LED_OFF(void)
{
	I2C_write_reg(IMAX1,0x3300);
	I2C_write_reg(IMAX2,0x0003);
	I2C_write_reg(LER1,0x001c);
	I2C_write_reg(CTRS1,0x00fc);
	I2C_write_reg(CMDR,0xa200);  //LED1 OFF
	I2C_write_reg(CMDR,0xa300);  //LED2 OFF
	I2C_write_reg(CMDR,0xa400);  //LED3 OFF
	I2C_write_reg(GCR,0x0003);
}

////////////////////////////////////////////////////////////////////////
// AW91xx Auto Calibration
////////////////////////////////////////////////////////////////////////
#ifdef AW_AUTO_CALI
unsigned char AW91xx_Auto_Cali(void)
{
	unsigned char i;
	unsigned char cali_dir[6];

	unsigned int buf[6];
	unsigned int ofr_cfg[6];
	unsigned int sen_num;
	
	if(cali_num == 0){
		ofr_cfg[0] = I2C_read_reg(0x13);
		ofr_cfg[1] = I2C_read_reg(0x14);
		ofr_cfg[2] = I2C_read_reg(0x15);
	}else{
		for(i=0; i<3; i++){
			ofr_cfg[i] = old_ofr_cfg[i];
		}	
	}
	
	I2C_write_reg(0x1e,0x3);
	for(i=0; i<6; i++){
		buf[i] = I2C_read_reg(0x36+i);
	}
	sen_num = I2C_read_reg(0x02);		// SLPR
	
	for(i=0; i<6; i++) 
		Ini_sum[i] = (cali_cnt==0)? (0) : (Ini_sum[i] + buf[i]);

	if(cali_cnt==4){
		for(i=0; i<6; i++){
			if((sen_num & (1<<i)) == 0)	{	// sensor used
				if((Ini_sum[i]>>2) < CALI_RAW_MIN){
					if((i%2) && ((ofr_cfg[i>>1]&0xFF00)==0x1000)){					// 0x10** -> 0x00**
						ofr_cfg[i>>1] = ofr_cfg[i>>1] & 0x00FF;
						cali_dir[i] = 2;
					}else if((i%2) && ((ofr_cfg[i>>1]&0xFF00)==0x0000)){			// 0x00**	no calibration
						cali_dir[i] = 0;
					}else if (((i%2)==0) && ((ofr_cfg[i>>1]&0x00FF)==0x0010)){		// 0x**10 -> 0x**00
						ofr_cfg[i>>1] = ofr_cfg[i>>1] & 0xFF00;
						cali_dir[i] = 2;				
					}else if (((i%2)==0) && ((ofr_cfg[i>>1]&0x00FF)==0x0000)){		// 0x**00 no calibration
						cali_dir[i] = 0;				
					}else{
						ofr_cfg[i>>1] = ofr_cfg[i>>1] - ((i%2)? (1<<8):1);
						cali_dir[i] = 2;				
					}
				}else if((Ini_sum[i]>>2) > CALI_RAW_MAX){
					if((i%2) && ((ofr_cfg[i>>1]&0xFF00)==0x1F00)){	// 0x1F** no calibration					
						cali_dir[i] = 0;
					}else if (((i%2)==0) && ((ofr_cfg[i>>1]&0x00FF)==0x001F)){	// 0x**1F no calibration
						cali_dir[i] = 0;				
					}else{
						ofr_cfg[i>>1] = ofr_cfg[i>>1] + ((i%2)? (1<<8):1);				
						cali_dir[i] = 1;				
					}
				}else{
					cali_dir[i] = 0;
				}
				
				if(cali_num > 0){
					if(cali_dir[i] != old_cali_dir[i]){
						cali_dir[i] = 0;
						ofr_cfg[i>>1] = old_ofr_cfg[i>>1];
					}
				}
			}
		}
		
		cali_flag = 0;
		for(i=0; i<6; i++){
			if((sen_num & (1<<i)) == 0)	{	// sensor used
				if(cali_dir[i] != 0){
					cali_flag = 1;
				}			
			}
		}
		if((cali_flag==0) && (cali_num==0)){
			cali_used = 0;
		}else{
			cali_used = 1;
		}
		
		if(cali_flag == 0){
			cali_num = 0;
			cali_cnt = 0;
			return 0;
		}
		
		I2C_write_reg(GCR, 0x0000);
		for(i=0; i<3; i++){
			I2C_write_reg(OFR1+i, ofr_cfg[i]);
		}
		I2C_write_reg(GCR, 0x0003);
		
		if(cali_num == (CALI_NUM -1)){	// no calibration
			cali_flag = 0;
			cali_num = 0;
			cali_cnt = 0;
			return 0;
		}
		
		for(i=0; i<6; i++){
			old_cali_dir[i] = cali_dir[i];
		}
		
		for(i=0; i<3; i++){
			old_ofr_cfg[i] = ofr_cfg[i];
		}
		
		cali_num ++;
	}

	if(cali_cnt < 4){
		cali_cnt ++;	
	}else{
		cali_cnt = 0;
	}
	
	return 1;
}
#endif

/////////////////////////////////////////////
// report left slip 
/////////////////////////////////////////////
void AW_left_slip(void)
{
	printk("AW9136 left slip \n");
	input_report_key(AW9136_ts->input_dev, KEY_F1, 1);
	input_sync(AW9136_ts->input_dev);
	input_report_key(AW9136_ts->input_dev, KEY_F1, 0);
	input_sync(AW9136_ts->input_dev);
}


/////////////////////////////////////////////
// report right slip 
/////////////////////////////////////////////
void AW_right_slip(void)
{
	printk("AW9136 right slip \n");
	input_report_key(AW9136_ts->input_dev, KEY_F2, 1);
	input_sync(AW9136_ts->input_dev);
	input_report_key(AW9136_ts->input_dev, KEY_F2, 0);
	input_sync(AW9136_ts->input_dev);
}


/////////////////////////////////////////////
// report MENU-BTN double click 
/////////////////////////////////////////////
void AW_left_double(void)
{
	printk("AW9136 Left double click \n");
	input_report_key(AW9136_ts->input_dev, KEY_PREVIOUSSONG, 1);
	input_sync(AW9136_ts->input_dev);
	input_report_key(AW9136_ts->input_dev, KEY_PREVIOUSSONG, 0);
	input_sync(AW9136_ts->input_dev);
}

/////////////////////////////////////////////
// report HOME-BTN double click 
/////////////////////////////////////////////
void AW_center_double(void)
{
	printk("AW9136 Center double click \n");
	input_report_key(AW9136_ts->input_dev, KEY_F3, 1);
	input_sync(AW9136_ts->input_dev);
	input_report_key(AW9136_ts->input_dev, KEY_F3, 0);
	input_sync(AW9136_ts->input_dev);
}
/////////////////////////////////////////////
// report BACK-BTN double click 
/////////////////////////////////////////////
void AW_right_double(void)
{
	printk("AW9136 Right_double click \n");
	input_report_key(AW9136_ts->input_dev, KEY_NEXTSONG, 1);
	input_sync(AW9136_ts->input_dev);
	input_report_key(AW9136_ts->input_dev, KEY_NEXTSONG, 0);
	input_sync(AW9136_ts->input_dev);
}

/////////////////////////////////////////////
// report MENU-BTN single click 
/////////////////////////////////////////////
void AW_left_press(void)
{
	input_report_key(AW9136_ts->input_dev, KEY_APPSELECT, 1);
	input_sync(AW9136_ts->input_dev);
	printk("AW9136 left press \n");
}

/////////////////////////////////////////////
// report HOME-BTN single click 
/////////////////////////////////////////////
void AW_center_press(void)
{
	/*printk("AW9136 center press \n");
	input_report_key(AW9136_ts->input_dev, KEY_HOMEPAGE, 1);
	input_sync(AW9136_ts->input_dev);*/
}
/////////////////////////////////////////////
// report BACK-BTN single click 
/////////////////////////////////////////////
void AW_right_press(void)
{
	input_report_key(AW9136_ts->input_dev, KEY_BACK, 1);
	input_sync(AW9136_ts->input_dev);
	printk("AW9136 right press \n");
}

/////////////////////////////////////////////
// report MENU-BTN single click 
/////////////////////////////////////////////
void AW_left_release(void)
{
	input_report_key(AW9136_ts->input_dev, KEY_APPSELECT, 0);
	input_sync(AW9136_ts->input_dev);
	printk("AW9136 left release\n");
}

/////////////////////////////////////////////
// report HOME-BTN single click 
/////////////////////////////////////////////
void AW_center_release(void)
{
	
	/*input_report_key(AW9136_ts->input_dev, KEY_HOMEPAGE, 0);
	input_sync(AW9136_ts->input_dev);
	printk("AW9136 center release \n");*/

}
/////////////////////////////////////////////
// report BACK-BTN single click 
/////////////////////////////////////////////
void AW_right_release(void)
{

	input_report_key(AW9136_ts->input_dev, KEY_BACK, 0);
	input_sync(AW9136_ts->input_dev);
	printk("AW9136 right release \n");
}


////////////////////////////////////////////////////
//
// Function : Cap-touch main program @ mobile sleep 
//            wake up after double-click/right_slip/left_slip
//
////////////////////////////////////////////////////
void AW_SleepMode_Proc(void)
{
	unsigned int buff1;

	//printk("%d Enter\n", __func__);
	printk("AW_SleepMode_Proc \n");
	
#ifdef AW_AUTO_CALI
	if(cali_flag){
		AW91xx_Auto_Cali();
		if(cali_flag == 0){	
			if(cali_used){
				I2C_write_reg(GCR,0x0000);	 // disable chip
			}
			I2C_write_reg(INTER,S_INTER);			
			I2C_write_reg(GIER,S_GIER);
			if(cali_used){
				I2C_write_reg(GCR,S_GCR);	 // enable chip
			}
		}
		return ;
	}
#endif
	
	if(debug_level == 0){
		buff1=I2C_read_reg(0x2e);			//read gesture interupt status
		if(buff1 == 0x10){
				AW_center_double();
		}else if(buff1 == 0x01){
				AW_right_slip();
		}else if (buff1 == 0x02){
				AW_left_slip();					
		}
	}
}

////////////////////////////////////////////////////
//
// Function : Cap-touch main pragram @ mobile normal state
//            press/release
//
////////////////////////////////////////////////////
void AW_NormalMode_Proc(void)
{
	unsigned int buff1,buff2;
	printk("AW_NormalMode_Proc \n");
	//printk("%d Enter\n", __func__);
	
#ifdef AW_AUTO_CALI	
	if(cali_flag){
		AW91xx_Auto_Cali();
		if(cali_flag == 0){	
			if(cali_used){
				I2C_write_reg(GCR,0x0000);	 // disable chip
			}
			I2C_write_reg(INTER,N_INTER);			
			I2C_write_reg(GIER,N_GIER);
			if(cali_used){
				I2C_write_reg(GCR,N_GCR);	 // enable chip
			}
		}
		return ;
	}
#endif
	if(debug_level == 0){
		buff2=I2C_read_reg(0x32);		//read key interupt status
		buff1=I2C_read_reg(0x31);
	
		printk("AW9136: reg0x31=0x%x, reg0x32=0x%x\n",buff1,buff2);
		
		if(buff2 & 0x10){						//S3 click
				if(buff1 == 0x00){
						//AW_left_release();
						AW_center_release();
				}else if(buff1 == 0x10){
						//AW_left_press();
						AW_center_press();
				}
		}else if(buff2 & 0x08){					//S2 click
				if(buff1 == 0x00){
						//AW_center_release();
						AW_left_release();
				}else if (buff1 == 0x08){
						//AW_center_press();
						AW_left_press();
				}
		}else if(buff2 & 0x04){					//S1 click
				if(buff1 == 0x00){
					   AW_right_release();
				}else if(buff1 == 0x04){
					 AW_right_press();
				}
		}
	}

}


static int AW9136_ts_clear_intr(struct i2c_client *client) 
{
	int res;

	res = I2C_read_reg(0x32);

	printk("%s: reg0x32=0x%x\n", __func__, res);

	return 0;
}

////////////////////////////////////////////////////
//
// Function : Interrupt sub-program
//            work in AW_SleepMode_Proc() or 
//            AW_NormalMode_Proc()
//
////////////////////////////////////////////////////
static void AW9136_ts_eint_work(struct work_struct *work)
{
	//printk("%d Enter\n", __func__);
	printk("AW9136_ts_eint_work \n");
	switch(WorkMode){
		case 1:	AW_SleepMode_Proc();	break;
		case 2:	AW_NormalMode_Proc();	break;
		default:	break;
	}
	
	AW9136_ts_clear_intr(aw9136_i2c_client);

	enable_irq(AW9136_ts->irq);
}


static irqreturn_t AW9136_ts_eint_func(int irq, void *desc)
{	
	printk("%s Enter\n", __func__);
	
	disable_irq_nosync(irq);

	if(AW9136_ts == NULL){
		printk("AW9136_ts == NULL");
		return  IRQ_NONE;
	}

	schedule_work(&AW9136_ts->eint_work);

	return IRQ_HANDLED;

}


int AW9136_ts_setup_eint(void)
{
	int ret = 0;
	u32 ints[2] = {0, 0};

	aw9136_int_pin = pinctrl_lookup_state(aw9136ctrl, "aw9136_int_pin");
	if (IS_ERR(aw9136_int_pin)) {
		ret = PTR_ERR(aw9136_int_pin);
		pr_debug("%s : pinctrl err, aw9136_int_pin\n", __func__);
	}

	if (AW9136_ts->irq_node) {
		of_property_read_u32_array(AW9136_ts->irq_node, "debounce", ints, ARRAY_SIZE(ints));
		gpio_set_debounce(ints[0], ints[1]);
		pinctrl_select_state(aw9136ctrl, aw9136_int_pin);
		printk("%s ints[0] = %d, ints[1] = %d!!\n", __func__, ints[0], ints[1]);

		AW9136_ts->irq = irq_of_parse_and_map(AW9136_ts->irq_node, 0);
		printk("%s irq = %d\n", __func__, AW9136_ts->irq);
		if (!AW9136_ts->irq) {
			printk("%s irq_of_parse_and_map fail!!\n", __func__);
			return -EINVAL;
		}
		if (request_irq(AW9136_ts->irq, AW9136_ts_eint_func, IRQ_TYPE_EDGE_FALLING, "AW9136-eint", NULL)) {
			printk("%s IRQ LINE NOT AVAILABLE!!\n", __func__);
			return -EINVAL;
		}
		enable_irq(AW9136_ts->irq);
	} else {
		printk("null irq node!!\n");
		return -EINVAL;
	}

    return 0;
}

////////////////////////////////////////////////////
//
// Function : AW9136 initial @ mobile goto sleep mode
//            enter SleepMode
//
////////////////////////////////////////////////////
///*
static int AW9136_i2c_suspend(struct i2c_client *client, pm_message_t msg) 
//static int AW9136_i2c_suspend(struct device *dev)

{
	printk("%s yy start\n", __func__);
	if(WorkMode != 1){
		AW_SleepMode();
		suspend_flag = 1;
#ifdef AW_AUTO_CALI
		cali_flag = 1;
		cali_num = 0;
		cali_cnt = 0;
#endif			
	}
	printk("%s yy Finish\n", __func__);

	return 0;
}
//*/
////////////////////////////////////////////////////
//
// Function : AW9136 initial @ mobile wake up
//            enter NormalMode 
//
////////////////////////////////////////////////////
///*
static int AW9136_i2c_resume(struct i2c_client *client)
//static int AW9136_i2c_resume(struct device *dev)
{	
	printk("%s yy start\n", __func__);
	if(WorkMode != 2){
		AW_NormalMode();
		suspend_flag = 0;
#ifdef AW_AUTO_CALI
		cali_flag = 1;
		cali_num = 0;
		cali_cnt = 0;
#endif				
	}
	printk("%s yy Finish\n", __func__);

	return 0;
}
//*/
//static SIMPLE_DEV_PM_OPS(AW9136_pm_ops, AW9136_i2c_suspend, AW9136_i2c_resume);
//SIMPLE_DEV_PM_OPS(AW9136_pm_ops, AW9136_i2c_suspend, AW9136_i2c_resume);
////////////////////////////////////////////////////
//
// Function : AW9136 initial @ mobile power on
//            enter NormalMode directly
//
////////////////////////////////////////////////////
static int AW9136_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id)
{

	struct input_dev *input_dev;
	int err = 0;
	int count =0;
	unsigned int reg_value,reg_value1; 
	
	printk("%s Enter\n", __func__);

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		err = -ENODEV;
		goto exit_check_functionality_failed;
	}

	printk("%s: kzalloc\n", __func__);
	AW9136_ts = kzalloc(sizeof(*AW9136_ts), GFP_KERNEL);
	if (!AW9136_ts)	{
		err = -ENOMEM;
		goto exit_alloc_data_failed;
	}

	AW9136_ts_config_pins();

	client->addr = 0x2C;								// chip  I2C address
	client->timing= 400;
	aw9136_i2c_client = client;
	i2c_set_clientdata(client, AW9136_ts);

	printk("I2C addr=%x", client->addr);
	for(count = 0; count < 5; count++){
		reg_value = I2C_read_reg(0x00);				//read chip ID
		printk("AW9136 chip ID = 0x%4x", reg_value);

		if (reg_value == 0xb223)
			break;

		msleep(20);

		if(count == 4) {
			err = -ENODEV;
			goto exit_create_singlethread;
		}
	}
	if (reg_value == 0xb223)
	{
		AW9136_ts->irq_node = of_find_compatible_node(NULL, NULL, "mediatek, aw9136-eint");
		//AW9136_ts->irq_node = of_find_compatible_node(NULL, NULL, "mediatek,aw9136");
		INIT_WORK(&AW9136_ts->eint_work, AW9136_ts_eint_work);
	}
	aw9136_resume_workqueue = create_singlethread_workqueue("aw9136_resume");

	INIT_WORK(&aw9136_resume_work, aw9136_resume_workqueue_callback);
	aw9136_fb_notifier.notifier_call = aw9136_fb_notifier_callback;
	if (fb_register_client(&aw9136_fb_notifier))
		printk("register fb_notifier fail!\n");
	
	input_dev = input_allocate_device();
	if (!input_dev){
		err = -ENOMEM;
		dev_err(&client->dev, "failed to allocate input device\n");
		goto exit_input_dev_alloc_failed;
	}
	
	AW9136_ts->input_dev = input_dev;


	//__set_bit(EV_KEY, input_dev->evbit);
	//__set_bit(EV_SYN, input_dev->evbit);
	
	//__set_bit(KEY_HOMEPAGE, input_dev->keybit);
	__set_bit(KEY_APPSELECT, input_dev->keybit);
	__set_bit(KEY_BACK, input_dev->keybit);
/*
	__set_bit(KEY_PREVIOUSSONG, input_dev->keybit);
	__set_bit(KEY_NEXTSONG, input_dev->keybit);

	__set_bit(KEY_F1, input_dev->keybit);
	__set_bit(KEY_F2, input_dev->keybit);
	__set_bit(KEY_F3, input_dev->keybit);
	__set_bit(KEY_F4, input_dev->keybit);
	__set_bit(KEY_F5, input_dev->keybit);
	__set_bit(KEY_F6, input_dev->keybit);
	__set_bit(KEY_F7, input_dev->keybit);
	__set_bit(KEY_F8, input_dev->keybit);
	__set_bit(KEY_F9, input_dev->keybit);
	__set_bit(KEY_F10, input_dev->keybit);
	__set_bit(KEY_F11, input_dev->keybit);
	__set_bit(KEY_F12, input_dev->keybit);
	*/
	
	input_dev->name		= AW9136_ts_NAME;		//dev_name(&client->dev)
	err = input_register_device(input_dev);
	if (err) {
		dev_err(&client->dev,
		"AW9136_i2c_probe: failed to register input device: %s\n",
		dev_name(&client->dev));
		goto exit_input_register_device_failed;
	}	
	input_set_capability(input_dev,EV_KEY,KEY_APPSELECT);
	input_set_capability(input_dev,EV_KEY,KEY_BACK);
	//input_set_capability(input_dev,EV_KEY,KEY_HOMEPAGE);
	AW9136_create_sysfs(client);
    
	WorkMode = 2;
	AW_NormalMode();

#ifdef AW_AUTO_CALI
	cali_flag = 1;
	cali_num = 0;
	cali_cnt = 0;
#endif
	reg_value1 = I2C_read_reg(0x01);
 	printk("AW9136 GCR = 0x%4x", reg_value1);
	if (reg_value == 0xb223)
	{
		AW9136_ts_setup_eint();
	}
	printk("%s Over\n", __func__);
    return 0;

exit_input_register_device_failed:
	input_free_device(input_dev);
exit_input_dev_alloc_failed:
	//free_irq(client->irq, AW9136_ts);
	cancel_work_sync(&AW9136_ts->eint_work);
exit_create_singlethread:
	printk("==singlethread error =\n");
	i2c_set_clientdata(client, NULL);
	kfree(AW9136_ts);
exit_alloc_data_failed:
exit_check_functionality_failed:
	//sprd_free_gpio_irq(AW9136_ts_setup.irq);
	return err;
}
/***********************************************************************************************
Name	:	 

Input	:	
                     

Output	:	

function	:	

***********************************************************************************************/
static int AW9136_i2c_remove(struct i2c_client *client)
{

	struct AW9136_ts_data *AW9136_ts = i2c_get_clientdata(client);

	printk("==AW9136_ts_remove=\n");
	
	input_unregister_device(AW9136_ts->input_dev);
	
	kfree(AW9136_ts);
	
	i2c_set_clientdata(client, NULL);
	return 0;
}

static const struct i2c_device_id AW9136_i2c_id[] = {
	//{ AW9136_ts_NAME, 0 },
	{"aw9136_ts", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, AW9136_ts_id);


//#if 1//#ifdef CONFIG_OF//CONFIG_OF=y
static const struct of_device_id AW9136_of_match[] = {
	//{.compatible = "awinic,touchkey"},
	//{.compatible = "mediatek,aw9136"},
	{.compatible = "aw9136_ts"},
	{},
};
//#endif
static struct i2c_driver AW9136_i2c_driver = {
	.probe		= AW9136_i2c_probe,
	.remove		= AW9136_i2c_remove,
	.id_table	= AW9136_i2c_id,
	//.suspend    = AW9136_i2c_suspend,
	//.resume     = AW9136_i2c_resume,
	.driver	= {
		.name	= "aw9136_ts",
		.owner	= THIS_MODULE,
//#if 1//#ifdef CONFIG_OF//CONFIG_OF=y
		.of_match_table = AW9136_of_match,
//#endif	
		//.pm = &AW9136_pm_ops,
	},
};


//////////////////////////////////////////////////////
//
// for adb shell and APK debug
//
//////////////////////////////////////////////////////
static ssize_t AW9136_show_debug(struct device* cd,struct device_attribute *attr, char* buf);
static ssize_t AW9136_store_debug(struct device* cd, struct device_attribute *attr,const char* buf, size_t len);
static ssize_t AW9136_get_reg(struct device* cd,struct device_attribute *attr, char* buf);
static ssize_t AW9136_write_reg(struct device* cd, struct device_attribute *attr,const char* buf, size_t len);
static ssize_t AW9136_get_adbBase(struct device* cd,struct device_attribute *attr, char* buf);
static ssize_t AW9136_get_rawdata(struct device* cd,struct device_attribute *attr, char* buf);
static ssize_t AW9136_get_delta(struct device* cd,struct device_attribute *attr, char* buf);
static ssize_t AW9136_get_irqstate(struct device* cd,struct device_attribute *attr, char* buf);
	

static DEVICE_ATTR(debug, 0664, AW9136_show_debug, AW9136_store_debug);
static DEVICE_ATTR(getreg, 0664, AW9136_get_reg, AW9136_write_reg);
static DEVICE_ATTR(adbbase, 0664, AW9136_get_adbBase, NULL);
static DEVICE_ATTR(rawdata, 0664, AW9136_get_rawdata, NULL);
static DEVICE_ATTR(delta, 0664, AW9136_get_delta, NULL);
static DEVICE_ATTR(getstate, 0664, AW9136_get_irqstate, NULL);


static ssize_t AW9136_show_debug(struct device* cd,struct device_attribute *attr, char* buf)
{
	ssize_t ret = 0;
	
	sprintf(buf, "AW9136 Debug %d\n",debug_level);
	
	ret = strlen(buf) + 1;

	return ret;
}

static ssize_t AW9136_store_debug(struct device* cd, struct device_attribute *attr,
		       const char* buf, size_t len)
{
	unsigned long on_off = simple_strtoul(buf, NULL, 10);
	debug_level = on_off;

	printk("%s: debug_level=%d\n",__func__, debug_level);
	
	return len;
}



static ssize_t AW9136_get_reg(struct device* cd,struct device_attribute *attr, char* buf)
{
	unsigned int reg_val[1];
	ssize_t len = 0;
	u8 i;
	disable_irq(AW9136_ts->irq);
	for(i=1;i<0x7F;i++) {
		reg_val[0] = I2C_read_reg(i);
		len += snprintf(buf+len, PAGE_SIZE-len, "reg%2X = 0x%4X, ", i,reg_val[0]);
	}
	enable_irq(AW9136_ts->irq);
	return len;
}

static ssize_t AW9136_write_reg(struct device* cd, struct device_attribute *attr,
		       const char* buf, size_t len)
{

	unsigned int databuf[2];
	disable_irq(AW9136_ts->irq);
	if(2 == sscanf(buf,"%x %x",&databuf[0], &databuf[1])) {
		I2C_write_reg((u8)databuf[0],databuf[1]);
	}
	enable_irq(AW9136_ts->irq);
	return len;
}

static ssize_t AW9136_get_adbBase(struct device* cd,struct device_attribute *attr, char* buf)
{
	unsigned int dataS1,dataS2,dataS3,dataS4,dataS5,dataS6;
	ssize_t len = 0;

	disable_irq(AW9136_ts->irq);
	len += snprintf(buf+len, PAGE_SIZE-len, "base: \n");
	I2C_write_reg(MCR,0x0003);

	dataS1=I2C_read_reg(0x36);
	dataS2=I2C_read_reg(0x37);
	dataS3=I2C_read_reg(0x38);
	dataS4=I2C_read_reg(0x39);
	dataS5=I2C_read_reg(0x3a);
	dataS6=I2C_read_reg(0x3b);
	len += snprintf(buf+len, PAGE_SIZE-len, "%d, ",dataS1);
	len += snprintf(buf+len, PAGE_SIZE-len, "%d, ",dataS2);
	len += snprintf(buf+len, PAGE_SIZE-len, "%d, ",dataS3);
	len += snprintf(buf+len, PAGE_SIZE-len, "%d, ",dataS4);
	len += snprintf(buf+len, PAGE_SIZE-len, "%d, ",dataS5);
	len += snprintf(buf+len, PAGE_SIZE-len, "%d, ",dataS6);

	len += snprintf(buf+len, PAGE_SIZE-len, "\n");

	enable_irq(AW9136_ts->irq);
	return len;
}

static ssize_t AW9136_get_rawdata(struct device* cd,struct device_attribute *attr, char* buf)
{
	unsigned int dataS1,dataS2,dataS3,dataS4,dataS5,dataS6;
	ssize_t len = 0;

	disable_irq(AW9136_ts->irq);
	len += snprintf(buf+len, PAGE_SIZE-len, "base: \n");
	I2C_write_reg(MCR,0x0003);

	dataS1=I2C_read_reg(0x36);
	dataS2=I2C_read_reg(0x37);
	dataS3=I2C_read_reg(0x38);
	dataS4=I2C_read_reg(0x39);
	dataS5=I2C_read_reg(0x3a);
	dataS6=I2C_read_reg(0x3b);
	len += snprintf(buf+len, PAGE_SIZE-len, "%d, ",dataS1);
	len += snprintf(buf+len, PAGE_SIZE-len, "%d, ",dataS2);
	len += snprintf(buf+len, PAGE_SIZE-len, "%d, ",dataS3);
	len += snprintf(buf+len, PAGE_SIZE-len, "%d, ",dataS4);
	len += snprintf(buf+len, PAGE_SIZE-len, "%d, ",dataS5);
	len += snprintf(buf+len, PAGE_SIZE-len, "%d, ",dataS6);

	len += snprintf(buf+len, PAGE_SIZE-len, "\n");

	enable_irq(AW9136_ts->irq);
	return len;
}

static ssize_t AW9136_get_delta(struct device* cd,struct device_attribute *attr, char* buf)
{
	unsigned int deltaS1,deltaS2,deltaS3,deltaS4,deltaS5,deltaS6;
	ssize_t len = 0;

	disable_irq(AW9136_ts->irq);
	len += snprintf(buf+len, PAGE_SIZE-len, "delta: \n");
	I2C_write_reg(MCR,0x0001);

	deltaS1=I2C_read_reg(0x36);if((deltaS1 & 0x8000) == 0x8000) { deltaS1 = 0; }
	deltaS2=I2C_read_reg(0x37);if((deltaS2 & 0x8000) == 0x8000) { deltaS2 = 0; }
	deltaS3=I2C_read_reg(0x38);if((deltaS3 & 0x8000) == 0x8000) { deltaS3 = 0; }
	deltaS4=I2C_read_reg(0x39);if((deltaS4 & 0x8000) == 0x8000) { deltaS4 = 0; }
	deltaS5=I2C_read_reg(0x3a);if((deltaS5 & 0x8000) == 0x8000) { deltaS5 = 0; }
	deltaS6=I2C_read_reg(0x3b);if((deltaS6 & 0x8000) == 0x8000) { deltaS6 = 0; }
	len += snprintf(buf+len, PAGE_SIZE-len, "%d, ",deltaS1);
	len += snprintf(buf+len, PAGE_SIZE-len, "%d, ",deltaS2);
	len += snprintf(buf+len, PAGE_SIZE-len, "%d, ",deltaS3);
	len += snprintf(buf+len, PAGE_SIZE-len, "%d, ",deltaS4);
	len += snprintf(buf+len, PAGE_SIZE-len, "%d, ",deltaS5);
	len += snprintf(buf+len, PAGE_SIZE-len, "%d, ",deltaS6);

	len += snprintf(buf+len, PAGE_SIZE-len, "\n");

	enable_irq(AW9136_ts->irq);
	return len;
}

static ssize_t AW9136_get_irqstate(struct device* cd,struct device_attribute *attr, char* buf)
{
	unsigned int keytouch,keyS1,keyS2,keyS3,keyS4,keyS5,keyS6;
	unsigned int gesture,slide1,slide2,slide3,slide4,doubleclick1,doubleclick2;
	ssize_t len = 0;

	disable_irq(AW9136_ts->irq);
	len += snprintf(buf+len, PAGE_SIZE-len, "keytouch: \n");

	keytouch=I2C_read_reg(0x31);
	if((keytouch&0x1) == 0x1) keyS1=1;else keyS1=0;
	if((keytouch&0x2) == 0x2) keyS2=1;else keyS2=0;
	if((keytouch&0x4) == 0x4) keyS3=1;else keyS3=0;
	if((keytouch&0x8) == 0x8) keyS4=1;else keyS4=0;
	if((keytouch&0x10) == 0x10) keyS5=1;else keyS5=0;
	if((keytouch&0x20) == 0x20) keyS6=1;else keyS6=0;

	len += snprintf(buf+len, PAGE_SIZE-len, "%d, ",keyS1);
	len += snprintf(buf+len, PAGE_SIZE-len, "%d, ",keyS2);
	len += snprintf(buf+len, PAGE_SIZE-len, "%d, ",keyS3);
	len += snprintf(buf+len, PAGE_SIZE-len, "%d, ",keyS4);
	len += snprintf(buf+len, PAGE_SIZE-len, "%d, ",keyS5);
	len += snprintf(buf+len, PAGE_SIZE-len, "%d, ",keyS6);
	len += snprintf(buf+len, PAGE_SIZE-len, "\n");

	len += snprintf(buf+len, PAGE_SIZE-len, "gesture: \n");			
	gesture=I2C_read_reg(0x2e);
	if(gesture == 0x1) slide1=1;else slide1=0;
	if(gesture == 0x2) slide2=1;else slide2=0;
	if(gesture == 0x4) slide3=1;else slide3=0;
	if(gesture == 0x8) slide4=1;else slide4=0;
	if(gesture == 0x10) doubleclick1=1;else doubleclick1=0;
	if(gesture == 0x200) doubleclick2=1;else doubleclick2=0;

	len += snprintf(buf+len, PAGE_SIZE-len, "%d, ",slide1);
	len += snprintf(buf+len, PAGE_SIZE-len, "%d, ",slide2);
	len += snprintf(buf+len, PAGE_SIZE-len, "%d, ",slide3);
	len += snprintf(buf+len, PAGE_SIZE-len, "%d, ",slide4);
	len += snprintf(buf+len, PAGE_SIZE-len, "%d, ",doubleclick1);
	len += snprintf(buf+len, PAGE_SIZE-len, "%d, ",doubleclick2);

	len += snprintf(buf+len, PAGE_SIZE-len, "\n");

	enable_irq(AW9136_ts->irq);
	return len;
}


static int AW9136_create_sysfs(struct i2c_client *client)
{
	int err;
	struct device *dev = &(client->dev);

	printk("%s", __func__);
	
	err = device_create_file(dev, &dev_attr_debug);
	err = device_create_file(dev, &dev_attr_getreg);
	err = device_create_file(dev, &dev_attr_adbbase);
	err = device_create_file(dev, &dev_attr_rawdata);
	err = device_create_file(dev, &dev_attr_delta);
	err = device_create_file(dev, &dev_attr_getstate);
	return err;
}
static void aw9136_resume_workqueue_callback(struct work_struct *work)
{
	printk("GTP touch_resume_workqueue_callback\n");
	//aw9136_ts_driver.resume(NULL);
	 AW9136_i2c_resume(NULL);
	aw9136_suspend_flag = 0;
}
static int aw9136_fb_notifier_callback(struct notifier_block *self, unsigned long event, void *data)
{
	struct fb_event *evdata = NULL;
	int blank;
	int err = 0;

	printk("aw9136_fb_notifier_callback\n");

	evdata = data;
	/* If we aren't interested in this event, skip it immediately ... */
	if (event != FB_EVENT_BLANK)
		return 0;

	blank = *(int *)evdata->data;
	printk("fb_notify(blank=%d)\n", blank);
	switch (blank) {
	case FB_BLANK_UNBLANK:
		printk("LCD ON Notify\n");
		if ( aw9136_suspend_flag) {
			err = queue_work(aw9136_resume_workqueue, &aw9136_resume_work);
			if (!err) {
				printk("start touch_resume_workqueue failed\n");
				return err;
			}
		}
		break;
	case FB_BLANK_POWERDOWN:
		printk("LCD OFF Notify\n");
		//if (&aw9136_ts_driver) {
			err = cancel_work_sync(&aw9136_resume_work);
			if (!err)
				printk("cancel touch_resume_workqueue err = %d\n", err);
			aw9136_mesg.event=0;
			//aw9136_ts_driver.suspend(NULL,aw9136_mesg);
			AW9136_i2c_suspend(NULL,aw9136_mesg);
		//}
		aw9136_suspend_flag = 1;
		break;
	default:
		break;
	}
	return 0;
}
/***********************************************************************************************
Name	:	 

Input	:	
                     

Output	:	

function	:	

***********************************************************************************************/
static int AW9136_ts_remove(struct platform_device *pdev)
{
	printk("%s start!\n", __func__);
	i2c_del_driver(&AW9136_i2c_driver);
	return 0;
}
//static struct i2c_board_info i2c_aw9136 __initdata = {I2C_BOARD_INFO("aw9136_ts", 0x2c) }; 

static int AW9136_ts_probe(struct platform_device *pdev)
{
	int ret;

	printk("%s start!\n", __func__);
	
	ret = AW9136_gpio_init(pdev);
	if (ret != 0) {
		printk("[%s] failed to init AW9136 pinctrl.\n", __func__);
		//return ret;
	} else {
		printk("[%s] Success to init AW9136 pinctrl.\n", __func__);
	}
	
	//i2c_register_board_info(2, &i2c_aw9136, 1);
	
	ret = i2c_add_driver(&AW9136_i2c_driver);
	if (ret != 0) {
		printk("[%s] failed to register AW9136 i2c driver.\n", __func__);
		//return ret;
	} else {
		printk("[%s] Success to register AW9136 i2c driver.\n", __func__);
	}

	return 0;
}
/*
struct platform_device AW9136_ts_device = {
	.name = "aw9136_ts-user",
	.id = -1,
};
*/
//#if 1//#ifdef CONFIG_OF//CONFIG_OF=y
static const struct of_device_id aw9136plt_of_match[] = {
	{.compatible = "aw9136_ts"},
	//{.compatible = "mediatek,aw9136"},
	{},
};
//#endif

static struct platform_driver AW9136_ts_driver = {
		.probe	 = AW9136_ts_probe,
		.remove	 = AW9136_ts_remove,
        .driver = {
                //.name   = "aw9136_ts-user",
				.name   = "aw9136_ts",
		//.name   = "mediatek,aw9136",
//#if 1//#ifdef CONFIG_OF//CONFIG_OF=y
		.of_match_table = aw9136plt_of_match,
//#endif
        }
};
/***********************************************************************************************
Name	:	 

Input	:	
                     

Output	:	

function	:	

***********************************************************************************************/
static int __init AW9136_ts_init(void)
{
	int ret;
	printk("%s Enter\n", __func__);
	/*
	ret = platform_device_register(&AW9136_ts_device);
	if (ret) {
		printk("****[%s] Unable to register device (%d)\n", __func__, ret);
		return ret;
	}
	*/
	
	ret = platform_driver_register(&AW9136_ts_driver);	
	if (ret) {
		printk("****[%s] Unable to register driver (%d)\n", __func__, ret);
		return ret;
	}		

	return ret;
}

/***********************************************************************************************
Name	:	 

Input	:	
                     

Output	:	

function	:	

***********************************************************************************************/
static void __exit AW9136_ts_exit(void)
{
	printk("%s Enter\n", __func__);
	platform_driver_unregister(&AW9136_ts_driver);	
}





module_init(AW9136_ts_init);
module_exit(AW9136_ts_exit);

MODULE_AUTHOR("<lijunjiea@AWINIC.com>");
MODULE_DESCRIPTION("AWINIC AW9136 Touch driver");
MODULE_LICENSE("GPL");
