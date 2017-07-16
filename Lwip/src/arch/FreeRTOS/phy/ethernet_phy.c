 /**
 * \file
 *
 * \brief API driver for DM9161A PHY component.
 *
 * Copyright (c) 2012 Atmel Corporation. All rights reserved.
 *
 * \asf_license_start
 *
 * \page License
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * 3. The name of Atmel may not be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * 4. This software may only be redistributed and used in connection with an
 *    Atmel microcontroller product.
 *
 * THIS SOFTWARE IS PROVIDED BY ATMEL "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT ARE
 * EXPRESSLY AND SPECIFICALLY DISCLAIMED. IN NO EVENT SHALL ATMEL BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * \asf_license_stop
 *
 */

#include "ethernet_phy.h"
#include "emac.h"
#include "mii.h"
#include "SysDebug.h"

/// @cond 0
/**INDENT-OFF**/
#ifdef __cplusplus
extern "C" {
#endif
/**INDENT-ON**/
/// @endcond

/**
 * \defgroup dm9161a_ethernet_phy_group PHY component (DM9161A)
 *
 * Driver for the dm9161a component. This driver provides access to the main 
 * features of the PHY.
 *
 * \section dependencies Dependencies
 * This driver depends on the following modules:
 * - \ref emac_group Ethernet Media Access Controller (EMAC) module. 
 *
 * @{
 */

/* Max PHY number */
#define ETH_PHY_MAX_ADDR   31

/* Ethernet PHY operation max retry count */
#define ETH_PHY_RETRY_MAX 1000000

/* Ethernet PHY operation timeout */
#define ETH_PHY_TIMEOUT 10

/**
 * \brief Find a valid PHY Address ( from addrStart to 31 ).
 *
 * \param p_emac   Pointer to the EMAC instance. 
 * \param uc_phy_addr PHY address.
 * \param uc_start_addr Start address of the PHY to be searched. 
 *
 * \return 0xFF when no valid PHY address is found. 
 */
static uint8_t ethernet_phy_find_valid(Emac *p_emac, uint8_t uc_phy_addr,
		uint8_t addrStart)
{
	uint32_t ul_value = 0;
	uint8_t uc_rc;
	uint8_t uc_cnt;
	uint8_t uc_phy_address = uc_phy_addr;

	emac_enable_management(p_emac, true);

	/* Check the current PHY address */
	uc_rc = uc_phy_address;
	if (emac_phy_read(p_emac, uc_phy_addr, MII_PHYID1, &ul_value) != EMAC_OK) {
	}

	/* Find another one */
	if (ul_value == 0xffff) {//(ul_value != MII_OUI_MSB) {    //支持DM9161，支持8720
		uc_rc = 0xFF;
		for (uc_cnt = addrStart; uc_cnt <= ETH_PHY_MAX_ADDR; uc_cnt++) {
			uc_phy_address = (uc_phy_address + 1) & 0x1F;
			emac_phy_read(p_emac, uc_phy_address, MII_PHYID1, &ul_value);
			if (ul_value != 0xffff) {//(ul_value == MII_OUI_MSB) {
				uc_rc = uc_phy_address;
				break;
			}
		}
	}

	emac_enable_management(p_emac, false);

	if (uc_rc != 0xFF) {
        //DTRACE(0, ("** Valid PHY Found: %d\n\r", uc_rc));
		emac_phy_read(p_emac, uc_phy_address, MII_DSCSR, &ul_value);
	}
	return uc_rc;
}


/**
 * \brief Perform a HW initialization to the PHY ( via RSTC ) and set up clocks.
 *
 * This should be called only once to initialize the PHY pre-settings.
 * The PHY address is the reset status of CRS, RXD[3:0] (the emacPins' pullups).
 * The COL pin is used to select MII mode on reset (pulled up for Reduced MII).
 * The RXDV pin is used to select test mode on reset (pulled up for test mode).
 * The above pins should be predefined for corresponding settings in resetPins.
 * The EMAC peripheral pins are configured after the reset is done.
 *
 * \param p_emac   Pointer to the EMAC instance. 
 * \param uc_phy_addr PHY address.
 * \param ul_mck EMAC MCK. 
 *
 * Return EMAC_OK if successfully, EMAC_TIMEOUT if timeout.
 */
uint8_t ethernet_phy_init(Emac *p_emac, uint8_t uc_phy_addr, uint32_t mck)
{
	uint8_t uc_rc = EMAC_TIMEOUT;
	uint8_t uc_phy;

	/* Configure EMAC runtime clock */
	uc_rc = emac_set_clock(p_emac, mck);
	if (uc_rc != EMAC_OK) {
		return EMAC_INVALID;//0;
	}

	/* Check PHY Address */
	uc_phy = ethernet_phy_find_valid(p_emac, uc_phy_addr, 0);
	if (uc_phy == 0xFF) {
        //DTRACE(0, ("PHY Access fail\n\r"));
		return EMAC_INVALID;//0;
	}
	if (uc_phy != uc_phy_addr) {
        //DTRACE(0, ("PHY Address config error\n\r"));
		ethernet_phy_reset(p_emac, uc_phy_addr);
	}
    
    //LAN8720
    //ethernet_phy_set_powerdown(p_emac, uc_phy_addr);

	return uc_rc;
}


/**
 * \brief Get the Link & speed settings, and automatically set up the EMAC with the
 * settings.
 *
 * \param p_emac   Pointer to the EMAC instance. 
 * \param uc_phy_addr PHY address.
 * \param uc_apply_setting_flag Set to 0 to not apply the PHY configurations, else to apply.
 *
 * Return EMAC_OK if successfully, EMAC_TIMEOUT if timeout. 
 */
uint8_t ethernet_phy_set_link(Emac *p_emac, uint8_t uc_phy_addr,
		uint8_t uc_apply_setting_flag)
{
	uint32_t ul_stat1;
	uint32_t ul_stat2;
	uint8_t uc_phy_address, uc_speed, uc_fd;
	uint8_t uc_rc = EMAC_TIMEOUT;

	emac_enable_management(p_emac, true);

	uc_phy_address = uc_phy_addr;

	uc_rc = emac_phy_read(p_emac, uc_phy_address, MII_BMSR, &ul_stat1);
	if (uc_rc != EMAC_OK) {
		/* Disable PHY management and start the EMAC transfer */
		emac_enable_management(p_emac, false);

		return uc_rc;
	}

	if ((ul_stat1 & MII_LINK_STATUS) == 0) {
		/* Disable PHY management and start the EMAC transfer */
		emac_enable_management(p_emac, false);

		return EMAC_INVALID;
	}

	if (uc_apply_setting_flag == 0) {
		/* Disable PHY management and start the EMAC transfer */
		emac_enable_management(p_emac, false);

		return uc_rc;
	}

	/* Re-configure Link speed */
	uc_rc = emac_phy_read(p_emac, uc_phy_address, MII_DSCSR, &ul_stat2);
	if (uc_rc != EMAC_OK) {
		/* Disable PHY management and start the EMAC transfer */
		emac_enable_management(p_emac, false);

		return uc_rc;
	}

	if ((ul_stat1 & MII_100BASE_TX_FD) && (ul_stat2 & MII_100FDX)) {
		/* Set EMAC for 100BaseTX and Full Duplex */
		uc_speed = true;
		uc_fd = true;
	}

	if ((ul_stat1 & MII_10BASE_T_FD) && (ul_stat2 & MII_10FDX)) {
		/* Set MII for 10BaseT and Full Duplex */
		uc_speed = false;
		uc_fd = true;
	}

	if ((ul_stat1 & MII_100BASE_T4_HD) && (ul_stat2 & MII_100HDX)) {
		/* Set MII for 100BaseTX and Half Duplex */
		uc_speed = true;
		uc_fd = false;
	}

	if ((ul_stat1 & MII_10BASE_T_HD) && (ul_stat2 & MII_10HDX)) {
		/* Set MII for 10BaseT and Half Duplex */
		uc_speed = false;
		uc_fd = false;
	}

	emac_set_speed(p_emac, uc_speed);
	emac_enable_full_duplex(p_emac, uc_fd);

	/* Start the EMAC transfers */
	emac_enable_management(p_emac, false);
	return uc_rc;
}


/**
 * \brief Issue an auto negotiation of the PHY.
 *
 * \param p_emac   Pointer to the EMAC instance. 
 * \param uc_phy_addr PHY address.
 *
 * Return EMAC_OK if successfully, EMAC_TIMEOUT if timeout. 
 */
uint8_t ethernet_phy_auto_negotiate(Emac *p_emac, uint8_t uc_phy_addr)
{
	uint32_t ul_retry_max = ETH_PHY_RETRY_MAX;
	uint32_t ul_value;
	uint32_t ul_phy_anar;
	uint32_t ul_phy_analpar;
	uint32_t ul_retry_count = 0;
	uint8_t uc_fd = 0;
	uint8_t uc_speed = 0;
	uint8_t uc_rc = EMAC_TIMEOUT;

	emac_enable_management(p_emac, true);

	/* Set up control register */
	uc_rc = emac_phy_read(p_emac, uc_phy_addr, MII_BMCR, &ul_value);
	if (uc_rc != EMAC_OK) {
		emac_enable_management(p_emac, false);
		return uc_rc;
	}

	ul_value &= ~MII_AUTONEG; /* Remove auto-negotiation enable */
	ul_value &= ~(MII_LOOPBACK | MII_POWER_DOWN);
	ul_value |= MII_ISOLATE; /* Electrically isolate PHY */
	uc_rc = emac_phy_write(p_emac, uc_phy_addr, MII_BMCR, ul_value);
	if (uc_rc != EMAC_OK) {
		emac_enable_management(p_emac, false);
		return uc_rc;
	}

	/* 
	 * Set the Auto_negotiation Advertisement Register.
	 * MII advertising for Next page.
	 * 100BaseTxFD and HD, 10BaseTFD and HD, IEEE 802.3.
	 */
	ul_phy_anar = MII_TX_FDX | MII_TX_HDX | MII_10_FDX | MII_10_HDX | 
			MII_AN_IEEE_802_3;
	uc_rc = emac_phy_write(p_emac, uc_phy_addr, MII_ANAR, ul_phy_anar);
	if (uc_rc != EMAC_OK) {
		emac_enable_management(p_emac, false);
		return uc_rc;
	}

	/* Read & modify control register */
	uc_rc = emac_phy_read(p_emac, uc_phy_addr, MII_BMCR, &ul_value);
	if (uc_rc != EMAC_OK) {
		emac_enable_management(p_emac, false);
		return uc_rc;
	}

	ul_value |= MII_SPEED_SELECT | MII_AUTONEG | MII_DUPLEX_MODE;
	uc_rc = emac_phy_write(p_emac, uc_phy_addr, MII_BMCR, ul_value);
	if (uc_rc != EMAC_OK) {
		emac_enable_management(p_emac, false);
		return uc_rc;
	}

	/* Restart auto negotiation */
	ul_value |= MII_RESTART_AUTONEG;
	ul_value &= ~MII_ISOLATE;
	uc_rc = emac_phy_write(p_emac, uc_phy_addr, MII_BMCR, ul_value);
	if (uc_rc != EMAC_OK) {
		emac_enable_management(p_emac, false);
		return uc_rc;
	}

	/* Check if auto negotiation is completed */
	while (1) {
		uc_rc = emac_phy_read(p_emac, uc_phy_addr, MII_BMSR, &ul_value);
		if (uc_rc != EMAC_OK) {
			emac_enable_management(p_emac, false);
			return uc_rc;
		}
		/* Done successfully */
		if (ul_value & MII_AUTONEG_COMP) {
            DTRACE(0, ("AutoNegotiate complete\n\r"));
			break;
		}

		/* Timeout check */
		if (ul_retry_max) {
			if (++ul_retry_count >= ul_retry_max) {
				emac_enable_management(p_emac, false);
				return EMAC_TIMEOUT;
			}
		}
	}

	/* Get the auto negotiate link partner base page */
	uc_rc = emac_phy_read(p_emac, uc_phy_addr, MII_ANLPAR, &ul_phy_analpar);
	if (uc_rc != EMAC_OK) {
		emac_enable_management(p_emac, false);
		return uc_rc;
	}

	/* Set up the EMAC link speed */
	if ((ul_phy_anar & ul_phy_analpar) & MII_TX_FDX) {
		/* Set MII for 100BaseTX and Full Duplex */
		uc_speed = true;
		uc_fd = true;
	} else if ((ul_phy_anar & ul_phy_analpar) & MII_10_FDX) {
		/* Set MII for 10BaseT and Full Duplex */
		uc_speed = false;
		uc_fd = true;
	} else if ((ul_phy_anar & ul_phy_analpar) & MII_TX_HDX) {
		/* Set MII for 100BaseTX and half Duplex */
		uc_speed = true;
		uc_fd = false;
	} else if ((ul_phy_anar & ul_phy_analpar) & MII_10_HDX) {
		/* Set MII for 10BaseT and half Duplex */
		uc_speed = false;
		uc_fd = false;
	}

	emac_set_speed(p_emac, uc_speed);
	emac_enable_full_duplex(p_emac, uc_fd);

	emac_enable_rmii(p_emac, ETH_PHY_MODE);
	emac_enable_transceiver_clock(p_emac, true);

	emac_enable_management(p_emac, false);
	return uc_rc;
}

/**
 * \brief Issue a SW reset to reset all registers of the PHY.
 *
 * \param p_emac   Pointer to the EMAC instance. 
 * \param uc_phy_addr PHY address.
 *
 * \Return EMAC_OK if successfully, EMAC_TIMEOUT if timeout.
 */
uint8_t ethernet_phy_reset(Emac *p_emac, uint8_t uc_phy_addr)
{
	uint32_t ul_bmcr = MII_RESET;
	uint8_t uc_phy_address = uc_phy_addr;
	uint32_t ul_timeout = ETH_PHY_TIMEOUT;
	uint8_t uc_rc = EMAC_TIMEOUT;

	emac_enable_management(p_emac, true);

	ul_bmcr = MII_RESET;
	emac_phy_write(p_emac, uc_phy_address, MII_BMCR, ul_bmcr);

	do {
		emac_phy_read(p_emac, uc_phy_address, MII_BMCR, &ul_bmcr);
		ul_timeout--;
	} while ((ul_bmcr & MII_RESET) && ul_timeout);

	emac_enable_management(p_emac, false);

	if (!ul_timeout) {
		uc_rc = EMAC_OK;
	}

	return (uc_rc);
}

//设置POWER DOWN 模式  节电
uint8_t ethernet_phy_set_powerdown(Emac *p_emac, uint8_t uc_phy_addr)
{
    uint32_t ul_value;
    uint8_t uc_rc = EMAC_TIMEOUT;
    
    emac_enable_management(p_emac, true);
    
    uc_rc = emac_phy_read(p_emac, uc_phy_addr, MII_DSCSR, &ul_value);
	if (uc_rc != EMAC_OK) {
		emac_enable_management(p_emac, false);
		return uc_rc;
	}
    
    ul_value |= MII_EDPWRDOWN;  //打开POWERDOWN 模式
	uc_rc = emac_phy_write(p_emac, uc_phy_addr, MII_DSCSR, ul_value);
    if (uc_rc != EMAC_OK) {
		emac_enable_management(p_emac, false);
		return uc_rc;
	}
    
    emac_enable_management(p_emac, false);
    
    return uc_rc;
}

/// @cond 0
/**INDENT-OFF**/
#ifdef __cplusplus
}
#endif
/**INDENT-ON**/
/// @endcond

/**
 * \}
 */
