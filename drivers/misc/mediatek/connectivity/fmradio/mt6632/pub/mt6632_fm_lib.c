/*
* Copyright (C) 2016 MediaTek Inc.
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License version 2 as
* published by the Free Software Foundation.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
* See http://www.gnu.org/licenses/gpl-2.0.html for more details.
*/

#include <linux/slab.h>
#include <linux/vmalloc.h>

#include "stp_exp.h"
#include "wmt_exp.h"

#include "fm_typedef.h"
#include "fm_dbg.h"
#include "fm_err.h"
#include "fm_interface.h"
#include "fm_stdlib.h"
#include "fm_patch.h"
#include "fm_utils.h"
#include "fm_link.h"
#include "fm_config.h"
#include "fm_cmd.h"

#include "mt6632_fm_reg.h"
#include "mt6632_fm_lib.h"

static struct fm_patch_tbl mt6632_patch_tbl[5] = {
	{FM_ROM_V1, "mt6632_fm_v1_patch.bin", "mt6632_fm_v1_coeff.bin", NULL, NULL},
	{FM_ROM_V2, "mt6632_fm_v2_patch.bin", "mt6632_fm_v2_coeff.bin", NULL, NULL},
	{FM_ROM_V3, "mt6632_fm_v3_patch.bin", "mt6632_fm_v3_coeff.bin", NULL, NULL},
	{FM_ROM_V4, "mt6632_fm_v4_patch.bin", "mt6632_fm_v4_coeff.bin", NULL, NULL},
	{FM_ROM_V5, "mt6632_fm_v5_patch.bin", "mt6632_fm_v5_coeff.bin", NULL, NULL},
};


static struct fm_hw_info mt6632_hw_info = {
	.chip_id = 0x00006632,
	.eco_ver = 0x00000000,
	.rom_ver = 0x00000000,
	.patch_ver = 0x00000000,
	.reserve = 0x00000000,
};

fm_u8 *cmd_buf;
struct fm_lock *cmd_buf_lock;
struct fm_res_ctx *fm_res;
static struct fm_callback *fm_cb_op;
static fm_u8 fm_packaging = 1;	/*0:QFN,1:WLCSP */
static fm_u32 fm_sant_flag;	/* 1,Short Antenna;  0, Long Antenna */
static fm_s32 mt6632_is_dese_chan(fm_u16 freq);
#if 0
static fm_s32 mt6632_mcu_dese(fm_u16 freq, void *arg);
#endif
static fm_s32 mt6632_gps_dese(fm_u16 freq, void *arg);

static fm_s32 mt6632_I2s_Setting(fm_s32 onoff, fm_s32 mode, fm_s32 sample);
static fm_u16 mt6632_chan_para_get(fm_u16 freq);
static fm_s32 mt6632_desense_check(fm_u16 freq, fm_s32 rssi);
static fm_bool mt6632_TDD_chan_check(fm_u16 freq);
static fm_s32 mt6632_soft_mute_tune(fm_u16 freq, fm_s32 *rssi, fm_bool *valid);

static fm_s32 mt6632_pwron(fm_s32 data)
{
	if (MTK_WCN_BOOL_FALSE == mtk_wcn_wmt_func_on(WMTDRV_TYPE_FM)) {
		WCN_DBG(FM_ERR | CHIP, "WMT turn on FM Fail!\n");
		return -FM_ELINK;
	}

	WCN_DBG(FM_NTC | CHIP, "WMT turn on FM OK!\n");
	return 0;
}

static fm_s32 mt6632_pwroff(fm_s32 data)
{
	if (MTK_WCN_BOOL_FALSE == mtk_wcn_wmt_func_off(WMTDRV_TYPE_FM)) {
		WCN_DBG(FM_ERR | CHIP, "WMT turn off FM Fail!\n");
		return -FM_ELINK;
	}

	WCN_DBG(FM_NTC | CHIP, "WMT turn off FM OK!\n");
	return 0;
}

static fm_u16 mt6632_get_chipid(void)
{
	return 0x6632;
}

/*  MT6632_SetAntennaType - set Antenna type
 *  @type - 1,Short Antenna;  0, Long Antenna
 */
static fm_s32 mt6632_SetAntennaType(fm_s32 type)
{
	fm_u16 dataRead;

	WCN_DBG(FM_NTC | CHIP, "set ana to %s\n", type ? "short" : "long");
	if (fm_packaging == 0) {
		fm_sant_flag = type;
	} else {
		fm_reg_read(FM_MAIN_CG2_CTRL, &dataRead);

		if (type)
			dataRead |= ANTENNA_TYPE;
		else
			dataRead &= (~ANTENNA_TYPE);

		fm_reg_write(FM_MAIN_CG2_CTRL, dataRead);
	}
	return 0;
}

static fm_s32 mt6632_GetAntennaType(void)
{
	fm_u16 dataRead;

	if (fm_packaging == 0)
		return fm_sant_flag;

	fm_reg_read(FM_MAIN_CG2_CTRL, &dataRead);
	WCN_DBG(FM_NTC | CHIP, "get ana type: %s\n", (dataRead & ANTENNA_TYPE) ? "short" : "long");

	if (dataRead & ANTENNA_TYPE)
		return FM_ANA_SHORT;	/* short antenna */
	else
		return FM_ANA_LONG;	/* long antenna */
}

static fm_s32 mt6632_Mute(fm_bool mute)
{
	fm_s32 ret = 0;
	fm_u16 dataRead;

	WCN_DBG(FM_NTC | CHIP, "set %s\n", mute ? "mute" : "unmute");
	fm_reg_read(FM_MAIN_CTRL, &dataRead);

	if (mute == 1)
		ret = fm_reg_write(FM_MAIN_CTRL, (dataRead & 0xFFDF) | 0x0020);
	else
		ret = fm_reg_write(FM_MAIN_CTRL, (dataRead & 0xFFDF));

	return ret;
}

static fm_s32 mt6632_rampdown_reg_op(fm_u8 *buf, fm_s32 buf_size)
{
	fm_s32 pkt_size = 4;

	if (NULL == buf) {
		WCN_DBG(FM_ERR | CHIP, "%s invalid pointer\n", __func__);
		return -1;
	}
	if (buf_size < TX_BUF_SIZE) {
		WCN_DBG(FM_ERR | CHIP, "%s invalid buf size(%d)\n", __func__, buf_size);
		return -2;
	}

	/* Clear DSP state */
	pkt_size += fm_bop_modify(FM_MAIN_CTRL, 0xFFF0, 0x0000, &buf[pkt_size], buf_size - pkt_size);
	/* Set DSP ramp down state */
	pkt_size += fm_bop_modify(FM_MAIN_CTRL, 0xFEFF, RAMP_DOWN, &buf[pkt_size], buf_size - pkt_size);
	/* @Wait for STC_DONE interrupt@ */
	pkt_size += fm_bop_rd_until(FM_MAIN_INTR, FM_INTR_STC_DONE, FM_INTR_STC_DONE, &buf[pkt_size],
				    buf_size - pkt_size);
	/* Clear DSP ramp down state */
	pkt_size += fm_bop_modify(FM_MAIN_CTRL, (~RAMP_DOWN), 0x0000, &buf[pkt_size], buf_size - pkt_size);
	/* Write 1 clear the STC_DONE interrupt status flag */
	pkt_size += fm_bop_modify(FM_MAIN_INTR, 0xFFFE, FM_INTR_STC_DONE, &buf[pkt_size], buf_size - pkt_size);

	return pkt_size - 4;
}
/*
 * mt6632_rampdown - f/w will wait for STC_DONE interrupt
 * @buf - target buf
 * @buf_size - buffer size
 * return package size
 */
static fm_s32 mt6632_rampdown(fm_u8 *buf, fm_s32 buf_size)
{
	fm_s32 pkt_size = 0;

	pkt_size = mt6632_rampdown_reg_op(buf, buf_size);
	return fm_op_seq_combine_cmd(buf, FM_RAMPDOWN_OPCODE, pkt_size);
}

static fm_s32 mt6632_RampDown(void)
{
	fm_s32 ret = 0;
	fm_u16 pkt_size;
	/* fm_u16 tmp; */

	WCN_DBG(FM_NTC | CHIP, "ramp down\n");

	ret = fm_reg_write(FM_MAIN_EXTINTRMASK, 0x0000);
	if (ret) {
		WCN_DBG(FM_ERR | CHIP, "ramp down write FM_MAIN_EXTINTRMASK failed\n");
		return ret;
	}

	if (FM_LOCK(cmd_buf_lock))
		return -FM_ELOCK;
	pkt_size = mt6632_rampdown(cmd_buf, TX_BUF_SIZE);
	ret = fm_cmd_tx(cmd_buf, pkt_size, FLAG_RAMPDOWN, SW_RETRY_CNT, RAMPDOWN_TIMEOUT, NULL);
	FM_UNLOCK(cmd_buf_lock);

	if (ret) {
		WCN_DBG(FM_ERR | CHIP, "ramp down failed\n");
		return ret;
	}

	ret = fm_reg_write(FM_MAIN_EXTINTRMASK, 0x0021);
	if (ret)
		WCN_DBG(FM_ERR | CHIP, "ramp down write FM_MAIN_EXTINTRMASK failed\n");

	return ret;
}

static fm_s32 mt6632_get_rom_version(void)
{
	fm_u16 tmp;
	fm_s32 ret;

	/* set download dsp coefficient mode */
	fm_reg_write(0x90, 0xe);
	/* fill in coefficient data */
	fm_reg_write(0x92, 0x0);
	/* reset download control */
	fm_reg_write(0x90, 0x40);
	/* disable memory control from host */
	fm_reg_write(0x90, 0x0);

	/* DSP rom code version request enable --- set 0x61 b15=1 */
	fm_set_bits(0x61, 0x8000, 0x7FFF);

	/* Release ASIP reset --- set 0x61 b1=1 */
	fm_set_bits(0x61, 0x0002, 0xFFFD);

	/* Enable ASIP power --- set 0x61 b0=0 */
	fm_set_bits(0x61, 0x0000, 0xFFFE);

	/* Wait DSP code version ready --- wait 1ms */
	do {
		fm_delayus(1000);
		ret = fm_reg_read(0x84, &tmp);
		/* ret=-4 means signal got when control FM. usually get sig 9 to kill FM process. */
		/* now cancel FM power up sequence is recommended. */
		if (ret)
			return ret;

		WCN_DBG(FM_DBG | CHIP, "0x84=%x\n", tmp);
	} while (tmp != 0x0001);

	/* Get FM DSP code version --- rd 0x83[15:8] */
	fm_reg_read(0x83, &tmp);
	WCN_DBG(FM_NTC | CHIP, "DSP ver=0x%x\n", tmp);
	tmp = (tmp >> 8);

	/* DSP rom code version request disable --- set 0x61 b15=0 */
	fm_set_bits(0x61, 0x0000, 0x7FFF);

	/* Reset ASIP --- set 0x61[1:0] = 1 */
	fm_set_bits(0x61, 0x0001, 0xFFFC);

	/* WCN_DBG(FM_NTC | CHIP, "ROM version: v%d\n", (fm_s32)tmp); */
	return (fm_s32) tmp;
}

static fm_s32 mt6632_pmic_read(fm_u8 addr, fm_u32 *val)
{
	fm_s32 ret = 0;
	fm_u8 read_reslut = 0xff;
	fm_u16 pkt_size;

	if (FM_LOCK(cmd_buf_lock))
		return -FM_ELOCK;
	pkt_size = fm_pmic_get_reg(cmd_buf, TX_BUF_SIZE, addr);
	ret = fm_cmd_tx(cmd_buf, pkt_size, FLAG_PMIC_READ, SW_RETRY_CNT, PMIC_CONTROL_TIMEOUT, fm_get_read_result);
	FM_UNLOCK(cmd_buf_lock);

	if (!ret && fm_res) {
		fm_memcpy(val, &fm_res->pmic_result[0], sizeof(fm_u32));
		read_reslut = fm_res->pmic_result[4];
	} else {
		WCN_DBG(FM_ERR | CHIP, "mt6632_pmic_read failed,ret(%d)\n", ret);
		return -1;
	}

	if (read_reslut) {
		WCN_DBG(FM_ERR | CHIP, "mt6632_pmic_read failed(%02x)\n", read_reslut);
		return -2;
	}
	return ret;
}

static fm_s32 mt6632_pmic_write(fm_u8 addr, fm_u32 val)
{
	fm_s32 ret = 0;
	fm_u8 read_reslut = 0xff;
	fm_u16 pkt_size;

	if (FM_LOCK(cmd_buf_lock))
		return -FM_ELOCK;
	pkt_size = fm_pmic_set_reg(cmd_buf, TX_BUF_SIZE, addr, val);
	ret = fm_cmd_tx(cmd_buf, pkt_size, FLAG_PMIC_MODIFY, SW_RETRY_CNT,
		    PMIC_CONTROL_TIMEOUT, fm_get_read_result);
	FM_UNLOCK(cmd_buf_lock);

	if (!ret && fm_res)
		read_reslut = fm_res->pmic_result[0];
	else {
		WCN_DBG(FM_ERR | CHIP, "mt6632_pmic_write failed,ret(%d)\n", ret);
		return -1;
	}

	if (read_reslut) {
		WCN_DBG(FM_ERR | CHIP, "mt6632_pmic_write failed(%02x)\n", read_reslut);
		return -2;
	}

	return ret;
}

static fm_s32 mt6632_pmic_mod(fm_u8 addr, fm_u32 mask_and, fm_u32 mask_or)
{
	fm_s32 ret = 0;
	fm_u8 read_reslut = 0xff;
	fm_u16 pkt_size;

	if (FM_LOCK(cmd_buf_lock))
		return -FM_ELOCK;
	pkt_size = fm_pmic_mod_reg(cmd_buf, TX_BUF_SIZE, addr, mask_and, mask_or);
	ret = fm_cmd_tx(cmd_buf, pkt_size, FLAG_PMIC_MODIFY, SW_RETRY_CNT,
		    PMIC_CONTROL_TIMEOUT, fm_get_read_result);
	FM_UNLOCK(cmd_buf_lock);

	if (!ret && fm_res)
		read_reslut = fm_res->pmic_result[0];
	else {
		WCN_DBG(FM_ERR | CHIP, "mt6632_pmic_mod failed,ret(%d)\n", ret);
		return -1;
	}

	if (read_reslut) {
		WCN_DBG(FM_ERR | CHIP, "mt6632_pmic_mod failed(%02x)\n", read_reslut);
		return -2;
	}
	return ret;
}

static fm_s32 mt6632_pmic_ctrl(void)
{
	fm_s32 ret = 0;
	fm_u32 val = 0;

	ret = mt6632_pmic_mod(0xa8, 0xFFFFFFF7, 0x00000008);
	if (!ret) {
		mt6632_pmic_read(0xa8, &val);
		WCN_DBG(FM_NTC | CHIP, "0xa8 = %08x\n", val);
	} else {
		WCN_DBG(FM_ERR | CHIP, "mt6632_pmic_ctrl 0xa8 failed(%d)\n", ret);
		return ret;
	}

	ret = mt6632_pmic_mod(0xc7, 0x7FFFFFCF, 0x00000000);
	if (!ret) {
		mt6632_pmic_read(0xc7, &val);
		WCN_DBG(FM_NTC | CHIP, "0xc7 = %08x\n", val);
	} else {
		WCN_DBG(FM_ERR | CHIP, "mt6632_pmic_ctrl 0xc7 failed(%d)\n", ret);
		return ret;
	}

	return ret;
}

static fm_s32 mt6632_pwrup_top_setting(void)
{
	fm_s32 ret = 0, value = 0;
	/* A0.1 Turn on FM buffer */
	ret = fm_host_reg_read(0x81020008, &value);
	if (ret) {
		WCN_DBG(FM_ERR | CHIP, " 0x81020008 rd failed\n");
		return ret;
	}
	ret = fm_host_reg_write(0x81020008, value | 0x30);
	if (ret) {
		WCN_DBG(FM_ERR | CHIP, " 0x81020008 wr failed\n");
		return ret;
	}
	/* A0.2 Set xtal no off when FM on */
	fm_delayus(20);
	/* A0.3 release power on reset */
	ret = fm_host_reg_read(0x81020008, &value);
	if (ret) {
		WCN_DBG(FM_ERR | CHIP, " 0x81020008 rd failed\n");
		return ret;
	}
	ret = fm_host_reg_write(0x81020008, value | 0x1);
	if (ret) {
		WCN_DBG(FM_ERR | CHIP, " 0x81020008 wr failed\n");
		return ret;
	}
	/* A0.4 enable fspi_mas_bclk_ck */
	ret = fm_host_reg_read(0x80000108, &value);
	if (ret) {
		WCN_DBG(FM_ERR | CHIP, " 0x80000108 rd failed\n");
		return ret;
	}
	ret = fm_host_reg_write(0x80000108, value | 0x00000100);
	if (ret) {
		WCN_DBG(FM_ERR | CHIP, " 0x80000108 wr failed\n");
		return ret;
	}
	return ret;
}

static fm_s32 mt6632_pwrdown_top_setting(void)
{
	fm_s32 ret = 0, value = 0;
	/* B0.1 disable fspi_mas_bclk_ck */
	ret = fm_host_reg_read(0x80000104, &value);
	if (ret) {
		WCN_DBG(FM_ERR | CHIP, " 0x80000104 rd failed\n");
		return ret;
	}
	ret = fm_host_reg_write(0x80000104, value | 0x00000100);
	if (ret) {
		WCN_DBG(FM_ERR | CHIP, " 0x80000104 wr failed\n");
		return ret;
	}
	/* B0.2 set power off reset */
	ret = fm_host_reg_read(0x81020008, &value);
	if (ret) {
		WCN_DBG(FM_ERR | CHIP, " 0x81020008 rd failed\n");
		return ret;
	}
	ret = fm_host_reg_write(0x81020008, value & 0xFFFFFFFE);
	if (ret) {
		WCN_DBG(FM_ERR | CHIP, " 0x81020008 wr failed\n");
		return ret;
	}
	/* B0.3 */
	fm_delayus(20);

	/* B0.4 disable MTCMOS & set Iso_en */
	ret = fm_host_reg_read(0x81020008, &value);
	if (ret) {
		WCN_DBG(FM_ERR | CHIP, " 0x81020008 rd failed\n");
		return ret;
	}
	ret = fm_host_reg_write(0x81020008, value & 0xFFFFFFEF);
	if (ret) {
		WCN_DBG(FM_ERR | CHIP, " 0x81020008 wr failed\n");
		return ret;
	}
#if 0
	/* B0.5 Turn off FM buffer */
	ret = fm_host_reg_read(0x8102123c, &value);
	if (ret) {
		WCN_DBG(FM_ERR | CHIP, " 0x8102123c rd failed\n");
		return ret;
	}
	ret = fm_host_reg_write(0x8102123c, value | 0x40);
	if (ret) {
		WCN_DBG(FM_ERR | CHIP, " 0x8102123c wr failed\n");
		return ret;
	}
	/* B0.6 Clear xtal no off when FM off */
	ret = fm_host_reg_read(0x81021134, &value);
	if (ret) {
		WCN_DBG(FM_ERR | CHIP, " 0x81021134 rd failed\n");
		return ret;
	}
	ret = fm_host_reg_write(0x81021134, value & 0xFFFFFF7F);
	if (ret) {
		WCN_DBG(FM_ERR | CHIP, " 0x81021134 wr failed\n");
		return ret;
	}
	/* B0.7 Clear top off always on when FM off */
	ret = fm_host_reg_read(0x81020010, &value);
	if (ret) {
		WCN_DBG(FM_ERR | CHIP, " 0x81020010 rd failed\n");
		return ret;
	}
	ret = fm_host_reg_write(0x81020010, value | 0x20000);
	if (ret) {
		WCN_DBG(FM_ERR | CHIP, " 0x81020010 wr failed\n");
		return ret;
	}
	/* B0.9 Disable PALDO when FM off */
	ret = fm_host_reg_read(0x81021430, &value);
	if (ret) {
		WCN_DBG(FM_ERR | CHIP, " 0x81021430 rd failed\n");
		return ret;
	}
	ret = fm_host_reg_write(0x81021430, value & 0x7FFFFFFF);
	if (ret) {
		WCN_DBG(FM_ERR | CHIP, " 0x81021430 wr failed\n");
		return ret;
	}
#endif
	return ret;
}

static fm_s32 mt6632_pwrup_DSP_download(struct fm_patch_tbl *patch_tbl)
{
#define PATCH_BUF_SIZE (4096*6)
	fm_s32 ret = 0;
	fm_s32 patch_len = 0;
	fm_u8 *dsp_buf = NULL;
	fm_u16 tmp_reg = 0;

	mt6632_hw_info.eco_ver = (fm_s32) mtk_wcn_wmt_hwver_get();
	WCN_DBG(FM_NTC | CHIP, "ECO version:0x%08x\n", mt6632_hw_info.eco_ver);
	mt6632_hw_info.eco_ver += 1;

	/* FM ROM code version request */
	ret = mt6632_get_rom_version();
	if (ret >= 0) {
		mt6632_hw_info.rom_ver = ret;
		WCN_DBG(FM_NTC | CHIP, "ROM version: v%d\n", mt6632_hw_info.rom_ver);
	} else {
		WCN_DBG(FM_ERR | CHIP, "get ROM version failed\n");
		/* ret=-4 means signal got when control FM. usually get sig 9 to kill FM process. */
		/* now cancel FM power up sequence is recommended. */
		return ret;
	}

	/* Wholechip FM Power Up: step 3, download patch */
	dsp_buf = fm_vmalloc(PATCH_BUF_SIZE);
	if (!dsp_buf) {
		WCN_DBG(FM_ERR | CHIP, "-ENOMEM\n");
		return -ENOMEM;
	}

	patch_len = fm_get_patch_path(mt6632_hw_info.rom_ver, dsp_buf, PATCH_BUF_SIZE, patch_tbl);
	if (patch_len <= 0) {
		WCN_DBG(FM_ALT | CHIP, " fm_get_patch_path failed\n");
		ret = patch_len;
		goto out;
	}

	ret = fm_download_patch((const fm_u8 *)dsp_buf, patch_len, IMG_PATCH);
	if (ret) {
		WCN_DBG(FM_ERR | CHIP, " DL DSPpatch failed\n");
		goto out;
	}

	patch_len = fm_get_coeff_path(mt6632_hw_info.rom_ver, dsp_buf, PATCH_BUF_SIZE, patch_tbl);
	if (patch_len <= 0) {
		WCN_DBG(FM_ALT | CHIP, " fm_get_coeff_path failed\n");
		ret = patch_len;
		goto out;
	}

	mt6632_hw_info.rom_ver += 1;

	tmp_reg = dsp_buf[38] | (dsp_buf[39] << 8);	/* to be confirmed */
	mt6632_hw_info.patch_ver = (fm_s32) tmp_reg;
	WCN_DBG(FM_NTC | CHIP, "Patch version: 0x%08x\n", mt6632_hw_info.patch_ver);

	if (ret == 1) {
		dsp_buf[4] = 0x00;	/* if we found rom version undefined, we should disable patch */
		dsp_buf[5] = 0x00;
	}

	ret = fm_download_patch((const fm_u8 *)dsp_buf, patch_len, IMG_COEFFICIENT);
	if (ret) {
		WCN_DBG(FM_ERR | CHIP, " DL DSPcoeff failed\n");
		goto out;
	}
	fm_reg_write(0x90, 0x0040);
	fm_reg_write(0x90, 0x0000);
out:
	if (dsp_buf) {
		fm_vfree(dsp_buf);
		dsp_buf = NULL;
	}
	return ret;
}

static fm_s32 mt6632_pwrup_clock_on_reg_op(fm_u8 *buf, fm_s32 buf_size)
{
	fm_s32 pkt_size = 4;
	fm_u16 de_emphasis;
	/* fm_u16 osc_freq; */

	if (NULL == buf) {
		WCN_DBG(FM_ERR | CHIP, "%s invalid pointer\n", __func__);
		return -1;
	}
	if (buf_size < TX_BUF_SIZE) {
		WCN_DBG(FM_ERR | CHIP, "%s invalid buf size(%d)\n", __func__, buf_size);
		return -2;
	}

	de_emphasis = fm_config.rx_cfg.deemphasis;
	de_emphasis &= 0x0001;	/* rang 0~1 */

	/* B1.1    Enable digital OSC */
	pkt_size += fm_bop_write(0x60, 0x0003, &buf[pkt_size], buf_size - pkt_size);	/* wr 60 3 */
	pkt_size += fm_bop_udelay(100, &buf[pkt_size], buf_size - pkt_size);	/* delay 100us */
	/* B1.3    Release HW clock gating */
	pkt_size += fm_bop_write(0x60, 0x0007, &buf[pkt_size], buf_size - pkt_size);	/* wr 60 7 */
	/* B1.4    Set FM long/short antenna:1: short_antenna  0: long antenna(default) */
	/* pkt_size += fm_bop_modify(0x61, 0xFFEF, 0x0000, &buf[pkt_size], buf_size - pkt_size); */
	/* B1.5    Set audio output mode (lineout/I2S) 0:lineout,  1:I2S */
	/*
	if (mt6632_fm_config.aud_cfg.aud_path == FM_AUD_ANALOG)
		pkt_size += fm_bop_modify(0x61, 0xFF7F, 0x0000, &buf[pkt_size], buf_size - pkt_size);
	else
		pkt_size += fm_bop_modify(0x61, 0xFF7F, 0x0080, &buf[pkt_size], buf_size - pkt_size);
	*/
	/* B1.6    Set deemphasis setting */
	pkt_size += fm_bop_modify(0x61, ~DE_EMPHASIS, (de_emphasis << 12), &buf[pkt_size], buf_size - pkt_size);

	/* pkt_size += fm_bop_modify(0x60, OSC_FREQ_MASK, (osc_freq << 4), &buf[pkt_size], buf_size - pkt_size); */

	return pkt_size - 4;
}
/*
 * mt6632_pwrup_clock_on - Wholechip FM Power Up: step 1, FM Digital Clock enable
 * @buf - target buf
 * @buf_size - buffer size
 * return package size
 */
static fm_s32 mt6632_pwrup_clock_on(fm_u8 *buf, fm_s32 buf_size)
{
	fm_s32 pkt_size = 0;

	pkt_size = mt6632_pwrup_clock_on_reg_op(buf, buf_size);
	return fm_op_seq_combine_cmd(buf, FM_ENABLE_OPCODE, pkt_size);
}

static fm_s32 mt6632_pwrup_digital_init_reg_op(fm_u8 *buf, fm_s32 buf_size)
{
	fm_s32 pkt_size = 4;

	if (NULL == buf) {
		WCN_DBG(FM_ERR | CHIP, "%s invalid pointer\n", __func__);
		return -1;
	}
	if (buf_size < TX_BUF_SIZE) {
		WCN_DBG(FM_ERR | CHIP, "%s invalid buf size(%d)\n", __func__, buf_size);
		return -2;
	}

	/* F1.4    Set appropriate interrupt mask behavior as desired(RX) */
	/* pkt_size += fm_bop_write(0x6A, 0x0021, &buf[pkt_size], buf_size - pkt_size);//wr 6A 0021 */
	pkt_size += fm_bop_write(0x6B, 0x0021, &buf[pkt_size], buf_size - pkt_size);	/* wr 6B 0021 */
	/* F1.9    Enable HW auto control */
	pkt_size += fm_bop_write(0x60, 0x000F, &buf[pkt_size], buf_size - pkt_size);	/* wr 60 f */
	/* F1.10   Release ASIP reset */
	pkt_size += fm_bop_modify(0x61, 0xFFFD, 0x0002, &buf[pkt_size], buf_size - pkt_size);	/* wr 61 D1=1 */
	/* F1.11   Enable ASIP power */
	pkt_size += fm_bop_modify(0x61, 0xFFFE, 0x0000, &buf[pkt_size], buf_size - pkt_size);	/* wr 61 D0=0 */
	pkt_size += fm_bop_udelay(100000, &buf[pkt_size], buf_size - pkt_size);	/* delay 100ms */
	/* F1.13   Check HW intitial complete */
	pkt_size += fm_bop_rd_until(0x64, 0x001F, 0x0002, &buf[pkt_size], buf_size - pkt_size);	/* Poll 64[0~4] = 2 */

	return pkt_size - 4;
}

/*
 * mt6632_pwrup_digital_init - Wholechip FM Power Up: step 4, FM Digital Init: fm_rgf_maincon
 * @buf - target buf
 * @buf_size - buffer size
 * return package size
 */
static fm_s32 mt6632_pwrup_digital_init(fm_u8 *buf, fm_s32 buf_size)
{
	fm_s32 pkt_size = 0;

	pkt_size = mt6632_pwrup_digital_init_reg_op(buf, buf_size);
	return fm_op_seq_combine_cmd(buf, FM_ENABLE_OPCODE, pkt_size);
}

static fm_s32 mt6632_pwrdown_reg_op(fm_u8 *buf, fm_s32 buf_size)
{
	fm_s32 pkt_size = 4;

	if (NULL == buf) {
		WCN_DBG(FM_ERR | CHIP, "%s invalid pointer\n", __func__);
		return -1;
	}
	if (buf_size < TX_BUF_SIZE) {
		WCN_DBG(FM_ERR | CHIP, "%s invalid buf size(%d)\n", __func__, buf_size);
		return -2;
	}

	/* Disable HW clock control */
	pkt_size += fm_bop_write(0x60, 0x0107, &buf[pkt_size], buf_size - pkt_size);	/* wr 60 107 */
	/* Reset ASIP */
	pkt_size += fm_bop_write(0x61, 0x0001, &buf[pkt_size], buf_size - pkt_size);	/* wr 61 0001 */
	/* digital core + digital rgf reset */
	pkt_size += fm_bop_modify(0x6E, 0xFFF8, 0x0000, &buf[pkt_size], buf_size - pkt_size);	/* wr 6E[0~2] 0 */
	pkt_size += fm_bop_modify(0x6E, 0xFFF8, 0x0000, &buf[pkt_size], buf_size - pkt_size);	/* wr 6E[0~2] 0 */
	pkt_size += fm_bop_modify(0x6E, 0xFFF8, 0x0000, &buf[pkt_size], buf_size - pkt_size);	/* wr 6E[0~2] 0 */
	pkt_size += fm_bop_modify(0x6E, 0xFFF8, 0x0000, &buf[pkt_size], buf_size - pkt_size);	/* wr 6E[0~2] 0 */
	/* Disable all clock */
	pkt_size += fm_bop_write(0x60, 0x0000, &buf[pkt_size], buf_size - pkt_size);	/* wr 60 0000 */
	/* Reset rgfrf */
	pkt_size += fm_bop_write(0x60, 0x4000, &buf[pkt_size], buf_size - pkt_size);	/* wr 60 4000 */
	pkt_size += fm_bop_write(0x60, 0x0000, &buf[pkt_size], buf_size - pkt_size);	/* wr 60 0000 */

	return pkt_size - 4;
}
/*
 * mt6632_pwrdown - Wholechip FM Power down: Digital Modem Power Down
 * @buf - target buf
 * @buf_size - buffer size
 * return package size
 */
static fm_s32 mt6632_pwrdown(fm_u8 *buf, fm_s32 buf_size)
{
	fm_s32 pkt_size = 0;

	pkt_size = mt6632_pwrdown_reg_op(buf, buf_size);
	return fm_op_seq_combine_cmd(buf, FM_ENABLE_OPCODE, pkt_size);
}

static fm_s32 mt6632_tune_reg_op(fm_u8 *buf, fm_s32 buf_size, fm_u16 freq, fm_u16 chan_para)
{
	fm_s32 pkt_size = 4;

	WCN_DBG(FM_ALT | CHIP, "%s enter mt6632_tune function\n", __func__);

	if (NULL == buf) {
		WCN_DBG(FM_ERR | CHIP, "%s invalid pointer\n", __func__);
		return -1;
	}
	if (buf_size < TX_BUF_SIZE) {
		WCN_DBG(FM_ERR | CHIP, "%s invalid buf size(%d)\n", __func__, buf_size);
		return -2;
	}

	if (0 == fm_get_channel_space(freq))
		freq *= 10;

	freq = (freq - 6400) * 2 / 10;

	/* Set desired channel & channel parameter */
#ifdef FM_TUNE_USE_POLL
	pkt_size += fm_bop_write(0x6A, 0x0000, &buf[pkt_size], buf_size - pkt_size);
	pkt_size += fm_bop_write(0x6B, 0x0000, &buf[pkt_size], buf_size - pkt_size);
#endif
	pkt_size += fm_bop_modify(FM_CHANNEL_SET, 0xFC00, freq, &buf[pkt_size], buf_size - pkt_size);
	/* channel para setting, D15~D12, D15: ATJ, D13: HL, D12: FA */
	pkt_size += fm_bop_modify(FM_CHANNEL_SET, 0x0FFF, (chan_para << 12), &buf[pkt_size], buf_size - pkt_size);
	/* Enable hardware controlled tuning sequence */
	pkt_size += fm_bop_modify(FM_MAIN_CTRL, 0xFFF8, TUNE, &buf[pkt_size], buf_size - pkt_size);
	/* Wait for STC_DONE interrupt */
#ifdef FM_TUNE_USE_POLL
	pkt_size += fm_bop_rd_until(FM_MAIN_INTR, FM_INTR_STC_DONE, FM_INTR_STC_DONE, &buf[pkt_size],
				    buf_size - pkt_size);
	/* Write 1 clear the STC_DONE interrupt status flag */
	pkt_size += fm_bop_modify(FM_MAIN_INTR, 0xFFFF, FM_INTR_STC_DONE, &buf[pkt_size], buf_size - pkt_size);
#endif

	WCN_DBG(FM_ALT | CHIP, "%s leave mt6632_tune function\n", __func__);

	return pkt_size - 4;
}

/*
 * mt6632_tune - execute tune action,
 * @buf - target buf
 * @buf_size - buffer size
 * @freq - 760 ~ 1080, 100KHz unit
 * return package size
 */
static fm_s32 mt6632_tune(fm_u8 *buf, fm_s32 buf_size, fm_u16 freq, fm_u16 chan_para)
{
	fm_s32 pkt_size = 0;

	pkt_size = mt6632_tune_reg_op(buf, buf_size, freq, chan_para);
	return fm_op_seq_combine_cmd(buf, FM_TUNE_OPCODE, pkt_size);
}

static fm_s32 mt6632_PowerUp(fm_u16 *chip_id, fm_u16 *device_id)
{
	fm_s32 ret = 0;
	fm_u16 pkt_size;
	fm_u16 tmp_reg = 0;

	if (chip_id == NULL) {
		WCN_DBG(FM_ERR | CHIP, "%s,invalid pointer\n", __func__);
		return -FM_EPARA;
	}
	if (device_id == NULL) {
		WCN_DBG(FM_ERR | CHIP, "%s,invalid pointer\n", __func__);
		return -FM_EPARA;
	}

	WCN_DBG(FM_DBG | CHIP, "pwr on seq......\n");

	ret = mt6632_pmic_ctrl();
	if (ret) {
		WCN_DBG(FM_ERR | CHIP, "mt6632_pmic_ctrl failed\n");
		return ret;
	}
	fm_delayus(240);

	ret = mt6632_pwrup_top_setting();
	if (ret) {
		WCN_DBG(FM_ERR | CHIP, "mt6632_pwrup_top_setting failed\n");
		return ret;
	}
	if (FM_LOCK(cmd_buf_lock))
		return -FM_ELOCK;
	pkt_size = mt6632_pwrup_clock_on(cmd_buf, TX_BUF_SIZE);
	ret = fm_cmd_tx(cmd_buf, pkt_size, FLAG_EN, SW_RETRY_CNT, EN_TIMEOUT, NULL);
	FM_UNLOCK(cmd_buf_lock);

	if (ret) {
		WCN_DBG(FM_ERR | CHIP, "mt6632_pwrup_clock_on failed\n");
		return ret;
	}
	/* read HW version */
	fm_reg_read(0x62, &tmp_reg);
	*chip_id = tmp_reg;
	*device_id = tmp_reg;
	mt6632_hw_info.chip_id = (fm_s32) tmp_reg;
	WCN_DBG(FM_NTC | CHIP, "chip_id:0x%04x\n", tmp_reg);

	if (mt6632_hw_info.chip_id != 0x6632) {
		WCN_DBG(FM_NTC | CHIP, "fm sys error!\n");
		return -FM_EPARA;
	}
	ret = mt6632_pwrup_DSP_download(mt6632_patch_tbl);
	if (ret) {
		WCN_DBG(FM_ERR | CHIP, "mt6632_pwrup_DSP_download failed\n");
		return ret;
	}

	if ((fm_config.aud_cfg.aud_path == FM_AUD_MRGIF)
	    || (fm_config.aud_cfg.aud_path == FM_AUD_I2S)) {
		mt6632_I2s_Setting(FM_I2S_ON, fm_config.aud_cfg.i2s_info.mode,
				   fm_config.aud_cfg.i2s_info.rate);
		/* mt_combo_audio_ctrl(COMBO_AUDIO_STATE_2); */
		mtk_wcn_cmb_stub_audio_ctrl((CMB_STUB_AIF_X) CMB_STUB_AIF_2);
	}
	/* Wholechip FM Power Up: step 4, FM Digital Init: fm_rgf_maincon */
	if (FM_LOCK(cmd_buf_lock))
		return -FM_ELOCK;
	pkt_size = mt6632_pwrup_digital_init(cmd_buf, TX_BUF_SIZE);
	ret = fm_cmd_tx(cmd_buf, pkt_size, FLAG_EN, SW_RETRY_CNT, EN_TIMEOUT, NULL);
	FM_UNLOCK(cmd_buf_lock);
	if (ret) {
		WCN_DBG(FM_ERR | CHIP, "mt6632_pwrup_digital_init failed\n");
		return ret;
	}

	/* FM RF fine tune setting */
	fm_reg_write(0x60, 0x7);
	fm_reg_write(0x2d, 0x1fa);
	fm_reg_write(0x60, 0xF);

	WCN_DBG(FM_NTC | CHIP, "pwr on seq ok\n");

	return ret;
}

static fm_s32 mt6632_PowerDown(void)
{
	fm_s32 ret = 0;
	fm_u16 pkt_size;

	WCN_DBG(FM_DBG | CHIP, "pwr down seq\n");

	if (FM_LOCK(cmd_buf_lock))
		return -FM_ELOCK;
	pkt_size = mt6632_pwrdown(cmd_buf, TX_BUF_SIZE);
	ret = fm_cmd_tx(cmd_buf, pkt_size, FLAG_EN, SW_RETRY_CNT, EN_TIMEOUT, NULL);
	FM_UNLOCK(cmd_buf_lock);

	if (ret) {
		WCN_DBG(FM_ERR | CHIP, "mt6632_pwrdown failed\n");
		return ret;
	}

	ret = mt6632_pwrdown_top_setting();
	if (ret) {
		WCN_DBG(FM_ERR | CHIP, "mt6632_pwrdown_top_setting failed\n");
		return ret;
	}

	return ret;
}

/* just for dgb */
static fm_bool mt6632_SetFreq(fm_u16 freq)
{
	fm_s32 ret = 0;
	fm_u16 pkt_size;
	fm_u16 chan_para = 0;
	fm_u16 tmp_reg[4];

	fm_cb_op->cur_freq_set(freq);

#if 0
	/* MCU clock adjust if need */
	ret = mt6632_mcu_dese(freq, NULL);
	if (ret < 0)
		WCN_DBG(FM_ERR | MAIN, "mt6632_mcu_dese FAIL:%d\n", ret);

	WCN_DBG(FM_INF | MAIN, "MCU %d\n", ret);
#endif

	/* GPS clock adjust if need */
	ret = mt6632_gps_dese(freq, NULL);
	if (ret < 0)
		WCN_DBG(FM_ERR | MAIN, "mt6632_gps_dese FAIL:%d\n", ret);

	WCN_DBG(FM_INF | MAIN, "GPS %d\n", ret);

	ret = fm_reg_write(0x60, 0x0007);
	if (ret)
		WCN_DBG(FM_ALT | MAIN, "set freq write 0x60 fail\n");

	if (mt6632_TDD_chan_check(freq)) {
		ret = fm_set_bits(0x30, 0x0004, 0xFFF9);	/* use TDD solution */
		if (ret)
			WCN_DBG(FM_ALT | MAIN, "set freq write 0x30 fail\n");
	} else {
		ret = fm_set_bits(0x30, 0x0000, 0xFFF9);	/* default use FDD solution */
		if (ret)
			WCN_DBG(FM_ALT | MAIN, "set freq write 0x30 fail\n");
	}
	ret = fm_reg_write(0x60, 0x000F);
	if (ret)
		WCN_DBG(FM_ALT | MAIN, "set freq write 0x60 fail\n");

	chan_para = mt6632_chan_para_get(freq);
	WCN_DBG(FM_DBG | CHIP, "%d chan para = %d\n", (fm_s32) freq, (fm_s32) chan_para);

	if (FM_LOCK(cmd_buf_lock))
		return fm_false;
	pkt_size = mt6632_tune(cmd_buf, TX_BUF_SIZE, freq, chan_para);
	ret = fm_cmd_tx(cmd_buf, pkt_size, FLAG_TUNE | FLAG_TUNE_DONE, SW_RETRY_CNT, TUNE_TIMEOUT, NULL);
	FM_UNLOCK(cmd_buf_lock);

	if (ret) {
		fm_reg_read(0x64, &tmp_reg[0]);
		fm_reg_read(0x69, &tmp_reg[1]);
		fm_reg_read(0x6c, &tmp_reg[2]);
		WCN_DBG(FM_ERR | CHIP, "mt6632_tune failed\n");
		WCN_DBG(FM_ERR | CHIP, "0x64[0x%04x],0x69[0x%04x],0x6c[0x%04x]\n", tmp_reg[0], tmp_reg[1], tmp_reg[2]);
		return fm_false;
	}

	WCN_DBG(FM_DBG | CHIP, "set freq to %d ok\n", freq);
	return fm_true;
}

#define FM_CQI_LOG_PATH "/mnt/sdcard/fmcqilog"

static fm_s32 mt6632_full_cqi_get(fm_s32 min_freq, fm_s32 max_freq, fm_s32 space, fm_s32 cnt)
{
	fm_s32 ret = 0;
	fm_u16 pkt_size;
	fm_u16 freq, orig_freq;
	fm_s32 i, j, k;
	fm_s32 space_val, max, min, num;
	struct mt6632_full_cqi *p_cqi;
	fm_u8 *cqi_log_title = "Freq, RSSI, PAMD, PR, FPAMD, MR, ATDC, PRX, ATDEV, SMGain, DltaRSSI\n";
	fm_u8 cqi_log_buf[100] = { 0 };
	fm_s32 pos;
	fm_u8 cqi_log_path[100] = { 0 };

	WCN_DBG(FM_NTC | CHIP, "6632 cqi log start\n");
	/* for soft-mute tune, and get cqi */
	freq = fm_cb_op->cur_freq_get();
	if (0 == fm_get_channel_space(freq))
		freq *= 10;

	/* get cqi */
	orig_freq = freq;
	if (0 == fm_get_channel_space(min_freq))
		min = min_freq * 10;
	else
		min = min_freq;

	if (0 == fm_get_channel_space(max_freq))
		max = max_freq * 10;
	else
		max = max_freq;

	if (space == 0x0001)
		space_val = 5;	/* 50Khz */
	else if (space == 0x0002)
		space_val = 10;	/* 100Khz */
	else if (space == 0x0004)
		space_val = 20;	/* 200Khz */
	else
		space_val = 10;

	num = (max - min) / space_val + 1;	/* Eg, (8760 - 8750) / 10 + 1 = 2 */
	for (k = 0; (10000 == orig_freq) && (0xffffffff == g_dbg_level) && (k < cnt); k++) {
		WCN_DBG(FM_NTC | CHIP, "cqi file:%d\n", k + 1);
		freq = min;
		pos = 0;
		fm_memcpy(cqi_log_path, FM_CQI_LOG_PATH, strlen(FM_CQI_LOG_PATH));
		sprintf(&cqi_log_path[strlen(FM_CQI_LOG_PATH)], "%d.txt", k + 1);
		fm_file_write(cqi_log_path, cqi_log_title, strlen(cqi_log_title), &pos);
		for (j = 0; j < num; j++) {
			if (FM_LOCK(cmd_buf_lock))
				return -FM_ELOCK;
			pkt_size = fm_full_cqi_req(cmd_buf, TX_BUF_SIZE, &freq, 1, 1);
			ret =
			    fm_cmd_tx(cmd_buf, pkt_size, FLAG_SM_TUNE, SW_RETRY_CNT,
				      SM_TUNE_TIMEOUT, fm_get_read_result);
			FM_UNLOCK(cmd_buf_lock);

			if (!ret && fm_res) {
				WCN_DBG(FM_NTC | CHIP, "smt cqi size %d\n", fm_res->cqi[0]);
				p_cqi = (struct mt6632_full_cqi *)&fm_res->cqi[2];
				for (i = 0; i < fm_res->cqi[1]; i++) {
					/* just for debug */
					WCN_DBG(FM_NTC | CHIP,
						   "freq %d, 0x%04x, 0x%04x, 0x%04x, 0x%04x, 0x%04x, 0x%04x, 0x%04x, 0x%04x, 0x%04x, 0x%04x\n",
						   p_cqi[i].ch, p_cqi[i].rssi, p_cqi[i].pamd,
						   p_cqi[i].pr, p_cqi[i].fpamd, p_cqi[i].mr,
						   p_cqi[i].atdc, p_cqi[i].prx, p_cqi[i].atdev,
						   p_cqi[i].smg, p_cqi[i].drssi);
					/* format to buffer */
					sprintf(cqi_log_buf,
						"%04d,%04x,%04x,%04x,%04x,%04x,%04x,%04x,%04x,%04x,%04x,\n",
						p_cqi[i].ch, p_cqi[i].rssi, p_cqi[i].pamd,
						p_cqi[i].pr, p_cqi[i].fpamd, p_cqi[i].mr,
						p_cqi[i].atdc, p_cqi[i].prx, p_cqi[i].atdev,
						p_cqi[i].smg, p_cqi[i].drssi);
					/* write back to log file */
					fm_file_write(cqi_log_path, cqi_log_buf, strlen(cqi_log_buf), &pos);
				}
			} else {
				WCN_DBG(FM_ERR | CHIP, "smt get CQI failed\n");
				ret = -1;
			}
			freq += space_val;
		}
		fm_cb_op->cur_freq_set(0);	/* avoid run too much times */
	}
	WCN_DBG(FM_NTC | CHIP, "6632 cqi log done\n");

	return ret;
}

/*
 * mt6632_GetCurRSSI - get current freq's RSSI value
 * RS=RSSI
 * If RS>511, then RSSI(dBm)= (RS-1024)/16*6
 *				   else RSSI(dBm)= RS/16*6
 */
static fm_s32 mt6632_GetCurRSSI(fm_s32 *pRSSI)
{
	fm_u16 tmp_reg;

	/* TODO: check reg */
	fm_reg_read(FM_RSSI_IND, &tmp_reg);
	tmp_reg = tmp_reg & 0x03ff;

	if (pRSSI) {
		*pRSSI = (tmp_reg > 511) ? (((tmp_reg - 1024) * 6) >> 4) : ((tmp_reg * 6) >> 4);
		WCN_DBG(FM_DBG | CHIP, "rssi:%d, dBm:%d\n", tmp_reg, *pRSSI);
	} else {
		WCN_DBG(FM_ERR | CHIP, "get rssi para error\n");
		return -FM_EPARA;
	}

	return 0;
}

static fm_u16 mt6632_vol_tbl[16] = {
	0x0000, 0x0519, 0x066A, 0x0814,
	0x0A2B, 0x0CCD, 0x101D, 0x1449,
	0x198A, 0x2027, 0x287A, 0x32F5,
	0x4027, 0x50C3, 0x65AD, 0x7FFF
};

static fm_s32 mt6632_SetVol(fm_u8 vol)
{
	fm_s32 ret = 0;

	/* TODO: check reg */
	vol = (vol > 15) ? 15 : vol;
	ret = fm_reg_write(0x7D, mt6632_vol_tbl[vol]);
	if (ret) {
		WCN_DBG(FM_ERR | CHIP, "Set vol=%d Failed\n", vol);
		return ret;
	}

	WCN_DBG(FM_DBG | CHIP, "Set vol=%d OK\n", vol);

	if (vol == 10) {
		fm_print_cmd_fifo();	/* just for debug */
		fm_print_evt_fifo();
	}
	return 0;
}

static fm_s32 mt6632_GetVol(fm_u8 *pVol)
{
	int ret = 0;
	fm_u16 tmp;
	fm_s32 i;

	if (pVol == NULL) {
		WCN_DBG(FM_ERR | CHIP, "%s,invalid pointer\n", __func__);
		return -FM_EPARA;
	}

	/* TODO: check reg */
	ret = fm_reg_read(0x7D, &tmp);
	if (ret) {
		*pVol = 0;
		WCN_DBG(FM_ERR | CHIP, "Get vol Failed\n");
		return ret;
	}

	for (i = 0; i < 16; i++) {
		if (mt6632_vol_tbl[i] == tmp) {
			*pVol = i;
			break;
		}
	}

	WCN_DBG(FM_DBG | CHIP, "Get vol=%d OK\n", *pVol);
	return 0;
}

static fm_s32 mt6632_dump_reg(void)
{
	fm_s32 i;
	fm_u16 TmpReg;

	for (i = 0; i < 0xff; i++) {
		fm_reg_read(i, &TmpReg);
		WCN_DBG(FM_NTC | CHIP, "0x%02x=0x%04x\n", i, TmpReg);
	}
	return 0;
}

static fm_bool mt6632_GetMonoStereo(fm_u16 *pMonoStereo)
{
#define FM_BF_STEREO 0x1000
	fm_u16 TmpReg;

	/* TODO: check reg */
	if (pMonoStereo) {
		fm_reg_read(FM_RSSI_IND, &TmpReg);
		*pMonoStereo = (TmpReg & FM_BF_STEREO) >> 12;
	} else {
		WCN_DBG(FM_ERR | CHIP, "MonoStero: para err\n");
		return fm_false;
	}

	WCN_DBG(FM_DBG | CHIP, "MonoStero:0x%04x\n", *pMonoStereo);
	return fm_true;
}

static fm_s32 mt6632_SetMonoStereo(fm_s32 MonoStereo)
{
	fm_s32 ret = 0;
#define FM_FORCE_MS 0x0008

	WCN_DBG(FM_DBG | CHIP, "set to %s\n", MonoStereo ? "mono" : "auto");
	/* TODO: check reg */

	fm_reg_write(0x60, 0x3007);

	if (MonoStereo)
		ret = fm_set_bits(0x75, FM_FORCE_MS, ~FM_FORCE_MS);
	else
		ret = fm_set_bits(0x75, 0x0000, ~FM_FORCE_MS);

	return ret;
}

static fm_s32 mt6632_GetCapArray(fm_s32 *ca)
{
	fm_u16 dataRead;
	fm_u16 tmp = 0;

	/* TODO: check reg */
	if (ca == NULL) {
		WCN_DBG(FM_ERR | CHIP, "%s,invalid pointer\n", __func__);
		return -FM_EPARA;
	}
	fm_reg_read(0x60, &tmp);
	fm_reg_write(0x60, tmp & 0xFFF7);	/* 0x60 D3=0 */

	fm_reg_read(0x26, &dataRead);
	*ca = dataRead;

	fm_reg_write(0x60, tmp);	/* 0x60 D3=1 */
	return 0;
}

/*
 * mt6632_GetCurPamd - get current freq's PAMD value
 * PA=PAMD
 * If PA>511 then PAMD(dB)=  (PA-1024)/16*6,
 *				else PAMD(dB)=PA/16*6
 */
static fm_bool mt6632_GetCurPamd(fm_u16 *pPamdLevl)
{
	fm_u16 tmp_reg;
	fm_u16 dBvalue, valid_cnt = 0;
	int i, total = 0;

	for (i = 0; i < 8; i++) {
		/* TODO: check reg */
		if (fm_reg_read(FM_ADDR_PAMD, &tmp_reg)) {
			*pPamdLevl = 0;
			return fm_false;
		}

		tmp_reg &= 0x03FF;
		dBvalue = (tmp_reg > 256) ? ((512 - tmp_reg) * 6 / 16) : 0;
		if (dBvalue != 0) {
			total += dBvalue;
			valid_cnt++;
			WCN_DBG(FM_DBG | CHIP, "[%d]PAMD=%d\n", i, dBvalue);
		}
		fm_delayms(3);
	}
	if (valid_cnt != 0)
		*pPamdLevl = total / valid_cnt;
	else
		*pPamdLevl = 0;

	WCN_DBG(FM_NTC | CHIP, "PAMD=%d\n", *pPamdLevl);
	return fm_true;
}

static fm_s32 MT6632_FMOverBT(fm_bool enable)
{
	fm_s32 ret = 0;

	WCN_DBG(FM_NTC | CHIP, "+%s():\n", __func__);

	if (enable == fm_true) {
		/* change I2S to slave mode and 48K sample rate */
		if (mt6632_I2s_Setting(FM_I2S_ON, FM_I2S_SLAVE, FM_I2S_48K))
			goto out;
		WCN_DBG(FM_NTC | CHIP, "set FM via BT controller\n");
	} else if (enable == fm_false) {
		/* change I2S to master mode and 44.1K sample rate */
		if (mt6632_I2s_Setting(FM_I2S_ON, FM_I2S_MASTER, FM_I2S_44K))
			goto out;
		WCN_DBG(FM_NTC | CHIP, "set FM via Host\n");
	} else {
		WCN_DBG(FM_ERR | CHIP, "%s()\n", __func__);
		ret = -FM_EPARA;
		goto out;
	}
out:
	WCN_DBG(FM_NTC | CHIP, "-%s():[ret=%d]\n", __func__, ret);
	return ret;
}

/*
 * mt6632_I2s_Setting - set the I2S state on MT6632
 * @onoff - I2S on/off
 * @mode - I2S mode: Master or Slave
 *
 * Return:0, if success; error code, if failed
 */
static fm_s32 mt6632_I2s_Setting(fm_s32 onoff, fm_s32 mode, fm_s32 sample)
{
	fm_u16 tmp_state = 0;
	fm_u16 tmp_mode = 0;
	fm_u16 tmp_sample = 0;
	fm_s32 ret = 0;

	if (onoff == FM_I2S_ON) {
		tmp_state = 0x0002;	/* I2S enable and standard I2S mode, 0x9B D0,D1=1 */
		fm_config.aud_cfg.i2s_info.status = FM_I2S_ON;
	} else if (onoff == FM_I2S_OFF) {
		tmp_state = 0x0000;	/* I2S  off, 0x9B D0,D1=0 */
		fm_config.aud_cfg.i2s_info.status = FM_I2S_OFF;
	} else {
		WCN_DBG(FM_ERR | CHIP, "%s():[onoff=%d]\n", __func__, onoff);
		ret = -FM_EPARA;
		goto out;
	}

	if (mode == FM_I2S_MASTER) {
		tmp_mode = 0x0000;	/* 6632 as I2S master, set 0x9B D3=0 */
		fm_config.aud_cfg.i2s_info.mode = FM_I2S_MASTER;
	} else if (mode == FM_I2S_SLAVE) {
		tmp_mode = 0x0008;	/* 6632 as I2S slave, set 0x9B D3=1 */
		fm_config.aud_cfg.i2s_info.mode = FM_I2S_SLAVE;
	} else {
		WCN_DBG(FM_ERR | CHIP, "%s():[mode=%d]\n", __func__, mode);
		ret = -FM_EPARA;
		goto out;
	}

	if (sample == FM_I2S_32K) {
		tmp_sample = 0x0000;	/* 6632 I2S 32KHz sample rate, 0x5F D11~12 */
		fm_config.aud_cfg.i2s_info.rate = FM_I2S_32K;
	} else if (sample == FM_I2S_44K) {
		tmp_sample = 0x0800;	/* 6632 I2S 44.1KHz sample rate */
		fm_config.aud_cfg.i2s_info.rate = FM_I2S_44K;
	} else if (sample == FM_I2S_48K) {
		tmp_sample = 0x1000;	/* 6632 I2S 48KHz sample rate */
		fm_config.aud_cfg.i2s_info.rate = FM_I2S_48K;
	} else {
		WCN_DBG(FM_ERR | CHIP, "%s():[sample=%d]\n", __func__, sample);
		ret = -FM_EPARA;
		goto out;
	}

	ret = fm_reg_write(0x60, 0x7);
	if (ret)
		goto out;

	ret = fm_set_bits(0x5F, tmp_sample, 0xE7FF);
	if (ret)
		goto out;

	ret = fm_set_bits(0x9B, tmp_mode, 0xFFF7);
	if (ret)
		goto out;

	ret = fm_set_bits(0x9B, tmp_state, 0xFFFD);
	if (ret)
		goto out;

	WCN_DBG(FM_NTC | CHIP, "[onoff=%s][mode=%s][sample=%d](0)33KHz,(1)44.1KHz,(2)48KHz\n",
		   (onoff == FM_I2S_ON) ? "On" : "Off", (mode == FM_I2S_MASTER) ? "Master" : "Slave", sample);
out:
	return ret;
}

static fm_s32 mt6632fm_get_audio_info(fm_audio_info_t *data)
{
	fm_memcpy(data, &fm_config.aud_cfg, sizeof(fm_audio_info_t));
	return 0;
}

static fm_s32 mt6632_i2s_info_get(fm_s32 *ponoff, fm_s32 *pmode, fm_s32 *psample)
{
	*ponoff = fm_config.aud_cfg.i2s_info.status;
	*pmode = fm_config.aud_cfg.i2s_info.mode;
	*psample = fm_config.aud_cfg.i2s_info.rate;

	return 0;
}

static fm_s32 mt6632_hw_info_get(struct fm_hw_info *req)
{
	if (req == NULL) {
		WCN_DBG(FM_ERR | CHIP, "%s,invalid pointer\n", __func__);
		return -FM_EPARA;
	}

	req->chip_id = mt6632_hw_info.chip_id;
	req->eco_ver = mt6632_hw_info.eco_ver;
	req->patch_ver = mt6632_hw_info.patch_ver;
	req->rom_ver = mt6632_hw_info.rom_ver;

	return 0;
}

static fm_s32 mt6632_pre_search(void)
{
	mt6632_RampDown();
	return 0;
}

static fm_s32 mt6632_restore_search(void)
{
	mt6632_RampDown();
	return 0;
}

/*
freq: 8750~10800
valid: fm_true-valid channel,fm_false-invalid channel
return: fm_true- smt success, fm_false-smt fail
*/
static fm_s32 mt6632_soft_mute_tune(fm_u16 freq, fm_s32 *rssi, fm_bool *valid)
{
	fm_s32 ret = 0;
	fm_u16 pkt_size;
	/* fm_u16 freq;//, orig_freq; */
	struct mt6632_full_cqi *p_cqi;
	fm_s32 RSSI = 0, PAMD = 0, MR = 0, ATDC = 0;
	fm_u32 PRX = 0, ATDEV = 0;
	fm_u16 softmuteGainLvl = 0;

	ret = mt6632_chan_para_get(freq);
	if (ret == 2)
		ret = fm_set_bits(FM_CHANNEL_SET, 0x2000, 0x0FFF);	/* mdf HiLo */
	else
		ret = fm_set_bits(FM_CHANNEL_SET, 0x0000, 0x0FFF);	/* clear FA/HL/ATJ */
#if 0
	fm_reg_write(0x60, 0x0007);
	if (mt6632_TDD_chan_check(freq))
		fm_set_bits(0x30, 0x0004, 0xFFF9);	/* use TDD solution */
	else
		fm_set_bits(0x30, 0x0000, 0xFFF9);	/* default use FDD solution */
	fm_reg_write(0x60, 0x000F);
#endif
	if (FM_LOCK(cmd_buf_lock))
		return -FM_ELOCK;
	pkt_size = fm_full_cqi_req(cmd_buf, TX_BUF_SIZE, &freq, 1, 1);
	ret = fm_cmd_tx(cmd_buf, pkt_size, FLAG_SM_TUNE, SW_RETRY_CNT, SM_TUNE_TIMEOUT, fm_get_read_result);
	FM_UNLOCK(cmd_buf_lock);

	if (!ret && fm_res) {
		WCN_DBG(FM_NTC | CHIP, "smt cqi size %d\n", fm_res->cqi[0]);
		p_cqi = (struct mt6632_full_cqi *)&fm_res->cqi[2];
		/* just for debug */
		WCN_DBG(FM_NTC | CHIP,
			   "freq %d, 0x%04x, 0x%04x, 0x%04x, 0x%04x, 0x%04x, 0x%04x, 0x%04x, 0x%04x, 0x%04x, 0x%04x\n",
			   p_cqi->ch, p_cqi->rssi, p_cqi->pamd, p_cqi->pr, p_cqi->fpamd, p_cqi->mr,
			   p_cqi->atdc, p_cqi->prx, p_cqi->atdev, p_cqi->smg, p_cqi->drssi);
		RSSI = ((p_cqi->rssi & 0x03FF) >= 512) ? ((p_cqi->rssi & 0x03FF) - 1024) : (p_cqi->rssi & 0x03FF);
		PAMD = ((p_cqi->pamd & 0x1FF) >= 256) ? ((p_cqi->pamd & 0x01FF) - 512) : (p_cqi->pamd & 0x01FF);
		MR = ((p_cqi->mr & 0x01FF) >= 256) ? ((p_cqi->mr & 0x01FF) - 512) : (p_cqi->mr & 0x01FF);
		ATDC = (p_cqi->atdc >= 32768) ? (65536 - p_cqi->atdc) : (p_cqi->atdc);
		if (ATDC < 0)
			ATDC = (~(ATDC)) - 1;	/* Get abs value of ATDC */

		PRX = (p_cqi->prx & 0x00FF);
		ATDEV = p_cqi->atdev;
		softmuteGainLvl = p_cqi->smg;
		/* check if the channel is valid according to each CQIs */
		if ((RSSI >= fm_config.rx_cfg.long_ana_rssi_th)
		    && (PAMD <= fm_config.rx_cfg.pamd_th)
		    && (ATDC <= fm_config.rx_cfg.atdc_th)
		    && (MR >= fm_config.rx_cfg.mr_th)
		    && (PRX >= fm_config.rx_cfg.prx_th)
		    && (ATDEV >= ATDC)	/* sync scan algorithm */
		    && (softmuteGainLvl >= fm_config.rx_cfg.smg_th)) {
			*valid = fm_true;
		} else {
			*valid = fm_false;
		}
		*rssi = RSSI;
/*		if(RSSI < -296)
			WCN_DBG(FM_NTC | CHIP, "rssi\n");
		else if(PAMD > -12)
			WCN_DBG(FM_NTC | CHIP, "PAMD\n");
		else if(ATDC > 3496)
			WCN_DBG(FM_NTC | CHIP, "ATDC\n");
		else if(MR < -67)
			WCN_DBG(FM_NTC | CHIP, "MR\n");
		else if(PRX < 80)
			WCN_DBG(FM_NTC | CHIP, "PRX\n");
		else if(ATDEV < ATDC)
			WCN_DBG(FM_NTC | CHIP, "ATDEV\n");
		else if(softmuteGainLvl < 16421)
			WCN_DBG(FM_NTC | CHIP, "softmuteGainLvl\n");
			*/
	} else {
		WCN_DBG(FM_ERR | CHIP, "smt get CQI failed\n");
		return fm_false;
	}
	WCN_DBG(FM_NTC | CHIP, "valid=%d\n", *valid);
	return fm_true;
}

/*
parm:
	parm.th_type: 0, RSSI. 1,desense RSSI. 2,SMG.
    parm.th_val: threshold value
*/
static fm_s32 mt6632_set_search_th(fm_s32 idx, fm_s32 val, fm_s32 reserve)
{
	switch (idx) {
	case 0: {
		fm_config.rx_cfg.long_ana_rssi_th = val;
		WCN_DBG(FM_NTC | CHIP, "set rssi th =%d\n", val);
		break;
	}
	case 1: {
		fm_config.rx_cfg.desene_rssi_th = val;
		WCN_DBG(FM_NTC | CHIP, "set desense rssi th =%d\n", val);
		break;
	}
	case 2: {
		fm_config.rx_cfg.smg_th = val;
		WCN_DBG(FM_NTC | CHIP, "set smg th =%d\n", val);
		break;
	}
	default:
		break;
	}
	return 0;
}

#if 0
static const fm_u16 mt6632_mcu_dese_list[] = {
	0			/* 7630, 7800, 7940, 8320, 9260, 9600, 9710, 9920, 10400, 10410 */
};

static const fm_u16 mt6632_gps_dese_list[] = {
	0			/* 7850, 7860 */
};
#endif

static const fm_s8 mt6632_chan_para_map[] = {
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	/* 6500~6595 */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	/* 6600~6695 */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	/* 6700~6795 */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	/* 6800~6895 */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	/* 6900~6995 */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	/* 7000~7095 */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	/* 7100~7195 */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	/* 7200~7295 */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	/* 7300~7395 */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	/* 7400~7495 */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	/* 7500~7595 */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	/* 7600~7695 */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	/* 7700~7795 */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	/* 7800~7895 */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	/* 7900~7995 */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	/* 8000~8095 */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	/* 8100~8195 */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	/* 8200~8295 */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	/* 8300~8395 */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	/* 8400~8495 */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	/* 8500~8595 */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	/* 8600~8695 */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	/* 8700~8795 */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	/* 8800~8895 */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	/* 8900~8995 */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	/* 9000~9095 */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	/* 9100~9195 */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	/* 9200~9295 */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	/* 9300~9395 */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	/* 9400~9495 */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	/* 9500~9595 */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	/* 9600~9695 */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	/* 9700~9795 */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	/* 9800~9895 */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	/* 9900~9995 */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	/* 10000~10095 */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	/* 10100~10195 */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	/* 10200~10295 */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	/* 10300~10395 */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	/* 10400~10495 */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	/* 10500~10595 */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	/* 10600~10695 */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	/* 10700~10795 */
	0			/* 10800 */
};

static const fm_u16 mt6632_TDD_list[] = {
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000,	/* 6500~6595 */
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000,	/* 6600~6695 */
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000,	/* 6700~6795 */
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000,	/* 6800~6895 */
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000,	/* 6900~6995 */
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000,	/* 7000~7095 */
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000,	/* 7100~7195 */
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000,	/* 7200~7295 */
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000,	/* 7300~7395 */
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000,	/* 7400~7495 */
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000,	/* 7500~7595 */
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000,	/* 7600~7695 */
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000,	/* 7700~7795 */
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000,	/* 7800~7895 */
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000,	/* 7900~7995 */
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000,	/* 8000~8095 */
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000,	/* 8100~8195 */
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000,	/* 8200~8295 */
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000,	/* 8300~8395 */
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000,	/* 8400~8495 */
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000,	/* 8500~8595 */
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000,	/* 8600~8695 */
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000,	/* 8700~8795 */
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000,	/* 8800~8895 */
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000,	/* 8900~8995 */
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000,	/* 9000~9095 */
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000,	/* 9100~9195 */
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000,	/* 9200~9295 */
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000,	/* 9300~9395 */
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000,	/* 9400~9495 */
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000,	/* 9500~9595 */
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000,	/* 9600~9695 */
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000,	/* 9700~9795 */
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000,	/* 9800~9895 */
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000,	/* 9900~9995 */
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000,	/* 10000~10095 */
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000,	/* 10100~10195 */
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000,	/* 10200~10295 */
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000,	/* 10300~10395 */
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000,	/* 10400~10495 */
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000,	/* 10500~10595 */
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000,	/* 10600~10695 */
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000,	/* 10700~10795 */
	0x0000			/* 10800 */
};

static const fm_u16 mt6632_TDD_Mask[] = {
	0x0001, 0x0010, 0x0100, 0x1000
};

static const fm_u16 mt6632_scan_dese_list[] = {
	7800, 9210, 9220, 9600, 9980, 10400, 10750, 10760
};


/* return value: 0, not a de-sense channel; 1, this is a de-sense channel; else error no */
static fm_s32 mt6632_is_dese_chan(fm_u16 freq)
{
	fm_s32 size;

	/* return 0;//HQA only :skip desense channel check. */
	size = sizeof(mt6632_scan_dese_list) / sizeof(mt6632_scan_dese_list[0]);

	if (0 == fm_get_channel_space(freq))
		freq *= 10;

	while (size) {
		if (mt6632_scan_dese_list[size - 1] == freq)
			return 1;

		size--;
	}

	return 0;
}

static fm_bool mt6632_TDD_chan_check(fm_u16 freq)
{
	fm_u32 i = 0;
	fm_u16 freq_tmp = freq;
	fm_s32 ret = 0;

	ret = fm_get_channel_space(freq_tmp);
	if (0 == ret)
		freq_tmp *= 10;
	else if (-1 == ret)
		return fm_false;

	i = (freq_tmp - 6500) / 5;
	if ((i / 4) >= (sizeof(mt6632_TDD_list) / sizeof(mt6632_TDD_list[0]))) {
		WCN_DBG(FM_ERR | CHIP, "Freq index out of range(%d),max(%zd)\n",
			i / 4, (sizeof(mt6632_TDD_list) / sizeof(mt6632_TDD_list[0])));
		return fm_false;
	}

	if (mt6632_TDD_list[i / 4] & mt6632_TDD_Mask[i % 4]) {
		WCN_DBG(FM_DBG | CHIP, "Freq %d use TDD solution\n", freq);
		return fm_true;
	} else
		return fm_false;
}

/*  return value:
1, is desense channel and rssi is less than threshold;
0, not desense channel or it is but rssi is more than threshold.*/
static fm_s32 mt6632_desense_check(fm_u16 freq, fm_s32 rssi)
{
	if (mt6632_is_dese_chan(freq)) {
		if (rssi < fm_config.rx_cfg.desene_rssi_th)
			return 1;

		WCN_DBG(FM_DBG | CHIP, "desen_rssi %d th:%d\n", rssi, fm_config.rx_cfg.desene_rssi_th);
	}
	return 0;
}

/* get channel parameter, HL side/ FA / ATJ */
static fm_u16 mt6632_chan_para_get(fm_u16 freq)
{
	fm_s32 pos, size;

	/* return 0;//for HQA only: skip FA/HL/ATJ */
	if (0 == fm_get_channel_space(freq))
		freq *= 10;

	if (freq < 6500)
		return 0;

	pos = (freq - 6500) / 5;

	size = sizeof(mt6632_chan_para_map) / sizeof(mt6632_chan_para_map[0]);

	pos = (pos < 0) ? 0 : pos;
	pos = (pos > (size - 1)) ? (size - 1) : pos;

	return mt6632_chan_para_map[pos];
}

static fm_s32 mt6632_gps_dese(fm_u16 freq, void *arg)
{
	/*fm_gps_desense_t state = FM_GPS_DESE_DISABLE;*/

	if (0 == fm_get_channel_space(freq))
		freq *= 10;

	WCN_DBG(FM_NTC | CHIP, "%s, [freq=%d]\n", __func__, (int)freq);

	/* request 6632 GPS change clk */
	if ((freq >= 7690) && (freq <= 8230)) {
		WCN_DBG(FM_NTC | CHIP, "gps desense disable\n");
		if (!mtk_wcn_wmt_dsns_ctrl(WMTDSNS_FM_GPS_DISABLE))
			return -1;
	}

	if ((freq >= 8460) && (freq <= 9120)) {
		WCN_DBG(FM_NTC | CHIP, "gps desense enable\n");
		if (!mtk_wcn_wmt_dsns_ctrl(WMTDSNS_FM_GPS_ENABLE))
			return -1;
	}

	return 1;
}


fm_s32 fm_low_ops_register(struct fm_callback *cb, struct fm_basic_interface *bi)
{
	fm_s32 ret = 0;
	/* Basic functions. */

	if (bi == NULL) {
		WCN_DBG(FM_ERR | CHIP, "%s,bi invalid pointer\n", __func__);
		return -FM_EPARA;
	}
	if (cb->cur_freq_get == NULL) {
		WCN_DBG(FM_ERR | CHIP, "%s,cb->cur_freq_get invalid pointer\n", __func__);
		return -FM_EPARA;
	}
	if (cb->cur_freq_set == NULL) {
		WCN_DBG(FM_ERR | CHIP, "%s,cb->cur_freq_set invalid pointer\n", __func__);
		return -FM_EPARA;
	}
	fm_cb_op = cb;

	bi->pwron = mt6632_pwron;
	bi->pwroff = mt6632_pwroff;
	bi->pmic_read = mt6632_pmic_read;
	bi->pmic_write = mt6632_pmic_write;
	bi->chipid_get = mt6632_get_chipid;
	bi->mute = mt6632_Mute;
	bi->rampdown = mt6632_RampDown;
	bi->pwrupseq = mt6632_PowerUp;
	bi->pwrdownseq = mt6632_PowerDown;
	bi->setfreq = mt6632_SetFreq;
	/* bi->low_pwr_wa = MT6632fm_low_power_wa_default; */
	bi->i2s_set = mt6632_I2s_Setting;
	bi->rssiget = mt6632_GetCurRSSI;
	bi->volset = mt6632_SetVol;
	bi->volget = mt6632_GetVol;
	bi->dumpreg = mt6632_dump_reg;
	bi->msget = mt6632_GetMonoStereo;
	bi->msset = mt6632_SetMonoStereo;
	bi->pamdget = mt6632_GetCurPamd;
	/* bi->em = mt6632_em_test; */
	bi->anaswitch = mt6632_SetAntennaType;
	bi->anaget = mt6632_GetAntennaType;
	bi->caparray_get = mt6632_GetCapArray;
	bi->hwinfo_get = mt6632_hw_info_get;
	bi->fm_via_bt = MT6632_FMOverBT;
	bi->i2s_get = mt6632_i2s_info_get;
	bi->is_dese_chan = mt6632_is_dese_chan;
	bi->softmute_tune = mt6632_soft_mute_tune;
	bi->desense_check = mt6632_desense_check;
	bi->cqi_log = mt6632_full_cqi_get;
	bi->pre_search = mt6632_pre_search;
	bi->restore_search = mt6632_restore_search;
	bi->set_search_th = mt6632_set_search_th;
	bi->get_aud_info = mt6632fm_get_audio_info;

	cmd_buf_lock = fm_lock_create("32_cmd");
	ret = fm_lock_get(cmd_buf_lock);

	cmd_buf = fm_zalloc(TX_BUF_SIZE + 1);

	if (!cmd_buf) {
		WCN_DBG(FM_ERR | CHIP, "6632 fm lib alloc tx buf failed\n");
		ret = -1;
	}

	return ret;
}

fm_s32 fm_low_ops_unregister(struct fm_basic_interface *bi)
{
	fm_s32 ret = 0;
	/* Basic functions. */
	if (bi == NULL) {
		WCN_DBG(FM_ERR | CHIP, "%s,bi invalid pointer\n", __func__);
		return -FM_EPARA;
	}

	if (cmd_buf) {
		fm_free(cmd_buf);
		cmd_buf = NULL;
	}

	ret = fm_lock_put(cmd_buf_lock);
	fm_memset(bi, 0, sizeof(struct fm_basic_interface));

	fm_cb_op = NULL;

	return ret;
}

