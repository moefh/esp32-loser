
#if ARDUINO_ARCH_ESP32
#include <tcpip_adapter.h>
#endif

#include <esp_wifi.h>
#include <cstring>
#include <esp_now.h>

#include "net.h"
#include "util.h"

static esp_now_peer_info_t peer_info;
static const uint8_t net_broadcast_addr[] = {
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff
};

static volatile int     net_tx_msg_sending;

static volatile int     net_rx_msg_first_available;
static volatile int     net_rx_msg_num_available;
static volatile uint8_t net_rx_msg_buf[NET_MSG_SIZE*NET_MSG_RX_QUEUE_LEN];

static int init_wifi()
{
#if ARDUINO_ARCH_ESP32
  tcpip_adapter_init();
#else
  if (esp_netif_init() != ESP_OK) return 1;
#endif
  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  cfg.static_tx_buf_num = 0;
  cfg.dynamic_tx_buf_num = 8;
  cfg.static_rx_buf_num = 5;
  cfg.dynamic_rx_buf_num = 8;
  if (esp_wifi_init(&cfg) != ESP_OK) return 1;
  if (esp_wifi_set_storage(WIFI_STORAGE_RAM) != ESP_OK) return 1;
  if (esp_wifi_set_mode(WIFI_MODE_STA) != ESP_OK) return 1;
  if (esp_wifi_start() != ESP_OK) return 1;
  return 0;
}

static void net_data_sent_callback(const uint8_t *mac_addr, esp_now_send_status_t status)
{
  net_tx_msg_sending = 0;
}

static void net_data_recv_callback(const uint8_t *mac_addr, const uint8_t *data, int len)
{
  if (net_rx_msg_num_available >= NET_MSG_RX_QUEUE_LEN) {
    return;  // queue full
  }
  if (len > NET_MSG_SIZE) {
    len = NET_MSG_SIZE;
  }
  int free_index = (net_rx_msg_first_available + net_rx_msg_num_available) % NET_MSG_RX_QUEUE_LEN;
  memcpy((void *) &net_rx_msg_buf[free_index*NET_MSG_SIZE], data, len);
  if (len < NET_MSG_SIZE) {
    memset((void *) &net_rx_msg_buf[free_index*NET_MSG_SIZE + len], 0, NET_MSG_SIZE - len);
  }
  net_rx_msg_num_available++;
}

int net_can_send_message()
{
  return ! net_tx_msg_sending;
}

int net_send_message(const uint8_t *data)
{
  net_tx_msg_sending = 1;
  esp_err_t result = esp_now_send(net_broadcast_addr, data, NET_MSG_SIZE);
  if (result != ESP_OK) {
    net_tx_msg_sending = 0;
    return 1;
  }
  return 0;
}

int net_message_available()
{
  return net_rx_msg_num_available > 0;
}

int net_read_message(uint8_t *data)
{
  if (net_rx_msg_num_available <= 0) {
    return 1;
  }

  memcpy(data, (void *) &net_rx_msg_buf[net_rx_msg_first_available*NET_MSG_SIZE], NET_MSG_SIZE);
  net_rx_msg_first_available = (net_rx_msg_first_available+1) % NET_MSG_RX_QUEUE_LEN;
  net_rx_msg_num_available--;
  return 0;
}

int net_init()
{
  net_tx_msg_sending = 0;
  net_rx_msg_num_available = 0;
  net_rx_msg_first_available = 0;

  if (init_wifi() != 0) {
    printf("ERROR initializing WiFi\n");
    return 1;
  }
  
  if (esp_now_init() != ESP_OK) {
    printf("ERROR initializing ESP-NOW\n");
    return 1;
  }
  
  if (esp_now_register_send_cb(net_data_sent_callback) != ESP_OK) {
    printf("ERROR registering send callback\n");
    return 1;
  }
    
  if (esp_now_register_recv_cb(net_data_recv_callback) != ESP_OK) {
    printf("ERROR registering recv callback\n");
    return 1;
  }
  
  memcpy(peer_info.peer_addr, net_broadcast_addr, sizeof(net_broadcast_addr));
  peer_info.channel = 0;
  peer_info.encrypt = false;
  if (esp_now_add_peer(&peer_info) != ESP_OK) {
    printf("ERROR adding peer\n");
    return 1;
  }

  printf("OK: network initialized\n");
  return 0;
}
