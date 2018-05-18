/*Transsion Top Secret*/
/*
 *
 * FocalTech TouchScreen driver.
 * 
 * Copyright (c) 2010-2015, Focaltech Ltd. All rights reserved.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

 /*******************************************************************************
*
* File Name: Focaltech_Gestrue.c
*
* Author: Xu YongFeng
*
* Created: 2015-01-29
*   
* Modify by mshl on 2015-03-20
*
* Abstract:
*
* Reference:
*
*******************************************************************************/

/*******************************************************************************
* 1.Included header files
*******************************************************************************/
#include "focaltech_core.h"
#include "include/tpd_common.h"
#if FTS_GESTRUE_EN
/*******************************************************************************
* Private constant and macro definitions using #define
*******************************************************************************/
#define GESTURE_LEFT							0x20
#define GESTURE_RIGHT							0x21
#define GESTURE_UP		    					0x22
#define GESTURE_DOWN							0x23
#define GESTURE_DOUBLECLICK						0x24
#define GESTURE_O		    					0x30
#define GESTURE_W		    					0x31
#define GESTURE_M		   	 					0x32
#define GESTURE_E		    					0x33
#define GESTURE_C		    					0x34
#define GESTURE_L		    					0x44
#define GESTURE_S		    					0x46
#define GESTURE_V		    					0x54
#define GESTURE_Z		    					0x65
#define FTS_GESTRUE_POINTS 						255
#define FTS_GESTRUE_POINTS_ONETIME  			62
#define FTS_GESTRUE_POINTS_HEADER 				8
#define FTS_GESTURE_OUTPUT_ADRESS 				0xD3
#define FTS_GESTURE_OUTPUT_UNIT_LENGTH 			4

#define GESTURE_LF			0xBB
#define GESTURE_RT			0xAA
#define GESTURE_down		0xAB
#define GESTURE_up			0xBA
#define GESTURE_DC			0xCC
#define GESTURE_o			0x6F
#define GESTURE_w			0x77
#define GESTURE_m			0x6D
#define GESTURE_e			0x65
#define GESTURE_c			0x63
#define GESTURE_s			0x73
#define GESTURE_v			0x76
#define GESTURE_z			0x7A
/*******************************************************************************
* Private enumerations, structures and unions using typedef
*******************************************************************************/


/*******************************************************************************
* Static variables
*******************************************************************************/
short pointnum 					= 0;
unsigned short coordinate_x[150] 	= {0};
unsigned short coordinate_y[150] 	= {0};


//extern int stk3x1x_get_psval(void);
int (*get_psval)(void);
#define GOODIX_MAGIC_NUMBER        'G'
#define NEGLECT_SIZE_MASK           (~(_IOC_SIZEMASK << _IOC_SIZESHIFT))

#define GESTURE_ENABLE_TOTALLY      _IO(GOODIX_MAGIC_NUMBER, 1)	// 1
#define GESTURE_DISABLE_TOTALLY     _IO(GOODIX_MAGIC_NUMBER, 2)
#define GESTURE_ENABLE_PARTLY       _IO(GOODIX_MAGIC_NUMBER, 3)
#define GESTURE_DISABLE_PARTLY      _IO(GOODIX_MAGIC_NUMBER, 4)
#define GESTURE_DATA_OBTAIN         (_IOR(GOODIX_MAGIC_NUMBER, 6, u8) & NEGLECT_SIZE_MASK)
#define GESTURE_DATA_ERASE          _IO(GOODIX_MAGIC_NUMBER, 7)

typedef enum
{
    DOZE_DISABLED = 0,
    DOZE_ENABLED = 1,
    DOZE_WAKEUP = 2,
}DOZE_T;
static DOZE_T doze_status = DOZE_DISABLED;
u8 g_gtp_gesture = 0;
static s32 setgesturestatus(unsigned int cmd,unsigned long arg);

#define GESTURE_NODE "gesture_function"
#define GESTURE_MAX_POINT_COUNT    64

#pragma pack(1)
typedef struct {
	u8 ic_msg[6];		/*from the first byte */
	u8 gestures[4];
	u8 data[3 + GESTURE_MAX_POINT_COUNT * 4 + 80];	/*80 bytes for extra data */
} st_gesture_data;
#pragma pack()

#define SETBIT(longlong, bit)   (longlong[bit/8] |=  (1 << bit%8))
#define CLEARBIT(longlong, bit) (longlong[bit/8] &=(~(1 << bit%8)))
#define QUERYBIT(longlong, bit) (!!(longlong[bit/8] & (1 << bit%8)))

int ft_gesture_enabled = 0;
static int gesturechar_enabled =0;
static int gesturedouble_enabled=0;

DOZE_T gesture_doze_status = DOZE_DISABLED;

static u8 gestures_flag[32];
static st_gesture_data gesture_data;
static struct mutex gesture_data_mutex;



static ssize_t gesture_status_show(struct device *dev,struct device_attribute *attr, char *buf)
{
return sprintf(buf,"%c:%d;\n%c:%d;\n%c:%d;\n%c:%d;\n%c:%d;\n%c:%d;\n%c:%d;\n%c:%d;\n%c:%d;\n%c:%d;\n%s:%d;\n%s:%d;\n",'w',QUERYBIT(gestures_flag,'w' ),'o',QUERYBIT(gestures_flag,'o'),'m',QUERYBIT(gestures_flag,'m'),'e',QUERYBIT(gestures_flag,'e'),'c',QUERYBIT(gestures_flag,'c'),'z',QUERYBIT(gestures_flag,'z'),'s',QUERYBIT(gestures_flag,'s'),'v',QUERYBIT(gestures_flag,'v'),'^',QUERYBIT(gestures_flag,0X5e),'>',QUERYBIT(gestures_flag,0x3e),"all",gesturechar_enabled,"cc",QUERYBIT(gestures_flag,0xcc));

}

static ssize_t gesture_status_store(struct device *dev,struct device_attribute *attr, const char *buf, size_t count)
{
return 0;

}
//static DEVICE_ATTR(gesture,(S_IWUSR|S_IRUGO|S_IWUGO),gesture_status_show, gesture_status_store);
static DEVICE_ATTR(gesture,0555,gesture_status_show, gesture_status_store);

static ssize_t gt1x_gesture_data_read(struct file *file, char __user * page, size_t size, loff_t * ppos)
{
	s32 ret = -1;
char buff[3]={'0'};
	if (*ppos) {
		return 0;
	}
	
ret=sprintf(buff,"%c",gesture_data.data[0]);
if(gesture_data.data[0]==0xcc)buff[0]='f';
ret = simple_read_from_buffer(page, size, ppos, buff, 1);
return ret;
	

}


static ssize_t gt1x_gesture_data_write(struct file *filp, const char __user * buf, size_t len, loff_t * off)
{
	int tempchar3=0;
	char tempchar2,tempchar1;
	u8 tempstatus=-1;
	char buff[4]={'0','0','0','\0'};
	s32 ret = copy_from_user(buff, buf, 3);
	if(ret){
		FTP_ERROR("copy_from_user failed.");
		return -EPERM;
	}
	if(buff[1]!='l'&&buff[1]!='0'&&buff[1]!='a'&&buff[1]!='b'&&buff[1]!='c'){
		FTP_INFO(" bad command format 0:%c 1:%c 2:%c!!!",buff[0],buff[1],buff[2]);
		return -EPERM;
	}

	if(buff[1]=='l' && (buff[2]=='1'||buff[2]=='2')){
		if(buff[2]=='2'){
			gesturechar_enabled=0;
			setgesturestatus(GESTURE_DISABLE_TOTALLY,0);
		}
		if(buff[2]=='1'){
			gesturechar_enabled=1;
			setgesturestatus(GESTURE_ENABLE_TOTALLY,1);
		}
		return  len;
	}
	
	if(buff[0]<'A'||buff[0]>'z'||(buff[2]!='1'&&buff[2]!='2')){
		FTP_INFO("bad command format 0:%c 1:%c 2:%c!!!",buff[0],buff[1],buff[2]);
		return -EPERM;
	}
	tempchar1=buff[0];
	tempchar2=buff[1];
	tempstatus=buff[2];

	switch(tempchar2)
	{
		case '0':
			switch(tempstatus)
			{
				case '2':
	                   		setgesturestatus(GESTURE_DISABLE_PARTLY,tempchar1);
	                    		break;
	                    	case '1':
	                    		setgesturestatus(GESTURE_ENABLE_PARTLY,tempchar1);
	                    		break;
	                    	default:
								return -EPERM;
	                    		break;          
			}
			break;
		default:
			buff[2]='\0';
			tempchar3= simple_strtoul(buff,NULL,16);

			switch(tempstatus)
			{                   
				case '2':
					if(tempchar3==0xcc)
						gesturedouble_enabled=0;
	                 			setgesturestatus(GESTURE_DISABLE_PARTLY,tempchar3);
	                    			break;
	                  	case '1':
					if(tempchar3==0xcc)
						gesturedouble_enabled=1;
	                    			setgesturestatus(GESTURE_ENABLE_PARTLY,tempchar3);
	                    			break;
	                    	default:
								return -EPERM;
	                    		break;               
			}
			break;
		}

	return len;
}
static const struct file_operations gt1x_fops = {
	.owner = THIS_MODULE,
	.read = gt1x_gesture_data_read,
	.write = gt1x_gesture_data_write,
};
static u8 is_all_dead(u8 * longlong, s32 size)
{
	int i = 0;
	u8 sum = 0;

	for (i = 0; i < size; i++) {
		sum |= longlong[i];
	}

	return !sum;
}


static void gesture_clear_wakeup_data(void)
{
	mutex_lock(&gesture_data_mutex);
	memset(gesture_data.data, 0, 4);
	mutex_unlock(&gesture_data_mutex);
}


static s32 setgesturestatus(unsigned int cmd,unsigned long arg)
{
u8 ret=0;
u32 value =  (u32) arg;;
switch (cmd & NEGLECT_SIZE_MASK) {
	case GESTURE_ENABLE_TOTALLY:
		FTP_DEBUG("ENABLE_GESTURE_TOTALLY");
		gesturechar_enabled=1;
		ft_gesture_enabled=((gesturechar_enabled)||(gesturedouble_enabled));
		break;

	case GESTURE_DISABLE_TOTALLY:
		FTP_DEBUG("DISABLE_GESTURE_TOTALLY");
		gesturechar_enabled=0;
		ft_gesture_enabled=((gesturechar_enabled)||(gesturedouble_enabled));
		break;

	case GESTURE_ENABLE_PARTLY:
		SETBIT(gestures_flag, (u8) value);
		ft_gesture_enabled = (gesturechar_enabled ||gesturedouble_enabled);
		FTP_DEBUG("ENABLE_GESTURE_PARTLY, gesture = 0x%02X, ft_gesture_enabled = %d", value, ft_gesture_enabled);
		break;

	case GESTURE_DISABLE_PARTLY:
		ret = QUERYBIT(gestures_flag, (u8) value);
		if (!ret) {
			break;
		}
		CLEARBIT(gestures_flag, (u8) value);

		if (is_all_dead(gestures_flag, sizeof(gestures_flag))) {
			ft_gesture_enabled = 0;
		}
		FTP_DEBUG("DISABLE_GESTURE_PARTLY, gesture = 0x%02X, ft_gesture_enabled = %d", value, ft_gesture_enabled);
		break;

	case GESTURE_DATA_OBTAIN:
		FTP_DEBUG("OBTAIN_GESTURE_DATA");

		mutex_lock(&gesture_data_mutex);
		if (gesture_data.data[1] > GESTURE_MAX_POINT_COUNT) {
			gesture_data.data[1] = GESTURE_MAX_POINT_COUNT;
		}
		if (gesture_data.data[3] > 80) {
			gesture_data.data[3] = 80;
		}
		ret = copy_to_user(((u8 __user *) arg), &gesture_data.data, 4 + gesture_data.data[1] * 4 + gesture_data.data[3]);
		mutex_unlock(&gesture_data_mutex);
		if (ret) {
			FTP_ERROR("ERROR when copy gesture data to user.");
			ret = -1;
		} else {
			ret = 4 + gesture_data.data[1] * 4 + gesture_data.data[3];
		}
		break;

	case GESTURE_DATA_ERASE:
		FTP_DEBUG("ERASE_GESTURE_DATA");
		gesture_clear_wakeup_data();
		break;
}



return 0;
}

static s32 gt1x_init_node(void)
{
	int ret=0;
	static struct kobject *gesture_status_kobj = NULL;
	struct proc_dir_entry *proc_entry = NULL;
	mutex_init(&gesture_data_mutex);
	memset(gestures_flag, 0, sizeof(gestures_flag));
	memset((u8 *) & gesture_data, 0, sizeof(st_gesture_data));

	proc_entry = proc_create(GESTURE_NODE, 0666, NULL, &gt1x_fops);
	if (proc_entry == NULL) {
		FTP_ERROR("Couldn't create proc entry[GESTURE_NODE]!");
		return -1;
	} else {
		FTP_INFO("Create proc entry[GESTURE_NODE] success!");
	}


	gesture_status_kobj = kobject_create_and_add("goodix", NULL);
	if (gesture_status_kobj == NULL) 
	{
		FTP_ERROR( "gesture_function create failed\n");
		ret = -ENOMEM;
		return ret;
	}

	ret = sysfs_create_file(gesture_status_kobj, &dev_attr_gesture.attr);
	if (ret) 
	{
		FTP_ERROR(" create_file gesture failed\n");
		return ret;
	}
	return 0;
}
/*
static void gt1x_deinit_node(void)
{
	remove_proc_entry(GESTURE_NODE, NULL);
}
*/
static void dump_gesture_reg(struct i2c_client *i2c_client)
{
	u8 reg_info[8] = {0};
	fts_read_reg(i2c_client, 0xd0, &reg_info[0]);
	fts_read_reg(i2c_client, 0xd1, &reg_info[1]);
	fts_read_reg(i2c_client, 0xd2, &reg_info[2]);
	fts_read_reg(i2c_client, 0xd5, &reg_info[3]);
	fts_read_reg(i2c_client, 0xd6, &reg_info[4]);
	fts_read_reg(i2c_client, 0xd7, &reg_info[5]);
	fts_read_reg(i2c_client, 0xd8, &reg_info[6]);
	FTP_INFO("d0:%x",reg_info[0]);
	FTP_INFO("d1:%x",reg_info[1]);
	FTP_INFO("d2:%x",reg_info[2]);
	FTP_INFO("d5:%x",reg_info[3]);
	FTP_INFO("d6:%x",reg_info[4]);
	FTP_INFO("d7:%x",reg_info[5]);
	FTP_INFO("d8:%x",reg_info[6]);
}
int gtp_enter_doze(struct i2c_client *i2c_client){
	int i = 0;
	u8 state = 0;
	FTP_INFO("[%s] enter doze\n",__func__);
	FTP_INFO("gesturechar:%d gesturedouble:%d ft_gesture_enabled:%d\n",gesturechar_enabled,gesturedouble_enabled,ft_gesture_enabled);
	ft_gesture_enabled=((gesturechar_enabled)||(gesturedouble_enabled));
	gesture_clear_wakeup_data();
	if(!ft_gesture_enabled){
		FTP_INFO("[%s] ft_gesture_enabled =%d\n",__func__,ft_gesture_enabled);
		return -1;
	}
#ifdef  TP_MONITOR_CTL
        fts_write_reg(i2c_client, 0x86, 1);
#endif
	fts_write_reg(i2c_client, 0xd0, 0x01);
	msleep(10);
	for(i=0;i<5;i++){
		FTP_INFO("enter doze write reg %d\n",i);
		fts_read_reg(i2c_client, 0xd0, &state);
		if(1==state){
			fts_write_reg(i2c_client, 0xd1, 0xff);
			fts_write_reg(i2c_client, 0xd2, 0xff);
			fts_write_reg(i2c_client, 0xd5, 0xff);
			fts_write_reg(i2c_client, 0xd6, 0xff);
			fts_write_reg(i2c_client, 0xd7, 0xff);
			fts_write_reg(i2c_client, 0xd8, 0xff);			
			FTP_INFO("TPD gesture write 0x01 to 0xd0\n");
			doze_status = DOZE_ENABLED;
			msleep(10);
			dump_gesture_reg(i2c_client);
			return 0;
		}else{
			fts_write_reg(i2c_client, 0xd0, 0x01);
			fts_write_reg(i2c_client, 0xd1, 0xff);
			fts_write_reg(i2c_client, 0xd2, 0xff);
			fts_write_reg(i2c_client, 0xd5, 0xff);
			fts_write_reg(i2c_client, 0xd6, 0xff);
			fts_write_reg(i2c_client, 0xd7, 0xff);
			fts_write_reg(i2c_client, 0xd8, 0xff);
			msleep(10);
		}
	}
	if(i>=5){
		FTP_ERROR("[%s] write 0x01 to d0 fail \n",__func__);
		return -1;
	}
	return -1;
}
 int gtp_leave_doze(struct i2c_client *i2c_client)
 {
	int i = 0;
	u8 state = 0;
	FTP_INFO("[%s] leave doze\n",__func__);
	if(ft_gesture_enabled){
		fts_write_reg(i2c_client, 0xd0, 0x00);
		msleep(10);
		for(i=0;i<5;i++){
			FTP_INFO("Leave doze write reg %d\n",i);
			fts_read_reg(i2c_client, 0xd0, &state);
			if(0==state){
				FTP_INFO("TPD gesture write 0x00 to 0xd0\n");
				doze_status = DOZE_DISABLED;
				return 0;
			}else{
				fts_write_reg(i2c_client, 0xd0, 0x00);
				msleep(10);
			}
		}
		if(i>=5){
			FTP_ERROR("[%s] write 0x00 to d0 fail \n",__func__);
			return -1;
		}
	}
	else{
		FTP_INFO("[%s] ft_gesture_enabled =%d\n",__func__,ft_gesture_enabled);
		return -1;
	}
	return -1;
 }




/*******************************************************************************
*   Name: fts_Gesture_init
*  Brief:
*  Input:
* Output: None
* Return: None
*******************************************************************************/
int fts_Gesture_init(struct input_dev *input_dev)
{
	int ret = 0;
	input_set_capability(input_dev, EV_KEY, KEY_POWER);
	input_set_capability(input_dev, EV_KEY, KEY_F13); 
	ret = gt1x_init_node();
	return ret;
}

/*******************************************************************************
*   Name: fts_check_gesture
*  Brief:
*  Input:
* Output: None
* Return: None
*******************************************************************************/
static int fts_check_gesture(struct input_dev *input_dev,int gesture_id)
{
	s32 psstatus = 0;
	FTP_INFO("fts gesture_id==0x%x\n ",gesture_id);
	switch(gesture_id)
	{
	        case GESTURE_LEFT:
				g_gtp_gesture = GESTURE_LF;
				break;
	        case GESTURE_RIGHT:
				g_gtp_gesture = GESTURE_RT;
				break;
	        case GESTURE_UP:
				g_gtp_gesture = GESTURE_up;
				break;
	        case GESTURE_DOWN:
				g_gtp_gesture = GESTURE_down;
				break;
	     	case GESTURE_DOUBLECLICK: 
				g_gtp_gesture = GESTURE_DC;
	            break;
	        case GESTURE_O:
				g_gtp_gesture = GESTURE_o;
	            break;
	        case GESTURE_W:
				g_gtp_gesture = GESTURE_w;
	            break;
	        case GESTURE_M:
				g_gtp_gesture = GESTURE_m;
	            break;
			case GESTURE_C:
				g_gtp_gesture = GESTURE_c;
				break;
	        case GESTURE_E:
				g_gtp_gesture = GESTURE_e;
	            break;
	        case GESTURE_S:
				g_gtp_gesture = GESTURE_s;
	            break;
	        case GESTURE_V:
				g_gtp_gesture = GESTURE_v;
	            break;
	        case GESTURE_Z:
				g_gtp_gesture = GESTURE_z;
	            break;
	        default:
				g_gtp_gesture = 0;
	            break;
	}
	// check 
	if(!QUERYBIT(gestures_flag,g_gtp_gesture)||(!gesturechar_enabled && g_gtp_gesture>='a'&&g_gtp_gesture<='z')){
		FTP_ERROR("this gesture has been disabled.");
		fts_write_reg(fts_i2c_client, 0xd3, 0);
		return -1;
	}
	if(get_psval){
		psstatus = get_psval();
		FTP_INFO("han_test_ps psstatus = %d\n",psstatus);
		if(psstatus == 1){
		    //ps is closed,can't report key
		    FTP_DEBUG("Can't wakeup by tp , ps value cover by something \n");
		}
	}
	else {
		FTP_DEBUG("get_psval() is not open!\n");
	}
	gesture_data.data[0] = g_gtp_gesture;
	gesture_data.data[1] = 0;
	gesture_data.data[2] = 0;
	gesture_data.data[3] = 0;

	if(psstatus == 0){
		input_report_key(tpd->dev, KEY_F13, 1);
		input_sync(tpd->dev);
		input_report_key(tpd->dev, KEY_F13, 0);
		input_sync(tpd->dev);
		return 0;
	}
	else{
		return -1;
	}
}

 /************************************************************************
*   Name: fts_read_Gestruedata
* Brief: read data from TP register
* Input: no
* Output: no
* Return: fail <0
***********************************************************************/
int fts_read_Gestruedata(void)
{
	unsigned char buf[2] = { 0 };
    int ret = -1;
	u8 state = 0;
    int gesture_id = 0;
	buf[0] = 0x00;
	//FTP_INFO("%s\n",__func__);
	if(DOZE_ENABLED == doze_status){
	    ret = fts_read_reg(fts_i2c_client, 0xd0,&state);
		if (ret<0){
			FTP_ERROR("[%s] read reg=d0 value fail\n",__func__);
			return -1;
		}
		FTP_DEBUG("[%s] read reg 0xd0 value:%x\n",__func__,&state);
		if(state ==1){
			ret = fts_read_reg(fts_i2c_client, 0xd3,buf);
			if(ret < 0){
				FTP_ERROR("[Focal][Touch] read reg = d3 value fail");
				return -1;
			}
			FTP_INFO("buf[0]:0x%x",buf[0]);
			if(buf[0]!=0xfe){
				gesture_id =  buf[0];
				ret = fts_check_gesture(fts_input_dev,gesture_id);
				if(ret){
					FTP_ERROR("[%s] fts_check_gesture error \n",__func__);
					return -1;
				}
			}
		}
		return 0;
	}
	return 1;
}
#endif
