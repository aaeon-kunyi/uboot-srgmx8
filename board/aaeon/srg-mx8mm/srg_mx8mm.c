// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright 2020 LYD Inc.
 */
#include <common.h>
#include <miiphy.h>
#include <netdev.h>
#include <asm/mach-imx/iomux-v3.h>
#include <asm-generic/gpio.h>
#include <asm/arch/imx8mm_pins.h>
#include <asm/arch/clock.h>
#include <asm/arch/sys_proto.h>
#include <asm/mach-imx/gpio.h>
#include <asm/mach-imx/mxc_i2c.h>
#include <i2c.h>
#include <asm/io.h>
#include <usb.h>

DECLARE_GLOBAL_DATA_PTR;

#define UART_PAD_CTRL	(PAD_CTL_DSE6 | PAD_CTL_FSEL1)
#define WDOG_PAD_CTRL	(PAD_CTL_DSE6 | PAD_CTL_ODE | PAD_CTL_PUE | PAD_CTL_PE)

static iomux_v3_cfg_t const uart_pads[] = {
    IMX8MM_PAD_UART2_RXD_UART2_RX | MUX_PAD_CTRL(UART_PAD_CTRL),
    IMX8MM_PAD_UART2_TXD_UART2_TX | MUX_PAD_CTRL(UART_PAD_CTRL),
};

static iomux_v3_cfg_t const wdog_pads[] = {
    IMX8MM_PAD_GPIO1_IO02_WDOG1_WDOG_B  | MUX_PAD_CTRL(WDOG_PAD_CTRL),
};

int board_early_init_f(void)
{
    struct wdog_regs *wdog = (struct wdog_regs *)WDOG1_BASE_ADDR;

    imx_iomux_v3_setup_multiple_pads(wdog_pads, ARRAY_SIZE(wdog_pads));

    set_wdog_reset(wdog);

    imx_iomux_v3_setup_multiple_pads(uart_pads, ARRAY_SIZE(uart_pads));

    init_uart_clk(1);

    return 0;
}

#if IS_ENABLED(CONFIG_FEC_MXC)
#define FEC_RST_PAD IMX_GPIO_NR(4, 22)
static iomux_v3_cfg_t const fec1_rst_pads[] = {
    IMX8MM_PAD_SAI2_RXC_GPIO4_IO22 | MUX_PAD_CTRL(NO_PAD_CTRL),
};

static void setup_iomux_fec(void)
{
    imx_iomux_v3_setup_multiple_pads(fec1_rst_pads,
                     ARRAY_SIZE(fec1_rst_pads));

    gpio_request(FEC_RST_PAD, "fec1_rst");
    gpio_direction_output(FEC_RST_PAD, 0);
    mdelay(15);
    gpio_direction_output(FEC_RST_PAD, 1);
    mdelay(20);
}

static int setup_fec(void)
{
    struct iomuxc_gpr_base_regs *gpr =
        (struct iomuxc_gpr_base_regs *)IOMUXC_GPR_BASE_ADDR;

    setup_iomux_fec();

    /* Use 125M anatop REF_CLK1 for ENET1, not from external */
    clrsetbits_le32(&gpr->gpr[1],
            BIT(13) | BIT(17), 0);
    udelay(100); /* wait clock */
    return 0;
}

int board_phy_config(struct phy_device *phydev)
{
    /* enable rgmii rxc skew and phy mode select to RGMII copper */
    phy_write(phydev, MDIO_DEVAD_NONE, 0x1d, 0x1f);
    phy_write(phydev, MDIO_DEVAD_NONE, 0x1e, 0x8);

    phy_write(phydev, MDIO_DEVAD_NONE, 0x1d, 0x00);
    phy_write(phydev, MDIO_DEVAD_NONE, 0x1e, 0x82ee);
    phy_write(phydev, MDIO_DEVAD_NONE, 0x1d, 0x05);
    phy_write(phydev, MDIO_DEVAD_NONE, 0x1e, 0x100);

    if (phydev->drv->config)
        phydev->drv->config(phydev);
    return 0;
}
#endif

#ifdef CONFIG_USB_EHCI_HCD
int board_usb_init(int index, enum usb_init_type init)
{
	int ret = 0;

	printf("board_usb_init %d, type %d\n", index, init);

	imx8m_usb_power(index, true);

	return ret;
}

int board_usb_cleanup(int index, enum usb_init_type init)
{
	int ret = 0;

	debug("board_usb_cleanup %d, type %d\n", index, init);

	imx8m_usb_power(index, false);
	return ret;
}
#endif /* end of CONFIG_USB_EHCI_HCD */

#define FSL_SIP_GPC			0xC2000000
#define FSL_SIP_CONFIG_GPC_PM_DOMAIN	0x3
#define DISPMIX				9
#define MIPI				10

int board_init(void)
{
    if (IS_ENABLED(CONFIG_FEC_MXC))
        setup_fec();
    /* call SMC */
    call_imx_sip(FSL_SIP_GPC, FSL_SIP_CONFIG_GPC_PM_DOMAIN, DISPMIX, true, 0);
    call_imx_sip(FSL_SIP_GPC, FSL_SIP_CONFIG_GPC_PM_DOMAIN, MIPI, true, 0);

    return 0;
}

int board_late_init(void)
{
#ifdef CONFIG_ENV_IS_IN_MMC
    board_late_mmc_env_init();
#endif

#ifdef CONFIG_ENV_VARS_UBOOT_RUNTIME_CONFIG
    env_set("board_name", "SRG-MX8MM");
    env_set("board_rev", "DEV1");
#endif
    return 0;
}

#ifdef CONFIG_FSL_FASTBOOT
#ifdef CONFIG_ANDROID_RECOVERY
int is_recovery_key_pressing(void)
{
    return 0; /*TODO*/
}
#endif /*CONFIG_ANDROID_RECOVERY*/
#endif /*CONFIG_FSL_FASTBOOT*/

#ifdef CONFIG_ANDROID_SUPPORT
bool is_power_key_pressed(void) {
    return (bool)(!!(readl(SNVS_HPSR) & (0x1 << 6)));
}
#endif
