/* drivers/input/touchscreen/gt9xx_config.h
 *
 * 2010 - 2016 Goodix Technology.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be a reference
 * to you, when you are integrating the GOODiX's CTP IC into your system,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 */
#ifndef _GT9XX_CONFIG_H_
#define _GT9XX_CONFIG_H_

/* ***************************PART2:TODO define********************************** */
/* STEP_1(REQUIRED):Change config table. */
/*TODO: puts the config info corresponded to your TP here, the following is just
a sample config, send this config should cause the chip cannot work normally*/
#define CTP_CFG_GROUP0 {\
0x41, 0x20, 0x03, 0x00, 0x05, 0x0A, 0x35, 0x00, 0x01, 0x08,\
0x28, 0x0A, 0x5A, 0x32, 0x03, 0x00, 0x01, 0x01, 0x00, 0x00,\
0x11, 0x11, 0x00, 0x17, 0x19, 0x1E, 0x14, 0x90, 0x30, 0xAA,\
0x14, 0x16, 0x60, 0x1A, 0x00, 0x00, 0x00, 0x03, 0x03, 0x11,\
0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,\
0x00, 0x0A, 0x23, 0x94, 0xC5, 0x02, 0x07, 0x00, 0x00, 0x04,\
0xD2, 0x0B, 0x00, 0xAB, 0x0E, 0x00, 0x8B, 0x12, 0x00, 0x6F,\
0x18, 0x00, 0x5D, 0x1F, 0x00, 0x5D, 0x00, 0x00, 0x00, 0x00,\
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,\
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,\
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,\
0x00, 0x00, 0x00, 0x01, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09,\
0x0C, 0x0D, 0x0E, 0x0F, 0x10, 0x11, 0x14, 0x15, 0x16, 0x17,\
0x18, 0x19, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,\
0x00, 0x00, 0x00, 0x02, 0x04, 0x06, 0x07, 0x08, 0x0A, 0x0C,\
0x0D, 0x0E, 0x0F, 0x10, 0x11, 0x12, 0x13, 0x14, 0x19, 0x1B,\
0x1C, 0x1E, 0x1F, 0x20, 0x21, 0x22, 0x23, 0x2A, 0x29, 0x28,\
0x27, 0x26, 0x25, 0x24, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,\
0x00, 0x00, 0x00, 0x00, 0xDD, 0x01\
}

#define GTP_CFG_GROUP0_CHARGER {\
}

/* TODO puts your group2 config info here,if need. */
#define CTP_CFG_GROUP1 {\
}

/* TODO puts your group2 config info here,if need. */
#define GTP_CFG_GROUP1_CHARGER {\
}

/* TODO puts your group3 config info here,if need. */
#define CTP_CFG_GROUP2 {\
}

/* TODO puts your group3 config info here,if need. */
#define GTP_CFG_GROUP2_CHARGER {\
}

/* TODO: define your config for Sensor_ID == 3 here, if needed */
#define CTP_CFG_GROUP3 {\
}

/* TODO puts your group3 config info here,if need. */
#define GTP_CFG_GROUP3_CHARGER {\
}

/* TODO: define your config for Sensor_ID == 4 here, if needed */
#define CTP_CFG_GROUP4 {\
}

/* TODO puts your group4 config info here,if need. */
#define GTP_CFG_GROUP4_CHARGER {\
}

/* TODO: define your config for Sensor_ID == 5 here, if needed */
#define CTP_CFG_GROUP5 {\
}

/* TODO puts your group5 config info here,if need. */
#define GTP_CFG_GROUP5_CHARGER {\
}


#endif /* _GT1X_CONFIG_H_ */
