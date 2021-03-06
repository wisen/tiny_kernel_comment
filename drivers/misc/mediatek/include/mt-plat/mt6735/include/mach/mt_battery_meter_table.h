/*Transsion Top Secret*/
/*
 * Copyright (C) 2015 MediaTek Inc.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#ifndef _CUST_BATTERY_METER_TABLE_H
#define _CUST_BATTERY_METER_TABLE_H

#define BAT_NTC_10 1
#define BAT_NTC_47 0

#if (BAT_NTC_10 == 1)
#define RBAT_PULL_UP_R             16900
#endif

#if (BAT_NTC_47 == 1)
#define RBAT_PULL_UP_R             61900
#endif

#define RBAT_PULL_UP_VOLT          1800

typedef struct _BATTERY_PROFILE_STRUCT {
	signed int percentage;
	signed int voltage;
} BATTERY_PROFILE_STRUCT, *BATTERY_PROFILE_STRUCT_P;

typedef struct _R_PROFILE_STRUCT {
	signed int resistance;
	signed int voltage;
} R_PROFILE_STRUCT, *R_PROFILE_STRUCT_P;

typedef enum {
	T1_0C,
	T2_25C,
	T3_50C
} PROFILE_TEMPERATURE;

#if (BAT_NTC_10 == 1)
BATT_TEMPERATURE Batt_Temperature_Table[] = {
	{-20, 68237},
	{-15, 53650},
	{-10, 42506},
	{ -5, 33892},
	{  0, 27219},
	{  5, 22021},
	{ 10, 17926},
	{ 15, 14674},
	{ 20, 12081},
	{ 25, 10000},
	{ 30, 8315},
	{ 35, 6948},
	{ 40, 5834},
	{ 45, 4917},
	{ 50, 4161},
	{ 55, 3535},
	{ 60, 3014}
};
#endif

#if (BAT_NTC_47 == 1)
	BATT_TEMPERATURE Batt_Temperature_Table[] = {
	{-20, 483954},
	{-15, 360850},
	{-10, 271697},
	{ -5, 206463},
	{  0, 158214},
	{  5, 122259},
	{ 10, 95227},
	{ 15, 74730},
	{ 20, 59065},
	{ 25, 47000},
	{ 30, 37643},
	{ 35, 30334},
	{ 40, 24591},
	{ 45, 20048},
	{ 50, 16433},
	{ 55, 13539},
	{ 60, 11210}
	};
#endif

/* T0 -10C */
BATTERY_PROFILE_STRUCT battery_profile_t0[] = {
	{0,4390},
	{2,4366},
	{3,4345},
	{5,4323},
	{7,4302},
	{8,4281},
	{10,4262},
	{12,4242},
	{13,4222},
	{15,4203},
	{17,4184},
	{18,4165},
	{20,4146},
	{21,4126},
	{23,4107},
	{25,4090},
	{26,4074},
	{28,4056},
	{30,4030},
	{31,4001},
	{33,3977},
	{35,3958},
	{36,3945},
	{38,3933},
	{40,3922},
	{41,3911},
	{43,3899},
	{45,3887},
	{46,3876},
	{48,3866},
	{50,3856},
	{51,3847},
	{53,3837},
	{55,3830},
	{56,3822},
	{58,3815},
	{60,3808},
	{61,3802},
	{63,3797},
	{64,3792},
	{66,3788},
	{68,3785},
	{69,3782},
	{71,3779},
	{73,3775},
	{74,3772},
	{76,3768},
	{78,3762},
	{79,3757},
	{81,3751},
	{83,3743},
	{84,3735},
	{86,3725},
	{88,3717},
	{89,3709},
	{91,3702},
	{93,3694},
	{94,3682},
	{96,3655},
	{97,3607},
	{98,3571},
	{99,3538},
	{99,3508},
	{99,3481},
	{100,3454},
	{100,3432},
	{100,3410},
	{100,3400},
	{100,3400}

};

/* T1 0C */
BATTERY_PROFILE_STRUCT battery_profile_t1[] = {
	{0,4392},
	{2,4372},
	{3,4354},
	{5,4334},
	{7,4315},
	{8,4295},
	{10,4275},
	{12,4256},
	{13,4237},
	{15,4217},
	{17,4198},
	{18,4179},
	{20,4161},
	{21,4142},
	{23,4125},
	{25,4106},
	{26,4091},
	{28,4077},
	{30,4059},
	{31,4034},
	{33,4010},
	{35,3991},
	{36,3976},
	{38,3960},
	{40,3944},
	{41,3930},
	{43,3915},
	{45,3901},
	{46,3888},
	{48,3877},
	{50,3867},
	{51,3858},
	{53,3849},
	{54,3841},
	{56,3834},
	{58,3826},
	{59,3820},
	{61,3814},
	{63,3808},
	{64,3804},
	{66,3799},
	{68,3794},
	{69,3790},
	{71,3787},
	{73,3783},
	{74,3780},
	{76,3776},
	{78,3772},
	{79,3767},
	{81,3761},
	{83,3753},
	{84,3743},
	{86,3734},
	{87,3723},
	{89,3711},
	{91,3706},
	{92,3703},
	{94,3698},
	{96,3683},
	{97,3620},
	{99,3512},
	{100,3400},
	{100,3400},
	{100,3400},
	{100,3400},
	{100,3400},
	{100,3400},
	{100,3400},
	{100,3400}

};

/* T2 25C */
BATTERY_PROFILE_STRUCT battery_profile_t2[] = {
	{0,4391},
	{2,4370},
	{3,4350},
	{5,4330},
	{7,4312},
	{8,4292},
	{10,4271},
	{12,4252},
	{13,4232},
	{15,4213},
	{16,4195},
	{18,4175},
	{20,4156},
	{21,4138},
	{23,4120},
	{25,4102},
	{26,4085},
	{28,4070},
	{30,4053},
	{31,4034},
	{33,4018},
	{35,4002},
	{36,3987},
	{38,3973},
	{40,3961},
	{41,3943},
	{43,3925},
	{45,3905},
	{46,3889},
	{48,3876},
	{49,3867},
	{51,3855},
	{53,3847},
	{54,3839},
	{56,3831},
	{58,3824},
	{59,3818},
	{61,3812},
	{63,3807},
	{64,3801},
	{66,3796},
	{68,3791},
	{69,3787},
	{71,3782},
	{73,3777},
	{74,3770},
	{76,3764},
	{77,3757},
	{79,3751},
	{81,3748},
	{82,3740},
	{84,3731},
	{86,3723},
	{87,3712},
	{89,3699},
	{91,3695},
	{92,3694},
	{94,3692},
	{96,3681},
	{97,3624},
	{99,3526},
	{100,3400},
	{100,3400},
	{100,3400},
	{100,3400},
	{100,3400},
	{100,3400},
	{100,3400},
	{100,3400}

};

/* T3 50C */
BATTERY_PROFILE_STRUCT battery_profile_t3[] = {
	{0,4382},
	{2,4361},
	{3,4343},
	{5,4325},
	{7,4305},
	{8,4286},
	{10,4267},
	{12,4246},
	{13,4226},
	{15,4207},
	{17,4187},
	{18,4168},
	{20,4149},
	{22,4130},
	{23,4112},
	{25,4094},
	{27,4076},
	{28,4059},
	{30,4042},
	{32,4026},
	{33,4009},
	{35,3995},
	{37,3980},
	{38,3966},
	{40,3951},
	{42,3935},
	{44,3914},
	{45,3894},
	{47,3879},
	{49,3867},
	{50,3857},
	{52,3847},
	{54,3839},
	{55,3830},
	{57,3823},
	{59,3815},
	{60,3808},
	{62,3802},
	{64,3797},
	{65,3791},
	{67,3786},
	{69,3781},
	{70,3775},
	{72,3766},
	{74,3754},
	{75,3747},
	{77,3740},
	{79,3732},
	{80,3727},
	{82,3720},
	{84,3713},
	{85,3703},
	{87,3694},
	{89,3681},
	{90,3676},
	{92,3674},
	{94,3671},
	{95,3664},
	{97,3612},
	{99,3522},
	{100,3400},
	{100,3400},
	{100,3400},
	{100,3400},
	{100,3400},
	{100,3400},
	{100,3400},
	{100,3400},
	{100,3400}

};

/* battery profile for actual temperature. The size should be the same as T1, T2 and T3 */
BATTERY_PROFILE_STRUCT battery_profile_temperature[] = {
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0}
};

/* T0 -10C */
R_PROFILE_STRUCT r_profile_t0[] = {
	{827,4390},
	{827,4366},
	{830,4345},
	{823,4323},
	{815,4302},
	{802,4281},
	{800,4262},
	{790,4242},
	{788,4222},
	{775,4203},
	{765,4184},
	{760,4165},
	{755,4146},
	{743,4126},
	{743,4107},
	{735,4090},
	{732,4074},
	{733,4056},
	{720,4030},
	{698,4001},
	{685,3977},
	{675,3958},
	{672,3945},
	{672,3933},
	{673,3922},
	{673,3911},
	{670,3899},
	{668,3887},
	{670,3876},
	{673,3866},
	{675,3856},
	{680,3847},
	{680,3837},
	{690,3830},
	{693,3822},
	{700,3815},
	{710,3808},
	{715,3802},
	{725,3797},
	{735,3792},
	{745,3788},
	{755,3785},
	{770,3782},
	{785,3779},
	{798,3775},
	{825,3772},
	{848,3768},
	{873,3762},
	{903,3757},
	{940,3751},
	{982,3743},
	{1038,3735},
	{1108,3725},
	{1205,3717},
	{1328,3709},
	{1478,3702},
	{1600,3694},
	{1698,3682},
	{1830,3655},
	{2018,3607},
	{1930,3571},
	{1850,3538},
	{1778,3508},
	{1738,3481},
	{1670,3454},
	{1585,3432},
	{1528,3410},
	{1508,3400},
	{1473,3400}

};

/* T1 0C */
R_PROFILE_STRUCT r_profile_t1[] = {
	{293,4392},
	{293,4372},
	{293,4354},
	{282,4334},
	{290,4315},
	{290,4295},
	{288,4275},
	{290,4256},
	{293,4237},
	{290,4217},
	{290,4198},
	{290,4179},
	{295,4161},
	{298,4142},
	{300,4125},
	{300,4106},
	{303,4091},
	{313,4077},
	{315,4059},
	{310,4034},
	{307,4010},
	{305,3991},
	{303,3976},
	{287,3960},
	{277,3944},
	{273,3930},
	{263,3915},
	{260,3901},
	{255,3888},
	{255,3877},
	{255,3867},
	{255,3858},
	{258,3849},
	{258,3841},
	{260,3834},
	{260,3826},
	{263,3820},
	{265,3814},
	{267,3808},
	{273,3804},
	{270,3799},
	{273,3794},
	{273,3790},
	{273,3787},
	{273,3783},
	{273,3780},
	{273,3776},
	{277,3772},
	{277,3767},
	{283,3761},
	{285,3753},
	{285,3743},
	{293,3734},
	{303,3723},
	{313,3711},
	{330,3706},
	{360,3703},
	{408,3698},
	{490,3683},
	{580,3620},
	{835,3512},
	{1423,3400},
	{1198,3400},
	{988,3400},
	{890,3400},
	{835,3400},
	{790,3400},
	{795,3400},
	{715,3400}

};

/* T2 25C */
R_PROFILE_STRUCT r_profile_t2[] = {
	{128,4391},
	{128,4370},
	{127,4350},
	{123,4330},
	{130,4312},
	{130,4292},
	{125,4271},
	{128,4252},
	{130,4232},
	{133,4213},
	{133,4195},
	{133,4175},
	{133,4156},
	{140,4138},
	{140,4120},
	{135,4102},
	{140,4085},
	{143,4070},
	{150,4053},
	{150,4034},
	{153,4018},
	{155,4002},
	{158,3987},
	{165,3973},
	{167,3961},
	{160,3943},
	{150,3925},
	{137,3905},
	{127,3889},
	{123,3876},
	{125,3867},
	{118,3855},
	{125,3847},
	{123,3839},
	{123,3831},
	{123,3824},
	{130,3818},
	{130,3812},
	{133,3807},
	{130,3801},
	{130,3796},
	{137,3791},
	{140,3787},
	{135,3782},
	{133,3777},
	{128,3770},
	{125,3764},
	{125,3757},
	{123,3751},
	{128,3748},
	{125,3740},
	{130,3731},
	{130,3723},
	{128,3712},
	{127,3699},
	{125,3695},
	{133,3694},
	{143,3692},
	{150,3681},
	{145,3624},
	{160,3526},
	{213,3400},
	{818,3400},
	{710,3400},
	{668,3400},
	{630,3400},
	{588,3400},
	{535,3400},
	{503,3400}
};

/* T3 50C */
R_PROFILE_STRUCT r_profile_t3[] = {
	{90,4382},
	{90,4361},
	{90,4343},
	{92,4325},
	{90,4305},
	{90,4286},
	{95,4267},
	{90,4246},
	{92,4226},
	{95,4207},
	{92,4187},
	{95,4168},
	{97,4149},
	{97,4130},
	{100,4112},
	{103,4094},
	{102,4076},
	{103,4059},
	{105,4042},
	{107,4026},
	{108,4009},
	{115,3995},
	{115,3980},
	{118,3966},
	{123,3951},
	{123,3935},
	{110,3914},
	{98,3894},
	{92,3879},
	{92,3867},
	{93,3857},
	{92,3847},
	{95,3839},
	{95,3830},
	{98,3823},
	{98,3815},
	{97,3808},
	{100,3802},
	{103,3797},
	{105,3791},
	{108,3786},
	{108,3781},
	{107,3775},
	{103,3766},
	{92,3754},
	{97,3747},
	{98,3740},
	{93,3732},
	{95,3727},
	{95,3720},
	{95,3713},
	{92,3703},
	{95,3694},
	{92,3681},
	{93,3676},
	{98,3674},
	{100,3671},
	{110,3664},
	{100,3612},
	{113,3522},
	{135,3400},
	{778,3400},
	{690,3400},
	{638,3400},
	{543,3400},
	{483,3400},
	{338,3400},
	{280,3400},
	{240,3400}
};

/* r-table profile for actual temperature. The size should be the same as T1, T2 and T3 */
R_PROFILE_STRUCT r_profile_temperature[] = {
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0}
};

/* ============================================================
// function prototype
// ============================================================*/
int fgauge_get_saddles(void);
BATTERY_PROFILE_STRUCT_P fgauge_get_profile(unsigned int temperature);

int fgauge_get_saddles_r_table(void);
R_PROFILE_STRUCT_P fgauge_get_profile_r_table(unsigned int temperature);

#endif	/*#ifndef _CUST_BATTERY_METER_TABLE_H*/

