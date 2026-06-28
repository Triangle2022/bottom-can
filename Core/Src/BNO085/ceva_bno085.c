#include "ceva_bno085.h"

#include <math.h>
#include <stdbool.h>
#include <string.h>

#include "main.h"
#include "sh2_SensorValue.h"
#include "sh2_err.h"
#include "sh2_hal.h"

#define BNO085_SPI_TIMEOUT_MS 100U
#define BNO085_INT_TIMEOUT_MS 500U
#define BNO085_RESET_DELAY_MS 10U
#define BNO085_REPORT_INTERVAL_US 10000U
#define BNO085_RAD_TO_DEG 57.29577951308232f
#define BNO085_AUTO_AFTER_RESET_MS 150U
#define BNO085_AUTO_AFTER_OPEN_MS 100U
#define BNO085_AUTO_AFTER_CALLBACK_MS 100U
#define BNO085_AUTO_AFTER_ENABLE_MS 50U
#define BNO085_AUTO_RETRY_MS 1000U

#define BNO085_CMD_RESET 1U
#define BNO085_CMD_OPEN 2U
#define BNO085_CMD_PROD 3U
#define BNO085_CMD_CALLBACK 4U
#define BNO085_CMD_ENABLE_ARVR 5U
#define BNO085_CMD_STREAM_ON 6U
#define BNO085_CMD_STREAM_OFF 7U

extern SPI_HandleTypeDef hspi2;

static sh2_Hal_t sh2_hal;
static sh2_ProductIds_t product_ids;
static uint8_t tx_zeros[SH2_HAL_MAX_TRANSFER_IN];
static uint8_t rx_discard[SH2_HAL_MAX_TRANSFER_OUT];
static bool sh2_is_open;
static bool stream_enabled;
static uint32_t auto_step;
static uint32_t auto_next_ms;

volatile uint32_t bno085_cmd;

volatile ceva_bno085_debug_t bno085_dbg = {
  .open_status = 999,
  .prod_status = 999,
  .enable_status = 999,
  .decode_status = 999,
  .cmd_status = 999,
};

static void csn(bool high)
{
  HAL_GPIO_WritePin(SPI2_CS_GPIO_Port, SPI2_CS_Pin,
                    high ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

static void rstn(bool high)
{
  HAL_GPIO_WritePin(SPI2_RST_GPIO_Port, SPI2_RST_Pin,
                    high ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

static uint32_t time_us(void)
{
  return HAL_GetTick() * 1000U;
}

static bool int_asserted(void)
{
  return HAL_GPIO_ReadPin(SPI2_INT_GPIO_Port, SPI2_INT_Pin) == GPIO_PIN_RESET;
}

static void update_io_debug(void)
{
  bno085_dbg.cmd = bno085_cmd;
  bno085_dbg.int_pin = int_asserted() ? 0U : 1U;
  bno085_dbg.rx_ready = int_asserted() ? 1U : 0U;
}

static bool wait_for_int(void)
{
  uint32_t start = HAL_GetTick();

  while (!int_asserted()) {
    if ((HAL_GetTick() - start) >= BNO085_INT_TIMEOUT_MS) {
      bno085_dbg.no_int_count++;
      update_io_debug();
      return false;
    }
  }

  update_io_debug();
  return true;
}

static bool tick_due(uint32_t deadline_ms)
{
  return (int32_t)(HAL_GetTick() - deadline_ms) >= 0;
}

static void auto_schedule(uint32_t next_step, uint32_t delay_ms)
{
  auto_step = next_step;
  auto_next_ms = HAL_GetTick() + delay_ms;
}

static void hardware_reset(void)
{
  csn(true);
  rstn(true);
  HAL_Delay(BNO085_RESET_DELAY_MS);
  rstn(false);
  HAL_Delay(BNO085_RESET_DELAY_MS);
  rstn(true);
  HAL_Delay(BNO085_RESET_DELAY_MS);

  bno085_dbg.reset_count++;
  update_io_debug();
}

static void update_packet_debug(const uint8_t *packet, uint16_t packet_len)
{
  uint8_t channel;
  uint8_t sequence;

  if ((packet == NULL) || (packet_len < 4U)) {
    return;
  }

  channel = packet[2];
  sequence = packet[3];

  bno085_dbg.last_packet_size = packet_len;
  bno085_dbg.last_header0 = packet[0];
  bno085_dbg.last_header1 = packet[1];
  bno085_dbg.last_header2 = packet[2];
  bno085_dbg.last_header3 = packet[3];
  bno085_dbg.last_channel = channel;
  bno085_dbg.last_sequence = sequence;
  bno085_dbg.last_payload0 = (packet_len > 4U) ? packet[4] : 0U;
  bno085_dbg.last_payload1 = (packet_len > 5U) ? packet[5] : 0U;
  bno085_dbg.last_payload2 = (packet_len > 6U) ? packet[6] : 0U;
  bno085_dbg.last_payload3 = (packet_len > 7U) ? packet[7] : 0U;

  if (channel < 6U) {
    bno085_dbg.channel_count[channel]++;
  }

  if ((bno085_dbg.read_count > 0U) &&
      (bno085_dbg.last_sequence == sequence)) {
    bno085_dbg.same_seq_count++;
  } else {
    bno085_dbg.new_seq_count++;
  }
}

static bool spi_read_bytes(uint8_t *buffer, uint16_t len)
{
  HAL_StatusTypeDef status;

  csn(false);
  status = HAL_SPI_TransmitReceive(&hspi2, tx_zeros, buffer, len,
                                   BNO085_SPI_TIMEOUT_MS);
  csn(true);

  if (status != HAL_OK) {
    bno085_dbg.read_error_count++;
    return false;
  }

  return true;
}

static bool spi_write_bytes(uint8_t *buffer, uint16_t len)
{
  HAL_StatusTypeDef status;

  csn(false);
  status = HAL_SPI_TransmitReceive(&hspi2, buffer, rx_discard, len,
                                   BNO085_SPI_TIMEOUT_MS);
  csn(true);

  if (status != HAL_OK) {
    bno085_dbg.write_error_count++;
    return false;
  }

  return true;
}

static int hal_open(sh2_Hal_t *self)
{
  (void)self;

  return wait_for_int() ? SH2_OK : SH2_ERR_IO;
}

static void hal_close(sh2_Hal_t *self)
{
  (void)self;

  csn(true);
  rstn(false);
  sh2_is_open = false;
  stream_enabled = false;
  update_io_debug();
}

static int hal_read(sh2_Hal_t *self, uint8_t *pBuffer, unsigned len,
                    uint32_t *t_us)
{
  uint16_t packet_len;

  (void)self;

  if ((pBuffer == NULL) || (len < 4U)) {
    return SH2_ERR_BAD_PARAM;
  }

  if (!wait_for_int()) {
    return 0;
  }

  if (t_us != NULL) {
    *t_us = time_us();
  }

  if (!spi_read_bytes(pBuffer, 4U)) {
    return 0;
  }

  packet_len = ((uint16_t)pBuffer[0] | ((uint16_t)pBuffer[1] << 8)) &
               (uint16_t)~0x8000U;

  if ((packet_len < 4U) || (packet_len > len) ||
      (packet_len > SH2_HAL_MAX_TRANSFER_IN)) {
    bno085_dbg.bad_packet_count++;
    return 0;
  }

  if (!wait_for_int()) {
    return 0;
  }

  if (!spi_read_bytes(pBuffer, packet_len)) {
    return 0;
  }

  update_packet_debug(pBuffer, packet_len);
  bno085_dbg.read_count++;

  return (int)packet_len;
}

static int hal_write(sh2_Hal_t *self, uint8_t *pBuffer, unsigned len)
{
  (void)self;

  if ((pBuffer == NULL) || (len == 0U) || (len > SH2_HAL_MAX_TRANSFER_OUT)) {
    return SH2_ERR_BAD_PARAM;
  }

  if (!wait_for_int()) {
    bno085_dbg.tx_wait_timeout_count++;
    return SH2_ERR_TIMEOUT;
  }

  if (!spi_write_bytes(pBuffer, (uint16_t)len)) {
    return 0;
  }

  bno085_dbg.write_count++;

  return (int)len;
}

static uint32_t hal_get_time_us(sh2_Hal_t *self)
{
  (void)self;

  return time_us();
}

static float clamp_float(float value, float min_value, float max_value)
{
  if (value < min_value) {
    return min_value;
  }
  if (value > max_value) {
    return max_value;
  }

  return value;
}

static void update_euler_from_quat(float i, float j, float k, float real)
{
  float sinr_cosp = 2.0f * ((real * i) + (j * k));
  float cosr_cosp = 1.0f - (2.0f * ((i * i) + (j * j)));
  float sinp = 2.0f * ((real * j) - (k * i));
  float siny_cosp = 2.0f * ((real * k) + (i * j));
  float cosy_cosp = 1.0f - (2.0f * ((j * j) + (k * k)));

  bno085_dbg.roll_deg = atan2f(sinr_cosp, cosr_cosp) * BNO085_RAD_TO_DEG;
  bno085_dbg.pitch_deg =
      asinf(clamp_float(sinp, -1.0f, 1.0f)) * BNO085_RAD_TO_DEG;
  bno085_dbg.yaw_deg = atan2f(siny_cosp, cosy_cosp) * BNO085_RAD_TO_DEG;
}

static void event_callback(void *cookie, sh2_AsyncEvent_t *pEvent)
{
  (void)cookie;

  if (pEvent == NULL) {
    return;
  }

  if (pEvent->eventId == SH2_RESET) {
    bno085_dbg.reset_event_count++;
  } else if (pEvent->eventId == SH2_GET_FEATURE_RESP) {
    bno085_dbg.get_feature_count++;
  }
}

static void sensor_callback(void *cookie, sh2_SensorEvent_t *pEvent)
{
  sh2_SensorValue_t value;

  (void)cookie;

  if (pEvent == NULL) {
    return;
  }

  bno085_dbg.callback_count++;
  bno085_dbg.report_id = pEvent->reportId;
  bno085_dbg.timestamp = pEvent->timestamp_uS;
  bno085_dbg.decode_status = sh2_decodeSensorEvent(&value, pEvent);

  if (bno085_dbg.decode_status != SH2_OK) {
    return;
  }

  if (value.sensorId == SH2_ARVR_STABILIZED_RV) {
    bno085_dbg.quat_i = value.un.arvrStabilizedRV.i;
    bno085_dbg.quat_j = value.un.arvrStabilizedRV.j;
    bno085_dbg.quat_k = value.un.arvrStabilizedRV.k;
    bno085_dbg.quat_real = value.un.arvrStabilizedRV.real;
    update_euler_from_quat(bno085_dbg.quat_i, bno085_dbg.quat_j,
                           bno085_dbg.quat_k, bno085_dbg.quat_real);
  } else if (value.sensorId == SH2_ACCELEROMETER) {
    bno085_dbg.accel_x = value.un.accelerometer.x;
    bno085_dbg.accel_y = value.un.accelerometer.y;
    bno085_dbg.accel_z = value.un.accelerometer.z;
  }
}

static int32_t enable_report(sh2_SensorId_t sensor_id)
{
  sh2_SensorConfig_t config = {0};

  config.reportInterval_us = BNO085_REPORT_INTERVAL_US;

  return sh2_setSensorConfig(sensor_id, &config);
}

static int32_t enable_reports(void)
{
  int32_t status;

  status = enable_report(SH2_ARVR_STABILIZED_RV);
  if (status != SH2_OK) {
    return status;
  }

  return enable_report(SH2_ACCELEROMETER);
}

static void run_command(uint32_t cmd)
{
  bno085_dbg.stage = cmd;
  bno085_dbg.cmd_status = 999;

  switch (cmd) {
  case BNO085_CMD_RESET:
    hardware_reset();
    bno085_dbg.cmd_status = SH2_OK;
    break;

  case BNO085_CMD_OPEN: {
    if (sh2_is_open) {
      sh2_close();
      sh2_is_open = false;
    }
    uint32_t reset_events_before = bno085_dbg.reset_event_count;
    bno085_dbg.open_status = sh2_open(&sh2_hal, event_callback, NULL);
    if ((bno085_dbg.open_status == SH2_OK) &&
        (bno085_dbg.reset_event_count == reset_events_before)) {
      bno085_dbg.open_status = SH2_ERR_TIMEOUT;
      sh2_close();
    }
    sh2_is_open = bno085_dbg.open_status == SH2_OK;
    bno085_dbg.cmd_status = bno085_dbg.open_status;
    break;
  }

  case BNO085_CMD_PROD:
    bno085_dbg.prod_status = sh2_getProdIds(&product_ids);
    bno085_dbg.cmd_status = bno085_dbg.prod_status;
    break;

  case BNO085_CMD_CALLBACK:
    bno085_dbg.cmd_status = sh2_setSensorCallback(sensor_callback, NULL);
    break;

  case BNO085_CMD_ENABLE_ARVR:
    bno085_dbg.enable_status = enable_reports();
    bno085_dbg.cmd_status = bno085_dbg.enable_status;
    break;

  case BNO085_CMD_STREAM_ON:
    stream_enabled = true;
    bno085_dbg.cmd_status = SH2_OK;
    break;

  case BNO085_CMD_STREAM_OFF:
    stream_enabled = false;
    bno085_dbg.cmd_status = SH2_OK;
    break;

  default:
    bno085_dbg.cmd_status = SH2_ERR_BAD_PARAM;
    break;
  }
}

static void run_auto_startup(void)
{
  if ((auto_step == 0U) || !tick_due(auto_next_ms)) {
    return;
  }

  switch (auto_step) {
  case BNO085_CMD_RESET:
    run_command(BNO085_CMD_RESET);
    auto_schedule(BNO085_CMD_OPEN, BNO085_AUTO_AFTER_RESET_MS);
    break;

  case BNO085_CMD_OPEN:
    run_command(BNO085_CMD_OPEN);
    if (bno085_dbg.cmd_status == SH2_OK) {
      auto_schedule(BNO085_CMD_CALLBACK, BNO085_AUTO_AFTER_OPEN_MS);
    } else {
      auto_schedule(BNO085_CMD_RESET, BNO085_AUTO_RETRY_MS);
    }
    break;

  case BNO085_CMD_CALLBACK:
    if (!sh2_is_open) {
      auto_schedule(BNO085_CMD_RESET, BNO085_AUTO_RETRY_MS);
      break;
    }
    run_command(BNO085_CMD_CALLBACK);
    if (bno085_dbg.cmd_status == SH2_OK) {
      auto_schedule(BNO085_CMD_ENABLE_ARVR, BNO085_AUTO_AFTER_CALLBACK_MS);
    } else {
      auto_schedule(BNO085_CMD_CALLBACK, BNO085_AUTO_RETRY_MS);
    }
    break;

  case BNO085_CMD_ENABLE_ARVR:
    if (!sh2_is_open) {
      auto_schedule(BNO085_CMD_RESET, BNO085_AUTO_RETRY_MS);
      break;
    }
    if (!int_asserted()) {
      bno085_dbg.stage = BNO085_CMD_ENABLE_ARVR;
      bno085_dbg.enable_status = SH2_ERR_TIMEOUT;
      bno085_dbg.cmd_status = SH2_ERR_TIMEOUT;
      bno085_dbg.no_int_count++;
      update_io_debug();
      auto_schedule(BNO085_CMD_ENABLE_ARVR, BNO085_AUTO_RETRY_MS);
      break;
    }
    run_command(BNO085_CMD_ENABLE_ARVR);
    if (bno085_dbg.cmd_status == SH2_OK) {
      auto_schedule(BNO085_CMD_STREAM_ON, BNO085_AUTO_AFTER_ENABLE_MS);
    } else {
      auto_schedule(BNO085_CMD_ENABLE_ARVR, BNO085_AUTO_RETRY_MS);
    }
    break;

  case BNO085_CMD_STREAM_ON:
    run_command(BNO085_CMD_STREAM_ON);
    auto_schedule(0U, 0U);
    break;

  default:
    auto_schedule(BNO085_CMD_RESET, BNO085_AUTO_RETRY_MS);
    break;
  }
}

void CEVA_BNO085_Init(void)
{
  memset((void *)&bno085_dbg, 0, sizeof(bno085_dbg));
  bno085_dbg.open_status = 999;
  bno085_dbg.prod_status = 999;
  bno085_dbg.enable_status = 999;
  bno085_dbg.decode_status = 999;
  bno085_dbg.cmd_status = 999;

  sh2_hal.open = hal_open;
  sh2_hal.close = hal_close;
  sh2_hal.read = hal_read;
  sh2_hal.write = hal_write;
  sh2_hal.getTimeUs = hal_get_time_us;

  csn(true);
  rstn(true);
  sh2_is_open = false;
  stream_enabled = false;
  auto_schedule(BNO085_CMD_RESET, 0U);
  update_io_debug();
}

void CEVA_BNO085_Service(void)
{
  uint32_t cmd = bno085_cmd;

  if (cmd != 0U) {
    bno085_cmd = 0U;
    auto_schedule(0U, 0U);
    run_command(cmd);
  }

  run_auto_startup();

  if (stream_enabled && sh2_is_open) {
    sh2_service();
    bno085_dbg.service_count++;
  }

  update_io_debug();
}

void CEVA_BNO085_EXTI_Callback(uint16_t gpio_pin)
{
  (void)gpio_pin;
}

void CEVA_BNO085_SPI_TxRxCpltCallback(void *hspi)
{
  (void)hspi;
}

void CEVA_BNO085_SPI_ErrorCallback(void *hspi)
{
  (void)hspi;
  bno085_dbg.spi_error_count++;
}
