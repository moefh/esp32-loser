
#ifndef NET_H_FILE
#define NET_H_FILE

#include <cstdint>

#define NET_MSG_SIZE          64
#define NET_MSG_RX_QUEUE_LEN  4  // up to N messages in receive queue

int net_init();

int net_can_send_message();
int net_send_message(const uint8_t *data);

int net_message_available();
int net_read_message(uint8_t *data);

#endif /* NET_H_FILE */
