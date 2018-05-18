/*Transsion Top Secret*/
#ifndef OV8855OTP
#define OV8855OTP

#include "ov8855mipiraw_Sensor.h"
struct otp_struct {
int flag; // bit[7]: info, bit[6]:wb, bit[5]:vcm, bit[4]:lenc
int module_integrator_id;
int lens_id;
int production_year;
int production_month;
int production_day;
int rg_ratio;
int bg_ratio;
int lenc[240];
int checksum;
int VCM_start;
int VCM_end;
int VCM_dir;
};

// return value:
// bit[7]: 0 no otp info, 1 valid otp info
// bit[6]: 0 no otp wb, 1 valib otp wb
// bit[5]: 0 no otp vcm, 1 valid otp vcm
// bit[4]: 0 no otp lenc/invalid otp lenc, 1 valid otp lenc
int read_otp(struct otp_struct *otp_ptr)
{
	int otp_flag, addr, temp, i;
	//set 0x5002[3] to ¡°0¡±
	int temp1;
	int checksum2=0;
	temp1 = read_cmos_sensor(0x5001);
	write_cmos_sensor(0x5001, (0x00 & 0x08) | (temp1 & (~0x08)));
	// read OTP into buffer
	write_cmos_sensor(0x3d84, 0xC0);
	write_cmos_sensor(0x3d88, 0x70); // OTP start address
	write_cmos_sensor(0x3d89, 0x10);
	write_cmos_sensor(0x3d8A, 0x72); // OTP end address
	write_cmos_sensor(0x3d8B, 0x0a);
	write_cmos_sensor(0x3d81, 0x01); // load otp into buffer
	mdelay(10);
	// OTP base information and WB calibration data
	otp_flag = read_cmos_sensor(0x7010);
	addr = 0;
	if((otp_flag & 0xc0) == 0x40) {
		addr = 0x7011; // base address of info group 1
	}
	else if((otp_flag & 0x30) == 0x10) {
		addr = 0x7019; // base address of info group 2
	}
	if(addr != 0) {
		(*otp_ptr).flag = 0xC0; // valid info and AWB in OTP
		(*otp_ptr).module_integrator_id = read_cmos_sensor(addr);
		 LOG_INF("module_integrator_id : 0x%x\n",(*otp_ptr).module_integrator_id);	

		(*otp_ptr).lens_id = read_cmos_sensor( addr + 1);
		(*otp_ptr).production_year = read_cmos_sensor( addr + 2);
		(*otp_ptr).production_month = read_cmos_sensor( addr + 3);
		(*otp_ptr).production_day = read_cmos_sensor(addr + 4);
		temp = read_cmos_sensor(addr + 7);
		(*otp_ptr).rg_ratio = (read_cmos_sensor(addr + 5)<<2) + ((temp>>6) & 0x03);
		(*otp_ptr).bg_ratio = (read_cmos_sensor(addr + 6)<<2) + ((temp>>4) & 0x03);
	}
	else {
		(*otp_ptr).flag = 0x00; // not info and AWB in OTP
		(*otp_ptr).module_integrator_id = 0;
		(*otp_ptr).lens_id = 0;
		(*otp_ptr).production_year = 0;
		(*otp_ptr).production_month = 0;
		(*otp_ptr).production_day = 0;
		(*otp_ptr).rg_ratio = 0;
		(*otp_ptr).bg_ratio = 0;
	}
	 LOG_INF("module_integrator_id : 0x%x,year = %d,month=%d,day =%d\n",(*otp_ptr).module_integrator_id,(*otp_ptr).production_year,(*otp_ptr).production_month,(*otp_ptr).production_day);	
	// OTP VCM Calibration
	otp_flag = read_cmos_sensor(0x7021);
	addr = 0;
	if((otp_flag & 0xc0) == 0x40) {
		addr = 0x7022; // base address of VCM Calibration group 1
	}
	else if((otp_flag & 0x30) == 0x10) {
		addr = 0x7025; // base address of VCM Calibration group 2
	}
	if(addr != 0) {
		(*otp_ptr).flag |= 0x20;
		temp = read_cmos_sensor(addr + 2);
		(* otp_ptr).VCM_start = (read_cmos_sensor(addr)<<2) | ((temp>>6) & 0x03);
		(* otp_ptr).VCM_end = (read_cmos_sensor(addr + 1) << 2) | ((temp>>4) & 0x03);
		(* otp_ptr).VCM_dir = (temp>>2) & 0x03;
	}
	else {
		(* otp_ptr).VCM_start = 0;
		(* otp_ptr).VCM_end = 0;
		(* otp_ptr).VCM_dir = 0;
	}
	// OTP Lenc Calibration
	otp_flag = read_cmos_sensor(0x7028);
	addr = 0;
	if((otp_flag & 0xc0) == 0x40) {
		addr = 0x7029; // base address of Lenc Calibration group 1
	}
	else if((otp_flag & 0x30) == 0x10) {
		addr = 0x711a; // base address of Lenc Calibration group 2
	}
	if(addr != 0) {
		for(i=0;i<240;i++) {
			(* otp_ptr).lenc[i]=read_cmos_sensor(addr + i);
			checksum2 += (* otp_ptr).lenc[i];
		}
		checksum2 = (checksum2)%255 +1;
		(*otp_ptr).checksum = read_cmos_sensor(addr + 240);
		if((*otp_ptr).checksum == checksum2){
			LOG_INF("ov8855 LSC checksum pass !\n");
			(*otp_ptr).flag |= 0x10;	
		}
		else{
			LOG_INF("ov8855 LSC checksum fail !set_checksum =%d,read_checksum=%d\n",(*otp_ptr).checksum,checksum2);
		}
	}
	else {
		for(i=0;i<240;i++) {
			(* otp_ptr).lenc[i]=0;
		}
	}
	for(i=0x7010;i<=0x720a;i++) {
		write_cmos_sensor(i,0); // clear OTP buffer, recommended use continuous write to accelarate
	}
	//set 0x5002[3] to ¡°1¡±
	temp1 = read_cmos_sensor(0x5001);
	write_cmos_sensor(0x5001, (0x08 & 0x08) | (temp1 & (~0x08)));
	return (*otp_ptr).flag;
}
// return value:
// bit[7]: 0 no otp info, 1 valid otp info
// bit[6]: 0 no otp wb, 1 valib otp wb
// bit[5]: 0 no otp vcm, 1 valid otp vcm
// bit[4]: 0 no otp lenc, 1 valid otp lenc

int apply_otp(struct otp_struct *otp_ptr)
{
//RG_Ratio_Typical and BG_Ratio_Typical get form O-Film  hejun
	int RG_Ratio_Typical = 0x12f, BG_Ratio_Typical = 0x171;
	int rg, bg, R_gain, G_gain, B_gain, Base_gain, temp, i;
	// apply OTP WB Calibration
	if ((*otp_ptr).flag & 0x40) {
		rg = (*otp_ptr).rg_ratio;
		bg = (*otp_ptr).bg_ratio;
		//calculate G gain
		R_gain = (RG_Ratio_Typical*1000) / rg;
		B_gain = (BG_Ratio_Typical*1000) / bg;
		G_gain = 1000;
		if (R_gain < 1000 || B_gain < 1000)
		{
			if (R_gain < B_gain)
				Base_gain = R_gain;
			else
				Base_gain = B_gain;
		}
		else
		{
			Base_gain = G_gain;
		}
		R_gain = 0x400 * R_gain / (Base_gain);
		B_gain = 0x400 * B_gain / (Base_gain);
		G_gain = 0x400 * G_gain / (Base_gain);

		LOG_INF("R_gain = %d, B_gain = %d, G_gain =%d, \n", R_gain, B_gain, G_gain);
		// update sensor WB gain
		if (R_gain>0x400) {
			write_cmos_sensor(0x5019, R_gain>>8);
			write_cmos_sensor(0x501a, R_gain & 0x00ff);
		}
		if (G_gain>0x400) {
			write_cmos_sensor(0x501b, G_gain>>8);
			write_cmos_sensor(0x501c, G_gain & 0x00ff);
		}
		if (B_gain>0x400) {
			write_cmos_sensor(0x501d, B_gain>>8);
			write_cmos_sensor(0x501e, B_gain & 0x00ff);
		}
	}
	// apply OTP Lenc Calibration
	if ((*otp_ptr).flag & 0x10) {
		temp = read_cmos_sensor(0x5000);
		temp = 0x20 | temp;
		write_cmos_sensor(0x5000, temp);
		for(i=0;i<240;i++) {
			write_cmos_sensor(0x5900 + i, (*otp_ptr).lenc[i]);
		}
	}
	return (*otp_ptr).flag;
}

#endif
