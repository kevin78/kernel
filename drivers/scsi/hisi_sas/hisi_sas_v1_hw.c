/*
 * Copyright (c) 2015 Linaro Ltd.
 * Copyright (c) 2015 Hisilicon Limited.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 */

#include "hisi_sas.h"
#include <linux/crc-t10dif.h>
#include <linux/debugfs.h>
#include <linux/fs.h>
#include <linux/module.h>
#include <linux/swab.h>
#include <linux/types.h>
#include <linux/t10-pi.h>
#include <scsi/scsi_cmnd.h>
#include <scsi/scsi_dbg.h>

#undef SAS_12G

/* global registers need init*/
#define DLVRY_QUEUE_ENABLE		0x0
#define IOST_BASE_ADDR_LO		0x8
#define IOST_BASE_ADDR_HI		0xc
#define ITCT_BASE_ADDR_LO		0x10
#define ITCT_BASE_ADDR_HI		0x14
#define BROKEN_MSG_ADDR_LO		0x18
#define BROKEN_MSG_ADDR_HI		0x1c
#define PHY_CONTEXT			0x20
#define PHY_STATE			0x24
#define PHY_PORT_NUM_MA			0x28
#define PORT_STATE			0x2c
#define PHY_CONN_RATE			0x30
#define HGC_TRANS_TASK_CNT_LIMIT	0x38
#define AXI_AHB_CLK_CFG			0x3c
#define HGC_SAS_TXFAIL_RETRY_CTRL	0x84
#define HGC_GET_ITV_TIME		0x90
#define DEVICE_MSG_WORK_MODE		0x94
#define I_T_NEXUS_LOSS_TIME		0xa0
#define BUS_INACTIVE_LIMIT_TIME	0xa8
#define REJECT_TO_OPEN_LIMIT_TIME	0xac
#define CFG_AGING_TIME			0xbc
#define CFG_AGING_TIME_ITCT_REL_OFF	0
#define CFG_AGING_TIME_ITCT_REL_MSK	0x1
#define HGC_DFX_CFG2			0xc0
#define FIS_LIST_BADDR_L		0xc4
#define CFG_1US_TIMER_TRSH		0xcc
#define CFG_SAS_CONFIG			0xd4
#define HGC_IOST_ECC_ADDR		0x140
#define HGC_IOST_ECC_ADDR_BAD_OFF	16
#define HGC_IOST_ECC_ADDR_BAD_MSK	0x3ff0000
#define HGC_DQ_ECC_ADDR			0x144
#define HGC_DQ_ECC_ADDR_BAD_OFF		16
#define HGC_DQ_ECC_ADDR_BAD_MSK		0xfff0000
#define HGC_INVLD_DQE_INFO		0x148
#define HGC_INVLD_DQE_INFO_DQ_OFF	0
#define HGC_INVLD_DQE_INFO_DQ_MSK	0xffff
#define HGC_INVLD_DQE_INFO_TYPE_OFF	16
#define HGC_INVLD_DQE_INFO_TYPE_MSK	0x10000
#define HGC_INVLD_DQE_INFO_FORCE_OFF	17
#define HGC_INVLD_DQE_INFO_FORCE_MSK	0x20000
#define HGC_INVLD_DQE_INFO_PHY_OFF	18
#define HGC_INVLD_DQE_INFO_PHY_MSK	0x40000
#define HGC_INVLD_DQE_INFO_ABORT_OFF	19
#define HGC_INVLD_DQE_INFO_ABORT_MSK	0x80000
#define HGC_INVLD_DQE_INFO_IPTT_OF_OFF	20
#define HGC_INVLD_DQE_INFO_IPTT_OF_MSK	0x100000
#define HGC_INVLD_DQE_INFO_SSP_ERR_OFF	21
#define HGC_INVLD_DQE_INFO_SSP_ERR_MSK	0x200000
#define HGC_INVLD_DQE_INFO_OFL_OFF	22
#define HGC_INVLD_DQE_INFO_OFL_MSK	0x400000
#define HGC_ITCT_ECC_ADDR		0x150
#define HGC_ITCT_ECC_ADDR_BAD_OFF	16
#define HGC_ITCT_ECC_ADDR_BAD_MSK	0x3ff0000
#define HGC_AXI_FIFO_ERR_INFO		0x154
#define INT_COAL_EN			0x1bc
#define OQ_INT_COAL_TIME		0x1c0
#define OQ_INT_COAL_CNT			0x1c4
#define ENT_INT_COAL_TIME		0x1c8
#define ENT_INT_COAL_CNT		0x1cc
#define OQ_INT_SRC			0x1d0
#define OQ_INT_SRC_MSK			0x1d4
#define ENT_INT_SRC1			0x1d8
#define ENT_INT_SRC2			0x1dc
#define ENT_INT_SRC2_DQ_CFG_ERR_OFF	25
#define ENT_INT_SRC2_DQ_CFG_ERR_MSK	0x2000000
#define ENT_INT_SRC2_CQ_CFG_ERR_OFF	27
#define ENT_INT_SRC2_CQ_CFG_ERR_MSK	0x8000000
#define ENT_INT_SRC2_AXI_WRONG_INT_OFF	28
#define ENT_INT_SRC2_AXI_WRONG_INT_MSK	0x10000000
#define ENT_INT_SRC2_AXI_OVERLF_INT_OFF	29
#define ENT_INT_SRC2_AXI_OVERLF_INT_MSK	0x20000000
#define ENT_INT_SRC_MSK1		0x1e0
#define ENT_INT_SRC_MSK2		0x1e4
#define SAS_ECC_INTR			0x1e8
#define SAS_ECC_INTR_DQ_ECC1B_OFF	0
#define SAS_ECC_INTR_DQ_ECC1B_MSK	0x1
#define SAS_ECC_INTR_DQ_ECCBAD_OFF	1
#define SAS_ECC_INTR_DQ_ECCBAD_MSK	0x2
#define SAS_ECC_INTR_IOST_ECC1B_OFF	2
#define SAS_ECC_INTR_IOST_ECC1B_MSK	0x4
#define SAS_ECC_INTR_IOST_ECCBAD_OFF	3
#define SAS_ECC_INTR_IOST_ECCBAD_MSK	0x8
#define SAS_ECC_INTR_ITCT_ECC1B_OFF	4
#define SAS_ECC_INTR_ITCT_ECC1B_MSK	0x10
#define SAS_ECC_INTR_ITCT_ECCBAD_OFF	5
#define SAS_ECC_INTR_ITCT_ECCBAD_MSK	0x20
#define SAS_ECC_INTR_MSK		0x1ec
#define HGC_ERR_STAT_EN			0x238
#define DLVRY_Q_0_BASE_ADDR_LO		0x260
#define DLVRY_Q_0_BASE_ADDR_HI		0x264
#define DLVRY_Q_0_DEPTH			0x268
#define DLVRY_Q_0_WR_PTR		0x26c
#define DLVRY_Q_0_RD_PTR		0x270
#define COMPL_Q_0_BASE_ADDR_LO		0x4e0
#define COMPL_Q_0_BASE_ADDR_HI		0x4e4
#define COMPL_Q_0_DEPTH			0x4e8
#define COMPL_Q_0_WR_PTR		0x4ec
#define COMPL_Q_0_RD_PTR		0x4f0
#define HGC_ECC_ERR			0x7d0

/* phy registers need init */
#define PORT_BASE			(0x800)

#define PHY_CFG				(PORT_BASE + 0x0)
#define PHY_CFG_ENA_OFF			0
#define PHY_CFG_ENA_MSK			0x1
#define PHY_CFG_SATA_OFF		1
#define PHY_CFG_SATA_MSK		0x2
#define PHY_CFG_DC_OPT_OFF		2
#define PHY_CFG_DC_OPT_MSK		0x4
#define PROG_PHY_LINK_RATE		(PORT_BASE + 0xc)
#define PROG_PHY_LINK_RATE_MAX_OFF	0
#define PROG_PHY_LINK_RATE_MAX_MSK	0xf
#define PROG_PHY_LINK_RATE_MIN_OFF	4
#define PROG_PHY_LINK_RATE_MIN_MSK	0xf0
#define PROG_PHY_LINK_RATE_OOB_OFF	8
#define PROG_PHY_LINK_RATE_OOB_MSK	0xf00
#define PHY_CTRL			(PORT_BASE + 0x14)
#define PHY_CTRL_RESET_OFF		0
#define PHY_CTRL_RESET_MSK		0x1
#define PHY_RATE_NEGO			(PORT_BASE + 0x30)
#define PHY_PCN				(PORT_BASE + 0x44)
#define SL_TOUT_CFG			(PORT_BASE + 0x8c)
#define SL_CONTROL			(PORT_BASE + 0x94)
#define SL_CONTROL_NOTIFY_EN_OFF	0
#define SL_CONTROL_NOTIFY_EN_MSK	0x1
#define TX_ID_DWORD0			(PORT_BASE + 0x9c)
#define TX_ID_DWORD1			(PORT_BASE + 0xa0)
#define TX_ID_DWORD2			(PORT_BASE + 0xa4)
#define TX_ID_DWORD3			(PORT_BASE + 0xa8)
#define TX_ID_DWORD4			(PORT_BASE + 0xaC)
#define TX_ID_DWORD5			(PORT_BASE + 0xb0)
#define TX_ID_DWORD6			(PORT_BASE + 0xb4)
#define RX_IDAF_DWORD0			(PORT_BASE + 0xc4)
#define RX_IDAF_DWORD1			(PORT_BASE + 0xc8)
#define RX_IDAF_DWORD2			(PORT_BASE + 0xcc)
#define RX_IDAF_DWORD3			(PORT_BASE + 0xd0)
#define RX_IDAF_DWORD4			(PORT_BASE + 0xd4)
#define RX_IDAF_DWORD5			(PORT_BASE + 0xd8)
#define RX_IDAF_DWORD6			(PORT_BASE + 0xdc)
#define RXOP_CHECK_CFG_H		(PORT_BASE + 0xfc)
#define DONE_RECEIVED_TIME		(PORT_BASE + 0x12c)
#define CON_CFG_DRIVER			(PORT_BASE + 0x130)
#define PHY_CONFIG2			(PORT_BASE + 0x1a8)
#define PHY_CONFIG2_FORCE_TXDEEMPH_OFF	3
#define PHY_CONFIG2_FORCE_TXDEEMPH_MSK	0x8
#define PHY_CONFIG2_TX_TRAIN_COMP_OFF	24
#define PHY_CONFIG2_TX_TRAIN_COMP_MSK	0x1000000
#define CHL_INT0			(PORT_BASE + 0x1b0)
#define CHL_INT0_PHYCTRL_NOTRDY_OFF	0
#define CHL_INT0_PHYCTRL_NOTRDY_MSK	0x1
#define CHL_INT0_SN_FAIL_NGR_OFF	2
#define CHL_INT0_SN_FAIL_NGR_MSK	0x4
#define CHL_INT0_DWS_LOST_OFF		4
#define CHL_INT0_DWS_LOST_MSK		0x10
#define CHL_INT0_SL_IDAF_FAIL_OFF	10
#define CHL_INT0_SL_IDAF_FAIL_MSK	0x400
#define CHL_INT0_ID_TIMEOUT_OFF	11
#define CHL_INT0_ID_TIMEOUT_MSK	0x800
#define CHL_INT0_SL_OPAF_FAIL_OFF	12
#define CHL_INT0_SL_OPAF_FAIL_MSK	0x1000
#define CHL_INT0_SL_PS_FAIL_OFF		21
#define CHL_INT0_SL_PS_FAIL_MSK		0x200000
#define CHL_INT1			(PORT_BASE + 0x1b4)
#define CHL_INT2			(PORT_BASE + 0x1b8)
#define CHL_INT2_RXEYEDIAG_DONE_OFF	9
#define CHL_INT2_RXEYEDIAG_DONE_MSK	0x200
#define CHL_INT2_SL_RX_BC_ACK_OFF	2
#define CHL_INT2_SL_RX_BC_ACK_MSK	0x4
#define CHL_INT2_SL_PHY_ENA_OFF		6
#define CHL_INT2_SL_PHY_ENA_MSK		0x40
#define CHL_INT0_MSK			(PORT_BASE + 0x1bc)
#define CHL_INT0_MSK_PHYCTRL_NOTRDY_OFF	0
#define CHL_INT0_MSK_PHYCTRL_NOTRDY_MSK	0x1
#define CHL_INT1_MSK			(PORT_BASE + 0x1c0)
#define CHL_INT2_MSK			(PORT_BASE + 0x1c4)
#define CHL_INT_COAL_EN			(PORT_BASE + 0x1d0)
#define DMA_TX_STATUS			(PORT_BASE + 0x2d0)
#define DMA_TX_STATUS_BUSY_OFF		0
#define DMA_TX_STATUS_BUSY_MSK		0x1
#define DMA_RX_STATUS			(PORT_BASE + 0x2e8)
#define DMA_RX_STATUS_BUSY_OFF		0
#define DMA_RX_STATUS_BUSY_MSK		0x1

#define AXI_CFG				(0x5100)
#define CONTROLLER_RESET_VALUE		(0x7ffff)

enum {
	HISI_SAS_PHY_BCAST_ACK = 0,
	HISI_SAS_PHY_SL_PHY_ENABLED,
	HISI_SAS_PHY_INT_REG0,
	HISI_SAS_PHY_INT_ABNORMAL,
	HISI_SAS_PHY_INT_NR
};

enum {
	DMA_TX_ERR_BASE = 0x0,
	DMA_RX_ERR_BASE = 0x100,
	TRANS_TX_FAIL_BASE = 0x200,
	TRANS_RX_FAIL_BASE = 0x300,

	/* dma tx */
	DMA_TX_DIF_CRC_ERR = DMA_TX_ERR_BASE, /* 0x0 */
	DMA_TX_DIF_APP_ERR, /* 0x1 */
	DMA_TX_DIF_RPP_ERR, /* 0x2 */
	DMA_TX_AXI_BUS_ERR, /* 0x3 */
	DMA_TX_DATA_SGL_OVERFLOW_ERR, /* 0x4 */
	DMA_TX_DIF_SGL_OVERFLOW_ERR, /* 0x5 */
	DMA_TX_UNEXP_XFER_RDY_ERR, /* 0x6 */
	DMA_TX_XFER_RDY_OFFSET_ERR, /* 0x7 */
	DMA_TX_DATA_UNDERFLOW_ERR, /* 0x8 */
	DMA_TX_XFER_RDY_LENGTH_OVERFLOW_ERR, /* 0x9 */

	/* dma rx */
	DMA_RX_BUFFER_ECC_ERR = DMA_RX_ERR_BASE, /* 0x100 */
	DMA_RX_DIF_CRC_ERR, /* 0x101 */
	DMA_RX_DIF_APP_ERR, /* 0x102 */
	DMA_RX_DIF_RPP_ERR, /* 0x103 */
	DMA_RX_RESP_BUFFER_OVERFLOW_ERR, /* 0x104 */
	DMA_RX_AXI_BUS_ERR, /* 0x105 */
	DMA_RX_DATA_SGL_OVERFLOW_ERR, /* 0x106 */
	DMA_RX_DIF_SGL_OVERFLOW_ERR, /* 0x107 */
	DMA_RX_DATA_OFFSET_ERR, /* 0x108 */
	DMA_RX_UNEXP_RX_DATA_ERR, /* 0x109 */
	DMA_RX_DATA_OVERFLOW_ERR, /* 0x10a */
	DMA_RX_DATA_UNDERFLOW_ERR, /* 0x10b */
	DMA_RX_UNEXP_RETRANS_RESP_ERR, /* 0x10c */

	/* trans tx */
	TRANS_TX_RSVD0_ERR = TRANS_TX_FAIL_BASE, /* 0x200 */
	TRANS_TX_PHY_NOT_ENABLE_ERR, /* 0x201 */
	TRANS_TX_OPEN_REJCT_WRONG_DEST_ERR, /* 0x202 */
	TRANS_TX_OPEN_REJCT_ZONE_VIOLATION_ERR, /* 0x203 */
	TRANS_TX_OPEN_REJCT_BY_OTHER_ERR, /* 0x204 */
	TRANS_TX_RSVD1_ERR, /* 0x205 */
	TRANS_TX_OPEN_REJCT_AIP_TIMEOUT_ERR, /* 0x206 */
	TRANS_TX_OPEN_REJCT_STP_BUSY_ERR, /* 0x207 */
	TRANS_TX_OPEN_REJCT_PROTOCOL_NOT_SUPPORT_ERR, /* 0x208 */
	TRANS_TX_OPEN_REJCT_RATE_NOT_SUPPORT_ERR, /* 0x209 */
	TRANS_TX_OPEN_REJCT_BAD_DEST_ERR, /* 0x20a */
	TRANS_TX_OPEN_BREAK_RECEIVE_ERR, /* 0x20b */
	TRANS_TX_LOW_PHY_POWER_ERR, /* 0x20c */
	TRANS_TX_OPEN_REJCT_PATHWAY_BLOCKED_ERR, /* 0x20d */
	TRANS_TX_OPEN_TIMEOUT_ERR, /* 0x20e */
	TRANS_TX_OPEN_REJCT_NO_DEST_ERR, /* 0x20f */
	TRANS_TX_OPEN_RETRY_ERR, /* 0x210 */
	TRANS_TX_RSVD2_ERR, /* 0x211 */
	TRANS_TX_BREAK_TIMEOUT_ERR, /* 0x212 */
	TRANS_TX_BREAK_REQUEST_ERR, /* 0x213 */
	TRANS_TX_BREAK_RECEIVE_ERR, /* 0x214 */
	TRANS_TX_CLOSE_TIMEOUT_ERR, /* 0x215 */
	TRANS_TX_CLOSE_NORMAL_ERR, /* 0x216 */
	TRANS_TX_CLOSE_PHYRESET_ERR, /* 0x217 */
	TRANS_TX_WITH_CLOSE_DWS_TIMEOUT_ERR, /* 0x218 */
	TRANS_TX_WITH_CLOSE_COMINIT_ERR, /* 0x219 */
	TRANS_TX_NAK_RECEIVE_ERR, /* 0x21a */
	TRANS_TX_ACK_NAK_TIMEOUT_ERR, /* 0x21b */
	TRANS_TX_CREDIT_TIMEOUT_ERR, /* 0x21c */
	TRANS_TX_IPTT_CONFLICT_ERR, /* 0x21d */
	TRANS_TX_TXFRM_TYPE_ERR, /* 0x21e */
	TRANS_TX_TXSMP_LENGTH_ERR, /* 0x21f */

	/* trans rx */
	TRANS_RX_FRAME_CRC_ERR = TRANS_RX_FAIL_BASE, /* 0x300 */
	TRANS_RX_FRAME_DONE_ERR, /* 0x301 */
	TRANS_RX_FRAME_ERRPRM_ERR, /* 0x302 */
	TRANS_RX_FRAME_NO_CREDIT_ERR, /* 0x303 */
	TRANS_RX_RSVD0_ERR, /* 0x304 */
	TRANS_RX_FRAME_OVERRUN_ERR, /* 0x305 */
	TRANS_RX_FRAME_NO_EOF_ERR, /* 0x306 */
	TRANS_RX_LINK_BUF_OVERRUN_ERR, /* 0x307 */
	TRANS_RX_BREAK_TIMEOUT_ERR, /* 0x308 */
	TRANS_RX_BREAK_REQUEST_ERR, /* 0x309 */
	TRANS_RX_BREAK_RECEIVE_ERR, /* 0x30a */
	TRANS_RX_CLOSE_TIMEOUT_ERR, /* 0x30b */
	TRANS_RX_CLOSE_NORMAL_ERR, /* 0x30c */
	TRANS_RX_CLOSE_PHYRESET_ERR, /* 0x30d */
	TRANS_RX_WITH_CLOSE_DWS_TIMEOUT_ERR, /* 0x30e */
	TRANS_RX_WITH_CLOSE_COMINIT_ERR, /* 0x30f */
	TRANS_RX_DATA_LENGTH0_ERR, /* 0x310 */
	TRANS_RX_BAD_HASH_ERR, /* 0x311 */
	TRANS_RX_XRDY_ZERO_ERR, /* 0x312 */
	TRANS_RX_SSP_FRAME_LEN_ERR, /* 0x313 */
	TRANS_RX_TRANS_RX_RSVD1_ERR, /* 0x314 */
	TRANS_RX_NO_BALANCE_ERR, /* 0x315 */
	TRANS_RX_TRANS_RX_RSVD2_ERR, /* 0x316 */
	TRANS_RX_TRANS_RX_RSVD3_ERR, /* 0x317 */
	TRANS_RX_BAD_FRAME_TYPE_ERR, /* 0x318 */
	TRANS_RX_SMP_FRAME_LEN_ERR, /* 0x319 */
	TRANS_RX_SMP_RESP_TIMEOUT_ERR /* 0x31a */
};

#define HISI_SAS_PHY_MAX_INT_NR (HISI_SAS_PHY_INT_NR * HISI_SAS_MAX_PHYS)
#define HISI_SAS_CQ_MAX_INT_NR (HISI_SAS_MAX_QUEUES)
#define HISI_SAS_FATAL_INT_NR (2)

#define HISI_SAS_MAX_INT_NR \
	(HISI_SAS_PHY_MAX_INT_NR + HISI_SAS_CQ_MAX_INT_NR +\
	HISI_SAS_FATAL_INT_NR)

struct hisi_sas_cmd_hdr_dw0_v1_hw {
	u32 abort_flag:2;
	u32 rsvd0:2;
	u32 t10_flds_pres:1;
	u32 resp_report:1;
	u32 tlr_ctrl:2;
	u32 phy_id:8;
	u32 force_phy:1;
	u32 port:3;
	u32 sata_reg_set:7;
	u32 priority:1;
	u32 mode:1;
	u32 cmd:3;
} __packed;

struct hisi_sas_cmd_hdr_dw1_v1_hw {
	u32 port_multiplier:4;
	u32 bist_activate:1;
	u32 atapi:1;
	u32 first_part_dma:1;
	u32 reset:1;
	u32 pir_pres:1;
	u32 enable_tlr:1;
	u32 verify_dtl:1;
	u32 rsvd1:1;
	u32 ssp_pass_through:1;
	u32 ssp_frame_type:3;
	u32 device_id:16;
} __packed;

struct hisi_sas_cmd_hdr_dw2_v1_hw {
	u32 cmd_frame_len:9;
	u32 leave_affil_open:1;
	u32 rsvd2:5;
	u32 max_resp_frame_len:9;
	u32 sg_mode:1;
	u32 first_burst:1;
	u32 rsvd3:6;
} __packed;

/* Completion queue header */
struct hisi_sas_complete_hdr_v1_hw {
	u32 iptt:16;
	u32 rsvd0:1;
	u32 cmd_complt:1;
	u32 err_rcrd_xfrd:1;
	u32 rspns_xfrd:1;
	u32 attention:1;
	u32 cmd_rcvd:1;
	u32 slot_rst_cmplt:1;
	u32 rspns_good:1;
	u32 abort_status:3;
	u32 io_cfg_err:1;
	u32 rsvd1:4;
} __packed;

struct hisi_sas_itct_v1_hw {
	/* qw0 */
	u64 dev_type:2;
	u64 valid:1;
	u64 break_reply_ena:1;
	u64 awt_control:1;
	u64 max_conn_rate:4;
	u64 valid_link_number:4;
	u64 port_id:3;
	u64 smp_timeout:16;
	u64 max_burst_byte:32;

	/* qw1 */
	u64 sas_addr;

	/* qw2 */
	u64 IT_nexus_loss_time:16;
	u64 bus_inactive_time_limit:16;
	u64 max_conn_time_limit:16;
	u64 reject_open_time_limit:16;

	/* qw3 */
	u64 curr_pathway_blk_cnt:8;
	u64 curr_transmit_dir:2;
	u64 tx_pri:2;
	u64 rsvd0:3;
	u64 awt_cont:1;
	u64 curr_awt:16;
	u64 curr_IT_nexus_loss_val:16;
	u64 tlr_enable:1;
	u64 catap:4;
	u64 curr_ncq_tag:5;
	u64 cpn:4;
	u64 cb:1;
	u64 rsvd1:1;

	/* qw4 */
	u64 sata_active_reg:32;
	u64 rsvd2:9;
	u64 ata_status:8;
	u64 eb:1;
	u64 rpn:4;
	u64 rb:1;
	u64 sata_tx_ata_p:4;
	u64 tpn:4;
	u64 tb:1;

	/* sw5-12 */
	u16 ncq_tag[32];

	/* qw13 */
	u64 non_ncq_iptt:16;
	u64 rsvd3:48;

	/* qw14-15 */
	u64 rsvd4;
	u64 rsvd5;
} __packed;

#ifdef SAS_DIF
struct protect_iu_v1_hw {
	u32 t10_insert_en:1;
	u32 t10_rmv_en:1;
	u32 t10_rplc_en:1;
	u32 t10_chk_en:1;
	u32 chk_dsbl_md:1;
	u32 incr_lbrt:1;
	u32 incr_lbat:1;
	u32 prd_dat_incl_t10:1;
	u32 _r_a:8;
	u32 app_proc_mode:1;
	u32 dif_fmt:1;
	u32 usr_dif_blk_sz:2;
	u32 usr_dt_sz:12;

	u32 lbrt_chk_val;
	u32 lbrt_gen_val;
	u32 lbat_chk_val:16;
	u32 lbat_chk_mask:16;

	u32 lbat_gen_val:16;
	u32 t10_chk_msk:8;
	u32 t10_rplc_msk:8;

	u32 crc_gen_seed_val:16;
	u32 crc_chk_seed_val:16;
} __packed;
#endif

static inline u32 hisi_sas_read32(struct hisi_hba *hisi_hba, u32 off)
{
	void __iomem *regs = hisi_hba->regs + off;

	return readl(regs);
}

static inline void hisi_sas_write32(struct hisi_hba *hisi_hba,
				    u32 off, u32 val)
{
	void __iomem *regs = hisi_hba->regs + off;

	writel(val, regs);
}

static inline void hisi_sas_phy_write32(struct hisi_hba *hisi_hba,
					int phy_no, u32 off, u32 val)
{
	void __iomem *regs = hisi_hba->regs + (0x400 * phy_no) + off;

	writel(val, regs);
}

static inline u32 hisi_sas_phy_read32(struct hisi_hba *hisi_hba,
				      int phy_no, u32 off)
{
	void __iomem *regs = hisi_hba->regs + (0x400 * phy_no) + off;

	return readl(regs);
}

static void config_phy_link_param_v1_hw(struct hisi_hba *hisi_hba,
					int phy_no,
					enum sas_linkrate linkrate)
{
	u32 rate = hisi_sas_phy_read32(hisi_hba, phy_no, PROG_PHY_LINK_RATE);
	u32 pcn;

	rate &= ~PROG_PHY_LINK_RATE_MAX_MSK;
	switch (linkrate) {
	case SAS_LINK_RATE_6_0_GBPS:
		rate |= SAS_LINK_RATE_6_0_GBPS << PROG_PHY_LINK_RATE_MAX_OFF;
		pcn = 0x80a80000;
		break;
	case SAS_LINK_RATE_12_0_GBPS:
		rate |= SAS_LINK_RATE_12_0_GBPS << PROG_PHY_LINK_RATE_MAX_OFF;
		pcn = 0x80aa0001;
		break;

	default:
		dev_warn(hisi_hba->dev, "%s unsupported linkrate, %d",
			 __func__, linkrate);
		return;
	}

	rate &= ~PROG_PHY_LINK_RATE_OOB_MSK;
	rate |= SAS_LINK_RATE_1_5_GBPS << PROG_PHY_LINK_RATE_OOB_OFF;
	rate &= ~PROG_PHY_LINK_RATE_MIN_MSK;
	rate |= SAS_LINK_RATE_1_5_GBPS << PROG_PHY_LINK_RATE_MIN_OFF;
	hisi_sas_phy_write32(hisi_hba, phy_no, PROG_PHY_LINK_RATE, rate);
	hisi_sas_phy_write32(hisi_hba, phy_no, PHY_PCN, pcn);
}

static void config_phy_opt_mode_v1_hw(struct hisi_hba *hisi_hba, int phy_no)
{
	/* assume not optical cable for now */
	u32 cfg = hisi_sas_phy_read32(hisi_hba, phy_no, PHY_CFG);

	cfg &= ~PHY_CFG_DC_OPT_MSK;
	cfg |= 1 << PHY_CFG_DC_OPT_OFF;
	hisi_sas_phy_write32(hisi_hba, phy_no, PHY_CFG, cfg);
}

static void config_tx_tfe_autoneg_v1_hw(struct hisi_hba *hisi_hba, int phy_no)
{
	u32 cfg = hisi_sas_phy_read32(hisi_hba, phy_no, PHY_CONFIG2);

	cfg &= ~PHY_CONFIG2_FORCE_TXDEEMPH_MSK;
	hisi_sas_phy_write32(hisi_hba, phy_no, PHY_CONFIG2, cfg);
}

static void config_id_frame_v1_hw(struct hisi_hba *hisi_hba, int phy_no)
{
	struct sas_identify_frame identify_frame;
	u32 *identify_buffer;

	memset(&identify_frame, 0, sizeof(identify_frame));
	identify_frame.dev_type = SAS_END_DEVICE;
	identify_frame.frame_type = 0;
	identify_frame._un1 = 1;
	identify_frame.initiator_bits = SAS_PROTOCOL_ALL;
	identify_frame.target_bits = SAS_PROTOCOL_NONE;
	memcpy(&identify_frame._un4_11[0], hisi_hba->sas_addr, SAS_ADDR_SIZE);
	memcpy(&identify_frame.sas_addr[0], hisi_hba->sas_addr,	SAS_ADDR_SIZE);
	identify_frame.phy_id = phy_no;
	identify_buffer = (u32 *)(&identify_frame);

	hisi_sas_phy_write32(hisi_hba, phy_no, TX_ID_DWORD0,
			__swab32(identify_buffer[0]));
	hisi_sas_phy_write32(hisi_hba, phy_no, TX_ID_DWORD1,
			identify_buffer[2]);
	hisi_sas_phy_write32(hisi_hba, phy_no, TX_ID_DWORD2,
			identify_buffer[1]);
	hisi_sas_phy_write32(hisi_hba, phy_no, TX_ID_DWORD3,
			identify_buffer[4]);
	hisi_sas_phy_write32(hisi_hba, phy_no, TX_ID_DWORD4,
			identify_buffer[3]);
	hisi_sas_phy_write32(hisi_hba, phy_no, TX_ID_DWORD5,
			__swab32(identify_buffer[5]));
}

static void init_id_frame_v1_hw(struct hisi_hba *hisi_hba)
{
	int i;

	for (i = 0; i < hisi_hba->n_phy; i++)
		config_id_frame_v1_hw(hisi_hba, i);
}


void hisi_sas_setup_itct_v1_hw(struct hisi_hba *hisi_hba,
			       struct hisi_sas_device *device)
{
	struct domain_device *dev = device->sas_device;
	u64 device_id = device->device_id;
	struct hisi_sas_itct_v1_hw *itct =
		(struct hisi_sas_itct_v1_hw *)&hisi_hba->itct[device_id];

	memset(itct, 0, sizeof(*itct));

	/* qw0 */
	switch (dev->dev_type) {
	case SAS_END_DEVICE:
	case SAS_EDGE_EXPANDER_DEVICE:
	case SAS_FANOUT_EXPANDER_DEVICE:
		itct->dev_type = HISI_SAS_DEV_TYPE_SSP;
		break;
	default:
		dev_warn(hisi_hba->dev,
			 "%s unsupported dev type (%d)\n",
			 __func__, dev->dev_type);
	}

	itct->valid = 1;
	itct->break_reply_ena = 0;
	itct->awt_control = 1;
	itct->max_conn_rate = dev->max_linkrate; /* doublecheck, see enum sas_linkrate */
	itct->valid_link_number = 1;
	itct->port_id = dev->port->id;
	itct->smp_timeout = 0;
	itct->max_burst_byte = 0;

	/* qw1 */
	memcpy(&itct->sas_addr, dev->sas_addr, SAS_ADDR_SIZE);
	itct->sas_addr = __swab64(itct->sas_addr);

	/* qw2 */
	itct->IT_nexus_loss_time = 500;
	itct->bus_inactive_time_limit = 0xff00;
	itct->max_conn_time_limit = 0xff00;
	itct->reject_open_time_limit = 0xff00;
}

static int free_device_v1_hw(struct hisi_hba *hisi_hba,
			     struct hisi_sas_device *dev)
{
	u64 dev_id = dev->device_id;
	struct hisi_sas_itct_v1_hw *itct =
		(struct hisi_sas_itct_v1_hw *)&hisi_hba->itct[dev_id];
	u32 reg_val = hisi_sas_read32(hisi_hba, CFG_AGING_TIME);

	reg_val |= CFG_AGING_TIME_ITCT_REL_MSK;
	hisi_sas_write32(hisi_hba, CFG_AGING_TIME, reg_val);

	/* free itct */
	udelay(1);
	reg_val = hisi_sas_read32(hisi_hba, CFG_AGING_TIME);
	reg_val &= ~CFG_AGING_TIME_ITCT_REL_MSK;
	hisi_sas_write32(hisi_hba, CFG_AGING_TIME, reg_val);

	itct->valid = 0;

	memset(dev, 0, sizeof(*dev));
	dev->device_id = dev_id;
	dev->dev_type = SAS_PHY_UNUSED;
	dev->dev_status = HISI_SAS_DEV_NORMAL;

	return 0;
}

#ifdef SAS_12G
extern void SRE_CommonSerdesEnableCTLEDFE(unsigned int node,
		unsigned int macro,
		unsigned int lane,
		unsigned int ulDsCfg);

static void serdes_enable_ctledfe_v1_hw(struct hisi_hba *hisi_hba, int phy_no)
{
	int ds_api = 0;
	int hilink_id;
	int cpu_node = 0;

	if (hisi_hba->id == 0) {
		hilink_id = 2;
		ds_api = phy_no;
		SRE_CommonSerdesEnableCTLEDFE(cpu_node, hilink_id, ds_api, 9);
	} else if (hisi_hba->id == 1) {
		if (phy_no < 4)
			hilink_id = 5;
		else
			hilink_id = 6;
		ds_api = phy_no % 4;
		SRE_CommonSerdesEnableCTLEDFE(cpu_node, hilink_id, ds_api, 9);
	} else
		BUG();
}

void config_serdes_12G_timer_handler_v1_hw(unsigned long arg)
{
	struct hisi_sas_phy *phy = (struct hisi_sas_phy *)arg;
	struct hisi_hba *hisi_hba = phy->hisi_hba;
	struct asd_sas_phy *sas_phy = &phy->sas_phy;
	int phy_no = sas_phy->id;
	u32 val;

	/* End training */
	val = hisi_sas_phy_read32(hisi_hba, phy_no, PHY_CONFIG2);
	if (!(val & PHY_CONFIG2_TX_TRAIN_COMP_MSK)) {
		val |= PHY_CONFIG2_TX_TRAIN_COMP_MSK;
		hisi_sas_phy_write32(hisi_hba, phy_no, PHY_CONFIG2, val);
	}

	udelay(1);

	/* Clear training */
	val = hisi_sas_phy_read32(hisi_hba, phy_no, PHY_CONFIG2);
	if (val & PHY_CONFIG2_TX_TRAIN_COMP_MSK) {
		val &= ~PHY_CONFIG2_TX_TRAIN_COMP_MSK;
		hisi_sas_phy_write32(hisi_hba, phy_no, PHY_CONFIG2, val);
	}
}

void phy_rx_eye_diag_done_v1_hw(struct hisi_hba *hisi_hba, int phy_no)
{
	struct hisi_sas_phy *phy = &hisi_hba->phy[phy_no];
	struct timer_list *timer = &phy->timer;

	serdes_enable_ctledfe_v1_hw(hisi_hba, phy_no);

	if (!timer_pending(timer)) {
		init_timer(timer);
		timer->data = (unsigned long)phy;
		timer->expires = jiffies + msecs_to_jiffies(160);
		timer->function = config_serdes_12G_timer_handler_v1_hw;
		add_timer(timer);
	} else {
		mod_timer(timer, jiffies + msecs_to_jiffies(160));
	}
}

extern unsigned int SRE_CommonSerdesLaneReset(unsigned int node,
			unsigned int ulMacroId,
			unsigned int ulDsNum,
			unsigned int ulDsCfg);

static void serdes_lane_reset_v1_hw(struct hisi_hba *hisi_hba, int phy_no)
{
	int ds_api = 0;
	int hilink_id;
	int cpu_node = 0;

	if (hisi_hba->id == 0) {
		hilink_id = 2;
		ds_api = phy_no;
		SRE_CommonSerdesLaneReset(cpu_node, hilink_id, ds_api, 6);
	} else if (hisi_hba->id == 1) {
		if (phy_no < 4)
			hilink_id = 5;
		else
			hilink_id = 6;
		ds_api = phy_no % 4;
		SRE_CommonSerdesLaneReset(cpu_node, hilink_id, ds_api, 6);
	} else
		BUG();
}
#endif

static int reset_hw_v1_hw(struct hisi_hba *hisi_hba)
{
	int i;
	unsigned long end_time;

	for (i = 0; i < hisi_hba->n_phy; i++) {
		u32 phy_ctrl = hisi_sas_phy_read32(hisi_hba, i, PHY_CTRL);

		phy_ctrl |= PHY_CTRL_RESET_MSK;
		hisi_sas_phy_write32(hisi_hba, i, PHY_CTRL, phy_ctrl);
	}
	udelay(50);

	/* Ensure DMA tx & rx idle */
	for (i = 0; i < hisi_hba->n_phy; i++) {
		u32 dma_tx_status, dma_rx_status;

		end_time = jiffies + msecs_to_jiffies(1000);

		while (1) {
			dma_tx_status = hisi_sas_phy_read32(hisi_hba, i,
							    DMA_TX_STATUS);
			dma_rx_status = hisi_sas_phy_read32(hisi_hba, i,
							    DMA_RX_STATUS);

			if (!(dma_tx_status & DMA_TX_STATUS_BUSY_MSK) &&
				!(dma_rx_status & DMA_RX_STATUS_BUSY_MSK))
				break;

			msleep(20);
			if (time_after(jiffies, end_time))
				return -EIO;
		}
	}

	/* Ensure axi bus idle */
	end_time = jiffies + msecs_to_jiffies(1000);
	while (1) {
		u32 axi_status =
			hisi_sas_read32(hisi_hba, AXI_CFG);

		if (axi_status == 0)
			break;

		msleep(20);
		if (time_after(jiffies, end_time))
			return -EIO;
	}

	/* Apply reset */
	writel(CONTROLLER_RESET_VALUE,
	       hisi_hba->ctrl_regs + hisi_hba->reset_reg[0]);
	writel(CONTROLLER_RESET_VALUE,
	       hisi_hba->ctrl_regs + hisi_hba->reset_reg[1] + 4);
	mdelay(1);
	/* De-reset (offset is 4) */
	writel(CONTROLLER_RESET_VALUE,
	       hisi_hba->ctrl_regs + hisi_hba->reset_reg[0] + 4);
	writel(CONTROLLER_RESET_VALUE,
	       hisi_hba->ctrl_regs + hisi_hba->reset_reg[1]);

	return 0;
}

static void init_reg_v1_hw(struct hisi_hba *hisi_hba)
{
	int i;

	/* Global registers init*/
	hisi_sas_write32(hisi_hba,
		DLVRY_QUEUE_ENABLE,
		(u32)((1ULL << hisi_hba->queue_count) - 1));
	hisi_sas_write32(hisi_hba, HGC_TRANS_TASK_CNT_LIMIT, 0x11);
	hisi_sas_write32(hisi_hba, DEVICE_MSG_WORK_MODE, 0x1);
	hisi_sas_write32(hisi_hba, HGC_SAS_TXFAIL_RETRY_CTRL, 0x1ff);
	hisi_sas_write32(hisi_hba, HGC_ERR_STAT_EN, 0x401);
	hisi_sas_write32(hisi_hba, CFG_1US_TIMER_TRSH, 0x64);
	hisi_sas_write32(hisi_hba, HGC_GET_ITV_TIME, 0x1);
	hisi_sas_write32(hisi_hba, I_T_NEXUS_LOSS_TIME, 0x64);
	hisi_sas_write32(hisi_hba, BUS_INACTIVE_LIMIT_TIME, 0x2710);
	hisi_sas_write32(hisi_hba, REJECT_TO_OPEN_LIMIT_TIME, 0x1);
	hisi_sas_write32(hisi_hba, CFG_AGING_TIME, 0x7a12);
	hisi_sas_write32(hisi_hba, HGC_DFX_CFG2, 0x9c40);
	hisi_sas_write32(hisi_hba, FIS_LIST_BADDR_L, 0x2);
	hisi_sas_write32(hisi_hba, INT_COAL_EN, 0xc);
	hisi_sas_write32(hisi_hba, OQ_INT_COAL_TIME, 0x186a0);
	hisi_sas_write32(hisi_hba, OQ_INT_COAL_CNT, 1);
	hisi_sas_write32(hisi_hba, ENT_INT_COAL_TIME, 0x1);
	hisi_sas_write32(hisi_hba, ENT_INT_COAL_CNT, 0x1);
	hisi_sas_write32(hisi_hba, OQ_INT_SRC, 0xffffffff);
	hisi_sas_write32(hisi_hba, OQ_INT_SRC_MSK, 0);
	hisi_sas_write32(hisi_hba, ENT_INT_SRC1, 0xffffffff);
	hisi_sas_write32(hisi_hba, ENT_INT_SRC_MSK1, 0);
	hisi_sas_write32(hisi_hba, ENT_INT_SRC2, 0xffffffff);
	hisi_sas_write32(hisi_hba, ENT_INT_SRC_MSK2, 0);
	hisi_sas_write32(hisi_hba, SAS_ECC_INTR_MSK, 0);
	hisi_sas_write32(hisi_hba, AXI_AHB_CLK_CFG, 0x2);
	hisi_sas_write32(hisi_hba, CFG_SAS_CONFIG, 0x22000000);

	for (i = 0; i < hisi_hba->n_phy; i++) {
		/* see g_astPortRegConfig */
		#ifdef SAS_12G
		hisi_sas_phy_write32(hisi_hba, i, PROG_PHY_LINK_RATE, 0x0000088a);
		hisi_sas_phy_write32(hisi_hba, i, PHY_CONFIG2, 0x8085cc8c);
		hisi_sas_phy_write32(hisi_hba, i, PHY_RATE_NEGO, 0x415ee00);
		hisi_sas_phy_write32(hisi_hba, i, PHY_PCN, 0x80aa0001);
		#else
		hisi_sas_phy_write32(hisi_hba, i, PROG_PHY_LINK_RATE, 0x88a);
		hisi_sas_phy_write32(hisi_hba, i, PHY_CONFIG2, 0x7c080);
		hisi_sas_phy_write32(hisi_hba, i, PHY_RATE_NEGO, 0x415ee00);
		hisi_sas_phy_write32(hisi_hba, i, PHY_PCN, 0x80a80000);
		#endif

		hisi_sas_phy_write32(hisi_hba, i, SL_TOUT_CFG, 0x7d7d7d7d);
		hisi_sas_phy_write32(hisi_hba, i, DONE_RECEIVED_TIME, 0x0);
		hisi_sas_phy_write32(hisi_hba, i, RXOP_CHECK_CFG_H, 0x1000);
		hisi_sas_phy_write32(hisi_hba, i, DONE_RECEIVED_TIME, 0);
		hisi_sas_phy_write32(hisi_hba, i, CON_CFG_DRIVER, 0x13f0a);
		hisi_sas_phy_write32(hisi_hba, i, CHL_INT_COAL_EN, 3);
		hisi_sas_phy_write32(hisi_hba, i, DONE_RECEIVED_TIME, 8);
	}

	for (i = 0; i < hisi_hba->queue_count; i++) {
		/* Delivery queue */
		hisi_sas_write32(hisi_hba,
				 DLVRY_Q_0_BASE_ADDR_HI + (i * 0x14),
				 DMA_ADDR_HI(hisi_hba->cmd_hdr_dma[i]));

		hisi_sas_write32(hisi_hba,
				 DLVRY_Q_0_BASE_ADDR_LO + (i * 0x14),
				 DMA_ADDR_LO(hisi_hba->cmd_hdr_dma[i]));

		hisi_sas_write32(hisi_hba,
				 DLVRY_Q_0_DEPTH + (i * 0x14),
				 HISI_SAS_QUEUE_SLOTS);

		/* Completion queue */
		hisi_sas_write32(hisi_hba,
				 COMPL_Q_0_BASE_ADDR_HI + (i * 0x14),
				 DMA_ADDR_HI(hisi_hba->complete_hdr_dma[i]));

		hisi_sas_write32(hisi_hba,
				 COMPL_Q_0_BASE_ADDR_LO + (i * 0x14),
				 DMA_ADDR_LO(hisi_hba->complete_hdr_dma[i]));

		hisi_sas_write32(hisi_hba, COMPL_Q_0_DEPTH + (i * 0x14),
				 HISI_SAS_QUEUE_SLOTS);
	}

	/* itct */
	hisi_sas_write32(hisi_hba, ITCT_BASE_ADDR_LO,
			 DMA_ADDR_LO(hisi_hba->itct_dma));

	hisi_sas_write32(hisi_hba, ITCT_BASE_ADDR_HI,
			 DMA_ADDR_HI(hisi_hba->itct_dma));

	/* iost */
	hisi_sas_write32(hisi_hba, IOST_BASE_ADDR_LO,
			 DMA_ADDR_LO(hisi_hba->iost_dma));

	hisi_sas_write32(hisi_hba, IOST_BASE_ADDR_HI,
			 DMA_ADDR_HI(hisi_hba->iost_dma));

	/* breakpoint */
	hisi_sas_write32(hisi_hba, BROKEN_MSG_ADDR_LO,
			 DMA_ADDR_LO(hisi_hba->breakpoint_dma));

	hisi_sas_write32(hisi_hba, BROKEN_MSG_ADDR_HI,
			 DMA_ADDR_HI(hisi_hba->breakpoint_dma));
}

#ifdef SAS_12G
extern void hilink_reg_init(void);
extern void HRD_SubDsafInit(void);
extern void HRD_SubPcieInit(void);
#endif

static int hw_init_v1_hw(struct hisi_hba *hisi_hba)
{
	int rc;

	rc = reset_hw_v1_hw(hisi_hba);
	if (rc) {
		dev_err(hisi_hba->dev, "hisi_sas_reset_hw failed, rc=%d", rc);
		return rc;
	}

	msleep(100);
	init_reg_v1_hw(hisi_hba);

	init_id_frame_v1_hw(hisi_hba);

	#ifdef SAS_12G
	hilink_reg_init();
	HRD_SubDsafInit();
	HRD_SubPcieInit();
	#endif

	return 0;
}

static void enable_phy_v1_hw(struct hisi_hba *hisi_hba, int phy_no)
{
	u32 cfg = hisi_sas_phy_read32(hisi_hba, phy_no, PHY_CFG);

	cfg |= PHY_CFG_ENA_MSK;
	hisi_sas_phy_write32(hisi_hba, phy_no, PHY_CFG, cfg);
}

static void disable_phy_v1_hw(struct hisi_hba *hisi_hba, int phy_no)
{
	u32 cfg = hisi_sas_phy_read32(hisi_hba, phy_no, PHY_CFG);

	cfg &= ~PHY_CFG_ENA_MSK;
	hisi_sas_phy_write32(hisi_hba, phy_no, PHY_CFG, cfg);
}

static void start_phy_v1_hw(struct hisi_hba *hisi_hba, int phy_no)
{
	config_id_frame_v1_hw(hisi_hba, phy_no);
	#ifdef SAS_12G
	config_phy_link_param_v1_hw(hisi_hba, phy_no, SAS_LINK_RATE_12_0_GBPS);
	#else
	config_phy_link_param_v1_hw(hisi_hba, phy_no, SAS_LINK_RATE_6_0_GBPS);
	#endif
	config_phy_opt_mode_v1_hw(hisi_hba, phy_no);
	config_tx_tfe_autoneg_v1_hw(hisi_hba, phy_no);
	enable_phy_v1_hw(hisi_hba, phy_no);
}

static void stop_phy_v1_hw(struct hisi_hba *hisi_hba, int phy_no)
{
	disable_phy_v1_hw(hisi_hba, phy_no);
}

static void hard_phy_reset_v1_hw(struct hisi_hba *hisi_hba, int phy_no)
{
	struct sas_ha_struct *sha = hisi_hba->sas;

	stop_phy_v1_hw(hisi_hba, phy_no);
	sas_drain_work(sha);
	msleep(100);
	start_phy_v1_hw(hisi_hba, phy_no);
}

static void start_phys_v1_hw(unsigned long data)
{
	struct hisi_hba *hisi_hba = (struct hisi_hba *)data;
	int i;

	for (i = 0; i < hisi_hba->n_phy; i++) {
		hisi_sas_phy_write32(hisi_hba, i, CHL_INT2_MSK, 0x12a);
		start_phy_v1_hw(hisi_hba, i);
	}
}

static void phys_up_v1_hw(struct hisi_hba *hisi_hba)
{
	int i;

	for (i = 0; i < hisi_hba->n_phy; i++) {
		hisi_sas_phy_write32(hisi_hba, i, CHL_INT2_MSK, 0x6a);
		hisi_sas_phy_read32(hisi_hba, i, CHL_INT2_MSK);
	}
}

static int start_phy_layer_v1_hw(struct hisi_hba *hisi_hba)
{
	struct timer_list *timer = &hisi_hba->timer;

	timer->data = (unsigned long)hisi_hba;
	timer->expires = jiffies + msecs_to_jiffies(1000);
	timer->function = start_phys_v1_hw;

	add_timer(timer);

	return 0;
}

static int phys_init_v1_hw(struct hisi_hba *hisi_hba)
{
	phys_up_v1_hw(hisi_hba);
	start_phy_layer_v1_hw(hisi_hba);

	return 0;
}

static int get_free_slot_v1_hw(struct hisi_hba *hisi_hba, int *q, int *s)
{
	u32 r, w;
	int queue = smp_processor_id() % hisi_hba->queue_count;

	while (1) {
		w = hisi_sas_read32(hisi_hba,
				    DLVRY_Q_0_WR_PTR + (queue * 0x14));
		r = hisi_sas_read32(hisi_hba,
				    DLVRY_Q_0_RD_PTR + (queue * 0x14));

		if (r == w+1 % HISI_SAS_QUEUE_SLOTS) {
			dev_warn(hisi_hba->dev, "%s full queue%d r=%d w=%d\n",
				 __func__, queue, r, w);
			queue = (queue + 1) % hisi_hba->queue_count;
			continue;
		}
		break;
	}

	*q = queue;
	*s = w;

	return 0;
}

void start_delivery_v1_hw(struct hisi_hba *hisi_hba)
{
	int dlvry_queue = hisi_hba->slot_prep->dlvry_queue;
	u32 w = hisi_sas_read32(hisi_hba,
				DLVRY_Q_0_WR_PTR + (dlvry_queue * 0x14));

	hisi_sas_write32(hisi_hba,
			 DLVRY_Q_0_WR_PTR + (dlvry_queue * 0x14),
			 ++w % HISI_SAS_QUEUE_SLOTS);
}

static int prep_prd_sge_v1_hw(struct hisi_hba *hisi_hba,
				 struct hisi_sas_slot *slot,
				 struct hisi_sas_cmd_hdr *hdr,
				 struct scatterlist *scatter,
				 int n_elem)
{
	struct scatterlist *sg;
	int i;

	if (n_elem > HISI_SAS_SGE_PAGE_CNT) {
		dev_err(hisi_hba->dev,
			"%s n_elem(%d) > HISI_SAS_SGE_PAGE_CNT",
			__func__, n_elem);
		return -EINVAL;
	}

	slot->sge_page = dma_pool_alloc(hisi_hba->sge_page_pool, GFP_ATOMIC,
					&slot->sge_page_dma);
	if (!slot->sge_page)
		return -ENOMEM;

	for_each_sg(scatter, sg, n_elem, i) {
		struct hisi_sas_sge *entry = &slot->sge_page->sge[i];

		entry->addr_lo = DMA_ADDR_LO(sg_dma_address(sg));
		entry->addr_hi = DMA_ADDR_HI(sg_dma_address(sg));
		entry->page_ctrl_0 = entry->page_ctrl_1 = 0;
		entry->data_len = sg_dma_len(sg);
		entry->data_off = 0;
	}

	hdr->prd_table_addr_lo = DMA_ADDR_LO(slot->sge_page_dma);
	hdr->prd_table_addr_hi = DMA_ADDR_HI(slot->sge_page_dma);

	hdr->data_sg_len = n_elem;

	return 0;
}

#ifdef SAS_DIF
static int prep_prd_sge_dif_v1_hw(struct hisi_hba *hisi_hba,
				 struct hisi_sas_slot *slot,
				 struct hisi_sas_cmd_hdr *hdr,
				 struct scatterlist *scatter,
				 int n_elem)
{
	struct scatterlist *sg;
	int i;

	if (n_elem > HISI_SAS_SGE_PAGE_CNT) {
		dev_err(hisi_hba->dev, "%s n_elem(%d) > HISI_SAS_SGE_PAGE_CNT",
			__func__, n_elem);
		return -EINVAL;
	}

	slot->sge_dif_page = dma_pool_alloc(hisi_hba->sge_dif_page_pool,
			GFP_ATOMIC,
			&slot->sge_dif_page_dma);
	if (!slot->sge_dif_page)
		return -ENOMEM;

	hdr->double_mode = 1;

	for_each_sg(scatter, sg, n_elem, i) {
		struct hisi_sas_sge *entry = &slot->sge_dif_page->sge[i];

		entry->addr_lo = DMA_ADDR_LO(sg_dma_address(sg));
		entry->addr_hi = DMA_ADDR_HI(sg_dma_address(sg));
		entry->page_ctrl_0 = entry->page_ctrl_1 = 0;
		entry->data_len = sg_dma_len(sg);
		entry->data_off = 0;
	}

	hdr->dif_prd_table_addr_lo = DMA_ADDR_LO(slot->sge_dif_page_dma);
	hdr->dif_prd_table_addr_hi = DMA_ADDR_HI(slot->sge_dif_page_dma);

	hdr->dif_sg_len = n_elem;
	return 0;
}
#endif

static int prep_smp_v1_hw(struct hisi_hba *hisi_hba,
			struct hisi_sas_tei *tei)
{
	struct sas_task *task = tei->task;
	struct hisi_sas_cmd_hdr *hdr = tei->hdr;
	struct domain_device *dev = task->dev;
	struct hisi_sas_port *port = tei->port;
	struct scatterlist *sg_req, *sg_resp;
	struct hisi_sas_device *hisi_sas_dev = dev->lldd_dev;
	dma_addr_t req_dma_addr;
	unsigned int req_len, resp_len;
	int elem, rc;
	struct hisi_sas_slot *slot = tei->slot;
	struct hisi_sas_cmd_hdr_dw0_v1_hw *dw0 =
		(struct hisi_sas_cmd_hdr_dw0_v1_hw *)&hdr->dw0;
	struct hisi_sas_cmd_hdr_dw1_v1_hw *dw1 =
		(struct hisi_sas_cmd_hdr_dw1_v1_hw *)&hdr->dw1;
	struct hisi_sas_cmd_hdr_dw2_v1_hw *dw2 =
		(struct hisi_sas_cmd_hdr_dw2_v1_hw *)&hdr->dw2;

	/*
	* DMA-map SMP request, response buffers
	*/
	/* req */
	sg_req = &task->smp_task.smp_req; /* this is the request frame - see alloc_smp_req() */
	elem = dma_map_sg(hisi_hba->dev, sg_req, 1, DMA_TO_DEVICE); /* map to dma address */
	if (!elem)
		return -ENOMEM;
	req_len = sg_dma_len(sg_req);
	req_dma_addr = sg_dma_address(sg_req);

	/* resp */
	sg_resp = &task->smp_task.smp_resp; /* this is the response frame - see alloc_smp_resp() */
	elem = dma_map_sg(hisi_hba->dev, sg_resp, 1, DMA_FROM_DEVICE);
	if (!elem) {
		rc = -ENOMEM;
		goto err_out_req;
	}
	resp_len = sg_dma_len(sg_resp);
	if ((req_len & 0x3) || (resp_len & 0x3)) {
		rc = -EINVAL;
		goto err_out_resp;
	}

	/* create header */
	/* dw0 */
	dw0->abort_flag = 0; /* not sure */
	/* hdr->t10_flds_pres not set in Higgs_PrepareSMP */
	/* hdr->resp_report, ->tlr_ctrl for SSP */
	/* dw0->phy_id not set as we do not force phy */
	dw0->force_phy = 0; /* do not force ordering in phy */
	dw0->port = port->id; /* double-check */
	/* hdr->sata_reg_set not applicable to smp */
	dw0->priority = 1; /* high priority */
	dw0->mode = 1; /* ini mode */
	dw0->cmd = 2; /* smp */

	/* dw1 */
	/* hdr->port_multiplier, ->bist_active, ->atapi */
	/* ->first_party_dma, ->reset only applies to stp */
	/* hdr->pir_pres, ->enable_tlr, ->ssp_pass_through */
	/* ->spp_frame_type only applicable to ssp */

	dw1->device_id = hisi_sas_dev->device_id; /* map itct entry */

	/* dw2 */
	dw2->cmd_frame_len = (req_len - 4) / 4; /* do not include the crc */
	/* hdr->leave_affil_open only applicable to stp */
	dw2->max_resp_frame_len = HISI_SAS_MAX_SMP_RESP_SZ/4;
	/* hdr->sg_mode, ->first_burst not applicable to smp */

	/* dw3 */
	/* dw3 */
	hdr->iptt = tei->iptt;
	hdr->tptt = 0;

	/* hdr->data_transfer_len not applicable to smp */

	/* hdr->first_burst_num not applicable to smp */

	/* hdr->dif_prd_table_len, ->prd_table_len not applicable to smp */

	/* hdr->double_mode, ->abort_iptt not applicable to smp */

	/* j00310691 do not use slot->command_table */
	hdr->cmd_table_addr_lo = DMA_ADDR_LO(req_dma_addr);
	hdr->cmd_table_addr_hi = DMA_ADDR_HI(req_dma_addr);

	hdr->sts_buffer_addr_lo = DMA_ADDR_LO(slot->status_buffer_dma);
	hdr->sts_buffer_addr_hi = DMA_ADDR_HI(slot->status_buffer_dma);

	/* hdr->prd_table_addr_lo not applicable to smp */

	/* hdr->prd_table_addr_hi not applicable to smp */

	/* hdr->dif_prd_table_addr_lo not applicable to smp */

	/* hdr->dif_prd_table_addr_hi not applicable to smp */

	return 0;

err_out_resp:
	dma_unmap_sg(hisi_hba->dev, &tei->task->smp_task.smp_resp, 1,
		     DMA_FROM_DEVICE);
err_out_req:
	dma_unmap_sg(hisi_hba->dev, &tei->task->smp_task.smp_req, 1,
		     DMA_TO_DEVICE);
	return rc;
}

static int prep_ssp_v1_hw(struct hisi_hba *hisi_hba,
		struct hisi_sas_tei *tei, int is_tmf,
		struct hisi_sas_tmf_task *tmf)
{
	struct sas_task *task = tei->task;
	struct hisi_sas_cmd_hdr *hdr = tei->hdr;
	struct domain_device *dev = task->dev;
	struct hisi_sas_device *hisi_sas_dev = dev->lldd_dev;
	struct hisi_sas_port *port = tei->port;
	struct sas_ssp_task *ssp_task = &task->ssp_task;
	struct scsi_cmnd *scsi_cmnd = ssp_task->cmd;
	int has_data = 0, rc;
	struct hisi_sas_slot *slot = tei->slot;
	u8 *buf_cmd, fburst = 0;
	struct hisi_sas_cmd_hdr_dw0_v1_hw *dw0 =
		(struct hisi_sas_cmd_hdr_dw0_v1_hw *)&hdr->dw0;
	struct hisi_sas_cmd_hdr_dw1_v1_hw *dw1 =
		(struct hisi_sas_cmd_hdr_dw1_v1_hw *)&hdr->dw1;
	struct hisi_sas_cmd_hdr_dw2_v1_hw *dw2 =
		(struct hisi_sas_cmd_hdr_dw2_v1_hw *)&hdr->dw2;

	dw1->pir_pres = 0;
#ifdef SAS_DIF
	u8 prot_type = scsi_get_prot_type(scsi_cmnd);
	u8 prot_op = scsi_get_prot_op(scsi_cmnd);
	union hisi_sas_command_table *cmd =
		(union hisi_sas_command_table *) slot->command_table;
	struct protect_iu_v1_hw *prot =
		(struct protect_iu_v1_hw *)&cmd->ssp.u.prot;

	if (prot_type != SCSI_PROT_DIF_TYPE0) {
		/* enable dif */
		dw0->t10_flds_pres = 1;
		dw1->pir_pres = 1;

		prot->usr_dt_sz = scsi_prot_interval(scsi_cmnd) / 4;
		/* user dif block is 8B, if 0x1 is 64B */
		prot->usr_dif_blk_sz = 0;	/* 8B */
		/*0x0: CRC+APP+REF, 0x1:REF+APP+CRC*/
		prot->dif_fmt = 0;
		/*0x0: APP pass through, 0x1:*/
		prot->app_proc_mode = 0;
		/*0x1: PRD Table contains the T10*/
		prot->prd_dat_incl_t10 = 1;
		/*0x0: APP is constant*/
		prot->incr_lbat = 0;
		/*0x1: Ref Tag is increment*/
		prot->incr_lbrt = 1;
		/*0x1: if App and Ref Tag is 0xFF, then don't check*/
		prot->chk_dsbl_md = 0;

		prot->lbat_chk_mask = 0;
		prot->t10_rplc_msk = 0xff;
		prot->t10_chk_msk = 0xff;
		prot->crc_gen_seed_val = 0;
		prot->crc_chk_seed_val = 0;

		hdr->double_mode = 0;

		switch (prot_op) {
		case SCSI_PROT_READ_STRIP:
			prot->t10_chk_en = 0;
			prot->t10_rmv_en = 1;
			prot->t10_insert_en = 0;
			prot->t10_rplc_en = 0;
			break;
		case SCSI_PROT_WRITE_INSERT:
			prot->t10_chk_en = 0;
			prot->t10_rmv_en = 0;
			prot->t10_insert_en = 1;
			prot->t10_rplc_en = 0;
			prot->lbrt_gen_val = cpu_to_le32((uint32_t)
					(0xffffffff & scsi_get_lba(scsi_cmnd)));
			break;
		case SCSI_PROT_WRITE_PASS:
			prot->t10_chk_en = 1;
			prot->t10_chk_msk = 0xff;
			prot->lbrt_chk_val = cpu_to_le32((uint32_t)
					(0xffffffff & scsi_get_lba(scsi_cmnd)));
			break;
		case SCSI_PROT_READ_PASS:
			/* fixme not check crc & app & ref tag */
			prot->t10_chk_en = 1;
			prot->t10_chk_msk = 0x0;
			break;
		default:
			prot->t10_chk_en = 0;
			prot->t10_chk_msk = 0x0;
		}

		if (scsi_prot_sg_count(scsi_cmnd)) {
			int n_elem = dma_map_sg(hisi_hba->dev,
					scsi_prot_sglist(scsi_cmnd),
					scsi_prot_sg_count(scsi_cmnd),
					task->data_dir);
			if (!n_elem) {
				rc = -ENOMEM;
				return rc;
			}

			rc = prep_prd_sge_dif_v1_hw(hisi_hba, slot, hdr,
					scsi_prot_sglist(scsi_cmnd),
					n_elem);
			if (rc)
				return rc;
		}
	}
#endif
	/* create header */
	/* dw0 */
	/* hdr->abort_flag set in Higgs_PrepareBaseSSP */
	/* hdr->t10_flds_pres set in Higgs_PreparePrdSge */
	dw0->resp_report = 1;
	dw0->tlr_ctrl = 0x2; /* Do not enable */
	/* dw0->phy_id not set as we do not force phy */
	dw0->force_phy = 0; /* do not force ordering in phy */
	dw0->port = port->id; /* double-check */
	/* hdr->sata_reg_set not applicable to smp */
	if (is_tmf)
		dw0->priority = 1;
	else
		dw0->priority = 0; /* ordinary priority */
	dw0->mode = 1; /* ini mode */
	dw0->cmd = 1; /* ssp */

	/* dw1 */
	/* hdr->port_multiplier, ->bist_active, ->atapi */
	/* ->first_party_dma, ->reset only applies to stp */
	/* hdr->pir_pres set in Higgs_PreparePrdSge */
	/* but see Higgs_PreparePrdSge */
	dw1->enable_tlr = 0;
	dw1->verify_dtl = 1;
	dw1->ssp_pass_through = 0; /* see Higgs_DQGlobalConfig */
	if (is_tmf) {
		dw1->ssp_frame_type = 3;
	} else {
		switch (scsi_cmnd->sc_data_direction) {
		case DMA_TO_DEVICE:
			dw1->ssp_frame_type = 2;
			has_data = 1;
			break;
		case DMA_FROM_DEVICE:
			dw1->ssp_frame_type = 1;
			has_data = 1;
			break;
		default:
			dw1->ssp_frame_type = 0;
		}
		hdr->data_transfer_len = scsi_transfer_length(scsi_cmnd);
	}

	dw1->device_id = hisi_sas_dev->device_id; /* map itct entry */

	/* dw2 */
	if (is_tmf) {
		dw2->cmd_frame_len = (sizeof(struct ssp_task_iu) +
			sizeof(struct ssp_frame_hdr) + 3) / 4;
	} else {
		dw2->cmd_frame_len = (sizeof(struct ssp_command_iu) +
			sizeof(struct ssp_frame_hdr) + 3) / 4;
	}
	/* hdr->leave_affil_open only applicable to stp */
	dw2->max_resp_frame_len = HISI_SAS_MAX_SSP_RESP_SZ/4;
	dw2->sg_mode = 0; /* see Higgs_DQGlobalConfig */
	dw2->first_burst = 0;

	/* dw3 */
	hdr->iptt = tei->iptt;
	hdr->tptt = 0;

	if (has_data) {
		rc = prep_prd_sge_v1_hw(hisi_hba, slot, hdr, task->scatter,
					tei->n_elem);
		if (rc)
			return rc;
	}

	/* dw4 */
	/*hdr->data_transfer_len = scsi_transfer_length(scsi_cmnd);*/
	/* dw5 */
	/* hdr->first_burst_num not set in Higgs code */

	/* dw6 */
	/* hdr->data_sg_len set in hisi_sas_prep_prd_sge */
	/* hdr->dif_sg_len not set in Higgs code */

	/* dw7 */
	/* hdr->double_mode is set only for DIF todo */
	/* hdr->abort_iptt set in Higgs_PrepareAbort */

	/* dw8,9 */
	/* j00310691 reference driver sets in Higgs_SendCommandHw */
	hdr->cmd_table_addr_lo = DMA_ADDR_LO(slot->command_table_dma);
	hdr->cmd_table_addr_hi = DMA_ADDR_HI(slot->command_table_dma);

	/* dw9,10 */
	/* j00310691 reference driver sets in Higgs_SendCommandHw */
	hdr->sts_buffer_addr_lo = DMA_ADDR_LO(slot->status_buffer_dma);
	hdr->sts_buffer_addr_hi = DMA_ADDR_HI(slot->status_buffer_dma);

	/* dw11,12 */
	/* hdr->prd_table_addr_lo, _hi set in hisi_sas_prep_prd_sge */

	/* hdr->dif_prd_table_addr_lo, _hi not set in Higgs code */
	buf_cmd = (u8 *)slot->command_table + sizeof(struct ssp_frame_hdr);
	/* fill in IU for TASK and Command Frame */
	if (task->ssp_task.enable_first_burst) {
		fburst = (1 << 7);
		dw2->first_burst = 1;
	}

	memcpy(buf_cmd, &task->ssp_task.LUN, 8);
	if (!is_tmf) {
		buf_cmd[9] = fburst | task->ssp_task.task_attr |
				(task->ssp_task.task_prio << 3);
		memcpy(buf_cmd + 12, task->ssp_task.cmd->cmnd,
				task->ssp_task.cmd->cmd_len);
	} else {
		buf_cmd[10] = tmf->tmf;
		switch (tmf->tmf) {
		case TMF_ABORT_TASK:
		case TMF_QUERY_TASK:
			buf_cmd[12] =
				(tmf->tag_of_task_to_be_managed >> 8) & 0xff;
			buf_cmd[13] =
				tmf->tag_of_task_to_be_managed & 0xff;
			break;
		default:
			break;
		}
	}

	return 0;
}

/* by default, task resp is complete */
static void slot_err_v1_hw(struct hisi_hba *hisi_hba,
			   struct sas_task *task,
			   struct hisi_sas_slot *slot)
{
	struct task_status_struct *tstat = &task->task_status;
	struct hisi_sas_err_record *err_record = slot->status_buffer;
	int rc = -1;

	switch (task->task_proto) {
	case SAS_PROTOCOL_SSP:
	{
		int error = -1;

		if (err_record->dma_tx_err_type) {
			/* dma tx err */
			error = ffs(err_record->dma_tx_err_type)
				- 1 + DMA_TX_ERR_BASE;
		} else if (err_record->dma_rx_err_type) {
			/* dma rx err */
			error = ffs(err_record->dma_rx_err_type)
				- 1 + DMA_RX_ERR_BASE;
		} else if (err_record->trans_tx_fail_type) {
			/* trans tx err */
			error = ffs(err_record->trans_tx_fail_type)
				- 1 + TRANS_TX_FAIL_BASE;
		} else if (err_record->trans_rx_fail_type) {
			/* trans rx err */
			error = ffs(err_record->trans_rx_fail_type)
				- 1 + TRANS_RX_FAIL_BASE;
		}

		switch (error) {
		case DMA_TX_DATA_UNDERFLOW_ERR:
		case DMA_RX_DATA_UNDERFLOW_ERR:
		{
			tstat->residual = 0;
			tstat->stat = SAS_DATA_UNDERRUN;

			break;
		}
		case DMA_TX_DATA_SGL_OVERFLOW_ERR:
		case DMA_TX_DIF_SGL_OVERFLOW_ERR:
		case DMA_TX_XFER_RDY_LENGTH_OVERFLOW_ERR:
		case DMA_RX_DATA_OVERFLOW_ERR:
		case TRANS_RX_FRAME_OVERRUN_ERR:
		case TRANS_RX_LINK_BUF_OVERRUN_ERR:
		{
			tstat->stat = SAS_DATA_OVERRUN;
			tstat->residual = 0;
			break;
		}
		case TRANS_TX_PHY_NOT_ENABLE_ERR:
		{
			tstat->stat = SAS_PHY_DOWN;
			tstat->resp = SAS_TASK_UNDELIVERED;
			break;
		}
		case TRANS_TX_OPEN_REJCT_WRONG_DEST_ERR:
		case TRANS_TX_OPEN_REJCT_ZONE_VIOLATION_ERR:
		case TRANS_TX_OPEN_REJCT_BY_OTHER_ERR:
		case TRANS_TX_OPEN_REJCT_AIP_TIMEOUT_ERR:
		case TRANS_TX_OPEN_REJCT_STP_BUSY_ERR:
		case TRANS_TX_OPEN_REJCT_PROTOCOL_NOT_SUPPORT_ERR:
		case TRANS_TX_OPEN_REJCT_RATE_NOT_SUPPORT_ERR:
		case TRANS_TX_OPEN_REJCT_BAD_DEST_ERR:
		case TRANS_TX_OPEN_BREAK_RECEIVE_ERR:
		case TRANS_TX_OPEN_REJCT_PATHWAY_BLOCKED_ERR:
		case TRANS_TX_OPEN_REJCT_NO_DEST_ERR:
		case TRANS_TX_OPEN_RETRY_ERR:
		{
			tstat->stat = SAS_OPEN_REJECT;
			tstat->open_rej_reason = SAS_OREJ_UNKNOWN;
			break;
		}
		case TRANS_TX_OPEN_TIMEOUT_ERR:
		{
			tstat->stat = SAS_OPEN_TO;
			break;
		}
		case TRANS_TX_NAK_RECEIVE_ERR:
		case TRANS_TX_ACK_NAK_TIMEOUT_ERR:
		{
			tstat->stat = SAS_NAK_R_ERR;
			break;
		}
		case TRANS_TX_CREDIT_TIMEOUT_ERR:
		case TRANS_TX_CLOSE_NORMAL_ERR:
		{
			task->task_state_flags &= ~SAS_TASK_STATE_DONE;
			rc = hisi_sas_handle_event(hisi_hba, task,
					SAS_ABORT_AND_RETRY);
			if (!rc) {
				tstat->stat = HISI_INTERNAL_EH_STAT;
				return;
			}
			break;
		}
		default:
		{
			tstat->stat = SAM_STAT_CHECK_CONDITION;
			break;
		}
		}
	}
		break;
	case SAS_PROTOCOL_SMP:
		tstat->stat = SAM_STAT_CHECK_CONDITION;
		break;

	case SAS_PROTOCOL_SATA:
	case SAS_PROTOCOL_STP:
	case SAS_PROTOCOL_SATA | SAS_PROTOCOL_STP:
	{
		dev_err(hisi_hba->dev,
			"%s SATA/STP not supported", __func__);
	}
		break;
	default:
		break;
	}

}

static int slot_complete_v1_hw(struct hisi_hba *hisi_hba,
			       struct hisi_sas_slot *slot,
			       int abort)
{
	struct sas_task *task = slot->task;
	struct hisi_sas_device *hisi_sas_dev;
	struct task_status_struct *tstat;
	struct domain_device *dev;
	enum exec_status sts;
	struct hisi_sas_complete_hdr_v1_hw *complete_queue =
			(struct hisi_sas_complete_hdr_v1_hw *)
			hisi_hba->complete_hdr[slot->cmplt_queue];
	struct hisi_sas_complete_hdr_v1_hw *complete_hdr;

	complete_hdr = &complete_queue[slot->cmplt_queue_slot];

	if (unlikely(!task || !task->lldd_task || !task->dev))
		return -1;

	tstat = &task->task_status;
	dev = task->dev;
	hisi_sas_dev = dev->lldd_dev;

	task->task_state_flags &=
		~(SAS_TASK_STATE_PENDING | SAS_TASK_AT_INITIATOR);
	task->task_state_flags |= SAS_TASK_STATE_DONE;

	memset(tstat, 0, sizeof(*tstat));
	tstat->resp = SAS_TASK_COMPLETE;

	/* when no device attaching, go ahead and complete by error handling */
	if (unlikely(!hisi_sas_dev || abort)) {
		if (!hisi_sas_dev)
			dev_dbg(hisi_hba->dev, "%s port has not device.\n",
				__func__);
		tstat->stat = SAS_PHY_DOWN;
		goto out;
	}

	if (complete_hdr->io_cfg_err) {
		u32 info_reg = hisi_sas_read32(hisi_hba, HGC_INVLD_DQE_INFO);

		if (info_reg & HGC_INVLD_DQE_INFO_DQ_MSK)
			dev_err(hisi_hba->dev,
				"%s slot [%d:%d] has dq IPTT err",
				__func__,
				slot->cmplt_queue, slot->cmplt_queue_slot);

		if (info_reg & HGC_INVLD_DQE_INFO_TYPE_MSK)
			dev_err(hisi_hba->dev,
				"%s slot [%d:%d] has dq type err",
				__func__,
				slot->cmplt_queue, slot->cmplt_queue_slot);

		if (info_reg & HGC_INVLD_DQE_INFO_FORCE_MSK)
			dev_err(hisi_hba->dev,
				"%s slot [%d:%d] has dq force phy err",
				__func__,
				slot->cmplt_queue, slot->cmplt_queue_slot);

		if (info_reg & HGC_INVLD_DQE_INFO_PHY_MSK)
			dev_err(hisi_hba->dev,
				"%s slot [%d:%d] has dq phy id err",
				__func__,
				slot->cmplt_queue, slot->cmplt_queue_slot);

		if (info_reg & HGC_INVLD_DQE_INFO_ABORT_MSK)
			dev_err(hisi_hba->dev,
				"%s slot [%d:%d] has dq abort flag err",
				__func__,
				slot->cmplt_queue, slot->cmplt_queue_slot);

		if (info_reg & HGC_INVLD_DQE_INFO_IPTT_OF_MSK)
			dev_err(hisi_hba->dev,
				"%s slot [%d:%d] has dq IPTT or ICT err",
				__func__,
				slot->cmplt_queue, slot->cmplt_queue_slot);

		if (info_reg & HGC_INVLD_DQE_INFO_SSP_ERR_MSK)
			dev_err(hisi_hba->dev,
				"%s slot [%d:%d] has dq SSP frame type err",
				__func__,
				slot->cmplt_queue, slot->cmplt_queue_slot);

		if (info_reg & HGC_INVLD_DQE_INFO_OFL_MSK)
			dev_err(hisi_hba->dev,
				"%s slot [%d:%d] has dq order frame len err",
				__func__,
				slot->cmplt_queue, slot->cmplt_queue_slot);

		tstat->resp = SAS_TASK_UNDELIVERED;
		tstat->stat = SAS_OPEN_REJECT;
		tstat->open_rej_reason = SAS_OREJ_UNKNOWN;
		goto out;
	}

	if (complete_hdr->err_rcrd_xfrd && !complete_hdr->rspns_xfrd) {
		slot_err_v1_hw(hisi_hba, task, slot);

		if (tstat->stat == HISI_INTERNAL_EH_STAT)
			return 0;

		goto out;
	}

	switch (task->task_proto) {
	case SAS_PROTOCOL_SSP:
	{
		/* j00310691 for SMP, IU contains just the SSP IU */
		struct ssp_response_iu *iu = slot->status_buffer +
			sizeof(struct hisi_sas_err_record);
		sas_ssp_task_response(hisi_hba->dev, task, iu);
		break;
	}
	case SAS_PROTOCOL_SMP:
	{
		void *to;
		struct scatterlist *sg_resp = &task->smp_task.smp_resp;

		tstat->stat = SAM_STAT_GOOD;
		to = kmap_atomic(sg_page(sg_resp));
		memcpy(to + sg_resp->offset,
		       slot->status_buffer +
		       sizeof(struct hisi_sas_err_record),
		       sg_dma_len(sg_resp));
		kunmap_atomic(to);
		break;
	}
	case SAS_PROTOCOL_SATA:
	case SAS_PROTOCOL_STP:
	case SAS_PROTOCOL_SATA | SAS_PROTOCOL_STP:
		dev_err(hisi_hba->dev, "%s SATA/STP not supported", __func__);
		break;

	default:
		tstat->stat = SAM_STAT_CHECK_CONDITION;
		break;
	}

	if (!slot->port->port_attached) {
		dev_err(hisi_hba->dev, "%s port %d has removed\n",
			__func__, slot->port->sas_port.id);
		tstat->stat = SAS_PHY_DOWN;
	}

out:
	if (hisi_sas_dev && hisi_sas_dev->running_req)
		hisi_sas_dev->running_req--;

	hisi_sas_slot_task_free(hisi_hba, task, slot);
	sts = tstat->stat;

	if (task->task_done)
		task->task_done(task);

	return sts;
}

static irqreturn_t int_phyup_v1_hw(int phy_no, void *p)
{
	struct hisi_hba *hisi_hba = p;
	u32 irq_value, context, port_id, link_rate;
	int i;
	struct hisi_sas_phy *phy = &hisi_hba->phy[phy_no];
	struct asd_sas_phy *sas_phy = &phy->sas_phy;
	u32 *frame_rcvd = (u32 *)sas_phy->frame_rcvd;
	struct sas_identify_frame *id = (struct sas_identify_frame *)frame_rcvd;
	irqreturn_t res = IRQ_HANDLED;

	irq_value = hisi_sas_phy_read32(hisi_hba, phy_no, CHL_INT2);

	if (!(irq_value & CHL_INT2_SL_PHY_ENA_MSK)) {
		dev_dbg(hisi_hba->dev, "%s irq_value = %x not set enable bit\n",
			__func__, irq_value);
		res = IRQ_NONE;
		goto end;
	}

	context = hisi_sas_read32(hisi_hba, PHY_CONTEXT);
	if (context & 1 << phy_no) {
		dev_err(hisi_hba->dev, "%s phy%d SATA attached equipment\n",
			__func__, phy_no);
		goto end;
	}

	port_id = (hisi_sas_read32(hisi_hba, PHY_PORT_NUM_MA) >> (4 * phy_no))
		  & 0xf;
	if (port_id == 0xf) {
		dev_err(hisi_hba->dev, "%s phy%d invalid portid\n",
			__func__, phy_no);
		res = IRQ_NONE;
		goto end;
	}

	for (i = 0; i < 6; i++) {
		u32 idaf = hisi_sas_phy_read32(hisi_hba, phy_no,
					RX_IDAF_DWORD0 + (i * 4));
		frame_rcvd[i] = __swab32(idaf);
	}

	if (id->dev_type == SAS_END_DEVICE) {
		u32 sl_control;

		sl_control = hisi_sas_phy_read32(hisi_hba, phy_no, SL_CONTROL);
		sl_control |= SL_CONTROL_NOTIFY_EN_MSK;
		hisi_sas_phy_write32(hisi_hba, phy_no, SL_CONTROL, sl_control);
		mdelay(1);
		sl_control = hisi_sas_phy_read32(hisi_hba, phy_no, SL_CONTROL);
		sl_control &= ~SL_CONTROL_NOTIFY_EN_MSK;
		hisi_sas_phy_write32(hisi_hba, phy_no, SL_CONTROL, sl_control);
	}

	/* Get the linkrate */
	link_rate = hisi_sas_read32(hisi_hba, PHY_CONN_RATE);
	link_rate = (link_rate >> (phy_no * 4)) & 0xf;
	sas_phy->linkrate = link_rate;
	sas_phy->oob_mode = SAS_OOB_MODE;
	memcpy(sas_phy->attached_sas_addr,
		&id->sas_addr, SAS_ADDR_SIZE);
	dev_info(hisi_hba->dev, "phyup phy%d id=%d link_rate=%d\n",
				phy_no,
				hisi_hba->id,
				link_rate);
	phy->port_id = port_id;
	phy->phy_type &= ~(PORT_TYPE_SAS | PORT_TYPE_SATA);
	phy->phy_type |= PORT_TYPE_SAS;
	phy->phy_attached = 1;
	phy->identify.device_type = id->dev_type;
	phy->frame_rcvd_size =	sizeof(struct sas_identify_frame);
	if (phy->identify.device_type == SAS_END_DEVICE)
		phy->identify.target_port_protocols =
			SAS_PROTOCOL_SSP;
	else if (phy->identify.device_type != SAS_PHY_UNUSED)
		phy->identify.target_port_protocols =
			SAS_PROTOCOL_SMP;

	hisi_sas_bytes_dmaed(hisi_hba, phy_no);

end:
	hisi_sas_phy_write32(hisi_hba, phy_no, CHL_INT2,
			CHL_INT2_SL_PHY_ENA_MSK);

	if (irq_value & CHL_INT2_SL_PHY_ENA_MSK) {
		/* Higgs_BypassChipBugUnmaskAbnormalIntr */
		u32 chl_int0 = hisi_sas_phy_read32(hisi_hba, phy_no, CHL_INT0);

		chl_int0 &= ~CHL_INT0_PHYCTRL_NOTRDY_MSK;
		hisi_sas_phy_write32(hisi_hba, phy_no, CHL_INT0, chl_int0);
		hisi_sas_phy_write32(hisi_hba, phy_no, CHL_INT0_MSK, 0x3ce3ee);
	}

	return res;
}

static irqreturn_t int_bcast_v1_hw(int phy_no, void *p)
{
	struct hisi_hba *hisi_hba = p;
	u32 irq_value;
	irqreturn_t res = IRQ_HANDLED;
	struct hisi_sas_phy *phy = &hisi_hba->phy[phy_no];
	struct asd_sas_phy *sas_phy = &phy->sas_phy;
	struct sas_ha_struct *sas_ha = hisi_hba->sas;

	dev_err(hisi_hba->dev, "%s\n", __func__);
	irq_value = hisi_sas_phy_read32(hisi_hba, phy_no, CHL_INT2);

	if (!(irq_value & CHL_INT2_SL_RX_BC_ACK_MSK)) {
		dev_err(hisi_hba->dev, "%s irq_value = %x not set enable bit",
			__func__, irq_value);
		res = IRQ_NONE;
		goto end;
	}

	sas_ha->notify_port_event(sas_phy, PORTE_BROADCAST_RCVD);

end:
	hisi_sas_phy_write32(hisi_hba, phy_no, CHL_INT2,
			CHL_INT2_SL_RX_BC_ACK_MSK);

	return res;
}

static irqreturn_t int_abnormal_v1_hw(int phy_no, void *p)
{
	struct hisi_hba *hisi_hba = p;
	u32 irq_value, irq_mask_old;

	/* mask_int0 */
	irq_mask_old = hisi_sas_phy_read32(hisi_hba, phy_no, CHL_INT0_MSK);
	hisi_sas_phy_write32(hisi_hba, phy_no, CHL_INT0_MSK, 0x3fffff);

	/* read int0 */
	irq_value = hisi_sas_phy_read32(hisi_hba, phy_no, CHL_INT0);

	if (irq_value & CHL_INT0_PHYCTRL_NOTRDY_MSK) {
		u32 phy_state = hisi_sas_read32(hisi_hba, PHY_STATE);
		#ifdef SAS_12G
		struct hisi_sas_phy *phy = &hisi_hba->phy[phy_no];
		struct timer_list *timer = &phy->timer;
		#endif

		hisi_sas_phy_down(hisi_hba,
			phy_no,
			(phy_state & 1 << phy_no) ? 1 : 0);
		#ifdef SAS_12G
		serdes_lane_reset_v1_hw(hisi_hba, phy_no);
		phy->eye_diag_done = 0;
		if (timer_pending(timer))
			del_timer(timer);
		#endif
	}

	if (irq_value & CHL_INT0_ID_TIMEOUT_MSK)
		dev_dbg(hisi_hba->dev,
			"ID_TIMEOUT phy%d identify timeout\n",
			phy_no);

	if (irq_value & CHL_INT0_DWS_LOST_MSK)
		dev_dbg(hisi_hba->dev,
			"DWS_LOST phy%d dws lost\n",
			phy_no);

	if (irq_value & CHL_INT0_SN_FAIL_NGR_MSK)
		dev_dbg(hisi_hba->dev,
			"SN_FAIL_NGR phy%d sn fail ngr\n",
			phy_no);

	if (irq_value & CHL_INT0_SL_IDAF_FAIL_MSK ||
		irq_value & CHL_INT0_SL_OPAF_FAIL_MSK)
		dev_dbg(hisi_hba->dev,
			"SL_IDAF/OPAF_FAIL phy%d check address frame err\n",
			phy_no);

	if (irq_value & CHL_INT0_SL_PS_FAIL_OFF)
		dev_dbg(hisi_hba->dev,
			"SL_PS_FAIL phy%d ps req fail\n",
			phy_no);

	/* write to zero */
	hisi_sas_phy_write32(hisi_hba, phy_no, CHL_INT0, irq_value);

	if (irq_value & CHL_INT0_PHYCTRL_NOTRDY_MSK)
		hisi_sas_phy_write32(hisi_hba, phy_no, CHL_INT0_MSK,
				0x3fffff & ~CHL_INT0_MSK_PHYCTRL_NOTRDY_MSK);
	else
		hisi_sas_phy_write32(hisi_hba, phy_no, CHL_INT0_MSK,
				irq_mask_old);

	return IRQ_HANDLED;
}

static irqreturn_t int_int1_v1_hw(int phy_no, void *p)
{
	struct hisi_hba *hisi_hba = p;
	u32 irq_value1, irq_value2;

	irq_value1 = hisi_sas_phy_read32(hisi_hba, phy_no, CHL_INT1);
	irq_value2 = hisi_sas_phy_read32(hisi_hba, phy_no, CHL_INT2);

	if (irq_value2 & CHL_INT2_RXEYEDIAG_DONE_MSK) {
		#ifdef SAS_12G
		struct hisi_sas_phy *phy = &hisi_hba->phy[phy_no];

		if (!phy->eye_diag_done) {
			phy_rx_eye_diag_done_v1_hw(hisi_hba, phy_no);
			phy->eye_diag_done = 1;
		}
		#endif
	}

	hisi_sas_phy_write32(hisi_hba, phy_no, CHL_INT1, irq_value1);
	hisi_sas_phy_write32(hisi_hba, phy_no, CHL_INT2, irq_value2);

	return IRQ_HANDLED;
}

/* Interrupts */
static irqreturn_t cq_interrupt_v1_hw(const int queue, void *p)
{
	struct hisi_hba *hisi_hba = p;
	struct hisi_sas_slot *slot;
	struct hisi_sas_complete_hdr_v1_hw *complete_queue =
			(struct hisi_sas_complete_hdr_v1_hw *)
			hisi_hba->complete_hdr[queue];
	u32 irq_value;
	u32 rd_point, wr_point;

	irq_value = hisi_sas_read32(hisi_hba, OQ_INT_SRC);

	hisi_sas_write32(hisi_hba, OQ_INT_SRC, 1 << queue);

	rd_point = hisi_sas_read32(hisi_hba,
			COMPL_Q_0_RD_PTR + (0x14 * queue));
	wr_point = hisi_sas_read32(hisi_hba,
			COMPL_Q_0_WR_PTR + (0x14 * queue));

	while (rd_point != wr_point) {
		struct hisi_sas_complete_hdr_v1_hw *complete_hdr;
		int iptt, slot_idx;

		complete_hdr = &complete_queue[rd_point];
		iptt = complete_hdr->iptt;
		slot_idx = iptt;
		slot = &hisi_hba->slot_info[slot_idx];



		slot->cmplt_queue_slot = rd_point;
		slot->cmplt_queue = queue;
		slot_complete_v1_hw(hisi_hba, slot, 0);

		if (++rd_point >= HISI_SAS_QUEUE_SLOTS)
			rd_point = 0;
	}

	/* update rd_point */
	hisi_sas_write32(hisi_hba, COMPL_Q_0_RD_PTR + (0x14 * queue), rd_point);
	return IRQ_HANDLED;
}

static irqreturn_t fatal_ecc_int_v1_hw(int irq, void *p)
{
	struct hisi_hba *hisi_hba = p;
	u32 ecc_int = hisi_sas_read32(hisi_hba, SAS_ECC_INTR);

	if (ecc_int & SAS_ECC_INTR_DQ_ECC1B_MSK) {
		u32 ecc_err = hisi_sas_read32(hisi_hba, HGC_ECC_ERR);

		panic("Fatal DQ 1b ECC interrupt on controller %d (0x%x)\n",
			hisi_hba->id, ecc_err);
	}

	if (ecc_int & SAS_ECC_INTR_DQ_ECCBAD_MSK) {
		u32 addr = (hisi_sas_read32(hisi_hba, HGC_DQ_ECC_ADDR) &
				HGC_DQ_ECC_ADDR_BAD_MSK) >>
				HGC_DQ_ECC_ADDR_BAD_OFF;

		panic("Fatal DQ RAM ECC interrupt on controller %d @ 0x%08x\n",
			hisi_hba->id, addr);
	}

	if (ecc_int & SAS_ECC_INTR_IOST_ECC1B_MSK) {
		u32 ecc_err = hisi_sas_read32(hisi_hba, HGC_ECC_ERR);

		panic("Fatal IOST 1b ECC interrupt on controller %d (0x%x)\n",
			hisi_hba->id, ecc_err);
	}

	if (ecc_int & SAS_ECC_INTR_IOST_ECCBAD_MSK) {
		u32 addr = (hisi_sas_read32(hisi_hba, HGC_IOST_ECC_ADDR) &
				HGC_IOST_ECC_ADDR_BAD_MSK) >>
				HGC_IOST_ECC_ADDR_BAD_OFF;

		panic("Fatal IOST RAM ECC interrupt on controller %d @ 0x%08x\n",
			hisi_hba->id, addr);
	}

	if (ecc_int & SAS_ECC_INTR_ITCT_ECCBAD_MSK) {
		u32 addr = (hisi_sas_read32(hisi_hba, HGC_ITCT_ECC_ADDR) &
				HGC_ITCT_ECC_ADDR_BAD_MSK) >>
				HGC_ITCT_ECC_ADDR_BAD_OFF;

		panic("Fatal TCT RAM ECC interrupt on controller %d @ 0x%08x\n",
			hisi_hba->id, addr);
	}

	if (ecc_int & SAS_ECC_INTR_ITCT_ECC1B_MSK) {
		u32 ecc_err = hisi_sas_read32(hisi_hba, HGC_ECC_ERR);

		panic("Fatal ITCT 1b ECC interrupt on controller %d (0x%x)\n",
			hisi_hba->id, ecc_err);
	}

	hisi_sas_write32(hisi_hba, SAS_ECC_INTR, ecc_int | 0x3f);

	return IRQ_HANDLED;
}

static irqreturn_t fatal_axi_int_v1_hw(int irq, void *p)
{
	struct hisi_hba *hisi_hba = p;
	u32 axi_int = hisi_sas_read32(hisi_hba, ENT_INT_SRC2);
	u32 axi_info = hisi_sas_read32(hisi_hba, HGC_AXI_FIFO_ERR_INFO);

	if (axi_int & ENT_INT_SRC2_DQ_CFG_ERR_MSK)
		panic("Fatal DQ_CFG_ERR interrupt on controller %d (0x%x)\n",
			hisi_hba->id, axi_info);

	if (axi_int & ENT_INT_SRC2_CQ_CFG_ERR_MSK)
		panic("Fatal CQ_CFG_ERR interrupt on controller %d (0x%x)\n",
			hisi_hba->id, axi_info);

	if (axi_int & ENT_INT_SRC2_AXI_WRONG_INT_MSK)
		panic("Fatal AXI_WRONG_INT interrupt on controller %d (0x%x)\n",
			hisi_hba->id, axi_info);

	if (axi_int & ENT_INT_SRC2_AXI_OVERLF_INT_MSK)
		panic("Fatal AXI_OVERLF_INT incorrect interrupt on controller %d (0x%x)\n",
			hisi_hba->id, axi_info);

	hisi_sas_write32(hisi_hba, ENT_INT_SRC2, axi_int | 0x30000000);

	return IRQ_HANDLED;
}

#define DECLARE_PHY_INT_HANDLER_GROUP(phy)\
	DECLARE_INT_HANDLER(int_bcast_v1_hw, phy)\
	DECLARE_INT_HANDLER(int_phyup_v1_hw, phy)\
	DECLARE_INT_HANDLER(int_abnormal_v1_hw, phy)\
	DECLARE_INT_HANDLER(int_int1_v1_hw, phy)\

#define DECLARE_PHY_INT_GROUP_PTR(phy)\
	INT_HANDLER_NAME(int_bcast_v1_hw, phy),\
	INT_HANDLER_NAME(int_phyup_v1_hw, phy),\
	INT_HANDLER_NAME(int_abnormal_v1_hw, phy),\
	INT_HANDLER_NAME(int_int1_v1_hw, phy),

DECLARE_PHY_INT_HANDLER_GROUP(0)
DECLARE_PHY_INT_HANDLER_GROUP(1)
DECLARE_PHY_INT_HANDLER_GROUP(2)
DECLARE_PHY_INT_HANDLER_GROUP(3)
DECLARE_PHY_INT_HANDLER_GROUP(4)
DECLARE_PHY_INT_HANDLER_GROUP(5)
DECLARE_PHY_INT_HANDLER_GROUP(6)
DECLARE_PHY_INT_HANDLER_GROUP(7)
DECLARE_PHY_INT_HANDLER_GROUP(8)

static const char phy_int_names[HISI_SAS_PHY_INT_NR][32] = {
	{"Bcast"},
	{"Phy Up"},
	{"Abnormal"},
	{"Int1"}
};

static const char cq_int_name[32] = "cq";
static const char fatal_int_name[HISI_SAS_FATAL_INT_NR][32] = {
	"fatal ecc",
	"fatal axi"
};

static irq_handler_t phy_interrupts[HISI_SAS_MAX_PHYS][HISI_SAS_PHY_INT_NR] = {
	{DECLARE_PHY_INT_GROUP_PTR(0)},
	{DECLARE_PHY_INT_GROUP_PTR(1)},
	{DECLARE_PHY_INT_GROUP_PTR(2)},
	{DECLARE_PHY_INT_GROUP_PTR(3)},
	{DECLARE_PHY_INT_GROUP_PTR(4)},
	{DECLARE_PHY_INT_GROUP_PTR(5)},
	{DECLARE_PHY_INT_GROUP_PTR(6)},
	{DECLARE_PHY_INT_GROUP_PTR(7)},
	{DECLARE_PHY_INT_GROUP_PTR(8)},
};

DECLARE_INT_HANDLER(cq_interrupt_v1_hw, 0)
DECLARE_INT_HANDLER(cq_interrupt_v1_hw, 1)
DECLARE_INT_HANDLER(cq_interrupt_v1_hw, 2)
DECLARE_INT_HANDLER(cq_interrupt_v1_hw, 3)
DECLARE_INT_HANDLER(cq_interrupt_v1_hw, 4)
DECLARE_INT_HANDLER(cq_interrupt_v1_hw, 5)
DECLARE_INT_HANDLER(cq_interrupt_v1_hw, 6)
DECLARE_INT_HANDLER(cq_interrupt_v1_hw, 7)
DECLARE_INT_HANDLER(cq_interrupt_v1_hw, 8)
DECLARE_INT_HANDLER(cq_interrupt_v1_hw, 9)
DECLARE_INT_HANDLER(cq_interrupt_v1_hw, 10)
DECLARE_INT_HANDLER(cq_interrupt_v1_hw, 11)
DECLARE_INT_HANDLER(cq_interrupt_v1_hw, 12)
DECLARE_INT_HANDLER(cq_interrupt_v1_hw, 13)
DECLARE_INT_HANDLER(cq_interrupt_v1_hw, 14)
DECLARE_INT_HANDLER(cq_interrupt_v1_hw, 15)
DECLARE_INT_HANDLER(cq_interrupt_v1_hw, 16)
DECLARE_INT_HANDLER(cq_interrupt_v1_hw, 17)
DECLARE_INT_HANDLER(cq_interrupt_v1_hw, 18)
DECLARE_INT_HANDLER(cq_interrupt_v1_hw, 19)
DECLARE_INT_HANDLER(cq_interrupt_v1_hw, 20)
DECLARE_INT_HANDLER(cq_interrupt_v1_hw, 21)
DECLARE_INT_HANDLER(cq_interrupt_v1_hw, 22)
DECLARE_INT_HANDLER(cq_interrupt_v1_hw, 23)
DECLARE_INT_HANDLER(cq_interrupt_v1_hw, 24)
DECLARE_INT_HANDLER(cq_interrupt_v1_hw, 25)
DECLARE_INT_HANDLER(cq_interrupt_v1_hw, 26)
DECLARE_INT_HANDLER(cq_interrupt_v1_hw, 27)
DECLARE_INT_HANDLER(cq_interrupt_v1_hw, 28)
DECLARE_INT_HANDLER(cq_interrupt_v1_hw, 29)
DECLARE_INT_HANDLER(cq_interrupt_v1_hw, 30)
DECLARE_INT_HANDLER(cq_interrupt_v1_hw, 31)

static irq_handler_t cq_interrupts[HISI_SAS_MAX_QUEUES] = {
	INT_HANDLER_NAME(cq_interrupt_v1_hw, 0),
	INT_HANDLER_NAME(cq_interrupt_v1_hw, 1),
	INT_HANDLER_NAME(cq_interrupt_v1_hw, 2),
	INT_HANDLER_NAME(cq_interrupt_v1_hw, 3),
	INT_HANDLER_NAME(cq_interrupt_v1_hw, 4),
	INT_HANDLER_NAME(cq_interrupt_v1_hw, 5),
	INT_HANDLER_NAME(cq_interrupt_v1_hw, 6),
	INT_HANDLER_NAME(cq_interrupt_v1_hw, 7),
	INT_HANDLER_NAME(cq_interrupt_v1_hw, 8),
	INT_HANDLER_NAME(cq_interrupt_v1_hw, 9),
	INT_HANDLER_NAME(cq_interrupt_v1_hw, 10),
	INT_HANDLER_NAME(cq_interrupt_v1_hw, 11),
	INT_HANDLER_NAME(cq_interrupt_v1_hw, 12),
	INT_HANDLER_NAME(cq_interrupt_v1_hw, 13),
	INT_HANDLER_NAME(cq_interrupt_v1_hw, 14),
	INT_HANDLER_NAME(cq_interrupt_v1_hw, 15),
	INT_HANDLER_NAME(cq_interrupt_v1_hw, 16),
	INT_HANDLER_NAME(cq_interrupt_v1_hw, 17),
	INT_HANDLER_NAME(cq_interrupt_v1_hw, 18),
	INT_HANDLER_NAME(cq_interrupt_v1_hw, 19),
	INT_HANDLER_NAME(cq_interrupt_v1_hw, 20),
	INT_HANDLER_NAME(cq_interrupt_v1_hw, 21),
	INT_HANDLER_NAME(cq_interrupt_v1_hw, 22),
	INT_HANDLER_NAME(cq_interrupt_v1_hw, 23),
	INT_HANDLER_NAME(cq_interrupt_v1_hw, 24),
	INT_HANDLER_NAME(cq_interrupt_v1_hw, 25),
	INT_HANDLER_NAME(cq_interrupt_v1_hw, 26),
	INT_HANDLER_NAME(cq_interrupt_v1_hw, 27),
	INT_HANDLER_NAME(cq_interrupt_v1_hw, 28),
	INT_HANDLER_NAME(cq_interrupt_v1_hw, 29),
	INT_HANDLER_NAME(cq_interrupt_v1_hw, 30),
	INT_HANDLER_NAME(cq_interrupt_v1_hw, 31)
};

static irq_handler_t fatal_interrupts[HISI_SAS_MAX_QUEUES] = {
	fatal_ecc_int_v1_hw,
	fatal_axi_int_v1_hw
};

static int interrupt_init_v1_hw(struct hisi_hba *hisi_hba)
{
	int i, j, irq, rc, id = hisi_hba->id;
	struct device *dev = hisi_hba->dev;
	char *int_names = hisi_hba->int_names;

	if (!hisi_hba->np)
		return -ENOENT;

	for (i = 0; i < hisi_hba->n_phy; i++) {
		for (j = 0; j < HISI_SAS_PHY_INT_NR; j++) {
			int idx = (i * HISI_SAS_PHY_INT_NR) + j;

			irq = irq_of_parse_and_map(hisi_hba->np, idx);
			if (!irq) {
				dev_err(dev, "%s [%d] fail map phy interrupt %d\n",
					__func__, hisi_hba->id, idx);
				return -ENOENT;
			}

			(void)snprintf(&int_names[idx * HISI_SAS_NAME_LEN],
					HISI_SAS_NAME_LEN,
					DRV_NAME" %s [%d %d]", phy_int_names[j],
					id, i);
			rc = devm_request_irq(dev, irq, phy_interrupts[i][j], 0,
					&int_names[idx * HISI_SAS_NAME_LEN],
					hisi_hba);
			if (rc) {
				dev_err(dev, "%s [%d] could not request phy interrupt %d, rc=%d\n",
					__func__, hisi_hba->id, irq, rc);
				return -ENOENT;
			}
		}
	}

	for (i = 0; i < hisi_hba->queue_count; i++) {
		int idx = (hisi_hba->n_phy * HISI_SAS_PHY_INT_NR) + i;

		irq = irq_of_parse_and_map(hisi_hba->np, idx);
		if (!irq) {
			dev_err(dev, "%s [%d] could not map cq interrupt %d\n",
				__func__, hisi_hba->id, idx);
			return -ENOENT;
		}
		(void)snprintf(&int_names[idx * HISI_SAS_NAME_LEN],
				HISI_SAS_NAME_LEN,
				DRV_NAME" %s [%d %d]", cq_int_name, id, i);
		rc = devm_request_irq(dev, irq, cq_interrupts[i], 0,
				&int_names[idx * HISI_SAS_NAME_LEN],
				hisi_hba);
		if (rc) {
			dev_err(dev, "%s [%d] could not request cq interrupt %d, rc=%d\n",
				__func__, hisi_hba->id, irq, rc);
			return -ENOENT;
		}
		idx++;
	}

	for (i = 0; i < HISI_SAS_FATAL_INT_NR; i++) {
		int idx = (hisi_hba->n_phy * HISI_SAS_PHY_INT_NR) +
				hisi_hba->queue_count + i;

		irq = irq_of_parse_and_map(hisi_hba->np, idx);
		if (!irq) {
			dev_err(dev, "%s [%d] could not map fatal interrupt %d\n",
				__func__, hisi_hba->id, idx);
			return -ENOENT;
		}
		(void)snprintf(&int_names[idx * HISI_SAS_NAME_LEN],
				HISI_SAS_NAME_LEN,
				DRV_NAME" %s [%d]", fatal_int_name[i], id);
		rc = devm_request_irq(dev, irq, fatal_interrupts[i], 0,
				&int_names[idx * HISI_SAS_NAME_LEN],
				hisi_hba);
		if (rc) {
			dev_err(dev, "%s [%d] could not request fatal interrupt %d, rc=%d\n",
				__func__, hisi_hba->id, irq, rc);
			return -ENOENT;
		}
		idx++;
	}

	return 0;
}

static int interrupt_openall_v1_hw(struct hisi_hba *hisi_hba)
{
	int i;
	u32 val;

	for (i = 0; i < hisi_hba->n_phy; i++) {
		/* Clear interrupt status */
		val = hisi_sas_phy_read32(hisi_hba, i, CHL_INT0);
		hisi_sas_phy_write32(hisi_hba, i, CHL_INT0, val);
		val = hisi_sas_phy_read32(hisi_hba, i, CHL_INT1);
		hisi_sas_phy_write32(hisi_hba, i, CHL_INT1, val);
		val = hisi_sas_phy_read32(hisi_hba, i, CHL_INT2);
		hisi_sas_phy_write32(hisi_hba, i, CHL_INT2, val);

		/* Unmask interrupt */
		hisi_sas_phy_write32(hisi_hba, i, CHL_INT0_MSK, 0x003ce3ee);
		hisi_sas_phy_write32(hisi_hba, i, CHL_INT1_MSK, 0x00017fff);
		hisi_sas_phy_write32(hisi_hba, i, CHL_INT2_MSK, 0x8000012a);

		/* bypass chip bug mask abnormal intr */
		hisi_sas_phy_write32(hisi_hba,
				     i,
				     CHL_INT0_MSK,
				     0x3fffff &
				     ~CHL_INT0_MSK_PHYCTRL_NOTRDY_MSK);
	}

	return 0;
}


static const struct hisi_sas_dispatch hisi_sas_dispatch_v1_hw = {
	.hw_init = hw_init_v1_hw,
	.phys_init = phys_init_v1_hw,
	.interrupt_init = interrupt_init_v1_hw,
	.interrupt_openall = interrupt_openall_v1_hw,
	.setup_itct = hisi_sas_setup_itct_v1_hw,
	.get_free_slot = get_free_slot_v1_hw,
	.start_delivery = start_delivery_v1_hw,
	.prep_ssp = prep_ssp_v1_hw,
	.prep_smp = prep_smp_v1_hw,
	.slot_complete = slot_complete_v1_hw,
	.phy_enable = enable_phy_v1_hw,
	.phy_disable = disable_phy_v1_hw,
	.hard_phy_reset = hard_phy_reset_v1_hw,
	.free_device = free_device_v1_hw,
	/* v1 hw does not support SATA/STP */
};

const struct hisi_sas_hba_info hisi_sas_hba_info_v1_hw = {
	.cq_hdr_sz = sizeof(struct hisi_sas_complete_hdr_v1_hw),
	.dispatch = &hisi_sas_dispatch_v1_hw,
#ifdef SAS_DIF
	.prot_cap = SHOST_DIF_TYPE1_PROTECTION |
		    SHOST_DIF_TYPE2_PROTECTION |
		    SHOST_DIF_TYPE3_PROTECTION |
			SHOST_DIX_TYPE1_PROTECTION |
			SHOST_DIX_TYPE2_PROTECTION |
			SHOST_DIX_TYPE3_PROTECTION,
#endif
};

