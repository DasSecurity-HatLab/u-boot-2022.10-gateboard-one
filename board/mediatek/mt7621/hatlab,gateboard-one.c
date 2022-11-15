#include "hatlab,gateboard-one.h"

#include <common.h>
#include <asm/io.h>
#include <asm/addrspace.h>

/*
    WDT_RST_N	GPIO	18
    MDIO		GPIO	20
    MDC			GPIO	21
*/

volatile static void _udelay(unsigned int usec)
{
    usec = (((F_CPU / 1000000) * usec) / 4) + 1;
    while (--usec)
        __asm__ __volatile__("");
}

static void _set_mdio_data(int val)
{
    void __iomem *base_gpio;

    base_gpio = (void __iomem *)CKSEG1ADDR(MT7621_GPIO_BASE);

    if (val)
        setbits_le32(base_gpio + MT7621_GPIO_DATA_0_REG, (1 << 20));
    else
        clrbits_le32(base_gpio + MT7621_GPIO_DATA_0_REG, (1 << 20));
}

static int _get_mdio_data(void)
{
    void __iomem *base_gpio;
    u32 data;

    base_gpio = (void __iomem *)CKSEG1ADDR(MT7621_GPIO_BASE);
    data = readl(base_gpio + MT7621_GPIO_DATA_0_REG);

    return (data & (1 << 20)) ? 1 : 0;
}

static void _set_mdc(int val)
{
    void __iomem *base_gpio;

    base_gpio = (void __iomem *)CKSEG1ADDR(MT7621_GPIO_BASE);

    if (val)
        setbits_le32(base_gpio + MT7621_GPIO_DATA_0_REG, (1 << 21));
    else
        clrbits_le32(base_gpio + MT7621_GPIO_DATA_0_REG, (1 << 21));
}

static void _set_mdio_dir(int val)
{
    void __iomem *base_gpio;

    base_gpio = (void __iomem *)CKSEG1ADDR(MT7621_GPIO_BASE);

    if (val)
        setbits_le32(base_gpio + MT7621_GPIO_CTRL_0_REG, (1 << 20));
    else
        clrbits_le32(base_gpio + MT7621_GPIO_CTRL_0_REG, (1 << 20));
}

/* MDIO must already be configured as output. */
static void _mdiobb_send_bit(int val)
{
    _set_mdio_data(val);
    _udelay(MDIO_DELAY);
    _set_mdc(1);
    _udelay(MDIO_DELAY);
    _set_mdc(0);
}

/* MDIO must already be configured as input. */
static int _mdiobb_get_bit(void)
{
    _udelay(MDIO_DELAY);
    _set_mdc(1);
    _udelay(MDIO_READ_DELAY);
    _set_mdc(0);

    return _get_mdio_data();
}

/* MDIO must already be configured as output. */
static void _mdiobb_send_num(u16 val, int bits)
{
    int i;

    for (i = bits - 1; i >= 0; i--)
        _mdiobb_send_bit((val >> i) & 1);
}

/* MDIO must already be configured as input. */
static u16 _mdiobb_get_num(int bits)
{
    int i;
    u16 ret = 0;

    for (i = bits - 1; i >= 0; i--)
    {
        ret <<= 1;
        ret |= _mdiobb_get_bit();
    }

    return ret;
}

/* Utility to send the preamble, address, and
 * register (common to read and write).
 */
static void _mdiobb_cmd(int read, u8 phy, u8 reg)
{
    int i;

    _set_mdio_dir(1);

    /*
     * Send a 32 bit preamble ('1's) with an extra '1' bit for good
     * measure.  The IEEE spec says this is a PHY optional
     * requirement.  The AMD 79C874 requires one after power up and
     * one after a MII communications error.  This means that we are
     * doing more preambles than we need, but it is safer and will be
     * much more robust.
     */

    for (i = 0; i < 32; i++)
        _mdiobb_send_bit(1);

    /* send the start bit (01) and the read opcode (10) or write (10) */
    _mdiobb_send_bit(0);
    _mdiobb_send_bit(1);
    _mdiobb_send_bit(read);
    _mdiobb_send_bit(!read);

    _mdiobb_send_num(phy, 5);
    _mdiobb_send_num(reg, 5);
}

static int _mdiobb_read(int phy, int reg)
{
    int ret, i;

    _mdiobb_cmd(MDIO_READ, phy, reg);
    _set_mdio_dir(0);

    /* check the turnaround bit: the PHY should be driving it to zero */
    if (_mdiobb_get_bit() != 0)
    {
        /* PHY didn't drive TA low -- flush any bits it
         * may be trying to send.
         */
        for (i = 0; i < 32; i++)
            _mdiobb_get_bit();

        return 0xffff;
    }

    ret = _mdiobb_get_num(16);
    _mdiobb_get_bit();
    return ret;
}

static int _mdiobb_write(int phy, int reg, u16 val)
{
    _mdiobb_cmd(MDIO_WRITE, phy, reg);

    /* send the turnaround (10) */
    _mdiobb_send_bit(1);
    _mdiobb_send_bit(0);

    _mdiobb_send_num(val, 16);

    _set_mdio_dir(0);
    _mdiobb_get_bit();
    return 0;
}

static void _mdiobb_init(void)
{
    void __iomem *base_sysctl, *base_gpio;

    base_sysctl = (void __iomem *)CKSEG1ADDR(MT7621_SYSCTL_BASE);
    base_gpio = (void __iomem *)CKSEG1ADDR(MT7621_GPIO_BASE);

    // Set MDC/MDIO as GPIO
    setbits_le32(base_sysctl + MT7621_SYS_GPIO_MODE_REG, REG_SET_VAL(MDIO_MODE, 1));
    // Set MDC as output GPIO
    setbits_le32(base_gpio + MT7621_GPIO_CTRL_0_REG, (1 << 21));
}

static void _mdiobb_exit(void)
{
    void __iomem *base_sysctl, *base_gpio;

    base_sysctl = (void __iomem *)CKSEG1ADDR(MT7621_SYSCTL_BASE);
    base_gpio = (void __iomem *)CKSEG1ADDR(MT7621_GPIO_BASE);

    // Set MDC as float GPIO
    clrbits_le32(base_gpio + MT7621_GPIO_CTRL_0_REG, (1 << 21));
    // Set MDC/MDIO back to MDIO
    setbits_le32(base_sysctl + MT7621_SYS_GPIO_MODE_REG, REG_SET_VAL(MDIO_MODE, 0));
}

void hatlab_gateboard_one_early_init(void)
{
    u16 phy_val;
    void __iomem *base_sysctl, *base_gpio;

    base_sysctl = (void __iomem *)CKSEG1ADDR(MT7621_SYSCTL_BASE);
    base_gpio = (void __iomem *)CKSEG1ADDR(MT7621_GPIO_BASE);

    // Reset MT7530
    setbits_le32(base_sysctl + MT7621_SYS_RSTCTL_REG, REG_SET_VAL(MCM_RST, 1));
    _udelay(10);
    clrbits_le32(base_sysctl + MT7621_SYS_RSTCTL_REG, REG_MASK(MCM_RST));
    _udelay(10);

    // Reset PIO
    setbits_le32(base_sysctl + MT7621_SYS_RSTCTL_REG, REG_SET_VAL(PIO_RST, 1));
    _udelay(10);
    clrbits_le32(base_sysctl + MT7621_SYS_RSTCTL_REG, REG_MASK(PIO_RST));
    _udelay(10);

    // Light up system led
    setbits_le32(base_gpio + MT7621_GPIO_CTRL_0_REG, (1 << 0));
    setbits_le32(base_gpio + MT7621_GPIO_DATA_0_REG, (1 << 0));

    // Set WDT_RST_N as output GPIO
    setbits_le32(base_sysctl + MT7621_SYS_GPIO_MODE_REG, REG_SET_VAL(WDT_MODE, 1));
    setbits_le32(base_gpio + MT7621_GPIO_CTRL_0_REG, (1 << 18));

    // Set RGMII2 as float GPIO for PHY bootstrap pin
    setbits_le32(base_sysctl + MT7621_SYS_GPIO_MODE_REG, REG_SET_VAL(RGMII2_MODE, 1));

    // WDT_RST_N = 1, NMOS drain, Peripherals RST assert, Peripherals DC-DC turn off
    setbits_le32(base_gpio + MT7621_GPIO_DATA_0_REG, (1 << 18));

    // Delay for peripherals power down
    _udelay(500000);

    // WDT_RST_N = 0, NMOS off, Peripherals RST deassert, Peripherals DC-DC turn on
    clrbits_le32(base_gpio + MT7621_GPIO_DATA_0_REG, (1 << 18));

    // Delay for peripherals power on
    _udelay(400000);

    // Set RGMII2 back to RGMII
    clrbits_le32(base_sysctl + MT7621_SYS_GPIO_MODE_REG, REG_MASK(RGMII2_MODE));

    // According to RTL8211FS datasheet, use bitbang MDIO turn off broadcast phy addr 0 before MT7530 init
    _mdiobb_init();
    phy_val = _mdiobb_read(7, 0x18);
    phy_val &= ~(1 << 13);
    _mdiobb_write(7, 0x18, phy_val);
    _mdiobb_exit();
}
