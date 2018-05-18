/*Transsion Top Secret*/
/*******************************************************************************
 *
 *Copyright (c) 2014-2015, The Linux Foundation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 *******************************************************************************/

/*******************************************************************************
 * Filename:wusb3801.c
 *
 * Author:
 * -------
 * ZhaoHongguang, hongguang.zhao@wecorp.com.cn oct/10/2015
 * 
 *******************************************************************************/

#include <alsps.h>
#include <cust_alsps.h>
#include "ltr579.h"
#include <linux/gpio.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_device.h>
#include <linux/of_gpio.h>
#include <linux/of_irq.h>
/******************************************************************************
 * configuration
*******************************************************************************/
#define LTR579_DEV_NAME     	"ltr579"

#define APS_TAG                  		"[ALS/PS] "
#define APS_FUN(f)               	pr_debug(APS_TAG"%s\n", __func__)
#define APS_ERR(fmt, args...)    	pr_err(APS_TAG"%s %d : "fmt, __func__, __LINE__, ##args)
#define APS_LOG(fmt, args...)   	pr_debug(APS_TAG fmt, ##args)
#define APS_DBG(fmt, args...)    	pr_debug(APS_TAG fmt, ##args)

#define I2C_FLAG_WRITE	0
#define I2C_FLAG_READ	1

#define GN_MTK_BSP_PS_DYNAMIC_CALI
//extern int (*get_psval)(void);
int ltr559_get_psval(void);
int ltr579_CMM_PPCOUNT_VALUE = 0x08;
int ltr579_CMM_CONTROL_VALUE = 0xE4;
int ZOOM_TIME = 4;
int dynamic_cali = 2047;
static u16 last_min_value = 2047;
static int als_gainrange;
//unsigned int alsps_int_gpio_number = 0;
//static int of_get_ltr579_platform_data(struct device *dev);

struct alsps_hw alsps_cust;
static struct alsps_hw *hw = &alsps_cust;
struct platform_device *alspsPltFmDev;

/* For alsp driver get cust info */
struct alsps_hw *get_cust_alsps(void)
{
	return &alsps_cust;
}

static struct i2c_client *ltr579_i2c_client;
static const struct i2c_device_id ltr579_i2c_id[] = { {LTR579_DEV_NAME, 0}, {} };
static int ltr579_init_flag = -1;
static volatile unsigned long intr_flag = 0;//added by peixuan.qiu

static DEFINE_MUTEX(ltr579_mutex);

/*------------------------------------------------------------------------*/
static int ltr579_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id);
static int ltr579_i2c_remove(struct i2c_client *client);
static int ltr579_i2c_detect(struct i2c_client *client, struct i2c_board_info *info);
static int ltr579_i2c_suspend(struct i2c_client *client, pm_message_t msg);
static int ltr579_i2c_resume(struct i2c_client *client);
static int ltr579_remove(void);
static int ltr579_local_init(void);
static int ltr579_read_ps(struct i2c_client * client,u16 * data);
static int ltr579_setup_eint(struct i2c_client *client);
static int set_psensor_threshold(struct i2c_client *client);
static int ltr579_i2c_write_reg(u8 regnum, u8 value);
static int ltr579_i2c_read_reg(u8 regnum);
/*------------------------------------------------------------------------*/

struct PS_CALI_DATA_STRUCT {
	int close;
	int far_away;
	int valid;
};

static struct PS_CALI_DATA_STRUCT ps_cali = { 0, 0, 0 };

static int intr_flag_value;
static unsigned long long int_top_time;

enum {
	CMC_BIT_ALS = 1,//number 1 indicates als has been enabled
	CMC_BIT_PS = 2,//number 2 indicates ps has been enabled
};

enum {
	CMC_TRC_ALS_DATA = 0x0001,
	CMC_TRC_PS_DATA  = 0x0002,
	CMC_TRC_EINT	 = 0x0004,
	CMC_TRC_IOCTL    = 0x0008,
	CMC_TRC_I2C	     = 0x0010,
	CMC_TRC_CVT_ALS  = 0x0020,
	CMC_TRC_CVT_PS   = 0x0040,
	CMC_TRC_CVT_AAL  = 0x0080,
	CMC_TRC_DEBUG    = 0x8000,
} CMC_TRC;

struct ltr579_i2c_addr {	/*define a series of i2c slave address */
	u8 write_addr;
	u8 ps_thd;		/*PS INT threshold */
};

struct ltr579_priv {
	struct alsps_hw *hw;
	struct i2c_client *client;
	struct work_struct irq_work;

	/*i2c address group */
	struct ltr579_i2c_addr addr;

	/*misc */
	u16 		als_modulus;
	atomic_t 	i2c_retry;
	atomic_t 	als_suspend;
	atomic_t 	als_debounce;	/*debounce time after enabling als */
	atomic_t 	als_deb_on;	/*indicates if the debounce is on */
	atomic_t	als_deb_end;	/*the jiffies representing the end of debounce */
	atomic_t 	ps_mask;	/*mask ps: always return far away */
	atomic_t 	ps_debounce;	/*debounce time after enabling ps */
	atomic_t 	ps_deb_on;	/*indicates if the debounce is on */
	atomic_t 	ps_deb_end;	/*the jiffies representing the end of debounce */
	atomic_t 	ps_suspend;
	atomic_t	trace;
	atomic_t  init_done;
	struct device_node *irq_node;
	int		irq;

	/*data */
	u16 als;
	u16 ps;
	u8 _align;
	u16 als_level_num;
	u16 als_value_num;
	u32 als_level[C_CUST_ALS_LEVEL - 1];
	u32 als_value[C_CUST_ALS_LEVEL];
	int ps_cali;

	atomic_t als_cmd_val;	/*the cmd value can't be read, stored in ram */
	atomic_t ps_cmd_val;	/*the cmd value can't be read, stored in ram */
	atomic_t ps_thd_val_high;	/*the cmd value can't be read, stored in ram */
	atomic_t ps_thd_val_low;	/*the cmd value can't be read, stored in ram */
	ulong enable;		/*enable mask <====> ps enable and als eanble*/
	ulong pending_intr;	/*pending interrupt */

	/*early suspend */
#if defined(CONFIG_HAS_EARLYSUSPEND)
//deprecated
	struct early_suspend early_drv;
#endif
};

#if 1//#ifdef CONFIG_OF//CONFIG_OF=y
static const struct of_device_id alsps_of_match[] = {
	{.compatible = "mediatek,alsps"},
	{},
};
#endif
/*----------------------------------------------------------------------------*/
static struct i2c_driver ltr579_i2c_driver = {
	.probe = ltr579_i2c_probe,
	.remove = ltr579_i2c_remove,
	.detect = ltr579_i2c_detect,
	.suspend = ltr579_i2c_suspend,
	.resume = ltr579_i2c_resume,
	.id_table = ltr579_i2c_id,
	.driver = {
		.name = LTR579_DEV_NAME,
		.of_match_table = alsps_of_match,
	},
};

static struct ltr579_priv *ltr579_obj;

/*------------------------i2c function for 89-------------------------------------*/
int ltr579_i2c_master_operate(struct i2c_client *client, char *buf, int count, int i2c_flag)
{
	int res = 0;

	mutex_lock(&ltr579_mutex);
	switch (i2c_flag) {
	case I2C_FLAG_WRITE:
		res = i2c_master_send(client, buf, count);	
		break;

	case I2C_FLAG_READ:
		res = i2c_master_send(client, buf, count & 0xFF);
		res = i2c_master_recv(client, buf, count >> 0x08);
		break;
	default:
		APS_LOG("ltr579_i2c_master_operate i2c_flag command not support!\n");
		break;
	}
	if (res <= 0)
		goto EXIT_ERR;

	mutex_unlock(&ltr579_mutex);
	return res;
 EXIT_ERR:
	mutex_unlock(&ltr579_mutex);
	APS_ERR("ltr579_i2c_transfer fail\n");
	return res;
}

/*----------------------------------------------------------------------------*/
int ltr579_get_addr(struct alsps_hw *hw, struct ltr579_i2c_addr *addr)
{
	if (!hw || !addr)
		return -EFAULT;

	addr->write_addr = hw->i2c_addr[0];
	return 0;
}

/*----------------------------------------------------------------------------*/
static void ltr579_power(struct alsps_hw *hw, unsigned int on)
{
	on = on;
}

/*----------------------------------------------------------------------------*/
static long ltr579_enable_als(struct i2c_client *client, int enable)
{
	int error = 0;
	int setctrl;

      if (enable == 1) {	

	setctrl = ltr579_i2c_read_reg(APS_RW_MAIN_CTRL);
	setctrl = setctrl | 0x02;// Enable ALS
	
	error = ltr579_i2c_write_reg(APS_RW_MAIN_CTRL, setctrl);	

	mdelay(WAKEUP_DELAY);

	/* =============== 
	 * ** IMPORTANT **
	 * ===============
	 * Other settings like timing and threshold to be set here, if required.
 	 * Not set and kept as device default for now.
 	 */
 	 
	  setctrl = ltr579_i2c_read_reg(APS_RW_MAIN_CTRL);
 	  if(error<0)
 	      APS_LOG("ltr579_als_enable ...ERROR\n");
 	  else
              APS_LOG("ltr579_als_enable ...OK\n");
		
	} else {
		APS_LOG("ALS(1): disable als only \n");

		setctrl = ltr579_i2c_read_reg(APS_RW_MAIN_CTRL);
		if(((setctrl & 0x03) == 0x02) ){	// do not repeat disable ALS 
	              setctrl = setctrl & 0xFD;	
		      error = ltr579_i2c_write_reg(APS_RW_MAIN_CTRL, setctrl); 
		}
		if(error<0)
			APS_LOG("ltr579_als_disable ...ERROR\n");
		else
			APS_LOG("ltr579_als_disable ...OK\n");
		
	}
	mdelay(WAKEUP_DELAY);

	return 0;
}


#ifdef GN_MTK_BSP_PS_DYNAMIC_CALI

/************************modified by HongGuang for dynamic calibrate***************************/
static int ltr579_dynamic_calibrate(void)
{
	int i = 0;
	int j = 0;
	int k = 0;
	u16 data = 0;
	u16 noise = 0;
	u16 data_total = 0;
	struct ltr579_priv *obj = ltr579_obj;

	APS_ERR("ltr579_dynamic_calibrate\n");
	
	if (!obj) 
		goto err;

	ltr579_i2c_write_reg(APS_RW_PS_MEAS_RATE, 0x5A);  
	mdelay(10);
	for (i = 0; i < 5; i++) {
	
		#if 0 //useless
		if (max++ > 5) {		
			atomic_set(&obj->ps_thd_val_high,  2047);
			atomic_set(&obj->ps_thd_val_low, 2047);

			goto err;
		}
		#endif
		mdelay(13);	//the frequecy of data updating is 10ms
		
		if ((ltr579_read_ps(obj->client, &data) < 0)) {
			k++;
			printk("ltr579_read_ps: k=%d\n", k);
			continue;
		}
		if ((data & 0x0800) == 0x0800) {
			data = last_min_value;
		}	//usef fo overflow
	
		if(data == 0){
			j++;
			printk("ltr579_read_ps: j=%d\n", j);
		}	
		data_total += data;
	}

	if ((j < (5 - k ))) {
		noise = (int)data_total / ((5 -k)- j); 
	}

	dynamic_cali = noise;

	if((noise !=0) && (noise < (last_min_value + 100)))//add by pengc at 20141029
	{
		last_min_value = noise;//add by pengc at 20141029
		if (noise < 100) {		
			atomic_set(&obj->ps_thd_val_high,  noise+60);//70
			atomic_set(&obj->ps_thd_val_low, noise+15); //50			
		} else if (noise < 200) {
			atomic_set(&obj->ps_thd_val_high,  noise+60);
			atomic_set(&obj->ps_thd_val_low, noise+15);
		} else if(noise < 300) {
			atomic_set(&obj->ps_thd_val_high,  noise+60);
			atomic_set(&obj->ps_thd_val_low, noise+15);
		} else if (noise < 400) {
			atomic_set(&obj->ps_thd_val_high,  noise+60);
			atomic_set(&obj->ps_thd_val_low, noise+20);
		} else if (noise < 600) {
			atomic_set(&obj->ps_thd_val_high,  noise+60);
			atomic_set(&obj->ps_thd_val_low, noise+20);
		} else if (noise < 1000) {
			atomic_set(&obj->ps_thd_val_high,  noise+60);
			atomic_set(&obj->ps_thd_val_low, noise+20);	
		} else if (noise < 1450) {
			atomic_set(&obj->ps_thd_val_high,  noise+80);
			atomic_set(&obj->ps_thd_val_low, noise+25);
		} else {
			atomic_set(&obj->ps_thd_val_high,  1600);
			atomic_set(&obj->ps_thd_val_low, 1550);
			//isadjust = 0;
			printk(KERN_ERR "ltr558 the proximity sensor structure is error\n");
		}
	}//add by pengc at 20141029	


//added by peixuan.qiu 2015-6-8 start used for debug
	{
		printk("<ps status><ltr579_dynamic_calibrate> end: " 
		       "noise=%d, ps_thd_val_low=%d, ps_thd_val_high=%d\n", 
				noise,
				atomic_read(&obj->ps_thd_val_low), 
				atomic_read(&obj->ps_thd_val_high));
	}
//added by peixuan.qiu  2015-6-8 end

	return 0;
err:
	APS_ERR("ltr579_dynamic_calibrate fail!!!\n");
	return -1;
}
#endif


/*----------------------------------------------------------------------------*/
static long ltr579_enable_ps(struct i2c_client *client, int enable)
{
	struct ltr579_priv *obj = i2c_get_clientdata(client);
	long res = 0;
        int setctrl;
	int error;
	u8 databuf[2];
	if (enable) {
	#if 0
		if(0 == obj->hw->polling_mode_ps)
                {

                        databuf[0] = APS_RW_INTERRUPT;
                        databuf[1] = 0x01;
                        res = i2c_master_send(client, databuf, 0x2);
                        if(res <= 0)
                        {
                             goto EXIT_ERR;;
                        }

                        databuf[0] = APS_RW_INTERRUPT_PERSIST;
                        databuf[1] = 0x02;
                        res = i2c_master_send(client, databuf, 0x2);
                        if(res <= 0)
                        {
                            goto EXIT_ERR;;
                        }
                }
	#endif	
		setctrl = ltr579_i2c_read_reg(APS_RW_MAIN_CTRL);
                if((setctrl & 0x01) == 0)//Check for PS enable?
                {
                        setctrl = setctrl | 0x01;
                        error = ltr579_i2c_write_reg(APS_RW_MAIN_CTRL, setctrl);
                        if(error<0)
                        {
                           APS_LOG("ltr579_ps_enable() error1\n");
                           return error;
                        }
                }
                 
                msleep(30);
		ltr579_dynamic_calibrate();
		ltr579_i2c_write_reg(APS_RW_PS_MEAS_RATE, 0x5C);  
		if (0 == obj->hw->polling_mode_ps) {
			set_psensor_threshold(obj->client);
                        
                        databuf[0] = APS_RW_INTERRUPT;
                        databuf[1] = 0x01;
                        res = i2c_master_send(client, databuf, 0x2);
                        if(res <= 0)
                        {
                             goto EXIT_ERR;;
                        }

                        databuf[0] = APS_RW_INTERRUPT_PERSIST;
                        databuf[1] = 0x01;
                        res = i2c_master_send(client, databuf, 0x2);
                        if(res <= 0)
                        {
                            goto EXIT_ERR;;
                        } 	
		}
		APS_ERR("enable ps OK APS_RW_PS_CONTR value = %x\n",databuf[1]);//23
		
	} 
	else 
	{
                setctrl = ltr579_i2c_read_reg(APS_RW_INTERRUPT);
                if((setctrl & 0x01) == 1)
                {
                   setctrl = setctrl & 0xFE;
                }
                error = ltr579_i2c_write_reg(APS_RW_INTERRUPT, setctrl);
	        if(error<0)
                    APS_LOG("ltr579_ps_int_disable ...ERROR\n");
                else
                    APS_LOG("ltr579_ps_int_disable ...OK\n");

		setctrl = ltr579_i2c_read_reg(APS_RW_MAIN_CTRL);
                if((setctrl & 0x01) == 1)
                {
                   setctrl = setctrl & 0xFE;
                }
		 error = ltr579_i2c_write_reg(APS_RW_MAIN_CTRL, setctrl);  //sang
                if(error<0)
                    APS_LOG("ltr579_ps_disable ...ERROR\n");
                else
                    APS_LOG("ltr579_ps_disable ...OK\n");
			
	        if(0 == obj->hw->polling_mode_ps)
                {
                    cancel_work_sync(&obj->irq_work);
                }
         }
	return 0;

 EXIT_ERR:
	APS_ERR("ltr579_enable_ps fail\n");
	return res;
}

/*----------------------------------------------------------------------------*/
/*for interrupt work mode support -- by liaoxl.lenovo 12.08.2011*/
#if 0
static int ltr579_check_and_clear_intr(struct i2c_client *client)
{
	int res, intp, intl;
	u8 buffer[2];
	u8 temp;

	//gpio_direction_input(alsps_int_gpio_number);
	//if (gpio_get_value(alsps_int_gpio_number) == 1)	/*skip if no interrupt */
		//return 0;

	buffer[0] = APS_RO_ALS_PS_STATUS;
	res = ltr579_i2c_master_operate(client, buffer, 0x101, I2C_FLAG_READ);
	if (res <= 0)
		goto EXIT_ERR;

		temp = buffer[0];
		res = 1;
		intp = 0;
		intl = 0;
		if(0 != (buffer[0] & 0x02))
		{
			res = 0;
			intp = 1;
		}
		if(0 != (buffer[0] & 0x08))
		{
			res = 0;
			intl = 1;		
		}
	
		if(0 == res)
		{
			if((1 == intp) && (0 == intl))
			{
				buffer[1] = buffer[0] & 0xfD;//set ps interrupt inactive
				
			}
			else if((0 == intp) && (1 == intl))
			{
				buffer[1] = buffer[0] & 0xf7;//set als interrupt inactive
			}
			else
			{
				buffer[1] = buffer[0] & 0xf5;//set ps & als interrupt inactive
			}
			buffer[0] = APS_RO_ALS_PS_STATUS	;
			res = i2c_master_send(client, buffer, 0x2);
			if(res <= 0)
			{
				goto EXIT_ERR;
			}
			else
			{
				res = 0;
			}
		}
	
		return res;
	
	EXIT_ERR:
		APS_ERR("ltr579_check_and_clear_intr fail\n");
		return 1;

}
#endif
/*----------------------------------------------------------------------------*/
static int ltr579_check_intr(struct i2c_client *client)
{
	int res, intp, intl;
	u8 buffer[2];

	//gpio_direction_input(alsps_int_gpio_number);
	//if (gpio_get_value(alsps_int_gpio_number) == 1)	/*skip if no interrupt */
		//return 0;

	buffer[0] = APS_RO_MAIN_STATUS;
	res = ltr579_i2c_master_operate(client, buffer, 0x101, I2C_FLAG_READ);
	if (res <= 0)
		goto EXIT_ERR;
	res = -1;
	intp = 0;
	intl = 0;
	if(0 != (buffer[0] & 0x02))
	{
		res = 0; //Ps int
		set_bit(1, &intr_flag);
		intp = 1;
	}
	if(0 != (buffer[0] & 0x10))
	{
		res = 2; //ALS int
		set_bit(2, &intr_flag);
		intl = 1;		
	}
	if(0 != (buffer[0] & 0x12))
	{
		res = 4; //ALS & PS int
		set_bit(1, &intr_flag);
		set_bit(2, &intr_flag);
		intl = 1;		
	}
	//APS_ERR("intr flag=%d, buffer[0]=%d\n", res, buffer[0]);
	return res;

 EXIT_ERR:
	APS_ERR("ltr579_check_intr fail\n");
	return 1;
}
#if 0
static int ltr579_clear_intr(struct i2c_client *client)
{
	int res;
	u8 buffer[2];

	APS_FUN();
	
	buffer[0] = APS_RO_ALS_PS_STATUS;
	res = i2c_master_send(client, buffer, 0x1);
	if(res <= 0)
	{
		goto EXIT_ERR;
	}
	res = i2c_master_recv(client, buffer, 0x1);
	if(res <= 0)
	{
		goto EXIT_ERR;
	}
	APS_DBG("buffer[0] = %d \n",buffer[0]);
	//buffer[1] = buffer[0] & 0x01;
	buffer[1] = buffer[0] & 0xfd;//modified by peixuan.qiu
	buffer[0] = APS_RO_ALS_PS_STATUS;

	res = i2c_master_send(client, buffer, 0x2);
	if(res <= 0)
	{
		goto EXIT_ERR;
	}
	else
	{
		res = 0;
	}

 EXIT_ERR:
	APS_ERR("ltr579_check_and_clear_intr fail\n");
	return 1;
}
#endif

static void ltr579_eint_func(void)
{
	struct ltr579_priv *obj = ltr579_obj;

	if (!obj)
		return;

	int_top_time = sched_clock();
	schedule_work(&obj->irq_work);
}

#if defined(CONFIG_OF)
static irqreturn_t alsps_interrupt_handler(int irq, void *dev_id)
{
	//printk("%s\n", __func__);
	
	ltr579_eint_func();
	disable_irq_nosync(ltr579_obj->irq);

	return IRQ_HANDLED;
}
#endif

/*----------------------------------------------------------------------------*/
static int ltr579_ps_set_thres(void)
{
	u8 databuf[2];
	struct i2c_client *client = ltr579_i2c_client;
	struct ltr579_priv *obj = i2c_get_clientdata(client);
	int res = 0;

	if (1 == ps_cali.valid) {
		databuf[0] = APS_RW_PS_THRES_LOW_0;
		databuf[1] = (u8) (ps_cali.far_away & 0x00FF);
		res = ltr579_i2c_master_operate(client, databuf, 0x2, I2C_FLAG_WRITE);
		if (res <= 0)
			goto EXIT_ERR;

		databuf[0] = APS_RW_PS_THRES_LOW_1;
		databuf[1] = (u8) ((ps_cali.far_away & 0xFF00) >> 8);
		res = ltr579_i2c_master_operate(client, databuf, 0x2, I2C_FLAG_WRITE);
		if (res <= 0)
			goto EXIT_ERR;

		databuf[0] = APS_RW_PS_THRES_UP_0;
		databuf[1] = (u8) (ps_cali.close & 0x00FF);
		res = ltr579_i2c_master_operate(client, databuf, 0x2, I2C_FLAG_WRITE);
		if (res <= 0)
			goto EXIT_ERR;

		databuf[0] = APS_RW_PS_THRES_UP_1;
		databuf[1] = (u8) ((ps_cali.close & 0xFF00) >> 8);
		res = ltr579_i2c_master_operate(client, databuf, 0x2, I2C_FLAG_WRITE);
		if (res <= 0)
			goto EXIT_ERR;

	} else {
		databuf[0] = APS_RW_PS_THRES_LOW_0;
		//databuf[1] = (u8)(750 & 0x00FF);
		databuf[1] = (u8)((atomic_read(&obj->ps_thd_val_low)) & 0x00FF);
		res = ltr579_i2c_master_operate(client, databuf, 0x2, I2C_FLAG_WRITE);
		if (res <= 0)
			goto EXIT_ERR;

		databuf[0] = APS_RW_PS_THRES_LOW_1;
		//databuf[1] = (u8)((750 & 0xFF00) >> 8);
		databuf[1] = (u8)((atomic_read(&obj->ps_thd_val_low) >> 8) & 0x00FF);
		res = ltr579_i2c_master_operate(client, databuf, 0x2, I2C_FLAG_WRITE);
		if (res <= 0)
			goto EXIT_ERR;

		databuf[0] = APS_RW_PS_THRES_UP_0;
		//databuf[1] = (u8)(900 & 0x00FF);
		databuf[1] = (u8)((atomic_read(&obj->ps_thd_val_high)) & 0x00FF);
		res = ltr579_i2c_master_operate(client, databuf, 0x2, I2C_FLAG_WRITE);
		if (res <= 0)
			goto EXIT_ERR;

		databuf[0] = APS_RW_PS_THRES_UP_1;
		//databuf[1] = (u8)((900 & 0xFF00) >> 8);
		databuf[1] = (u8)((atomic_read(&obj->ps_thd_val_high) >> 8) & 0x00FF);
		res = ltr579_i2c_master_operate(client, databuf, 0x2, I2C_FLAG_WRITE);
		if (res <= 0)
			goto EXIT_ERR;
	}
EXIT_ERR:
	return res;
}

static int ltr579_init_client(struct i2c_client *client)
{
	struct ltr579_priv *obj = i2c_get_clientdata(client);
	u8 databuf[2];
	int res = 0;
	int error;

	databuf[0] = APS_RW_PS_LED;//pulse frequecny 60kHZ,pulse current 50mA
	databuf[1] = 0x36;
	res = ltr579_i2c_master_operate(client, databuf, 0x2, I2C_FLAG_WRITE);
	if (res <= 0)
		goto EXIT_ERR;

	
	databuf[0] = APS_RW_PS_N_PULSES;     //pulses = 8
	databuf[1] = 0x10;
	res = ltr579_i2c_master_operate(client, databuf, 0x2, I2C_FLAG_WRITE);
	if (res <= 0)
		goto EXIT_ERR;


	databuf[0] = APS_RW_PS_MEAS_RATE;//measurement rate
	databuf[1] = 0x5C;//50ms 11bit
	res = ltr579_i2c_master_operate(client, databuf, 0x2, I2C_FLAG_WRITE);
	if (res <= 0)
		goto EXIT_ERR;


	databuf[0] = APS_RW_ALS_MEAS_RATE;
	databuf[1] = 0x22;				//18bit 100ms
	res = ltr579_i2c_master_operate(client, databuf, 0x2, I2C_FLAG_WRITE);
	if (res <= 0)
		goto EXIT_ERR;
	
	als_gainrange = ALS_RANGE_18;
	switch (als_gainrange)
	{
		case ALS_RANGE_1:
			error = ltr579_i2c_write_reg(APS_RW_ALS_GAIN, MODE_ALS_Range1);
			break;

		case ALS_RANGE_3:
			error = ltr579_i2c_write_reg(APS_RW_ALS_GAIN, MODE_ALS_Range3);
			break;

		case ALS_RANGE_6:
			error = ltr579_i2c_write_reg(APS_RW_ALS_GAIN, MODE_ALS_Range6);
			break;
			
		case ALS_RANGE_9:
			error = ltr579_i2c_write_reg(APS_RW_ALS_GAIN, MODE_ALS_Range9);
			break;
			
		case ALS_RANGE_18:
			error = ltr579_i2c_write_reg(APS_RW_ALS_GAIN, MODE_ALS_Range18);
			break;
		
		default:
			error = ltr579_i2c_write_reg(APS_RW_ALS_GAIN, MODE_ALS_Range3);			
			APS_ERR("ALS sensor gainrange %d!\n", als_gainrange);
			break;
	}
	
	if(error<0){
 	    APS_ERR("ltr579_als_enable ...ERROR\n");
	}
	/*for interrupt work mode support -- by liaoxl.lenovo 12.08.2011 */
	if (0 == obj->hw->polling_mode_ps) {

		res = ltr579_ps_set_thres();
		if (res <= 0) {
			APS_ERR("setup thres fail!!! res=%d", res);
			goto EXIT_ERR;
		}
		
		databuf[0] = APS_RW_INTERRUPT;
		databuf[1] = 0x01;//only ps measurement can trigger interrupt 
		res = ltr579_i2c_master_operate(client, databuf, 0x2, I2C_FLAG_WRITE);
		if (res <= 0)
			goto EXIT_ERR;
		
		databuf[0] = APS_RW_INTERRUPT_PERSIST;
		databuf[1] = 0x03;
		res = ltr579_i2c_master_operate(client, databuf, 0x2, I2C_FLAG_WRITE);
		if (res <= 0)
			goto EXIT_ERR;
		
		res = ltr579_setup_eint(client);
		if (res != 0) {
			APS_ERR("setup eint: %d\n", res);
			goto EXIT_ERR;
		}
	#if 0
		if((res = ltr579_check_and_clear_intr(client)))
		{
			APS_ERR("check/clear intr: %d\n", res);
			//    return res;
		}
	#endif

	}

	return 0;

 EXIT_ERR:
	APS_ERR("init dev: %d\n", res);
	return res;
}

/******************************************************************************
 * Function Configuration
******************************************************************************/
int ltr579_read_als(struct i2c_client *client, u16 *data)
{
	//struct ltr579_priv *obj = i2c_get_clientdata(client);
	int luxdata_int;
	u8 buffer[3];
	long res = 0;
	
	buffer[0] = LTR579_ALS_DATA_0;
	res = ltr579_i2c_master_operate(client, buffer, 0x301, I2C_FLAG_READ);
	if (res <= 0)
		goto EXIT_ERR;		
		
	luxdata_int = (buffer[2] * 256* 256) + (buffer[1] * 256) + buffer[0];
	//APS_DBG("luxdata_int = %d,buffer[2]=%d,buffer[1]=%d,buffer[0]=%d\n",luxdata_int,buffer[2],buffer[1],buffer[0]);

    if(luxdata_int == 0)
    {
        goto EXIT_ERR;
    }

	*data = luxdata_int;			    //formula: ALS counts * 0.8/gain/int , int=1
	
	//APS_DBG("als_value_lux = %d\n", luxdata_int);
	
EXIT_ERR:
	return 0;
}

#define MAX_ELM 30
static unsigned int record[MAX_ELM];
static int rct=0,full=0;
static long lux_sum=0;
/*----------------modified by hongguang for avglux function-----------------------*/		
static int get_avg_lux(unsigned int lux)
{
	int lux_a;
	if(rct >= MAX_ELM)
		full=1;

	if(full){
		rct %= MAX_ELM;
		lux_sum -= record[rct];
	}
	lux_sum += lux;
	record[rct]=lux;
	rct++;
	if(full){
	lux_a = lux_sum / MAX_ELM;
	}else{
	lux_a = lux_sum /rct;
	}
	return lux_a;
}
/*----------------------------------------------------------------------------*/

static int ltr579_get_als_value(struct ltr579_priv *obj, u16 als)
{
	int idx;
	int invalid = 0;

	for (idx = 0; idx < obj->als_level_num; idx++) {
		if (als < obj->hw->als_level[idx])
			break;
	}

	if (idx >= obj->als_value_num) {
		APS_ERR("ltr579_get_als_value exceed range\n");
		idx = obj->als_value_num - 1;
	}

	if (1 == atomic_read(&obj->als_deb_on)) {
		unsigned long endt = atomic_read(&obj->als_deb_end);
		if (time_after(jiffies, endt))
			atomic_set(&obj->als_deb_on, 0);
		if (1 == atomic_read(&obj->als_deb_on))
			invalid = 1;
	}

	if (!invalid) {
#if 0//defined(CONFIG_MTK_AAL_SUPPORT)
		int level_high = obj->hw->als_level[idx];
		int level_low = (idx > 0) ? obj->hw->als_level[idx - 1] : 0;
		int level_diff = level_high - level_low;
		int value_high = obj->hw->als_value[idx];
		int value_low = (idx > 0) ? obj->hw->als_value[idx - 1] : 0;
		int value_diff = value_high - value_low;
		int value = 0;

		if ((level_low >= level_high) || (value_low >= value_high))
			value = value_low;
		else
			value =
				(level_diff * value_low + (als - level_low) * value_diff +
				 ((level_diff + 1) >> 1)) / level_diff;

		//APS_DBG("ALS: %d [%d, %d] => %d [%d, %d]\n", als, level_low, level_high, value,
			//value_low, value_high);
		return value;
#endif
		return obj->hw->als_value[idx];
	}
	return -1;
}

/*----------------------------------------------------------------------------*/
static int  ltr579_read_ps(struct i2c_client *client, u16 *data)
{
	u8 buffer[2];
	u16 temp_data;
	long res = 0;

	if (client == NULL) {
		APS_DBG("CLIENT CANN'T EQUL NULL\n");
		return -1;
	}

	buffer[0] = APS_RO_PS_DATA_0;
	res = ltr579_i2c_master_operate(client, buffer, 0x201, I2C_FLAG_READ);
	if (res <= 0)
		goto EXIT_ERR;
//add strong light overflow

	temp_data = buffer[0] | ((buffer[1] & 0x07)<< 8);
	*data = temp_data;
	//printk("%s : ps_raw_data=%d\n", __func__, temp_data);
	return 0;

 EXIT_ERR:
	APS_ERR("ltr579_read_ps fail\n");
	return res;
}

/*----------------------------------------------------------------------------*/
#if 0
static int ltr579_get_ps_value(struct ltr579_priv *obj, u16 ps)
{
	int val;		/* mask = atomic_read(&obj->ps_mask); */
	int invalid = 0;
	static int val_temp = 1;

	if (ps_cali.valid == 1) {//ps calibration available
		if (ps > ps_cali.close) {
			val = 0;	/*close */
			val_temp = 0;
			intr_flag_value = 1;
		} 
		else if (ps < ps_cali.far_away) {
			val = 1;	/*far away */
			val_temp = 1;
			intr_flag_value = 0;
		} else
			val = val_temp;
	} else {
		if (ps > atomic_read(&obj->ps_thd_val_high)) {
			val = 0;	/*close */
			val_temp = 0;
			intr_flag_value = 1;
		} else if (ps < atomic_read(&obj->ps_thd_val_low)) {
			val = 1;	/*far away */
			val_temp = 1;
			intr_flag_value = 0;
		} else
			val = val_temp;

	}

	if (atomic_read(&obj->ps_suspend)) {
		invalid = 1;
	} 
	else if (1 == atomic_read(&obj->ps_deb_on)) {//in debounce mode, sensor's value is inavailable
		unsigned long endt = atomic_read(&obj->ps_deb_end);
		if (time_after(jiffies, endt))
			atomic_set(&obj->ps_deb_on, 0);
		if (1 == atomic_read(&obj->ps_deb_on))
			invalid = 1;
	} 
	else if (obj->als > 45000) {
		/* invalid = 1; */
		APS_DBG("ligh too high will result to failt proximiy\n");
		return 1;	/*far away */
	}

	if (!invalid) {
		APS_ERR("val=%d\n", val);
		return val;
	} else {
		return -1;
	}
}
#endif 

static int ltr579_get_ps_value(void)
{
	int ps_flag;
	int val;
	int error=-1;
	int buffer=0;

	buffer = ltr579_i2c_read_reg(APS_RO_MAIN_STATUS);
	APS_DBG("Main status = %d\n", buffer);
	if (buffer < 0){
	    
	    APS_DBG("MAIN status read error\n");
		return error;
	}

	ps_flag = buffer & 0x04;
	ps_flag = ps_flag >>2;
	if(ps_flag==1) //Near
	{
	 	intr_flag_value =1;
		val=0;
	}
	else if(ps_flag ==0) //Far
	{
		intr_flag_value =0;
		val=1;
	}
    APS_DBG("ps_flag = %d, val = %d\n", ps_flag,val);
    
	return val;
}

/*----------------------------------------------------------------------------*/
/* #define DEBUG_ltr579 */
static void ltr579_irq_work(struct work_struct *work)
{
	struct ltr579_priv *obj = (struct ltr579_priv *)container_of(work, struct ltr579_priv, irq_work);
	int err;
	struct hwm_sensor_data sensor_data;
//	u8 databuf[3];
//	int res = 0;
//	int noise = 0;
//	int temp_noise;

	//printk("%s\n", __func__);
        printk("ZYQ  ltr579_irq_work\n");	
	err = ltr579_check_intr(obj->client);
	if ((err < 0) && (!test_and_clear_bit(1, &intr_flag))) {
		APS_ERR("ltr579_irq_work check intrs exception: %d\n", err);
	} 
	else
	{
		/* get raw data */
		ltr579_read_ps(obj->client, &obj->ps);
		APS_LOG("ltr579 int top half time = %lld\n", int_top_time);

//for saturation
		if ((obj->ps & 0x0800) == 0x0800) {
			sensor_data.values[0] = 1;
			sensor_data.value_divide = 1;
			sensor_data.status = SENSOR_STATUS_ACCURACY_MEDIUM;
			APS_ERR("ltr579 read ps data exception = %d \n",sensor_data.values[0]);
			if ((err = ps_report_interrupt_data(sensor_data.values[0]))) {
				APS_ERR("call hwmsen_get_interrupt_data fail = %d\n", err);
			}
		
			//ltr579_clear_intr(obj->client);
			enable_irq(obj->irq);
			return;
		}

		sensor_data.values[0] = ltr579_get_ps_value();		//ltr579_get_ps_value(obj, obj->ps);
		sensor_data.value_divide = 1;
		sensor_data.status = SENSOR_STATUS_ACCURACY_MEDIUM;
//signal interrupt function add
		#if 0
		if (intr_flag_value) {//intr_flag_value changed in ltr579_get_ps_value
			databuf[0] = APS_RW_PS_THRES_LOW_0;
			databuf[1] = (u8) ((atomic_read(&obj->ps_thd_val_low)) & 0x00FF);
			res = ltr579_i2c_master_operate(obj->client, databuf, 0x2, I2C_FLAG_WRITE);
			if (res <= 0) goto EXIT_ERR;

			databuf[0] = APS_RW_PS_THRES_LOW_1;
			databuf[1] = (u8) (((atomic_read(&obj->ps_thd_val_low)) & 0xFF00) >> 8);
			res = ltr579_i2c_master_operate(obj->client, databuf, 0x2, I2C_FLAG_WRITE);
			if (res <= 0) goto EXIT_ERR;

			databuf[0] = APS_RW_PS_THRES_UP_0;
			databuf[1] = (u8) (0x00FF);
			res = ltr579_i2c_master_operate(obj->client, databuf, 0x2, I2C_FLAG_WRITE);
			if (res <= 0) goto EXIT_ERR;

			databuf[0] = APS_RW_PS_THRES_UP_1;
			databuf[1] = (u8) ((0xFF00) >> 8);
			res = ltr579_i2c_master_operate(obj->client, databuf, 0x2, I2C_FLAG_WRITE);
			if (res <= 0) goto EXIT_ERR;

		}
		else 
		{

#ifdef GN_MTK_BSP_PS_DYNAMIC_CALI
			
/************************modified by HongGuang for dynamic calibrate***************************/
			noise = obj->ps;	
			if(noise > (dynamic_cali + 30)/*noise < (dynamic_cali - 100)*/) {//modified by peixuan.qu
				dynamic_cali = noise;
				if(noise < 100){
					atomic_set(&obj->ps_thd_val_high,  noise+70);
					atomic_set(&obj->ps_thd_val_low, noise+30);
				}else if(noise < 200){
					atomic_set(&obj->ps_thd_val_high,  noise+100);
					atomic_set(&obj->ps_thd_val_low, noise+80);
				}else if(noise < 300){
					atomic_set(&obj->ps_thd_val_high,  noise+120);
					atomic_set(&obj->ps_thd_val_low, noise+100);
				}else if(noise < 400){
					atomic_set(&obj->ps_thd_val_high,  noise+130);
					atomic_set(&obj->ps_thd_val_low, noise+100);
				}else if(noise < 600){
					atomic_set(&obj->ps_thd_val_high,  noise+150);
					atomic_set(&obj->ps_thd_val_low, noise+120);
				}else if(noise < 1000){
					atomic_set(&obj->ps_thd_val_high,  noise+160);
					atomic_set(&obj->ps_thd_val_low, noise+140);	
				}else if(noise < 1450){
					atomic_set(&obj->ps_thd_val_high,  noise+400);
					atomic_set(&obj->ps_thd_val_low, noise+300);
				}else{
					atomic_set(&obj->ps_thd_val_high,  1600);
					atomic_set(&obj->ps_thd_val_low, 1450);
					APS_ERR( "ltr579 the proximity sensor ps raw data is big\n");
				}
			}
#endif

			if(obj->ps  > 50){
				temp_noise = obj->ps - 50;
			}else{
				temp_noise = 0;
			}

			databuf[0] = APS_RW_PS_THRES_LOW_0;
			//databuf[1] = (u8) (0 & 0x00FF);
			databuf[1] = (u8)(temp_noise & 0x00FF);
			res = ltr579_i2c_master_operate(obj->client, databuf, 0x2, I2C_FLAG_WRITE);
			if (res <= 0) 
				goto EXIT_ERR;

			databuf[0] = APS_RW_PS_THRES_LOW_1;
			//databuf[1] = (u8) ((0 & 0xFF00) >> 8);
			databuf[1] = (u8)((temp_noise & 0xFF00) >> 8);
			res = ltr579_i2c_master_operate(obj->client, databuf, 0x2, I2C_FLAG_WRITE);
			if (res <= 0) 
				goto EXIT_ERR;

			databuf[0] = APS_RW_PS_THRES_UP_0;
			databuf[1] = (u8) ((atomic_read(&obj->ps_thd_val_high)) & 0x00FF);
			res = ltr579_i2c_master_operate(obj->client, databuf, 0x2, I2C_FLAG_WRITE);
			if (res <= 0) 
				goto EXIT_ERR;


			databuf[0] = APS_RW_PS_THRES_UP_1;
			databuf[1] = (u8) (((atomic_read(&obj->ps_thd_val_high)) & 0xFF00) >> 8);
			res = ltr579_i2c_master_operate(obj->client, databuf, 0x2, I2C_FLAG_WRITE);
			if (res <= 0) 
				goto EXIT_ERR;

		}
#endif
		/*-------------------modified by hongguang@wecorp---------------------------------
		//let up layer to know
		if((err = hwmsen_get_interrupt_data(ID_PROXIMITY, &sensor_data)))
		{
		  APS_ERR("call hwmsen_get_interrupt_data fail = %d\n", err);
		}
		------------------------------------------------------*/
		/*-------------------modified by hongguang@wecorp-------------------------------*/
		if(ps_report_interrupt_data(sensor_data.values[0]))
		{	
			APS_ERR("call ps_report_interrupt_data fail\n");
		}
                
		#if 0 
	         noise = obj->ps;
                    if(sensor_data.values[0]==1){
                      if(noise > (dynamic_cali + 30)/*noise < (dynamic_cali - 100)*/) {//modified by peixuan.qu
                                dynamic_cali = noise;
                                if(noise < 100){
                                        atomic_set(&obj->ps_thd_val_high,  noise+40);
                                        atomic_set(&obj->ps_thd_val_low, noise+22);
                                }else if(noise < 200){
                                        atomic_set(&obj->ps_thd_val_high,  noise+40);
                                        atomic_set(&obj->ps_thd_val_low, noise+22);
                                }else if(noise < 300){
                                        atomic_set(&obj->ps_thd_val_high,  noise+40);
                                        atomic_set(&obj->ps_thd_val_low, noise+20);
                                }else if(noise < 400){
                                        atomic_set(&obj->ps_thd_val_high,  noise+40);
                                        atomic_set(&obj->ps_thd_val_low, noise+20);
                                }else if(noise < 600){
                                        atomic_set(&obj->ps_thd_val_high,  noise+50);
                                        atomic_set(&obj->ps_thd_val_low, noise+20);
                                }else if(noise < 1000){
                                        atomic_set(&obj->ps_thd_val_high,  noise+50);
                                        atomic_set(&obj->ps_thd_val_low, noise+20);
                                }else if(noise < 1450){
                                        atomic_set(&obj->ps_thd_val_high,  noise+70);
                                        atomic_set(&obj->ps_thd_val_low, noise+25);
                                }else{
                                        atomic_set(&obj->ps_thd_val_high,  1600);
                                        atomic_set(&obj->ps_thd_val_low, 1450);
                                        APS_ERR( "ltr579 the proximity sensor ps raw data is big\n");
                                }
                        }
		    //ltr579_ps_set_thres();	
			set_psensor_threshold(obj->client);
		}	
		#endif	

	}
	
 //EXIT_ERR:
	//ltr579_clear_intr(obj->client);
	enable_irq(obj->irq);
}


/******************************************************************************
 * Function Configuration
******************************************************************************/
static int ltr579_open(struct inode *inode, struct file *file)
{
	file->private_data = ltr579_i2c_client;

	if (!file->private_data) {
		APS_ERR("null pointer!!\n");
		return -EINVAL;
	}

	return nonseekable_open(inode, file);
}

/*----------------------------------------------------------------------------*/

static int ltr579_release(struct inode *inode, struct file *file)
{
	file->private_data = NULL;
	return 0;
}

/*----------------------------------------------------------------------------*/
static int set_psensor_threshold(struct i2c_client *client)
{
	struct ltr579_priv *obj = i2c_get_clientdata(client);
	u8 databuf[3];
	int res = 0;

	APS_ERR("set_psensor_threshold function high: 0x%x, low:0x%x\n",
		atomic_read(&obj->ps_thd_val_high), atomic_read(&obj->ps_thd_val_low));

	databuf[0] = APS_RW_PS_THRES_LOW_0;
	databuf[1] = (u8) (atomic_read(&obj->ps_thd_val_low) & 0x00FF);
	res = ltr579_i2c_master_operate(client, databuf, 0x2, I2C_FLAG_WRITE);
	if (res <= 0)
		return -1;

	databuf[0] = APS_RW_PS_THRES_LOW_1;
	databuf[1] = (u8) ((atomic_read(&obj->ps_thd_val_low) & 0xFF00) >> 8);
	res = ltr579_i2c_master_operate(client, databuf, 0x2, I2C_FLAG_WRITE);
	if (res <= 0)
		return -1;

	databuf[0] = APS_RW_PS_THRES_UP_0;
	databuf[1] = (u8) (atomic_read(&obj->ps_thd_val_high) & 0x00FF);
	res = ltr579_i2c_master_operate(client, databuf, 0x2, I2C_FLAG_WRITE);
	if (res <= 0)
		return -1;

	databuf[0] = APS_RW_PS_THRES_UP_1;
	databuf[1] = (u8) ((atomic_read(&obj->ps_thd_val_high) & 0xFF00) >> 8);
	res = ltr579_i2c_master_operate(client, databuf, 0x2, I2C_FLAG_WRITE);
	if (res <= 0)
		return -1;

	return 0;
}

/*----------------------------------------------------------------------------*/

static long ltr579_unlocked_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	struct i2c_client *client = (struct i2c_client *)file->private_data;
	struct ltr579_priv *obj = i2c_get_clientdata(client);
	long err = 0;
	void __user *ptr = (void __user *)arg;
	int dat;
	uint32_t enable;
	int ps_result;
	int ps_cali;
	int threshold[2];

	switch (cmd) {
	case ALSPS_SET_PS_MODE:
		if (copy_from_user(&enable, ptr, sizeof(enable))) {
			err = -EFAULT;
			goto err_out;
		}
		if (enable) {
			err = ltr579_enable_ps(obj->client, 1);
			if (err) {
				APS_ERR("enable ps fail: %ld\n", err);
				goto err_out;
			}
			set_bit(CMC_BIT_PS, &obj->enable);
		} else {
			err = ltr579_enable_ps(obj->client, 0);
			if (err) {
				APS_ERR("disable ps fail: %ld\n", err);
				goto err_out;
			}
			clear_bit(CMC_BIT_PS, &obj->enable);
		}
		break;

	case ALSPS_GET_PS_MODE:
		enable = test_bit(CMC_BIT_PS, &obj->enable) ? (1) : (0);
		if (copy_to_user(ptr, &enable, sizeof(enable))) {
			err = -EFAULT;
			goto err_out;
		}
		break;

	case ALSPS_GET_PS_DATA:
		err = ltr579_read_ps(obj->client, &obj->ps);
		if (err)
			goto err_out;

		APS_LOG("ps value=%d\n", obj->ps);
		dat = ltr579_get_ps_value();	//ltr579_get_ps_value(obj, obj->ps);
		if (copy_to_user(ptr, &dat, sizeof(dat))) {
			err = -EFAULT;
			goto err_out;
		}
		break;

	case ALSPS_GET_PS_RAW_DATA:
		err = ltr579_read_ps(obj->client, &obj->ps);
		if (err)
			goto err_out;
		
		dat = obj->ps;
		if (copy_to_user(ptr, &dat, sizeof(dat))) {
			err = -EFAULT;
			goto err_out;
		}
		break;

	case ALSPS_SET_ALS_MODE:
		if (copy_from_user(&enable, ptr, sizeof(enable))) {
			err = -EFAULT;
			goto err_out;
		}
		if (enable) {
			err = ltr579_enable_als(obj->client, 1);
			if (err) {
				APS_ERR("enable als fail: %ld\n", err);
				goto err_out;
			}
			set_bit(CMC_BIT_ALS, &obj->enable);
		} else {
			err = ltr579_enable_als(obj->client, 0);
			if (err) {
				APS_ERR("disable als fail: %ld\n", err);
				goto err_out;
			}
			clear_bit(CMC_BIT_ALS, &obj->enable);
		}
		break;

	case ALSPS_GET_ALS_MODE:
		enable = test_bit(CMC_BIT_ALS, &obj->enable) ? (1) : (0);
		if (copy_to_user(ptr, &enable, sizeof(enable))) {
			err = -EFAULT;
			goto err_out;
		}
		break;

	case ALSPS_GET_ALS_DATA:
		err = ltr579_read_als(obj->client, &obj->als);
		if (err)
			goto err_out;

		/*----------------modified by hongguang for avglux function-----------------------*/		
		obj->als = get_avg_lux(obj->als);

		dat = ltr579_get_als_value(obj, obj->als);
		if (copy_to_user(ptr, &dat, sizeof(dat))) {
			err = -EFAULT;
			goto err_out;
		}
		break;

	case ALSPS_GET_ALS_RAW_DATA:
		err = ltr579_read_als(obj->client, &obj->als);
		if (err)
			goto err_out;
		
		dat = obj->als;
		if (copy_to_user(ptr, &dat, sizeof(dat))) {
			err = -EFAULT;
			goto err_out;
		}
		break;
	/*-----------------for factory mode test-----------------*/
	case ALSPS_GET_PS_TEST_RESULT:
		err = ltr579_read_ps(obj->client, &obj->ps);
		if (err)
			goto err_out;

		if (obj->ps > atomic_read(&obj->ps_thd_val_high))
			ps_result = 1;
		else
			ps_result = 0;

		if (copy_to_user(ptr, &ps_result, sizeof(ps_result))) {
			err = -EFAULT;
			goto err_out;
		}
		break;

	case ALSPS_IOCTL_CLR_CALI:
		if (copy_from_user(&dat, ptr, sizeof(dat))) {
			err = -EFAULT;
			goto err_out;
		}
		if (dat == 0)
			obj->ps_cali = 0;
		break;

	case ALSPS_IOCTL_GET_CALI:
		ps_cali = obj->ps_cali;
		if (copy_to_user(ptr, &ps_cali, sizeof(ps_cali))) {
			err = -EFAULT;
			goto err_out;
		}
		break;

	case ALSPS_IOCTL_SET_CALI:
		if (copy_from_user(&ps_cali, ptr, sizeof(ps_cali))) {
			err = -EFAULT;
			goto err_out;
		}

		obj->ps_cali = ps_cali;
		break;

	case ALSPS_SET_PS_THRESHOLD:
		if (copy_from_user(threshold, ptr, sizeof(threshold))) {
			err = -EFAULT;
			goto err_out;
		}
		APS_ERR("%s set threshold high: 0x%x, low: 0x%x\n", __func__, threshold[0],
			threshold[1]);
		atomic_set(&obj->ps_thd_val_high, (threshold[0] + obj->ps_cali));
		atomic_set(&obj->ps_thd_val_low, (threshold[1] + obj->ps_cali));	/* need to confirm */

		set_psensor_threshold(obj->client);

		break;

	case ALSPS_GET_PS_THRESHOLD_HIGH:
		threshold[0] = atomic_read(&obj->ps_thd_val_high) - obj->ps_cali;
		APS_ERR("%s get threshold high: 0x%x\n", __func__, threshold[0]);
		if (copy_to_user(ptr, &threshold[0], sizeof(threshold[0]))) {
			err = -EFAULT;
			goto err_out;
		}
		break;

	case ALSPS_GET_PS_THRESHOLD_LOW:
		threshold[0] = atomic_read(&obj->ps_thd_val_low) - obj->ps_cali;
		APS_ERR("%s get threshold low: 0x%x\n", __func__, threshold[0]);
		if (copy_to_user(ptr, &threshold[0], sizeof(threshold[0]))) {
			err = -EFAULT;
			goto err_out;
		}
		break;
	default:
		APS_ERR("%s not supported = 0x%04x", __func__, cmd);
		err = -ENOIOCTLCMD;
		break;
	}

 err_out:
	return err;
}

/*----------------------------------------------------------------------------*/

static const struct file_operations ltr579_fops = {
	.owner = THIS_MODULE,
	.open = ltr579_open,
	.release = ltr579_release,
	.unlocked_ioctl = ltr579_unlocked_ioctl,
};

/*----------------------------------------------------------------------------*/

static struct miscdevice ltr579_miscdev = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "als_ps",
	.fops = &ltr579_fops,
};

/*----------------------------------------------------------------------------*/
static int ltr579_i2c_suspend(struct i2c_client *client, pm_message_t msg)
{
/* struct ltr579_priv *obj = i2c_get_clientdata(client); */
/* int err; */
	APS_FUN();
#if 0
	if (msg.event == PM_EVENT_SUSPEND) {
		if (!obj) {
			APS_ERR("null pointer!!\n");
			return -EINVAL;
		}

		atomic_set(&obj->als_suspend, 1);
		err = ltr579_enable_als(client, 0);
		if (err) {
			APS_ERR("disable als: %d\n", err);
			return err;
		}

		atomic_set(&obj->ps_suspend, 1);
		err = ltr579_enable_ps(client, 0);
		if (err) {
			APS_ERR("disable ps:  %d\n", err);
			return err;
		}

		ltr579_power(obj->hw, 0);
	}
#endif
	return 0;
}

/*----------------------------------------------------------------------------*/
static int ltr579_i2c_resume(struct i2c_client *client)
{
/* struct ltr579_priv *obj = i2c_get_clientdata(client); */
/* int err; */
	APS_FUN();
#if 0
	if (!obj) {
		APS_ERR("null pointer!!\n");
		return -EINVAL;
	}

	ltr579_power(obj->hw, 1);
	err = ltr579_init_client(client);
	if (err) {
		APS_ERR("initialize client fail!!\n");
		return err;
	}
	atomic_set(&obj->als_suspend, 0);
	if (test_bit(CMC_BIT_ALS, &obj->enable)) {
		err = ltr579_enable_als(client, 1);
		if (err)
			APS_ERR("enable als fail: %d\n", err);

	}
	atomic_set(&obj->ps_suspend, 0);
	if (test_bit(CMC_BIT_PS, &obj->enable)) {
		err = ltr579_enable_ps(client, 1);
		if (err)
			APS_ERR("enable ps fail: %d\n", err);

	}
#endif
	return 0;
}

/*----------------------------------------------------------------------------*/
#if defined(CONFIG_HAS_EARLYSUSPEND)
static void ltr579_early_suspend(struct early_suspend *h)
{				/*early_suspend is only applied for ALS */
	struct ltr579_priv *obj = container_of(h, struct ltr579_priv, early_drv);
	int err;

	APS_FUN();

	if (!obj) {
		APS_ERR("null pointer!!\n");
		return;
	}
#if 1
	atomic_set(&obj->als_suspend, 1);
	if (test_bit(CMC_BIT_ALS, &obj->enable)) {
		err = ltr579_enable_als(obj->client, 0);
		if (err)
			APS_ERR("disable als fail: %d\n", err);

	}
#endif
}

/*----------------------------------------------------------------------------*/
static void ltr579_late_resume(struct early_suspend *h)
{				/*early_suspend is only applied for ALS */
	struct ltr579_priv *obj = container_of(h, struct ltr579_priv, early_drv);
	int err;

	APS_FUN();

	if (!obj) {
		APS_ERR("null pointer!!\n");
		return;
	}
#if 1
	atomic_set(&obj->als_suspend, 0);
	if (test_bit(CMC_BIT_ALS, &obj->enable)) {
		err = ltr579_enable_als(obj->client, 1);
		if (err)
			APS_ERR("enable als fail: %d\n", err);

	}
#endif
}
#endif

/*----------------------------------------------------------------------------*/
static int temp_als;
static int ALS_FLAG;

int ltr579_ps_operate(void *self, uint32_t command, void *buff_in, int size_in,
			void *buff_out, int size_out, int *actualout)
{
	int value;
	int err = 0;

	struct hwm_sensor_data *sensor_data;
	struct ltr579_priv *obj = (struct ltr579_priv *)self;

	/* APS_FUN(f); */
	switch (command) {
	case SENSOR_DELAY:
		if ((buff_in == NULL) || (size_in < sizeof(int))) {
			APS_ERR("Set delay parameter error!\n");
			err = -EINVAL;
		}
		/* Do nothing */
		break;

	case SENSOR_ENABLE:
		if ((buff_in == NULL) || (size_in < sizeof(int))) {
			APS_ERR("Enable sensor parameter error!\n");
			err = -EINVAL;
		} else {
			value = *(int *)buff_in;
			if (value) {
				err = ltr579_enable_ps(obj->client, 1);
				if (err) {
					APS_ERR("enable ps fail: %d\n", err);
					return -1;
				}
				set_bit(CMC_BIT_PS, &obj->enable);
#if 1
				if (!test_bit(CMC_BIT_ALS, &obj->enable)) {
					ALS_FLAG = 1;
					err = ltr579_enable_als(obj->client, 1);
					if (err) {
						APS_ERR("enable als fail: %d\n", err);
						return -1;
					}
				}
#endif
			} else {
				err = ltr579_enable_ps(obj->client, 0);
				if (err) {
					APS_ERR("disable ps fail: %d\n", err);
					return -1;
				}
				clear_bit(CMC_BIT_PS, &obj->enable);
#if 1
				if (ALS_FLAG == 1) {
					err = ltr579_enable_als(obj->client, 0);
					if (err) {
						APS_ERR("disable als fail: %d\n", err);
						return -1;
					}
					ALS_FLAG = 0;
				}
#endif
			}
		}
		break;

	case SENSOR_GET_DATA:
		if ((buff_out == NULL) || (size_out < sizeof(struct hwm_sensor_data))) {
			APS_ERR("get sensor data parameter error!\n");
			err = -EINVAL;
		} else {
			sensor_data = (struct hwm_sensor_data *) buff_out;
			ltr579_read_ps(obj->client, &obj->ps);
			APS_ERR("ltr579_ps_operate als data=%d!\n", obj->als);
			sensor_data->values[0] = ltr579_get_ps_value();    //ltr579_get_ps_value(obj, obj->ps);
			sensor_data->value_divide = 1;
			sensor_data->status = SENSOR_STATUS_ACCURACY_MEDIUM;
		}
		break;
	default:
		APS_ERR("proxmy sensor operate function no this parameter %d!\n", command);
		err = -1;
		break;
	}

	return err;
}


int ltr579_als_operate(void *self, uint32_t command, void *buff_in, int size_in,
			 void *buff_out, int size_out, int *actualout)
{
	int err = 0;
	int value;
	struct hwm_sensor_data *sensor_data;
	struct ltr579_priv *obj = (struct ltr579_priv *)self;

	switch (command) {
	case SENSOR_DELAY:
		if ((buff_in == NULL) || (size_in < sizeof(int))) {
			APS_ERR("Set delay parameter error!\n");
			err = -EINVAL;
		}
		/* Do nothing */
		break;

	case SENSOR_ENABLE:
		if ((buff_in == NULL) || (size_in < sizeof(int))) {
			APS_ERR("Enable sensor parameter error!\n");
			err = -EINVAL;
		} else {
			value = *(int *)buff_in;
			if (value) {
				err = ltr579_enable_als(obj->client, 1);
				if (err) {
					APS_ERR("enable als fail: %d\n", err);
					return -1;
				}
				set_bit(CMC_BIT_ALS, &obj->enable);
			} else {
				err = ltr579_enable_als(obj->client, 0);
				if (err) {
					APS_ERR("disable als fail: %d\n", err);
					return -1;
				}
				clear_bit(CMC_BIT_ALS, &obj->enable);
			}

		}
		break;

	case SENSOR_GET_DATA:
		if ((buff_out == NULL) || (size_out < sizeof(struct hwm_sensor_data))) {
			APS_ERR("get sensor data parameter error!\n");
			err = -EINVAL;
		} else {
			sensor_data = (struct hwm_sensor_data *) buff_out;
			/*yucong MTK add for fixing known issue */
			ltr579_read_als(obj->client, &obj->als);
			if (obj->als == 0) {
				sensor_data->values[0] = temp_als;
			} else {
				u16 b[2];
				int i;

				for (i = 0; i < 2; i++) {
					ltr579_read_als(obj->client, &obj->als);
					b[i] = obj->als;
				}
				(b[1] > b[0]) ? (obj->als = b[0]) : (obj->als = b[1]);
				/*----------------modified by hongguang for avglux function-----------------------*/		
				obj->als = get_avg_lux(obj->als);
				sensor_data->values[0] = ltr579_get_als_value(obj, obj->als);
				temp_als = sensor_data->values[0];
			}
			sensor_data->value_divide = 1;
			sensor_data->status = SENSOR_STATUS_ACCURACY_MEDIUM;
		}
		break;
	default:
		APS_ERR("light sensor operate function no this parameter %d!\n", command);
		err = -1;
		break;
	}

	return err;
}


/*----------------------------------------------------------------------------*/
static int ltr579_i2c_detect(struct i2c_client *client, struct i2c_board_info *info)
{
	strcpy(info->type, LTR579_DEV_NAME);
	return 0;
}
/* if use  this typ of enable , Gsensor should report inputEvent(x, y, z ,stats, div) to HAL */
static int als_open_report_data(int open)
{
	/* should queuq work to report event if  is_report_input_direct=true */
	return 0;
}

/* if use  this typ of enable , Gsensor only enabled but not report inputEvent to HAL */

static int als_enable_nodata(int en)
{
	int res = 0;
#ifdef CUSTOM_KERNEL_SENSORHUB
	SCP_SENSOR_HUB_DATA req;
	int len;
#endif /* #ifdef CUSTOM_KERNEL_SENSORHUB */

	APS_LOG("ltr579_obj als enable value = %d\n", en);

#ifdef CUSTOM_KERNEL_SENSORHUB
	if (atomic_read(&ltr579_obj->init_done)) {
		req.activate_req.sensorType = ID_LIGHT;
		req.activate_req.action = SENSOR_HUB_ACTIVATE;
		req.activate_req.enable = en;
		len = sizeof(req.activate_req);
		res = SCP_sensorHub_req_send(&req, &len, 1);
	} else
		APS_ERR("sensor hub has not been ready!!\n");

	mutex_lock(&ltr579_mutex);
	if (en)
		set_bit(CMC_BIT_ALS, &ltr579_obj->enable);
	else
		clear_bit(CMC_BIT_ALS, &ltr579_obj->enable);
	mutex_unlock(&ltr579_mutex);
#else /* #ifdef CUSTOM_KERNEL_SENSORHUB */
	mutex_lock(&ltr579_mutex);
	if (en)
		set_bit(CMC_BIT_ALS, &ltr579_obj->enable);
	else
		clear_bit(CMC_BIT_ALS, &ltr579_obj->enable);
	mutex_unlock(&ltr579_mutex);
	if (!ltr579_obj) {
		APS_ERR("ltr579_obj is null!!\n");
		return -1;
	}
	res = ltr579_enable_als(ltr579_obj->client, en);
#endif /* #ifdef CUSTOM_KERNEL_SENSORHUB */
	if (res) {
		APS_ERR("als_enable_nodata is failed!!\n");
		return -1;
	}
	return 0;
}

static int als_set_delay(u64 ns)
{
	return 0;
}

static int als_get_data(int *value, int *status)
{
	int err = 0;
#ifdef CUSTOM_KERNEL_SENSORHUB
	SCP_SENSOR_HUB_DATA req;
	int len;
#endif /* #ifdef CUSTOM_KERNEL_SENSORHUB */

#ifdef CUSTOM_KERNEL_SENSORHUB
	if (atomic_read(&ltr579_obj->init_done)) {
		req.get_data_req.sensorType = ID_LIGHT;
		req.get_data_req.action = SENSOR_HUB_GET_DATA;
		len = sizeof(req.get_data_req);
		err = SCP_sensorHub_req_send(&req, &len, 1);
	if (err)
		APS_ERR("SCP_sensorHub_req_send fail!\n");
	else {
		*value = req.get_data_rsp.int16_Data[0];
		*status = SENSOR_STATUS_ACCURACY_MEDIUM;
	}

	if (atomic_read(&ltr579_obj->trace) & CMC_TRC_PS_DATA)
		APS_LOG("value = %d\n", *value);
	else {
		APS_ERR("sensor hub hat not been ready!!\n");
		err = -1;
	}
#else /* #ifdef CUSTOM_KERNEL_SENSORHUB */
	if (!ltr579_obj) {
		APS_ERR("ltr579_obj is null!!\n");
		return -1;
	}
	err = ltr579_read_als(ltr579_obj->client, &ltr579_obj->als);
	if (err)
		err = -1;
	else {
		*value = ltr579_get_als_value(ltr579_obj, ltr579_obj->als);
		if (*value < 0)
			err = -1;
		*status = SENSOR_STATUS_ACCURACY_MEDIUM;
	}
#endif /* #ifdef CUSTOM_KERNEL_SENSORHUB */

	return err;
}

/* if use  this typ of enable , Gsensor should report inputEvent(x, y, z ,stats, div) to HAL */
static int ps_open_report_data(int open)
{
	/* should queuq work to report event if  is_report_input_direct=true */
	return 0;
}

/* if use  this typ of enable , Gsensor only enabled but not report inputEvent to HAL */

static int ps_enable_nodata(int en)
{
	int res = 0;
#ifdef CUSTOM_KERNEL_SENSORHUB
	SCP_SENSOR_HUB_DATA req;
	int len;
#endif /* #ifdef CUSTOM_KERNEL_SENSORHUB */

	APS_ERR("ltr579_obj ps enable value = %d\n", en);

#ifdef CUSTOM_KERNEL_SENSORHUB
	if (atomic_read(&ltr579_obj->init_done)) {
		req.activate_req.sensorType = ID_PROXIMITY;
		req.activate_req.action = SENSOR_HUB_ACTIVATE;
		req.activate_req.enable = en;
		len = sizeof(req.activate_req);
		res = SCP_sensorHub_req_send(&req, &len, 1);
	} else
		APS_ERR("sensor hub has not been ready!!\n");

	mutex_lock(&ltr579_mutex);
	if (en)
		set_bit(CMC_BIT_PS, &ltr579_obj->enable);
	else
		clear_bit(CMC_BIT_PS, &ltr579_obj->enable);
	mutex_unlock(&ltr579_mutex);
#else /* #ifdef CUSTOM_KERNEL_SENSORHUB */
	mutex_lock(&ltr579_mutex);
	if (en)
		set_bit(CMC_BIT_PS, &ltr579_obj->enable);

	else
		clear_bit(CMC_BIT_PS, &ltr579_obj->enable);

	mutex_unlock(&ltr579_mutex);
	if (!ltr579_obj) {
		APS_ERR("ltr579_obj is null!!\n");
		return -1;
	}
	res = ltr579_enable_ps(ltr579_obj->client, en);
#endif /* #ifdef CUSTOM_KERNEL_SENSORHUB */

	if (res) {
		APS_ERR("als_enable_nodata is failed!!\n");
		return -1;
	}
	return 0;

}

static int ps_set_delay(u64 ns)
{
	return 0;
}

static int ps_get_data(int *value, int *status)
{
	int err = 0;
#ifdef CUSTOM_KERNEL_SENSORHUB
	SCP_SENSOR_HUB_DATA req;
	int len;
#endif /* #ifdef CUSTOM_KERNEL_SENSORHUB */

#ifdef CUSTOM_KERNEL_SENSORHUB
	if (atomic_read(&ltr579_obj->init_done)) {
		req.get_data_req.sensorType = ID_PROXIMITY;
		req.get_data_req.action = SENSOR_HUB_GET_DATA;
		len = sizeof(req.get_data_req);
		err = SCP_sensorHub_req_send(&req, &len, 1);
	if (err) {
		APS_ERR("SCP_sensorHub_req_send fail!\n");
		*value = -1;
		err = -1;
	} else {
		*value = req.get_data_rsp.int16_Data[0];
		*status = SENSOR_STATUS_ACCURACY_MEDIUM;
	}

	if (atomic_read(&ltr579_obj->trace) & CMC_TRC_PS_DATA)
		APS_LOG("value = %d\n", *value)
	else {
		APS_ERR("sensor hub has not been ready!!\n");
		err = -1;
	}
#else /* #ifdef CUSTOM_KERNEL_SENSORHUB */
	APS_FUN();
	if (!ltr579_obj) {
		APS_ERR("ltr579_obj is null!!\n");
		return -1;
	}
	err = ltr579_read_ps(ltr579_obj->client, &ltr579_obj->ps);
	if (err)
		err = -1;
	else {
		*value = ltr579_get_ps_value();		//ltr579_get_ps_value(ltr579_obj, ltr579_obj->ps);
		if (*value < 0)
			err = -1;
		*status = SENSOR_STATUS_ACCURACY_MEDIUM;
	}
	APS_ERR("ps=%d, ps_value=%d\n", ltr579_obj->ps, *value);
#endif /* #ifdef CUSTOM_KERNEL_SENSORHUB */

	return err;
}
/*----------------------------------------------------------------------------*/


/*----------------------------------------------------------------------------*/
static ssize_t ltr579_show_status(struct device_driver *ddri, char *buf)
{
	ssize_t len = 0;
	
	if(!ltr579_obj)
	{
		APS_ERR("ltr579_obj is null!!\n");
		return 0;
	}
	
	if(ltr579_obj->hw)
	{
	
		len += snprintf(buf+len, PAGE_SIZE-len, "CUST: %d, (%d %d)\n", 
			ltr579_obj->hw->i2c_num, ltr579_obj->hw->power_id, ltr579_obj->hw->power_vol);
		
	}
	else
	{
		len += snprintf(buf+len, PAGE_SIZE-len, "CUST: NULL\n");
	}


	len += snprintf(buf+len, PAGE_SIZE-len, "MISC: %d %d\n", atomic_read(&ltr579_obj->als_suspend), atomic_read(&ltr579_obj->ps_suspend));

	return len;
}

/*----------------------------------------------------------------------------*/
static ssize_t ltr579_store_status(struct device_driver *ddri, const char *buf, size_t count)
{
	int status1,ret;
	if(!ltr579_obj)
	{
		APS_ERR("ltr579_obj is null!!\n");
		return 0;
	}
	
	if(1 == sscanf(buf, "%d ", &status1))
	{ 
	    ret=ltr579_enable_ps(ltr579_obj->client, status1);
	
	}
	else
	{
		APS_DBG("invalid content: '%s', length = %d\n", buf, (int)count);
	}
	return count;    
}

/*---------------------------------------------------------------------------------*/

static int ltr579_i2c_read_reg(u8 regnum)
{
    u8 buffer[1],reg_value[1];
	int res = 0;
	mutex_lock(&ltr579_mutex);	

	buffer[0]= regnum;
	res = i2c_master_send(ltr579_obj->client, buffer, 0x1);
	if(res <= 0)	{
		mutex_unlock(&ltr579_mutex);
	   	APS_ERR("read reg send res = %d\n",res);
		return res;
	}
	res = i2c_master_recv(ltr579_obj->client, reg_value, 0x1);
	if(res <= 0)
	{
		mutex_unlock(&ltr579_mutex);
		APS_ERR("read reg recv res = %d\n",res);
		return res;
	}
	mutex_unlock(&ltr579_mutex);	
	return reg_value[0];
}

// I2C Write
static int ltr579_i2c_write_reg(u8 regnum, u8 value)
{
	u8 databuf[2];    
	int res = 0;
   
	databuf[0] = regnum;   
	databuf[1] = value;
	res = i2c_master_send(ltr579_obj->client, databuf, 0x2);

	if (res < 0) {
		APS_ERR("wirte reg send res = %d\n",res);
		return res;
	} else {
		return 0;
	}
}

/*----------------------------------------------------------------------------*/
static ssize_t ltr579_show_reg(struct device_driver *ddri, char *buf)
{
	int i,len=0;
	int reg[]={0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0d,0x0e,0x0f,
				0x19,0x1A,0x1B,0x1C,0x1D,0x1E,0x1F,0x20,0x21,0x22,0x23,0x24,0x25,0x26};
	for(i=0;i<27;i++)
		{
		len += snprintf(buf+len, PAGE_SIZE-len, "reg:0x%04X value: 0x%04X\n", reg[i],ltr579_i2c_read_reg(reg[i]));	

	    }
	return len;
}
/*----------------------------------------------------------------------------*/
static ssize_t ltr579_store_reg(struct device_driver *ddri, const char *buf, size_t count)
{
	int ret,value;
	//u8 reg;
	int reg;
	if(!ltr579_obj)
	{
		APS_ERR("ltr579_obj is null!!\n");
		return 0;
	}
	
	if(2 == sscanf(buf, "%x %x ", &reg,&value))
	//if(1 == sscanf(buf, "%x", &value))
	{ 
		APS_DBG("before write reg: %x, reg_value = %x  write value=%x\n", reg,ltr579_i2c_read_reg(reg),value);
	    ret=ltr579_i2c_write_reg(reg,value);
		APS_DBG("after write reg: %x, reg_value = %x\n", reg,ltr579_i2c_read_reg(reg));
	}
	else
	{
		APS_DBG("invalid content: '%s', length = %d\n", buf, (int)count);
	}
	return count;    
}

/*----------------------------------------------------------------------------*/
static DRIVER_ATTR(status,  S_IWUSR | S_IRUGO, ltr579_show_status,  ltr579_store_status);
static DRIVER_ATTR(reg,     S_IWUSR | S_IRUGO, ltr579_show_reg,   ltr579_store_reg);
/*----------------------------------------------------------------------------*/

static  struct driver_attribute *ltr579_attr_list[] = {
    &driver_attr_status,
    &driver_attr_reg,
};

/*----------------------------------------------------------------------------*/
static int ltr579_create_attr(struct device_driver*driver) 
{
	int idx, err = 0;
	int num = (int)(sizeof(ltr579_attr_list)/sizeof(ltr579_attr_list[0]));

	if (driver == NULL)
	{
		return -EINVAL;
	}

	for(idx = 0; idx < num; idx++)
	{
		if((err = driver_create_file(driver, ltr579_attr_list[idx])))
		{            
			APS_ERR("driver_create_file (%s) = %d\n", ltr579_attr_list[idx]->attr.name, err);
			break;
		}
	}    
	return err;
}
/*----------------------------------------------------------------------------*/

static int ltr579_delete_attr(struct device_driver *driver)
{
	int idx ,err = 0;
	int num = (int)(sizeof(ltr579_attr_list)/sizeof(ltr579_attr_list[0]));

	if (!driver)
	return -EINVAL;

	for (idx = 0; idx < num; idx++) 
	{
		driver_remove_file(driver, ltr579_attr_list[idx]);
	}
	
	return err;
}

/*----------------------------------------------------------------------------*/
int ltr559_get_psval(void)	
{
	int err;	
	int psval;
        int setctrl;	
	struct ltr579_priv *obj=NULL;	
	printk("[ALS/PS] ltr579_get_psval\n");	
	if(!ltr579_obj) {	
		APS_ERR("ltr579_obj is null!!\n");	
		return -1;	
	}	
	obj = ltr579_obj;
        
        setctrl = ltr579_i2c_read_reg(APS_RW_MAIN_CTRL);
        if((setctrl & 0x01) == 0)//Check for PS enable?
        {
          setctrl = setctrl | 0x01;
          err = ltr579_i2c_write_reg(APS_RW_MAIN_CTRL, setctrl);
          if(err<0)
          {
            APS_LOG("ltr579_ps_enable() error1\n");
            return err;
          }
        }
        msleep(10); 
        ltr579_read_ps(obj->client, &obj->ps);
	psval = obj->ps > atomic_read(&obj->ps_thd_val_high) ? 0 : 1;
	if(!test_bit(CMC_BIT_PS, &obj->enable)) {
                setctrl = ltr579_i2c_read_reg(APS_RW_MAIN_CTRL);
                if((setctrl & 0x01) == 1)
                {
                  setctrl = setctrl & 0xFE;
                }
                err = ltr579_i2c_write_reg(APS_RW_MAIN_CTRL, setctrl);
	}	
	printk("[ALS/PS] ltr579 psval state %d \n",psval);
	//check close or far 	
	if(!psval)
	{	
		//close
		return 1;
	}
	else
	{
		//far
		return 0;
	}	
}	
extern void app_get_als_ps_sensor_name(char *als_ps_name);
static int ltr579_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	struct ltr579_priv *obj;
	struct als_control_path als_ctl = {0};
	struct als_data_path als_data = {0};
	struct ps_control_path ps_ctl = {0};
	struct ps_data_path ps_data = {0};
	int err = 0;
	//printk("client->addr=0x%x\n", client->addr);	
	
	APS_FUN();

	obj = kzalloc(sizeof(*obj), GFP_KERNEL);
	if (!(obj)) {
		err = -ENOMEM;
		goto exit;
	}
	memset(obj, 0, sizeof(*obj));
	ltr579_obj = obj;
	obj->hw = hw;
	ltr579_get_addr(obj->hw, &obj->addr);//obj->addr->writer_addr = obj->hw->i2c_addr[0];

	/*for interrupt work mode support -- by liaoxl.lenovo 12.08.2011 */
	INIT_WORK(&obj->irq_work, ltr579_irq_work);

	obj->client = client;
	i2c_set_clientdata(client, obj);
	
	atomic_set(&obj->als_debounce, 50);
	atomic_set(&obj->als_deb_on, 0);
	atomic_set(&obj->als_deb_end, 0);
	atomic_set(&obj->ps_debounce, 10);
	atomic_set(&obj->ps_deb_on, 0);
	atomic_set(&obj->ps_deb_end, 0);
	atomic_set(&obj->ps_mask, 0);
	atomic_set(&obj->als_suspend, 0);
	atomic_set(&obj->als_cmd_val, 0xDF);
	atomic_set(&obj->ps_cmd_val, 0xC1);
	atomic_set(&obj->ps_thd_val_high, obj->hw->ps_threshold_high);
	atomic_set(&obj->ps_thd_val_low, obj->hw->ps_threshold_low);
	atomic_set(&obj->init_done,  0);
	obj->irq_node = of_find_compatible_node(NULL, NULL, "mediatek, als-eint");

	obj->enable = 0;
	obj->pending_intr = 0;
	obj->als_level_num = sizeof(obj->hw->als_level) / sizeof(obj->hw->als_level[0]);
	obj->als_value_num = sizeof(obj->hw->als_value) / sizeof(obj->hw->als_value[0]);
	/*
	  *Lenovo-sw chenlj2 add 2011-06-03,modified gain 16 to 1/5 according to actual thing
	  * (1/Gain)*(400/Tine), this value is fix after init ATIME and CONTROL register value 
	  */
	obj->als_modulus = (400 * 100 * ZOOM_TIME) / (1 * 150);
	//(400)/16*2.72 here is amplify *100 / *16 
	BUG_ON(sizeof(obj->als_level) != sizeof(obj->hw->als_level));
	memcpy(obj->als_level, obj->hw->als_level, sizeof(obj->als_level));
	BUG_ON(sizeof(obj->als_value) != sizeof(obj->hw->als_value));
	memcpy(obj->als_value, obj->hw->als_value, sizeof(obj->als_value));
	atomic_set(&obj->i2c_retry, 3);
	set_bit(CMC_BIT_ALS, &obj->enable);//ʹ\C4ܹ\E2\B8\D0
	set_bit(CMC_BIT_PS, &obj->enable);

	obj->ps_cali = 0;

	ltr579_i2c_client = client;

	err = ltr579_init_client(client);//device init
	if (err) 
		goto exit_init_failed;
	
	APS_LOG("ltr579_init_client() OK!\n");
	//get_psval = ltr559_get_psval;
	err = misc_register(&ltr579_miscdev);
	if (err) {
		APS_ERR("ltr579_miscdev register failed\n");
		//goto exit_misc_device_register_failed;
	}


	if((err = ltr579_create_attr(&ltr579_i2c_driver.driver)))
	{
		printk(KERN_ERR "create attribute err = %d\n", err);
		//goto exit_create_attr_failed;
	}
	

	als_ctl.is_use_common_factory = false;
	ps_ctl.is_use_common_factory = false;

#if defined(CONFIG_HAS_EARLYSUSPEND)
	obj->early_drv.level = EARLY_SUSPEND_LEVEL_DISABLE_FB - 1;
	obj->early_drv.suspend = ltr579_early_suspend;
	obj->early_drv.resume = ltr579_late_resume;
	register_early_suspend(&obj->early_drv);
#endif

//als
	als_ctl.open_report_data = als_open_report_data;
	als_ctl.enable_nodata = als_enable_nodata;
	als_ctl.set_delay  = als_set_delay;
	als_ctl.is_report_input_direct = false;
	
#ifdef CUSTOM_KERNEL_SENSORHUB
	als_ctl.is_support_batch = obj->hw->is_batch_supported_als;
#else
	als_ctl.is_support_batch = false;
#endif

	err = als_register_control_path(&als_ctl);
	if (err) {
		APS_ERR("register fail = %d\n", err);
		goto exit_sensor_obj_attach_fail;
	}
	
	als_data.get_data = als_get_data;
	als_data.vender_div = 100;
	err = als_register_data_path(&als_data);
	if (err) {
		APS_ERR("tregister fail = %d\n", err);
		goto exit_sensor_obj_attach_fail;
	}

//ps	
	ps_ctl.open_report_data = ps_open_report_data;
	ps_ctl.enable_nodata = ps_enable_nodata;
	ps_ctl.set_delay  = ps_set_delay;
	ps_ctl.is_report_input_direct = false;
#ifdef CUSTOM_KERNEL_SENSORHUB
	ps_ctl.is_support_batch = obj->hw->is_batch_supported_ps;
#else
	ps_ctl.is_support_batch = false;
#endif
	err = ps_register_control_path(&ps_ctl);
	if (err) {
		APS_ERR("register fail = %d\n", err);
		goto exit_sensor_obj_attach_fail;
	}

	ps_data.get_data = ps_get_data;
	ps_data.vender_div = 100;
	err = ps_register_data_path(&ps_data);
	if (err) {
		APS_ERR("tregister fail = %d\n", err);
		goto exit_sensor_obj_attach_fail;
	}

//batch
	err = batch_register_support_info(ID_LIGHT, als_ctl.is_support_batch, 1, 0);
	if (err)
		APS_ERR("register light batch support err = %d\n", err);

	err = batch_register_support_info(ID_PROXIMITY, ps_ctl.is_support_batch, 1, 0);
	if (err)
		APS_ERR("register proximity batch support err = %d\n", err);


	app_get_als_ps_sensor_name("ltr579"); 
	
	ltr579_init_flag = 0;
	APS_ERR("%s: OK\n", __func__);
	return 0;

/*exit_create_attr_failed:*/
exit_sensor_obj_attach_fail:
	misc_deregister(&ltr579_miscdev);
//exit_misc_device_register_failed:
exit_init_failed:
	/* i2c_detach_client(client); */
	/* exit_kfree: */
	kfree(obj);
exit:
	//gpio_free(alsps_int_gpio_number);
	ltr579_i2c_client = NULL;
	APS_ERR("%s: err = %d\n", __func__, err);
	ltr579_init_flag = -1;
	return err;
}
/*----------------------------------------------------------------------------*/
static int ltr579_i2c_remove(struct i2c_client *client)
{
	int err;

	if((err = ltr579_delete_attr(&ltr579_i2c_driver.driver)))
	{
		APS_ERR("ltr579_delete_attr fail: %d\n", err);
	}

	err = misc_deregister(&ltr579_miscdev);
	if (err)
		APS_ERR("misc_deregister fail: %d\n", err);


	//gpio_free(alsps_int_gpio_number);
	ltr579_i2c_client = NULL;
	i2c_unregister_device(client);
	kfree(i2c_get_clientdata(client));

	return 0;
}
/*----------------------------------------------------------------------------*/
static int  ltr579_local_init(void)
{
	ltr579_power(hw, 1);
	if (i2c_add_driver(&ltr579_i2c_driver)) {
		APS_ERR("add driver error\n");
		return -1;
	}
	if (-1 == ltr579_init_flag) {
		APS_ERR("ltr579_local_init fail with ltr579_init_flag=%d\n", ltr579_init_flag);
		return -1;
	}
	return 0;
}

/*----------------------------------------------------------------------------*/
static int ltr579_remove(void)
{
	APS_FUN();
	ltr579_power(hw, 0);
	i2c_del_driver(&ltr579_i2c_driver);
	return 0;
}
/*----------------------------------------------------------------------------*/

//added by peixuan.qiu start 20151119
static int ltr579_setup_eint(struct i2c_client *client)
{
#ifdef CUSTOM_KERNEL_SENSORHUB
	int err = 0;

	err = SCP_sensorHub_rsp_registration(ID_PROXIMITY, cm36652_irq_handler);//???
#else /* #ifdef CUSTOM_KERNEL_SENSORHUB */

	int ret;
	struct pinctrl *pinctrl;
	struct pinctrl_state *pins_default;
	struct pinctrl_state *pins_cfg;
	u32 ints[2] = {0, 0};

	alspsPltFmDev = get_alsps_platformdev();
/* gpio setting */
	pinctrl = devm_pinctrl_get(&alspsPltFmDev->dev);//get pinctrl
	if (IS_ERR(pinctrl)) {
		ret = PTR_ERR(pinctrl);
		APS_ERR("Cannot find alsps pinctrl!\n");
	}
	pins_default = pinctrl_lookup_state(pinctrl, "pin_default");
	if (IS_ERR(pins_default)) {
		ret = PTR_ERR(pins_default);
		APS_ERR("Cannot find alsps pinctrl default!\n");

	}

	pins_cfg = pinctrl_lookup_state(pinctrl, "pin_cfg");
	if (IS_ERR(pins_cfg)) {
		ret = PTR_ERR(pins_cfg);
		APS_ERR("Cannot find alsps pinctrl pin_cfg!\n");

	}
/* eint request */
	if (ltr579_obj->irq_node) {
		of_property_read_u32_array(ltr579_obj->irq_node, "debounce", ints, ARRAY_SIZE(ints));
		gpio_request(ints[0], "p-sensor");
		gpio_set_debounce(ints[0], ints[1]);
		pinctrl_select_state(pinctrl, pins_cfg);
		APS_ERR("ints[0] = %d, ints[1] = %d!!\n", ints[0], ints[1]);

		ltr579_obj->irq = irq_of_parse_and_map(ltr579_obj->irq_node, 0);
		APS_ERR("ltr579_obj->irq = %d\n", ltr579_obj->irq);
		if (!ltr579_obj->irq) {
			APS_ERR("irq_of_parse_and_map fail!!\n");
			return -EINVAL;
		}
		if (request_irq(ltr579_obj->irq, alsps_interrupt_handler, IRQF_TRIGGER_LOW, "als_ps", NULL)) {
			APS_ERR("IRQ LINE NOT AVAILABLE!!\n");
			return -EINVAL;
		}
		//enable_irq(ltr579_obj->irq);
	} else {
		APS_ERR("null irq node!!\n");
		return -EINVAL;
	}

#endif /* #ifdef CUSTOM_KERNEL_SENSORHUB */
	return 0;
}
//added by peixuan.qiu end 20151119


/*----------------------------------------------------------------------------*/

static struct alsps_init_info ltr579_init_info = {
		.name = "ltr579",
		.init = ltr579_local_init,
		.uninit = ltr579_remove,
};
static int __init ltr579_init(void)
{
	const char *name = "mediatek,ltr579";

	APS_FUN();

	hw = get_alsps_dts_func(name, hw);
	APS_LOG("[ltr579]i2c_num=%d, i2c_addr=%x, ps_polling_mode=%d\n", hw->i2c_num, hw->i2c_addr[0], hw->polling_mode_ps);
	if (!hw)
		APS_ERR("get dts info fail\n");
	alsps_driver_add(&ltr579_init_info);
	return 0;
}

/*----------------------------------------------------------------------------*/
static void __exit ltr579_exit(void)
{
	APS_FUN();
}

module_init(ltr579_init);
module_exit(ltr579_exit);
MODULE_AUTHOR("HongGuang Zhao");
MODULE_DESCRIPTION("modified by peixuan.qiu for android_M mt6735");
MODULE_LICENSE("GPL");

