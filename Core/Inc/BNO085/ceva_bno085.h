#ifndef CEVA_BNO085_H
#define CEVA_BNO085_H

#include <stdint.h>

#include "sh2.h"

typedef struct {
  int32_t open_status;
  int32_t prod_status;
  int32_t enable_status;
  int32_t decode_status;
  uint32_t reset_count;
  uint32_t read_count;
  uint32_t write_count;
  uint32_t service_count;
  uint32_t callback_count;
  uint32_t no_int_count;
  uint32_t read_error_count;
  uint32_t write_error_count;
  uint32_t bad_packet_count;
  uint32_t spi_done_count;
  uint32_t spi_error_count;
  uint32_t int_count;
  uint32_t tx_wait_timeout_count;
  uint32_t reset_event_count;
  uint32_t get_feature_count;
  uint32_t same_seq_count;
  uint32_t new_seq_count;
  uint32_t channel_count[6];
  uint32_t rx_len;
  uint32_t tx_len;
  uint32_t cmd;
  uint32_t stage;
  int32_t cmd_status;
  uint16_t last_packet_size;
  uint8_t spi_state;
  uint8_t rx_ready;
  uint8_t int_pin;
  uint8_t last_header0;
  uint8_t last_header1;
  uint8_t last_header2;
  uint8_t last_header3;
  uint8_t last_channel;
  uint8_t last_sequence;
  uint8_t last_payload0;
  uint8_t last_payload1;
  uint8_t last_payload2;
  uint8_t last_payload3;
  uint8_t report_id;
  uint64_t timestamp;
  float accel_x;
  float accel_y;
  float accel_z;
} ceva_bno085_debug_t;

extern volatile ceva_bno085_debug_t bno085_dbg;
extern volatile uint32_t bno085_cmd;

void CEVA_BNO085_Init(void);
void CEVA_BNO085_Service(void);
void CEVA_BNO085_EXTI_Callback(uint16_t gpio_pin);
void CEVA_BNO085_SPI_TxRxCpltCallback(void *hspi);
void CEVA_BNO085_SPI_ErrorCallback(void *hspi);

#endif
