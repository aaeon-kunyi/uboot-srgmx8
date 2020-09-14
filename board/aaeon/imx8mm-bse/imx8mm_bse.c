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

#define FEC_RST_PAD IMX_GPIO_NR(4, 22)
static iomux_v3_cfg_t const fec1_rst_pads[] = {
    IMX8MM_PAD_SAI2_RXC_GPIO4_IO22 | MUX_PAD_CTRL(NO_PAD_CTRL),
};

#define ECSPI_PAD_CTRL	(PAD_CTL_DSE6 | PAD_CTL_FSEL2 | PAD_CTL_HYS)
static iomux_v3_cfg_t const tpm_spi_pads[] = {
	IMX8MM_PAD_ECSPI2_SCLK_ECSPI2_SCLK | MUX_PAD_CTRL(ECSPI_PAD_CTRL),
	IMX8MM_PAD_ECSPI2_MOSI_ECSPI2_MOSI | MUX_PAD_CTRL(ECSPI_PAD_CTRL),
	IMX8MM_PAD_ECSPI2_MISO_ECSPI2_MISO | MUX_PAD_CTRL(ECSPI_PAD_CTRL),
	IMX8MM_PAD_ECSPI2_SS0_ECSPI2_SS0 | MUX_PAD_CTRL(NO_PAD_CTRL),
};

#define TPM_RST_PAD IMX_GPIO_NR(3, 24)
static iomux_v3_cfg_t const tpm_rst_pads[] = {
	IMX8MM_PAD_SAI5_RXD3_GPIO3_IO24 | MUX_PAD_CTRL(NO_PAD_CTRL),
};

#define USB1_VBUS_PAD	 IMX_GPIO_NR(1, 3)
#define USB2_VBUS_PAD	 IMX_GPIO_NR(1, 1)
#define USB3_VBUS_PAD	 IMX_GPIO_NR(1, 5)
#define USB4_VBUS_PAD	 IMX_GPIO_NR(1, 6)
#define USB5_VBUS_PAD	 IMX_GPIO_NR(1, 7)
#define USB6_VBUS_PAD	 IMX_GPIO_NR(1, 8)
static iomux_v3_cfg_t const vbus_pads[] = {
	IMX8MM_PAD_GPIO1_IO03_GPIO1_IO3 | MUX_PAD_CTRL(NO_PAD_CTRL),
	IMX8MM_PAD_GPIO1_IO01_GPIO1_IO1 | MUX_PAD_CTRL(NO_PAD_CTRL),
	IMX8MM_PAD_GPIO1_IO05_GPIO1_IO5 | MUX_PAD_CTRL(NO_PAD_CTRL),
	IMX8MM_PAD_GPIO1_IO06_GPIO1_IO6 | MUX_PAD_CTRL(NO_PAD_CTRL),
	IMX8MM_PAD_GPIO1_IO07_GPIO1_IO7 | MUX_PAD_CTRL(NO_PAD_CTRL),
	IMX8MM_PAD_GPIO1_IO08_GPIO1_IO8 | MUX_PAD_CTRL(NO_PAD_CTRL),
};

#define BOARD_VERSEL1  IMX_GPIO_NR(4, 24)
#define BOARD_VERSEL2  IMX_GPIO_NR(4, 25)
static iomux_v3_cfg_t const brdver_pads[] = {
	IMX8MM_PAD_SAI2_TXFS_GPIO4_IO24 | MUX_PAD_CTRL(NO_PAD_CTRL),
	IMX8MM_PAD_SAI2_TXC_GPIO4_IO25 | MUX_PAD_CTRL(NO_PAD_CTRL),
};

static unsigned int bse_board_version(void)
{
    unsigned int ret = 0;
    imx_iomux_v3_setup_multiple_pads(brdver_pads,
        ARRAY_SIZE(brdver_pads));
    gpio_request(BOARD_VERSEL1, "brd_vsel1");
    gpio_request(BOARD_VERSEL2, "brd_vsel2");
    gpio_direction_input(BOARD_VERSEL1);
    gpio_direction_input(BOARD_VERSEL2);

	if (gpio_get_value(BOARD_VERSEL1) > 0)
        ret |= (1 << 0);
	if (gpio_get_value(BOARD_VERSEL2) > 0)
        ret |= (1 << 1);

	gpio_free(BOARD_VERSEL1);
	gpio_free(BOARD_VERSEL2);
	return ret;
}

static void bse_board_reset(void)
{
    imx_iomux_v3_setup_multiple_pads(fec1_rst_pads,
        ARRAY_SIZE(fec1_rst_pads));
    imx_iomux_v3_setup_multiple_pads(tpm_spi_pads,
        ARRAY_SIZE(tpm_spi_pads));
    imx_iomux_v3_setup_multiple_pads(tpm_rst_pads,
        ARRAY_SIZE(tpm_rst_pads));
    imx_iomux_v3_setup_multiple_pads(vbus_pads,
        ARRAY_SIZE(vbus_pads));

    gpio_request(FEC_RST_PAD, "fec1_rst");
    gpio_direction_output(FEC_RST_PAD, 0);
    gpio_request(TPM_RST_PAD, "tpm_rst");
    gpio_direction_output(TPM_RST_PAD, 0);
    gpio_request(USB1_VBUS_PAD, "usb1_vbus");
    gpio_direction_output(USB1_VBUS_PAD, 1);
    gpio_request(USB2_VBUS_PAD, "usb2_vbus");
    gpio_direction_output(USB2_VBUS_PAD, 1);
    gpio_request(USB3_VBUS_PAD, "usb3_vbus");
    gpio_direction_output(USB3_VBUS_PAD, 1);
    gpio_request(USB4_VBUS_PAD, "usb4_vbus");
    gpio_direction_output(USB4_VBUS_PAD, 1);
    gpio_request(USB5_VBUS_PAD, "usb5_vbus");
    gpio_direction_output(USB5_VBUS_PAD, 1);
    gpio_request(USB6_VBUS_PAD, "usb6_vbus");
    gpio_direction_output(USB6_VBUS_PAD, 1);
    mdelay(15);
    gpio_direction_output(FEC_RST_PAD, 1);
    gpio_direction_output(TPM_RST_PAD, 1);
    gpio_direction_output(USB1_VBUS_PAD, 0);
    gpio_direction_output(USB2_VBUS_PAD, 0);
    gpio_direction_output(USB2_VBUS_PAD, 0);
    gpio_direction_output(USB2_VBUS_PAD, 0);
    gpio_direction_output(USB2_VBUS_PAD, 0);
    gpio_direction_output(USB2_VBUS_PAD, 0);
    gpio_free(TPM_RST_PAD);
    mdelay(20);
}

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
static int setup_fec(void)
{
    struct iomuxc_gpr_base_regs *gpr =
        (struct iomuxc_gpr_base_regs *)IOMUXC_GPR_BASE_ADDR;

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

#define FSL_SIP_GPC			0xC2000000
#define FSL_SIP_CONFIG_GPC_PM_DOMAIN	0x3
#define DISPMIX				9
#define MIPI				10

int board_init(void)
{
    unsigned long ver = 0;
    bse_board_reset();

    if (IS_ENABLED(CONFIG_FEC_MXC))
        setup_fec();
    /* call SMC */
    call_imx_sip(FSL_SIP_GPC, FSL_SIP_CONFIG_GPC_PM_DOMAIN, DISPMIX, true, 0);
    call_imx_sip(FSL_SIP_GPC, FSL_SIP_CONFIG_GPC_PM_DOMAIN, MIPI, true, 0);

	ver = bse_board_version();
	env_set_ulong("brdver", ver);
    return 0;
}

int board_late_init(void)
{
    unsigned long ver = 0;    
#ifdef CONFIG_ENV_IS_IN_MMC
    board_late_mmc_env_init();
#endif

#ifdef CONFIG_ENV_VARS_UBOOT_RUNTIME_CONFIG
    env_set("board_name", "IMX8MM-BSE");
    env_set("board_rev", "DVT");
	ver = bse_board_version();
	env_set_ulong("brdver", ver);    
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
