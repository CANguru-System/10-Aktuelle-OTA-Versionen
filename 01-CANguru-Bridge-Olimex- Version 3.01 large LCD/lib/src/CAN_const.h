
#ifndef CANCONST_H
#define CANCONST_H

const uint8_t CAN_FRAME_SIZE = 13; /* maximum datagram size */
#define CAN_MSG_FLAG_EXTD_ 0b01
#define CAN_MSG_FLAG_SS_ 0b00
#define CAN_MSG_FLAG_RTR_ 0b00
#define CAN_EFF_MASK 0x1FFFFFFFU /* extended frame format (EFF) */

#endif 