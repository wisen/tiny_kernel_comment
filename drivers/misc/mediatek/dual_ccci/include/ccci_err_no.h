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

#ifndef __CCCI_ERR_NO_H__
#define __CCCI_ERR_NO_H__

/*  CCCI error number region */
#define CCCI_ERR_MODULE_INIT_START_ID   (0)
#define CCCI_ERR_COMMON_REGION_START_ID    (100)
#define CCCI_ERR_CCIF_REGION_START_ID    (200)
#define CCCI_ERR_CCCI_REGION_START_ID    (300)
#define CCCI_ERR_LOAD_IMG_START_ID        (400)

/*  CCCI error number */
#define CCCI_ERR_MODULE_INIT_OK                      (CCCI_ERR_MODULE_INIT_START_ID+0)
#define CCCI_ERR_INIT_DEV_NODE_FAIL                 (CCCI_ERR_MODULE_INIT_START_ID+1)
#define CCCI_ERR_INIT_PLATFORM_FAIL                 (CCCI_ERR_MODULE_INIT_START_ID+2)
#define CCCI_ERR_MK_DEV_NODE_FAIL                   (CCCI_ERR_MODULE_INIT_START_ID+3)
#define CCCI_ERR_INIT_LOGIC_LAYER_FAIL              (CCCI_ERR_MODULE_INIT_START_ID+4)
#define CCCI_ERR_INIT_MD_CTRL_FAIL                  (CCCI_ERR_MODULE_INIT_START_ID+5)
#define CCCI_ERR_INIT_CHAR_DEV_FAIL                 (CCCI_ERR_MODULE_INIT_START_ID+6)
#define CCCI_ERR_INIT_TTY_FAIL                      (CCCI_ERR_MODULE_INIT_START_ID+7)
#define CCCI_ERR_INIT_IPC_FAIL                      (CCCI_ERR_MODULE_INIT_START_ID+8)
#define CCCI_ERR_INIT_RPC_FAIL                      (CCCI_ERR_MODULE_INIT_START_ID+9)
#define CCCI_ERR_INIT_FS_FAIL                       (CCCI_ERR_MODULE_INIT_START_ID+10)
#define CCCI_ERR_INIT_CCMNI_FAIL                    (CCCI_ERR_MODULE_INIT_START_ID+11)
#define CCCI_ERR_INIT_VIR_CHAR_FAIL                 (CCCI_ERR_MODULE_INIT_START_ID+12)

/*  ---- Common */
#define CCCI_ERR_FATAL_ERR                            (CCCI_ERR_COMMON_REGION_START_ID+0)
#define CCCI_ERR_ASSERT_ERR                            (CCCI_ERR_COMMON_REGION_START_ID+1)
#define CCCI_ERR_MD_IN_RESET                        (CCCI_ERR_COMMON_REGION_START_ID+2)
#define CCCI_ERR_RESET_NOT_READY                    (CCCI_ERR_COMMON_REGION_START_ID+3)
#define CCCI_ERR_GET_MEM_FAIL                        (CCCI_ERR_COMMON_REGION_START_ID+4)
#define CCCI_ERR_GET_SMEM_SETTING_FAIL                (CCCI_ERR_COMMON_REGION_START_ID+5)
#define CCCI_ERR_INVALID_PARAM                        (CCCI_ERR_COMMON_REGION_START_ID+6)
#define CCCI_ERR_LARGE_THAN_BUF_SIZE                (CCCI_ERR_COMMON_REGION_START_ID+7)
#define CCCI_ERR_GET_MEM_LAYOUT_FAIL                (CCCI_ERR_COMMON_REGION_START_ID+8)
#define CCCI_ERR_MEM_CHECK_FAIL                        (CCCI_ERR_COMMON_REGION_START_ID+9)
#define CCCI_IPO_H_RESTORE_FAIL                        (CCCI_ERR_COMMON_REGION_START_ID+10)

/*  ---- CCIF */
#define CCCI_ERR_CCIF_NOT_READY                        (CCCI_ERR_CCIF_REGION_START_ID+0)
#define CCCI_ERR_CCIF_CALL_BACK_HAS_REGISTERED        (CCCI_ERR_CCIF_REGION_START_ID+1)
#define CCCI_ERR_CCIF_GET_NULL_POINTER                (CCCI_ERR_CCIF_REGION_START_ID+2)
#define CCCI_ERR_CCIF_UN_SUPPORT                    (CCCI_ERR_CCIF_REGION_START_ID+3)
#define CCCI_ERR_CCIF_NO_PHYSICAL_CHANNEL            (CCCI_ERR_CCIF_REGION_START_ID+4)
#define CCCI_ERR_CCIF_INVALID_RUNTIME_LEN            (CCCI_ERR_CCIF_REGION_START_ID+5)
#define CCCI_ERR_CCIF_INVALID_MD_SYS_ID                (CCCI_ERR_CCIF_REGION_START_ID+6)
#define CCCI_ERR_CCIF_GET_HW_INFO_FAIL                (CCCI_ERR_CCIF_REGION_START_ID+9)

/*  ---- CCCI */
#define CCCI_ERR_INVALID_LOGIC_CHANNEL_ID            (CCCI_ERR_CCCI_REGION_START_ID+0)
#define CCCI_ERR_PUSH_RX_DATA_TO_TX_CHANNEL            (CCCI_ERR_CCCI_REGION_START_ID+1)
#define CCCI_ERR_REG_CALL_BACK_FOR_TX_CHANNEL        (CCCI_ERR_CCCI_REGION_START_ID+2)
#define CCCI_ERR_LOGIC_CH_HAS_REGISTERED            (CCCI_ERR_CCCI_REGION_START_ID+3)
#define CCCI_ERR_MD_NOT_READY                        (CCCI_ERR_CCCI_REGION_START_ID+4)
#define CCCI_ERR_ALLOCATE_MEMORY_FAIL                (CCCI_ERR_CCCI_REGION_START_ID+5)
#define CCCI_ERR_CREATE_CCIF_INSTANCE_FAIL            (CCCI_ERR_CCCI_REGION_START_ID+6)
#define CCCI_ERR_REPEAT_CHANNEL_ID                    (CCCI_ERR_CCCI_REGION_START_ID+7)
#define CCCI_ERR_KFIFO_IS_NOT_READY                    (CCCI_ERR_CCCI_REGION_START_ID+8)
#define CCCI_ERR_GET_NULL_POINTER                    (CCCI_ERR_CCCI_REGION_START_ID+9)
#define CCCI_ERR_GET_RX_DATA_FROM_TX_CHANNEL        (CCCI_ERR_CCCI_REGION_START_ID+10)
#define CCCI_ERR_CHANNEL_NUM_MIS_MATCH                (CCCI_ERR_CCCI_REGION_START_ID+11)
#define CCCI_ERR_START_ADDR_NOT_4BYTES_ALIGN        (CCCI_ERR_CCCI_REGION_START_ID+12)
#define CCCI_ERR_NOT_DIVISIBLE_BY_4                    (CCCI_ERR_CCCI_REGION_START_ID+13)
#define CCCI_ERR_MD_AT_EXCEPTION                    (CCCI_ERR_CCCI_REGION_START_ID+14)
#define CCCI_ERR_MD_CB_HAS_REGISTER                    (CCCI_ERR_CCCI_REGION_START_ID+15)

/*  ---- Load image error */
#define CCCI_ERR_LOAD_IMG_NOMEM                        (CCCI_ERR_LOAD_IMG_START_ID+0)
#define CCCI_ERR_LOAD_IMG_FILE_OPEN                    (CCCI_ERR_LOAD_IMG_START_ID+1)
#define CCCI_ERR_LOAD_IMG_FILE_READ                    (CCCI_ERR_LOAD_IMG_START_ID+2)
#define CCCI_ERR_LOAD_IMG_KERN_READ                    (CCCI_ERR_LOAD_IMG_START_ID+3)
#define CCCI_ERR_LOAD_IMG_NO_ADDR                    (CCCI_ERR_LOAD_IMG_START_ID+4)
#define CCCI_ERR_LOAD_IMG_NO_FIRST_BOOT                (CCCI_ERR_LOAD_IMG_START_ID+5)
#define CCCI_ERR_LOAD_IMG_LOAD_FIRM                    (CCCI_ERR_LOAD_IMG_START_ID+6)
#define CCCI_ERR_LOAD_IMG_FIRM_NULL                    (CCCI_ERR_LOAD_IMG_START_ID+7)
#define CCCI_ERR_LOAD_IMG_CHECK_HEAD                (CCCI_ERR_LOAD_IMG_START_ID+8)
#define CCCI_ERR_LOAD_IMG_SIGN_FAIL                    (CCCI_ERR_LOAD_IMG_START_ID+9)
#define CCCI_ERR_LOAD_IMG_CIPHER_FAIL                (CCCI_ERR_LOAD_IMG_START_ID+10)
#define CCCI_ERR_LOAD_IMG_MD_CHECK                    (CCCI_ERR_LOAD_IMG_START_ID+11)
#define CCCI_ERR_LOAD_IMG_DSP_CHECK                    (CCCI_ERR_LOAD_IMG_START_ID+12)
#define CCCI_ERR_LOAD_IMG_ABNORAL_SIZE                (CCCI_ERR_LOAD_IMG_START_ID+13)

#endif				/* __CCCi_ERR_NO_H__ */
