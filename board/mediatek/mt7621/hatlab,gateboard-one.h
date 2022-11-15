#pragma once

#define F_CPU 500000000L

#define MT7621_SYSCTL_BASE 0x1e000000
#define MT7621_GPIO_BASE 0x1e000600

#define MT7621_SYS_RSTCTL_REG 0x34
#define ETH_RST_S 23
#define ETH_RST_M 0x01
#define PIO_RST_S 13
#define PIO_RST_M 0x01
#define MCM_RST_S 2
#define MCM_RST_M 0x01

#define MT7621_SYS_GPIO_MODE_REG 0x60
#define RGMII2_MODE_S 15
#define RGMII2_MODE_M 0x01
#define MDIO_MODE_S 12
#define MDIO_MODE_M 0x03
#define WDT_MODE_S 8
#define WDT_MODE_M 0x03

#define MT7621_GPIO_CTRL_0_REG 0x00

#define MT7621_GPIO_DATA_0_REG 0x20

#define MDIO_READ 1
#define MDIO_WRITE 0

#define MDIO_DELAY 3
#define MDIO_READ_DELAY 4

#define REG_SET_VAL(_name, _val) \
    (((_name##_M) & (_val)) << (_name##_S))

#define REG_MASK(_name) \
    ((_name##_M) << (_name##_S))

#define REG_GET_VAL(_name, _val) \
    (((_val) >> (_name##_S)) & (_name##_M))
