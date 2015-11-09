/*
 * Copyright (c) 2014-2015 Hisilicon Limited.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <linux/delay.h>
#include <linux/of_mdio.h>
#include "hns_dsaf_main.h"
#include "hns_dsaf_mac.h"
#include "hns_dsaf_gmac.h"

static void hns_gmac_enable(void *mac_drv, enum mac_commom_mode mode)
{
	struct mac_driver *drv = (struct mac_driver *)mac_drv;

	/*enable GE rX/tX */
	if ((MAC_COMM_MODE_TX == mode) || (MAC_COMM_MODE_RX_AND_TX == mode))
		dsaf_set_dev_bit(drv, GMAC_PORT_EN_REG, GMAC_PORT_TX_EN_B, 1);

	if ((MAC_COMM_MODE_RX == mode) || (MAC_COMM_MODE_RX_AND_TX == mode))
		dsaf_set_dev_bit(drv, GMAC_PORT_EN_REG, GMAC_PORT_RX_EN_B, 1);
}

static void hns_gmac_disable(void *mac_drv, enum mac_commom_mode mode)
{
	struct mac_driver *drv = (struct mac_driver *)mac_drv;

	/*disable GE rX/tX */
	if ((MAC_COMM_MODE_TX == mode) || (MAC_COMM_MODE_RX_AND_TX == mode))
		dsaf_set_dev_bit(drv, GMAC_PORT_EN_REG, GMAC_PORT_TX_EN_B, 0);

	if ((MAC_COMM_MODE_RX == mode) || (MAC_COMM_MODE_RX_AND_TX == mode))
		dsaf_set_dev_bit(drv, GMAC_PORT_EN_REG, GMAC_PORT_RX_EN_B, 0);
}

/**
*hns_gmac_get_en - get port enable
*@mac_drv:mac device
*@rx:rx enable
*@tx:tx enable
*/
static void hns_gmac_get_en(void *mac_drv, u32 *rx, u32 *tx)
{
	struct mac_driver *drv = (struct mac_driver *)mac_drv;
	u32 porten;

	porten = dsaf_read_dev(drv, GMAC_PORT_EN_REG);
	*tx = dsaf_get_bit(porten, GMAC_PORT_TX_EN_B);
	*rx = dsaf_get_bit(porten, GMAC_PORT_RX_EN_B);
}

static void hns_gmac_free(void *mac_drv)
{
	struct mac_driver *drv = (struct mac_driver *)mac_drv;
	struct dsaf_device *dsaf_dev
		= (struct dsaf_device *)dev_get_drvdata(drv->dev);

	u32 mac_id = drv->mac_id;

	hns_dsaf_ge_srst_by_port(dsaf_dev, mac_id, 0);
}

static void hns_gmac_set_tx_auto_pause_frames(void *mac_drv, u16 newval)
{
	struct mac_driver *drv = (struct mac_driver *)mac_drv;

	dsaf_set_dev_field(drv, GMAC_FC_TX_TIMER_REG, GMAC_FC_TX_TIMER_M,
			   GMAC_FC_TX_TIMER_S, newval);
}

static void hns_gmac_get_tx_auto_pause_frames(void *mac_drv, u16 *newval)
{
	struct mac_driver *drv = (struct mac_driver *)mac_drv;

	*newval = dsaf_get_dev_field(drv, GMAC_FC_TX_TIMER_REG,
				     GMAC_FC_TX_TIMER_M, GMAC_FC_TX_TIMER_S);
}

static void hns_gmac_set_rx_auto_pause_frames(void *mac_drv, u32 newval)
{
	struct mac_driver *drv = (struct mac_driver *)mac_drv;

	dsaf_set_dev_bit(drv, GMAC_PAUSE_EN_REG,
			 GMAC_PAUSE_EN_RX_FDFC_B, !!newval);
}

static void hns_gmac_config_max_frame_length(void *mac_drv, u16 newval)
{
	struct mac_driver *drv = (struct mac_driver *)mac_drv;

	dsaf_set_dev_field(drv, GMAC_MAX_FRM_SIZE_REG, GMAC_MAX_FRM_SIZE_M,
			   GMAC_MAX_FRM_SIZE_S, newval);

	dsaf_set_dev_field(drv, GAMC_RX_MAX_FRAME, GMAC_MAX_FRM_SIZE_M,
			   GMAC_MAX_FRM_SIZE_S, newval);
}

static void hns_gmac_config_an_mode(void *mac_drv, u8 newval)
{
	struct mac_driver *drv = (struct mac_driver *)mac_drv;

	dsaf_set_dev_bit(drv, GMAC_TRANSMIT_CONTROL_REG,
			 GMAC_TX_AN_EN_B, !!newval);
}

static void hns_gmac_tx_loop_pkt_dis(void *mac_drv)
{
	u32 tx_loop_pkt_pri;
	struct mac_driver *drv = (struct mac_driver *)mac_drv;

	tx_loop_pkt_pri = dsaf_read_dev(drv, GMAC_TX_LOOP_PKT_PRI_REG);
	dsaf_set_bit(tx_loop_pkt_pri, GMAC_TX_LOOP_PKT_EN_B, 1);
	dsaf_set_bit(tx_loop_pkt_pri, GMAC_TX_LOOP_PKT_HIG_PRI_B, 0);
	dsaf_write_dev(drv, GMAC_TX_LOOP_PKT_PRI_REG, tx_loop_pkt_pri);
}

static void hns_gmac_set_duplex_type(void *mac_drv, u8 newval)
{
	struct mac_driver *drv = (struct mac_driver *)mac_drv;

	dsaf_set_dev_bit(drv, GMAC_DUPLEX_TYPE_REG,
			 GMAC_DUPLEX_TYPE_B, !!newval);
}

static void hns_gmac_get_duplex_type(void *mac_drv,
				     enum hns_gmac_duplex_mdoe *duplex_mode)
{
	struct mac_driver *drv = (struct mac_driver *)mac_drv;

	*duplex_mode = (enum hns_gmac_duplex_mdoe)dsaf_get_dev_bit(
		drv, GMAC_DUPLEX_TYPE_REG, GMAC_DUPLEX_TYPE_B);
}

static void hns_gmac_get_port_mode(void *mac_drv, enum hns_port_mode *port_mode)
{
	struct mac_driver *drv = (struct mac_driver *)mac_drv;

	*port_mode = (enum hns_port_mode)dsaf_get_dev_field(
		drv, GMAC_PORT_MODE_REG, GMAC_PORT_MODE_M, GMAC_PORT_MODE_S);
}

static void hns_gmac_port_mode_get(void *mac_drv,
				   struct hns_gmac_port_mode_cfg *port_mode)
{
	u32 tx_ctrl;
	u32 recv_ctrl;
	struct mac_driver *drv = (struct mac_driver *)mac_drv;

	port_mode->port_mode = (enum hns_port_mode)dsaf_get_dev_field(
		drv, GMAC_PORT_MODE_REG, GMAC_PORT_MODE_M, GMAC_PORT_MODE_S);

	tx_ctrl = dsaf_read_dev(drv, GMAC_TRANSMIT_CONTROL_REG);
	recv_ctrl = dsaf_read_dev(drv, GMAC_RECV_CONTROL_REG);

	port_mode->max_frm_size =
		dsaf_get_dev_field(drv, GMAC_MAX_FRM_SIZE_REG,
				   GMAC_MAX_FRM_SIZE_M, GMAC_MAX_FRM_SIZE_S);
	port_mode->short_runts_thr =
		dsaf_get_dev_field(drv, GMAC_SHORT_RUNTS_THR_REG,
				   GMAC_SHORT_RUNTS_THR_M,
				   GMAC_SHORT_RUNTS_THR_S);

	port_mode->pad_enable = dsaf_get_bit(tx_ctrl, GMAC_TX_PAD_EN_B);
	port_mode->crc_add = dsaf_get_bit(tx_ctrl, GMAC_TX_CRC_ADD_B);
	port_mode->an_enable = dsaf_get_bit(tx_ctrl, GMAC_TX_AN_EN_B);

	port_mode->runt_pkt_en =
		dsaf_get_bit(recv_ctrl, GMAC_RECV_CTRL_RUNT_PKT_EN_B);
	port_mode->strip_pad_en =
		dsaf_get_bit(recv_ctrl, GMAC_RECV_CTRL_STRIP_PAD_EN_B);
}

static void hns_gmac_pause_frm_cfg(void *mac_drv, u32 rx_pause_en,
				   u32 tx_pause_en)
{
	u32 pause_en;
	struct mac_driver *drv = (struct mac_driver *)mac_drv;

	pause_en = dsaf_read_dev(drv, GMAC_PAUSE_EN_REG);
	dsaf_set_bit(pause_en, GMAC_PAUSE_EN_RX_FDFC_B, !!rx_pause_en);
	dsaf_set_bit(pause_en, GMAC_PAUSE_EN_TX_FDFC_B, !!tx_pause_en);
	dsaf_write_dev(drv, GMAC_PAUSE_EN_REG, pause_en);
}

static void hns_gmac_get_pausefrm_cfg(void *mac_drv, u32 *rx_pause_en,
				      u32 *tx_pause_en)
{
	u32 pause_en;
	struct mac_driver *drv = (struct mac_driver *)mac_drv;

	pause_en = dsaf_read_dev(drv, GMAC_PAUSE_EN_REG);

	*rx_pause_en = dsaf_get_bit(pause_en, GMAC_PAUSE_EN_RX_FDFC_B);
	*tx_pause_en = dsaf_get_bit(pause_en, GMAC_PAUSE_EN_TX_FDFC_B);
}

static int hns_gmac_adjust_link(void *mac_drv, enum mac_speed speed,
				u32 full_duplex)
{
	u32 tx_ctrl;
	struct mac_driver *drv = (struct mac_driver *)mac_drv;

	dsaf_set_dev_bit(drv, GMAC_DUPLEX_TYPE_REG,
			 GMAC_DUPLEX_TYPE_B, !!full_duplex);

	switch (speed) {
	case MAC_SPEED_10:
		dsaf_set_dev_field(
			drv, GMAC_PORT_MODE_REG,
			GMAC_PORT_MODE_M, GMAC_PORT_MODE_S, 0x6);
		break;
	case MAC_SPEED_100:
		dsaf_set_dev_field(
			drv, GMAC_PORT_MODE_REG,
			GMAC_PORT_MODE_M, GMAC_PORT_MODE_S, 0x7);
		break;
	case MAC_SPEED_1000:
		dsaf_set_dev_field(
			drv, GMAC_PORT_MODE_REG,
			GMAC_PORT_MODE_M, GMAC_PORT_MODE_S, 0x8);
		break;
	default:
		dev_err(drv->dev,
			"hns_gmac_adjust_link fail, speed 0x%x dsaf%d mac%d\n",
			speed, drv->dsaf_id, drv->mac_id);
		return -EINVAL;
	}

	tx_ctrl = dsaf_read_dev(drv, GMAC_TRANSMIT_CONTROL_REG);
	dsaf_set_bit(tx_ctrl, GMAC_TX_PAD_EN_B, 1);
	dsaf_set_bit(tx_ctrl, GMAC_TX_CRC_ADD_B, 1);
	dsaf_write_dev(drv, GMAC_TRANSMIT_CONTROL_REG, tx_ctrl);

	dsaf_set_dev_bit(drv, GMAC_MODE_CHANGE_EN_REG,
			 GMAC_MODE_CHANGE_EB_B, 1);

	return 0;
}

static void hns_gmac_init(void *mac_drv)
{
	u32 port;
	struct mac_driver *drv = (struct mac_driver *)mac_drv;
	struct dsaf_device *dsaf_dev
		= (struct dsaf_device *)dev_get_drvdata(drv->dev);

	port = drv->mac_id;

	hns_dsaf_ge_srst_by_port(dsaf_dev, port, 0);
	mdelay(10);
	hns_dsaf_ge_srst_by_port(dsaf_dev, port, 1);
	mdelay(10);
	hns_gmac_disable(mac_drv, MAC_COMM_MODE_RX_AND_TX);
	hns_gmac_tx_loop_pkt_dis(mac_drv);
}

void hns_gmac_update_stats(void *mac_drv)
{
	struct mac_hw_stats *hw_stats = NULL;
	struct mac_driver *drv = (struct mac_driver *)mac_drv;

	hw_stats = &drv->mac_cb->hw_stats;

	/* RX */
	hw_stats->rx_good_bytes
		+= dsaf_read_dev(drv, GMAC_RX_OCTETS_TOTAL_OK_REG);
	hw_stats->rx_bad_bytes
		+= dsaf_read_dev(drv, GMAC_RX_OCTETS_BAD_REG);
	hw_stats->rx_uc_pkts += dsaf_read_dev(drv, GMAC_RX_UC_PKTS_REG);
	hw_stats->rx_mc_pkts += dsaf_read_dev(drv, GMAC_RX_MC_PKTS_REG);
	hw_stats->rx_bc_pkts += dsaf_read_dev(drv, GMAC_RX_BC_PKTS_REG);
	hw_stats->rx_64bytes
		+= dsaf_read_dev(drv, GMAC_RX_PKTS_64OCTETS_REG);
	hw_stats->rx_65to127
		+= dsaf_read_dev(drv, GMAC_RX_PKTS_65TO127OCTETS_REG);
	hw_stats->rx_128to255
		+= dsaf_read_dev(drv, GMAC_RX_PKTS_128TO255OCTETS_REG);
	hw_stats->rx_256to511
		+= dsaf_read_dev(drv, GMAC_RX_PKTS_255TO511OCTETS_REG);
	hw_stats->rx_512to1023
		+= dsaf_read_dev(drv, GMAC_RX_PKTS_512TO1023OCTETS_REG);
	hw_stats->rx_1024to1518
		+= dsaf_read_dev(drv, GMAC_RX_PKTS_1024TO1518OCTETS_REG);
	hw_stats->rx_1519tomax
		+= dsaf_read_dev(drv, GMAC_RX_PKTS_1519TOMAXOCTETS_REG);
	hw_stats->rx_fcs_err += dsaf_read_dev(drv, GMAC_RX_FCS_ERRORS_REG);
	hw_stats->rx_vlan_pkts += dsaf_read_dev(drv, GMAC_RX_TAGGED_REG);
	hw_stats->rx_data_err += dsaf_read_dev(drv, GMAC_RX_DATA_ERR_REG);
	hw_stats->rx_align_err
		+= dsaf_read_dev(drv, GMAC_RX_ALIGN_ERRORS_REG);
	hw_stats->rx_oversize
		+= dsaf_read_dev(drv, GMAC_RX_LONG_ERRORS_REG);
	hw_stats->rx_jabber_err
		+= dsaf_read_dev(drv, GMAC_RX_JABBER_ERRORS_REG);
	hw_stats->rx_pfc_tc0
		+= dsaf_read_dev(drv, GMAC_RX_PAUSE_MACCTRL_FRAM_REG);
	hw_stats->rx_unknown_ctrl
		+= dsaf_read_dev(drv, GMAC_RX_UNKNOWN_MACCTRL_FRAM_REG);
	hw_stats->rx_long_err
		+= dsaf_read_dev(drv, GMAC_RX_VERY_LONG_ERR_CNT_REG);
	hw_stats->rx_minto64
		+= dsaf_read_dev(drv, GMAC_RX_RUNT_ERR_CNT_REG);
	hw_stats->rx_under_min
		+= dsaf_read_dev(drv, GMAC_RX_SHORT_ERR_CNT_REG);
	hw_stats->rx_filter_pkts
		+= dsaf_read_dev(drv, GMAC_RX_FILT_PKT_CNT_REG);
	hw_stats->rx_filter_bytes
		+= dsaf_read_dev(drv, GMAC_RX_OCTETS_TOTAL_FILT_REG);
	hw_stats->rx_fifo_overrun_err
		+= dsaf_read_dev(drv, GMAC_RX_OVERRUN_CNT_REG);
	hw_stats->rx_len_err
		+= dsaf_read_dev(drv, GMAC_RX_LENGTHFIELD_ERR_CNT_REG);
	hw_stats->rx_comma_err
		+= dsaf_read_dev(drv, GMAC_RX_FAIL_COMMA_CNT_REG);

	/* TX */
	hw_stats->tx_good_bytes
		+= dsaf_read_dev(drv, GMAC_OCTETS_TRANSMITTED_OK_REG);
	hw_stats->tx_bad_bytes
		+= dsaf_read_dev(drv, GMAC_OCTETS_TRANSMITTED_BAD_REG);
	hw_stats->tx_uc_pkts += dsaf_read_dev(drv, GMAC_TX_UC_PKTS_REG);
	hw_stats->tx_mc_pkts += dsaf_read_dev(drv, GMAC_TX_MC_PKTS_REG);
	hw_stats->tx_bc_pkts += dsaf_read_dev(drv, GMAC_TX_BC_PKTS_REG);
	hw_stats->tx_64bytes
		+= dsaf_read_dev(drv, GMAC_TX_PKTS_64OCTETS_REG);
	hw_stats->tx_65to127
		+= dsaf_read_dev(drv, GMAC_TX_PKTS_65TO127OCTETS_REG);
	hw_stats->tx_128to255
		+= dsaf_read_dev(drv, GMAC_TX_PKTS_128TO255OCTETS_REG);
	hw_stats->tx_256to511
		+= dsaf_read_dev(drv, GMAC_TX_PKTS_255TO511OCTETS_REG);
	hw_stats->tx_512to1023
		+= dsaf_read_dev(drv, GMAC_TX_PKTS_512TO1023OCTETS_REG);
	hw_stats->tx_1024to1518
		+= dsaf_read_dev(drv, GMAC_TX_PKTS_1024TO1518OCTETS_REG);
	hw_stats->tx_1519tomax
		+= dsaf_read_dev(drv, GMAC_TX_PKTS_1519TOMAXOCTETS_REG);
	hw_stats->tx_jabber_err
		+= dsaf_read_dev(drv, GMAC_TX_EXCESSIVE_LENGTH_DROP_REG);
	hw_stats->tx_underrun_err
		+= dsaf_read_dev(drv, GMAC_TX_UNDERRUN_REG);
	hw_stats->tx_vlan += dsaf_read_dev(drv, GMAC_TX_TAGGED_REG);
	hw_stats->tx_crc_err += dsaf_read_dev(drv, GMAC_TX_CRC_ERROR_REG);
	hw_stats->tx_pfc_tc0
		+= dsaf_read_dev(drv, GMAC_TX_PAUSE_FRAMES_REG);
}

static void hns_gmac_reset(void *mac_drv, u32 wait)
{
	u8 port;
	struct mac_driver *drv = (struct mac_driver *)mac_drv;
	struct dsaf_device *dsaf_dev
		= (struct dsaf_device *)dev_get_drvdata(drv->dev);

	port = drv->mac_id;
	mdelay(10);
	/* 0:request reset ge hw*/
	hns_dsaf_ge_srst_by_port(dsaf_dev, port, 0);

	mdelay(10);
	/* 1:drop request reset ge hw*/
	hns_dsaf_ge_srst_by_port(dsaf_dev, port, 1);

	mdelay(10);
}

static void hns_gmac_set_mac_addr(void *mac_drv, char *mac_addr)
{
	struct mac_driver *drv = (struct mac_driver *)mac_drv;

	if (drv->mac_id >= DSAF_SERVICE_NW_NUM) {
		u32 high_val = mac_addr[1] | (mac_addr[0] << 8);

		u32 low_val = mac_addr[5] | (mac_addr[4] << 8)
			| (mac_addr[3] << 16) | (mac_addr[2] << 24);
		dsaf_write_dev(drv, GMAC_STATION_ADDR_LOW_2_REG, low_val);
		dsaf_write_dev(drv, GMAC_STATION_ADDR_HIGH_2_REG, high_val);
	}
}

static int hns_gmac_config_loopback(void *mac_drv, enum hnae_loop loop_mode,
				    u8 enable)
{
	struct mac_driver *drv = (struct mac_driver *)mac_drv;

	switch (loop_mode) {
	case MAC_INTERNALLOOP_MAC:
		dsaf_set_dev_bit(drv, GMAC_LOOP_REG, GMAC_LP_REG_CF2MI_LP_EN_B,
				 !!enable);
		break;
	default:
		dev_err(drv->dev, "loop_mode error\n");
		return -EINVAL;
	}

	return 0;
}

static void hns_gmac_config_pad_and_crc(void *mac_drv, u8 newval)
{
	u32 tx_ctrl;
	struct mac_driver *drv = (struct mac_driver *)mac_drv;

	tx_ctrl = dsaf_read_dev(drv, GMAC_TRANSMIT_CONTROL_REG);
	dsaf_set_bit(tx_ctrl, GMAC_TX_PAD_EN_B, !!newval);
	dsaf_set_bit(tx_ctrl, GMAC_TX_CRC_ADD_B, !!newval);
	dsaf_write_dev(drv, GMAC_TRANSMIT_CONTROL_REG, tx_ctrl);
}

static void hns_gmac_get_id(void *mac_drv, u8 *mac_id)
{
	struct mac_driver *drv = (struct mac_driver *)mac_drv;

	*mac_id = drv->mac_id + drv->dsaf_id * DSAF_PORT_NUM;
}

static void hns_gmac_get_info(void *mac_drv, struct mac_info *mac_info)
{
	enum hns_gmac_duplex_mdoe duplex;
	enum hns_port_mode speed;
	u32 rx_pause;
	u32 tx_pause;
	u32 rx;
	u32 tx;
	u16 fc_tx_timer;
	struct hns_gmac_port_mode_cfg port_mode = { GMAC_10M_MII, 0 };

	hns_gmac_port_mode_get(mac_drv, &port_mode);
	mac_info->pad_and_crc_en = port_mode.crc_add && port_mode.pad_enable;
	mac_info->auto_neg = port_mode.an_enable;

	hns_gmac_get_tx_auto_pause_frames(mac_drv, &fc_tx_timer);
	mac_info->tx_pause_time = fc_tx_timer;

	hns_gmac_get_en(mac_drv, &rx, &tx);
	mac_info->port_en = rx && tx;

	hns_gmac_get_duplex_type(mac_drv, &duplex);
	mac_info->duplex = duplex;

	hns_gmac_get_port_mode(mac_drv, &speed);
	switch (speed) {
	case GMAC_10M_SGMII:
		mac_info->speed = MAC_SPEED_10;
		break;
	case GMAC_100M_SGMII:
		mac_info->speed = MAC_SPEED_100;
		break;
	case GMAC_1000M_SGMII:
		mac_info->speed = MAC_SPEED_1000;
		break;
	default:
		mac_info->speed = 0;
		break;
	}

	hns_gmac_get_pausefrm_cfg(mac_drv, &rx_pause, &tx_pause);
	mac_info->rx_pause_en = rx_pause;
	mac_info->tx_pause_en = tx_pause;
}

static void hns_gmac_autoneg_stat(void *mac_drv, u32 *enable)
{
	struct mac_driver *drv = (struct mac_driver *)mac_drv;

	*enable = dsaf_get_dev_bit(drv, GMAC_TRANSMIT_CONTROL_REG,
				   GMAC_TX_AN_EN_B);
}

static void hns_gmac_get_link_status(void *mac_drv, u32 *link_stat)
{
	struct mac_driver *drv = (struct mac_driver *)mac_drv;

	*link_stat = dsaf_get_dev_bit(drv, GMAC_AN_NEG_STATE_REG,
				      GMAC_AN_NEG_STAT_RX_SYNC_OK_B);
}

static void hns_gmac_get_regs(void *mac_drv, void *data)
{
	u32 *regs = data;
	int i;
	struct mac_driver *drv = (struct mac_driver *)mac_drv;

	/* base config registers */
	regs[0] = dsaf_read_dev(drv, GMAC_DUPLEX_TYPE_REG);
	regs[1] = dsaf_read_dev(drv, GMAC_FD_FC_TYPE_REG);
	regs[2] = dsaf_read_dev(drv, GMAC_FC_TX_TIMER_REG);
	regs[3] = dsaf_read_dev(drv, GMAC_FD_FC_ADDR_LOW_REG);
	regs[4] = dsaf_read_dev(drv, GMAC_FD_FC_ADDR_HIGH_REG);
	regs[5] = dsaf_read_dev(drv, GMAC_IPG_TX_TIMER_REG);
	regs[6] = dsaf_read_dev(drv, GMAC_PAUSE_THR_REG);
	regs[7] = dsaf_read_dev(drv, GMAC_MAX_FRM_SIZE_REG);
	regs[8] = dsaf_read_dev(drv, GMAC_PORT_MODE_REG);
	regs[9] = dsaf_read_dev(drv, GMAC_PORT_EN_REG);
	regs[10] = dsaf_read_dev(drv, GMAC_PAUSE_EN_REG);
	regs[11] = dsaf_read_dev(drv, GMAC_SHORT_RUNTS_THR_REG);
	regs[12] = dsaf_read_dev(drv, GMAC_AN_NEG_STATE_REG);
	regs[13] = dsaf_read_dev(drv, GMAC_TX_LOCAL_PAGE_REG);
	regs[14] = dsaf_read_dev(drv, GMAC_TRANSMIT_CONTROL_REG);
	regs[15] = dsaf_read_dev(drv, GMAC_REC_FILT_CONTROL_REG);
	regs[16] = dsaf_read_dev(drv, GMAC_PTP_CONFIG_REG);

	/* rx static registers */
	regs[17] = dsaf_read_dev(drv, GMAC_RX_OCTETS_TOTAL_OK_REG);
	regs[18] = dsaf_read_dev(drv, GMAC_RX_OCTETS_BAD_REG);
	regs[19] = dsaf_read_dev(drv, GMAC_RX_UC_PKTS_REG);
	regs[20] = dsaf_read_dev(drv, GMAC_RX_MC_PKTS_REG);
	regs[21] = dsaf_read_dev(drv, GMAC_RX_BC_PKTS_REG);
	regs[22] = dsaf_read_dev(drv, GMAC_RX_PKTS_64OCTETS_REG);
	regs[23] = dsaf_read_dev(drv, GMAC_RX_PKTS_65TO127OCTETS_REG);
	regs[24] = dsaf_read_dev(drv, GMAC_RX_PKTS_128TO255OCTETS_REG);
	regs[25] = dsaf_read_dev(drv, GMAC_RX_PKTS_255TO511OCTETS_REG);
	regs[26] = dsaf_read_dev(drv, GMAC_RX_PKTS_512TO1023OCTETS_REG);
	regs[27] = dsaf_read_dev(drv, GMAC_RX_PKTS_1024TO1518OCTETS_REG);
	regs[28] = dsaf_read_dev(drv, GMAC_RX_PKTS_1519TOMAXOCTETS_REG);
	regs[29] = dsaf_read_dev(drv, GMAC_RX_FCS_ERRORS_REG);
	regs[30] = dsaf_read_dev(drv, GMAC_RX_TAGGED_REG);
	regs[31] = dsaf_read_dev(drv, GMAC_RX_DATA_ERR_REG);
	regs[32] = dsaf_read_dev(drv, GMAC_RX_ALIGN_ERRORS_REG);
	regs[33] = dsaf_read_dev(drv, GMAC_RX_LONG_ERRORS_REG);
	regs[34] = dsaf_read_dev(drv, GMAC_RX_JABBER_ERRORS_REG);
	regs[35] = dsaf_read_dev(drv, GMAC_RX_PAUSE_MACCTRL_FRAM_REG);
	regs[36] = dsaf_read_dev(drv, GMAC_RX_UNKNOWN_MACCTRL_FRAM_REG);
	regs[37] = dsaf_read_dev(drv, GMAC_RX_VERY_LONG_ERR_CNT_REG);
	regs[38] = dsaf_read_dev(drv, GMAC_RX_RUNT_ERR_CNT_REG);
	regs[39] = dsaf_read_dev(drv, GMAC_RX_SHORT_ERR_CNT_REG);
	regs[40] = dsaf_read_dev(drv, GMAC_RX_FILT_PKT_CNT_REG);
	regs[41] = dsaf_read_dev(drv, GMAC_RX_OCTETS_TOTAL_FILT_REG);

	/* tx static registers */
	regs[42] = dsaf_read_dev(drv, GMAC_OCTETS_TRANSMITTED_OK_REG);
	regs[43] = dsaf_read_dev(drv, GMAC_OCTETS_TRANSMITTED_BAD_REG);
	regs[44] = dsaf_read_dev(drv, GMAC_TX_UC_PKTS_REG);
	regs[45] = dsaf_read_dev(drv, GMAC_TX_MC_PKTS_REG);
	regs[46] = dsaf_read_dev(drv, GMAC_TX_BC_PKTS_REG);
	regs[47] = dsaf_read_dev(drv, GMAC_TX_PKTS_64OCTETS_REG);
	regs[48] = dsaf_read_dev(drv, GMAC_TX_PKTS_65TO127OCTETS_REG);
	regs[49] = dsaf_read_dev(drv, GMAC_TX_PKTS_128TO255OCTETS_REG);
	regs[50] = dsaf_read_dev(drv, GMAC_TX_PKTS_255TO511OCTETS_REG);
	regs[51] = dsaf_read_dev(drv, GMAC_TX_PKTS_512TO1023OCTETS_REG);
	regs[52] = dsaf_read_dev(drv, GMAC_TX_PKTS_1024TO1518OCTETS_REG);
	regs[53] = dsaf_read_dev(drv, GMAC_TX_PKTS_1519TOMAXOCTETS_REG);
	regs[54] = dsaf_read_dev(drv, GMAC_TX_EXCESSIVE_LENGTH_DROP_REG);
	regs[55] = dsaf_read_dev(drv, GMAC_TX_UNDERRUN_REG);
	regs[56] = dsaf_read_dev(drv, GMAC_TX_TAGGED_REG);
	regs[57] = dsaf_read_dev(drv, GMAC_TX_CRC_ERROR_REG);
	regs[58] = dsaf_read_dev(drv, GMAC_TX_PAUSE_FRAMES_REG);

	regs[59] = dsaf_read_dev(drv, GAMC_RX_MAX_FRAME);
	regs[60] = dsaf_read_dev(drv, GMAC_LINE_LOOP_BACK_REG);
	regs[61] = dsaf_read_dev(drv, GMAC_CF_CRC_STRIP_REG);
	regs[62] = dsaf_read_dev(drv, GMAC_MODE_CHANGE_EN_REG);
	regs[63] = dsaf_read_dev(drv, GMAC_SIXTEEN_BIT_CNTR_REG);
	regs[64] = dsaf_read_dev(drv, GMAC_LD_LINK_COUNTER_REG);
	regs[65] = dsaf_read_dev(drv, GMAC_LOOP_REG);
	regs[66] = dsaf_read_dev(drv, GMAC_RECV_CONTROL_REG);
	regs[67] = dsaf_read_dev(drv, GMAC_VLAN_CODE_REG);
	regs[68] = dsaf_read_dev(drv, GMAC_RX_OVERRUN_CNT_REG);
	regs[69] = dsaf_read_dev(drv, GMAC_RX_LENGTHFIELD_ERR_CNT_REG);
	regs[70] = dsaf_read_dev(drv, GMAC_RX_FAIL_COMMA_CNT_REG);

	regs[71] = dsaf_read_dev(drv, GMAC_STATION_ADDR_LOW_0_REG);
	regs[72] = dsaf_read_dev(drv, GMAC_STATION_ADDR_HIGH_0_REG);
	regs[73] = dsaf_read_dev(drv, GMAC_STATION_ADDR_LOW_1_REG);
	regs[74] = dsaf_read_dev(drv, GMAC_STATION_ADDR_HIGH_1_REG);
	regs[75] = dsaf_read_dev(drv, GMAC_STATION_ADDR_LOW_2_REG);
	regs[76] = dsaf_read_dev(drv, GMAC_STATION_ADDR_HIGH_2_REG);
	regs[77] = dsaf_read_dev(drv, GMAC_STATION_ADDR_LOW_3_REG);
	regs[78] = dsaf_read_dev(drv, GMAC_STATION_ADDR_HIGH_3_REG);
	regs[79] = dsaf_read_dev(drv, GMAC_STATION_ADDR_LOW_4_REG);
	regs[80] = dsaf_read_dev(drv, GMAC_STATION_ADDR_HIGH_4_REG);
	regs[81] = dsaf_read_dev(drv, GMAC_STATION_ADDR_LOW_5_REG);
	regs[82] = dsaf_read_dev(drv, GMAC_STATION_ADDR_HIGH_5_REG);
	regs[83] = dsaf_read_dev(drv, GMAC_STATION_ADDR_LOW_MSK_0_REG);
	regs[84] = dsaf_read_dev(drv, GMAC_STATION_ADDR_HIGH_MSK_0_REG);
	regs[85] = dsaf_read_dev(drv, GMAC_STATION_ADDR_LOW_MSK_1_REG);
	regs[86] = dsaf_read_dev(drv, GMAC_STATION_ADDR_HIGH_MSK_1_REG);
	regs[87] = dsaf_read_dev(drv, GMAC_MAC_SKIP_LEN_REG);
	regs[88] = dsaf_read_dev(drv, GMAC_TX_LOOP_PKT_PRI_REG);

	/* mark end of mac regs */
	for (i = 89; i < 96; i++)
		regs[i] = 0xaaaaaaaa;
}

static void hns_gmac_get_ethtool_stats(void *mac_drv,
				       struct ethtool_stats *cmd, u64 *data)
{
	u64 *buf = data;
	struct mac_driver *drv = (struct mac_driver *)mac_drv;
	struct mac_hw_stats *hw_stats = NULL;

	hw_stats = &drv->mac_cb->hw_stats;

	hns_gmac_update_stats(drv);

	buf[0] = hw_stats->rx_good_bytes;
	buf[1] = hw_stats->rx_bad_bytes;
	buf[2] = hw_stats->rx_uc_pkts;
	buf[3] = hw_stats->rx_mc_pkts;
	buf[4] = hw_stats->rx_bc_pkts;
	buf[5] = hw_stats->rx_64bytes;
	buf[6] = hw_stats->rx_65to127;
	buf[7] = hw_stats->rx_128to255;
	buf[8] = hw_stats->rx_256to511;
	buf[9] = hw_stats->rx_512to1023;
	buf[10] = hw_stats->rx_1024to1518;
	buf[11] = hw_stats->rx_1519tomax;
	buf[12] = hw_stats->rx_fcs_err;
	buf[13] = hw_stats->rx_vlan_pkts;
	buf[14] = hw_stats->rx_data_err;
	buf[15] = hw_stats->rx_align_err;
	buf[16] = hw_stats->rx_long_err;
	buf[17] = hw_stats->rx_jabber_err;
	buf[18] = hw_stats->rx_pfc_tc0;
	buf[19] = hw_stats->rx_unknown_ctrl;
	buf[20] = hw_stats->rx_oversize;
	buf[21] = hw_stats->rx_minto64;
	buf[22] = hw_stats->rx_under_min;
	buf[23] = hw_stats->rx_filter_pkts;
	buf[24] = hw_stats->rx_filter_bytes;

	buf[25] = hw_stats->tx_good_bytes;
	buf[26] = hw_stats->tx_bad_bytes;
	buf[27] = hw_stats->tx_uc_pkts;
	buf[28] = hw_stats->tx_mc_pkts;
	buf[29] = hw_stats->tx_bc_pkts;
	buf[30] = hw_stats->tx_64bytes;
	buf[31] = hw_stats->tx_65to127;
	buf[32] = hw_stats->tx_128to255;
	buf[33] = hw_stats->tx_256to511;
	buf[34] = hw_stats->tx_512to1023;
	buf[35] = hw_stats->tx_1024to1518;
	buf[36] = hw_stats->tx_1519tomax;
	buf[37] = hw_stats->tx_jabber_err;
	buf[38] = hw_stats->tx_underrun_err;
	buf[39] = hw_stats->tx_vlan;
	buf[40] = hw_stats->tx_crc_err;
	buf[41] = hw_stats->tx_pfc_tc0;

	buf[42] = dsaf_read_dev(drv, GMAC_DUPLEX_TYPE_REG);
	buf[43] = dsaf_read_dev(drv, GMAC_FD_FC_TYPE_REG);
	buf[44] = dsaf_read_dev(drv, GMAC_FC_TX_TIMER_REG);
	buf[45] = dsaf_read_dev(drv, GMAC_PAUSE_THR_REG);
	buf[46] = dsaf_read_dev(drv, GMAC_MAX_FRM_SIZE_REG);
	buf[47] = dsaf_read_dev(drv, GMAC_PORT_MODE_REG);
	buf[48] = dsaf_read_dev(drv, GMAC_PORT_EN_REG);
	buf[49] = dsaf_read_dev(drv, GMAC_PAUSE_EN_REG);
	buf[50] = dsaf_read_dev(drv, GMAC_SHORT_RUNTS_THR_REG);
	buf[51] = dsaf_read_dev(drv, GMAC_AN_NEG_STATE_REG);
	buf[52] = dsaf_read_dev(drv, GMAC_TX_LOCAL_PAGE_REG);
	buf[53] = dsaf_read_dev(drv, GMAC_TRANSMIT_CONTROL_REG);
	buf[54] = dsaf_read_dev(drv, GMAC_REC_FILT_CONTROL_REG);
	buf[55] = dsaf_read_dev(drv, GMAC_LINE_LOOP_BACK_REG);
	buf[56] = dsaf_read_dev(drv, GMAC_CF_CRC_STRIP_REG);
	buf[57] = dsaf_read_dev(drv, GMAC_MODE_CHANGE_EN_REG);
	buf[58] = dsaf_read_dev(drv, GMAC_SIXTEEN_BIT_CNTR_REG);
	buf[59] = dsaf_read_dev(drv, GMAC_LD_LINK_COUNTER_REG);
	buf[60] = dsaf_read_dev(drv, GMAC_LOOP_REG);
	buf[61] = dsaf_read_dev(drv, GMAC_RECV_CONTROL_REG);
	buf[62] = dsaf_read_dev(drv, GMAC_VLAN_CODE_REG);
	buf[63] = dsaf_read_dev(drv, GMAC_RX_OVERRUN_CNT_REG);
	buf[64] = dsaf_read_dev(drv, GMAC_RX_FAIL_COMMA_CNT_REG);
}

static void hns_gmac_get_strings(void *mac_drv, u32 stringset, u8 *data)
{
	char *buff = (char *)data;

	snprintf(buff, ETH_GSTRING_LEN, "GMAC_RX_OCTETS_TOTAL_OK");
	buff = buff + ETH_GSTRING_LEN;
	snprintf(buff, ETH_GSTRING_LEN, "GMAC_RX_OCTETS_BAD");
	buff = buff + ETH_GSTRING_LEN;
	snprintf(buff, ETH_GSTRING_LEN, "GMAC_RX_UC_PKTS");
	buff = buff + ETH_GSTRING_LEN;
	snprintf(buff, ETH_GSTRING_LEN, "GMAC_RX_MC_PKTS");
	buff = buff + ETH_GSTRING_LEN;
	snprintf(buff, ETH_GSTRING_LEN, "GMAC_RX_BC_PKTS");
	buff = buff + ETH_GSTRING_LEN;
	snprintf(buff, ETH_GSTRING_LEN, "GMAC_RX_PKTS_64OCTETS");
	buff = buff + ETH_GSTRING_LEN;
	snprintf(buff, ETH_GSTRING_LEN, "GMAC_RX_PKTS_65TO127");
	buff = buff + ETH_GSTRING_LEN;
	snprintf(buff, ETH_GSTRING_LEN, "GMAC_RX_PKTS_128TO255");
	buff = buff + ETH_GSTRING_LEN;
	snprintf(buff, ETH_GSTRING_LEN, "GMAC_RX_PKTS_255TO511");
	buff = buff + ETH_GSTRING_LEN;
	snprintf(buff, ETH_GSTRING_LEN, "GMAC_RX_PKTS_512TO1023");
	buff = buff + ETH_GSTRING_LEN;
	snprintf(buff, ETH_GSTRING_LEN, "GMAC_RX_PKTS_1024TO1518");
	buff = buff + ETH_GSTRING_LEN;
	snprintf(buff, ETH_GSTRING_LEN, "GMAC_RX_PKTS_1519TOMAX");
	buff = buff + ETH_GSTRING_LEN;
	snprintf(buff, ETH_GSTRING_LEN, "GMAC_RX_FCS_ERRORS");
	buff = buff + ETH_GSTRING_LEN;
	snprintf(buff, ETH_GSTRING_LEN, "GMAC_RX_TAGGED");
	buff = buff + ETH_GSTRING_LEN;
	snprintf(buff, ETH_GSTRING_LEN, "GMAC_RX_DATA_ERR");
	buff = buff + ETH_GSTRING_LEN;
	snprintf(buff, ETH_GSTRING_LEN, "GMAC_RX_ALIGN_ERRORS");
	buff = buff + ETH_GSTRING_LEN;
	snprintf(buff, ETH_GSTRING_LEN, "GMAC_RX_LONG_ERRORS");
	buff = buff + ETH_GSTRING_LEN;
	snprintf(buff, ETH_GSTRING_LEN, "GMAC_RX_JABBER_ERRORS");
	buff = buff + ETH_GSTRING_LEN;
	snprintf(buff, ETH_GSTRING_LEN, "GMAC_RX_PAUSE_MACCONTROL");
	buff = buff + ETH_GSTRING_LEN;
	snprintf(buff, ETH_GSTRING_LEN, "GMAC_RX_UNKNOWN_MACCONTROL");
	buff = buff + ETH_GSTRING_LEN;
	snprintf(buff, ETH_GSTRING_LEN, "GMAC_RX_VERY_LONG_ERR");
	buff = buff + ETH_GSTRING_LEN;
	snprintf(buff, ETH_GSTRING_LEN, "GMAC_RX_RUNT_ERR");
	buff = buff + ETH_GSTRING_LEN;
	snprintf(buff, ETH_GSTRING_LEN, "GMAC_RX_SHORT_ERR");
	buff = buff + ETH_GSTRING_LEN;
	snprintf(buff, ETH_GSTRING_LEN, "GMAC_RX_FILT_PKT");
	buff = buff + ETH_GSTRING_LEN;
	snprintf(buff, ETH_GSTRING_LEN, "GMAC_RX_OCTETS_TOTAL_FILT");
	buff = buff + ETH_GSTRING_LEN;

	snprintf(buff, ETH_GSTRING_LEN, "GMAC_OCTETS_TRANSMITTED_OK");
	buff = buff + ETH_GSTRING_LEN;
	snprintf(buff, ETH_GSTRING_LEN, "GMAC_OCTETS_TRANSMITTED_BAD");
	buff = buff + ETH_GSTRING_LEN;
	snprintf(buff, ETH_GSTRING_LEN, "GMAC_TX_UC_PKTS");
	buff = buff + ETH_GSTRING_LEN;
	snprintf(buff, ETH_GSTRING_LEN, "GMAC_TX_MC_PKTS");
	buff = buff + ETH_GSTRING_LEN;
	snprintf(buff, ETH_GSTRING_LEN, "GMAC_TX_BC_PKTS");
	buff = buff + ETH_GSTRING_LEN;
	snprintf(buff, ETH_GSTRING_LEN, "GMAC_TX_PKTS_64OCTETS");
	buff = buff + ETH_GSTRING_LEN;
	snprintf(buff, ETH_GSTRING_LEN, "GMAC_TX_PKTS_65TO127");
	buff = buff + ETH_GSTRING_LEN;
	snprintf(buff, ETH_GSTRING_LEN, "GMAC_TX_PKTS_128TO255");
	buff = buff + ETH_GSTRING_LEN;
	snprintf(buff, ETH_GSTRING_LEN, "GMAC_TX_PKTS_255TO511");
	buff = buff + ETH_GSTRING_LEN;
	snprintf(buff, ETH_GSTRING_LEN, "GMAC_TX_PKTS_512TO1023");
	buff = buff + ETH_GSTRING_LEN;
	snprintf(buff, ETH_GSTRING_LEN, "GMAC_TX_PKTS_1024TO1518");
	buff = buff + ETH_GSTRING_LEN;
	snprintf(buff, ETH_GSTRING_LEN, "GMAC_TX_PKTS_1519TOMAX");
	buff = buff + ETH_GSTRING_LEN;
	snprintf(buff, ETH_GSTRING_LEN, "GMAC_TX_EXCESSIVE_LENGTH_DROP");
	buff = buff + ETH_GSTRING_LEN;
	snprintf(buff, ETH_GSTRING_LEN, "GMAC_TX_UNDERRUN");
	buff = buff + ETH_GSTRING_LEN;
	snprintf(buff, ETH_GSTRING_LEN, "GMAC_TX_TAGGED");
	buff = buff + ETH_GSTRING_LEN;
	snprintf(buff, ETH_GSTRING_LEN, "GMAC_TX_CRC_ERROR");
	buff = buff + ETH_GSTRING_LEN;
	snprintf(buff, ETH_GSTRING_LEN, "GMAC_TX_PAUSE_FRAMES");
	buff = buff + ETH_GSTRING_LEN;

	snprintf(buff, ETH_GSTRING_LEN, "GMAC_DUPLEX_TYPE");
	buff = buff + ETH_GSTRING_LEN;
	snprintf(buff, ETH_GSTRING_LEN, "GMAC_FD_FC_TYPE");
	buff = buff + ETH_GSTRING_LEN;
	snprintf(buff, ETH_GSTRING_LEN, "GMAC_FC_TX_TIMER");
	buff = buff + ETH_GSTRING_LEN;
	snprintf(buff, ETH_GSTRING_LEN, "GMAC_PAUSE_THR");
	buff = buff + ETH_GSTRING_LEN;
	snprintf(buff, ETH_GSTRING_LEN, "GMAC_MAX_FRM_SIZE");
	buff = buff + ETH_GSTRING_LEN;
	snprintf(buff, ETH_GSTRING_LEN, "GMAC_PORT_MODE");
	buff = buff + ETH_GSTRING_LEN;
	snprintf(buff, ETH_GSTRING_LEN, "GMAC_PORT_EN");
	buff = buff + ETH_GSTRING_LEN;
	snprintf(buff, ETH_GSTRING_LEN, "GMAC_PAUSE_EN");
	buff = buff + ETH_GSTRING_LEN;
	snprintf(buff, ETH_GSTRING_LEN, "GMAC_SHORT_RUNTS_THR");
	buff = buff + ETH_GSTRING_LEN;

	snprintf(buff, ETH_GSTRING_LEN, "GMAC_AN_NEG_STATE");
	buff = buff + ETH_GSTRING_LEN;
	snprintf(buff, ETH_GSTRING_LEN, "GMAC_TX_LOCAL_PAGE");
	buff = buff + ETH_GSTRING_LEN;
	snprintf(buff, ETH_GSTRING_LEN, "GMAC_TRANSMIT");
	buff = buff + ETH_GSTRING_LEN;
	snprintf(buff, ETH_GSTRING_LEN, "GMAC_REC_FILT_CONTROL");
	buff = buff + ETH_GSTRING_LEN;
	snprintf(buff, ETH_GSTRING_LEN, "GMAC_LINE_LOOP_BACK");
	buff = buff + ETH_GSTRING_LEN;
	snprintf(buff, ETH_GSTRING_LEN, "GMAC_CF_CRC_STRIP");
	buff = buff + ETH_GSTRING_LEN;
	snprintf(buff, ETH_GSTRING_LEN, "GMAC_MODE_CHANGE_EN");
	buff = buff + ETH_GSTRING_LEN;
	snprintf(buff, ETH_GSTRING_LEN, "GMAC_SIXTEEN_BIT_CNTR");
	buff = buff + ETH_GSTRING_LEN;
	snprintf(buff, ETH_GSTRING_LEN, "GMAC_LD_LINK_COUNTER");
	buff = buff + ETH_GSTRING_LEN;
	snprintf(buff, ETH_GSTRING_LEN, "GMAC_LOOP_REG");
	buff = buff + ETH_GSTRING_LEN;
	snprintf(buff, ETH_GSTRING_LEN, "GMAC_RECV_COMTROL");
	buff = buff + ETH_GSTRING_LEN;
	snprintf(buff, ETH_GSTRING_LEN, "GMAC_VLAN_CODE");
	buff = buff + ETH_GSTRING_LEN;
	snprintf(buff, ETH_GSTRING_LEN, "GMAC_RX_OVERRUN_CNT");
	buff = buff + ETH_GSTRING_LEN;
	snprintf(buff, ETH_GSTRING_LEN, "GMAC_RX_FAIL_COMMA");
}

static int hns_gmac_get_sset_count(int stringset)
{
	if (stringset == ETH_SS_STATS)
		return ETH_GMAC_STATIC_NUM;

	return 0;
}

static int hns_gmac_get_regs_count(void)
{
	return ETH_GMAC_DUMP_NUM;
}

void *hns_gmac_config(struct hns_mac_cb *mac_cb, struct mac_params *mac_param)
{
	struct mac_driver *mac_drv;

	mac_drv = devm_kzalloc(mac_cb->dev, sizeof(*mac_drv), GFP_KERNEL);
	if (!mac_drv)
		return NULL;

	mac_drv->mac_init = hns_gmac_init;
	mac_drv->mac_enable = hns_gmac_enable;
	mac_drv->mac_disable = hns_gmac_disable;
	mac_drv->mac_free = hns_gmac_free;
	mac_drv->adjust_link = hns_gmac_adjust_link;
	mac_drv->set_tx_auto_pause_frames = hns_gmac_set_tx_auto_pause_frames;
	mac_drv->config_max_frame_length = hns_gmac_config_max_frame_length;
	mac_drv->mac_pausefrm_cfg = hns_gmac_pause_frm_cfg;

	mac_drv->dsaf_id = mac_param->dsaf_id;
	mac_drv->mac_id = mac_param->mac_id;
	mac_drv->mac_mode = mac_param->mac_mode;
	mac_drv->io_base = mac_param->vaddr;
	mac_drv->dev = mac_param->dev;
	mac_drv->mac_cb = mac_cb;

	mac_drv->mac_reset = hns_gmac_reset;
	mac_drv->set_mac_addr = hns_gmac_set_mac_addr;
	mac_drv->set_an_mode = hns_gmac_config_an_mode;
	mac_drv->config_loopback = hns_gmac_config_loopback;
	mac_drv->config_pad_and_crc = hns_gmac_config_pad_and_crc;
	mac_drv->config_half_duplex = hns_gmac_set_duplex_type;
	mac_drv->set_rx_ignore_pause_frames = hns_gmac_set_rx_auto_pause_frames;
	mac_drv->mac_get_id = hns_gmac_get_id;
	mac_drv->get_info = hns_gmac_get_info;
	mac_drv->autoneg_stat = hns_gmac_autoneg_stat;
	mac_drv->get_pause_enable = hns_gmac_get_pausefrm_cfg;
	mac_drv->get_link_status = hns_gmac_get_link_status;
	mac_drv->get_regs = hns_gmac_get_regs;
	mac_drv->get_regs_count = hns_gmac_get_regs_count;
	mac_drv->get_ethtool_stats = hns_gmac_get_ethtool_stats;
	mac_drv->get_sset_count = hns_gmac_get_sset_count;
	mac_drv->get_strings = hns_gmac_get_strings;
	mac_drv->update_stats = hns_gmac_update_stats;

	return (void *)mac_drv;
}
