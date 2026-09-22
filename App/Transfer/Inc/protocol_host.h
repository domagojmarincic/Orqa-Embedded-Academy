#ifndef PROTOCOL_HOST_H
#define PROTOCOL_HOST_H

#include <stdint.h>
#include <stdbool.h>

#define HOST_CMD_SET_PACKET_SIZE      0x01U
#define HOST_CMD_START_TRANSFER       0x02U
#define HOST_CMD_DATA_BLOCK_WITH_CRC  0x03U
#define HOST_CMD_END_TRANSFER         0x04U
#define HOST_CMD_ACK                  0x05U
#define HOST_CMD_NACK                 0x06U

#define HOST_PACKET_SIZE   256U

void ProtocolHost_Init(void);
void ProtocolHost_StartTransfer(void);
void ProtocolHost_Process(void);
bool ProtocolHost_IsDone(void);

#endif
