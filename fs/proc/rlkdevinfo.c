/*Transsion Top Secret*/
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#define NAME_LENGTH 32

static char *all_mem_name[] =
{  
	"H9TQ17ABJTMCUR_KUM", 
	"KMRE1000BM_B512",
	"16EMCP16_EL3DT527",
	"KMFE10012M_B214",
	"KMQE10013M_B318"
};
static char lcm_name[NAME_LENGTH] = "no";
static char ctp_name[NAME_LENGTH] = "no";
static char front_camera_name[NAME_LENGTH] = "no";
static char back_camera_name[NAME_LENGTH] = "no";
static char g_sensor_name[NAME_LENGTH] = "no";
static char als_ps_sensor_name[NAME_LENGTH] = "no";
static char m_sensor_name[NAME_LENGTH] = "no";
static char memory_name[NAME_LENGTH] = "no";
static char fingerprint_name[NAME_LENGTH] = "no";

void app_get_lcm_name(char *name)  //lcm
{
     if(ctp_name!=NULL)
        strcpy(lcm_name,name);
}
EXPORT_SYMBOL(app_get_lcm_name);

void app_get_ctp_name(char *name)  //ctp
{
     if(ctp_name!=NULL)
        strcpy(ctp_name,name);
}
EXPORT_SYMBOL(app_get_ctp_name);
 
void app_get_back_sensor_name(char *back_name)  //front_camera
{
     if(back_name!=NULL)
        strcpy(back_camera_name,back_name);
}
EXPORT_SYMBOL(app_get_back_sensor_name); 

void app_get_front_sensor_name(char *front_name) //back_camera
{	 
     if(front_name!=NULL)	 
       strcpy(front_camera_name,front_name);
}
EXPORT_SYMBOL(app_get_front_sensor_name); 

void app_get_g_sensor_name(char *g_name) //g_sensor
{
     if(g_name!=NULL)
       strcpy(g_sensor_name,g_name);
}
EXPORT_SYMBOL(app_get_g_sensor_name); 

void app_get_als_ps_sensor_name(char *als_ps_name) //als_ps_sensor
{
     if(als_ps_name!=NULL)
       strcpy(als_ps_sensor_name,als_ps_name);
}
EXPORT_SYMBOL(app_get_als_ps_sensor_name); 

void app_get_m_sensor_name(char *m_name) //m_sensor
{
     if(m_name!=NULL)
       strcpy(m_sensor_name,m_name);
}
EXPORT_SYMBOL(app_get_m_sensor_name); 

void app_get_fingerprint_name(char *name)  //fingerprint
{
      if(name != NULL)
         strcpy(fingerprint_name, name);
 }
EXPORT_SYMBOL(app_get_fingerprint_name);


// +++++++++++++memory info+++++++++++++
#if 1
#define NUM_EMI_RECORD 4
static int first_time = 0;
static void get_mem(void);

static void get_mem(){
	char *p = NULL;
	int i = -1;
	p = strstr(saved_command_line, "mem_index=");
	if (NULL == p) {
	   	return;
	}
	p += 10;
	i = simple_strtol(p, NULL, 10);
	if (i < 0 || i >=  NUM_EMI_RECORD)
		return;
	strcpy(memory_name,all_mem_name[i]);
}
#endif
// -------------memory info-------------

static int rlkdevinfo_proc_show(struct seq_file *m, void *v)
{
	int i = 0;
	seq_printf(m, "[LCM=%s]\n", lcm_name);
	seq_printf(m, "[CTP=%s]\n", ctp_name);
	seq_printf(m, "[FRONT_CAMERA=%s]\n", front_camera_name);
	seq_printf(m, "[BACK_CAMERA=%s]\n", back_camera_name);
	seq_printf(m, "[G_SENSOR=%s]\n", g_sensor_name);
	seq_printf(m, "[ALSPS_SENSOR=%s]\n", als_ps_sensor_name);
	seq_printf(m, "[M_SENSOR=%s]\n", m_sensor_name);
	//格式必须是"[xxx=%s]\n",否则工厂模式指纹测试会失败
        seq_printf(m, "[FINGERPRINT=%s]\n", fingerprint_name);
#if 1
	if (!first_time){
		get_mem();
		first_time = 1;
	}
	seq_printf(m, "[CURRENT MEMORY=%s]\n", memory_name);
	seq_printf(m, "[ALL MEMORY=\n");
	for (i = 0; i < NUM_EMI_RECORD; i++) {
		seq_printf(m, " (%d):%s\n", i,all_mem_name[i]);
	}
	seq_printf(m, "]\n");
#endif
	return 0;
}

static int rlkdevinfo_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, rlkdevinfo_proc_show, NULL);
}

static const struct file_operations rlkdevinfo_proc_fops = {
	.open		= rlkdevinfo_proc_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= single_release,
};

//  /proc/rlkdevinfo
static int __init proc_rlkdevinfo_init(void)
{
	proc_create("rlkdevinfo", 0, NULL, &rlkdevinfo_proc_fops);
	return 0;
}
fs_initcall(proc_rlkdevinfo_init);
