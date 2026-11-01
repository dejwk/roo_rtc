#include <algorithm>

#include "driver/i2c_master.h"
#include "driver/spi_master.h"
#include "registers.h"

struct i2c_master_bus_t {};
struct i2c_master_dev_t {};
struct spi_device_t {};
namespace {
i2c_master_bus_t bus;
i2c_master_dev_t device;
spi_device_t spi;
}  // namespace
extern "C" {
esp_err_t i2c_master_get_bus_handle(i2c_port_num_t,
                                    i2c_master_bus_handle_t* out) {
  *out = &bus;
  return ESP_OK;
}
esp_err_t i2c_new_master_bus(const i2c_master_bus_config_t*,
                             i2c_master_bus_handle_t*) {
  return ESP_FAIL;
}
esp_err_t i2c_del_master_bus(i2c_master_bus_handle_t) { return ESP_OK; }
esp_err_t i2c_master_bus_add_device(i2c_master_bus_handle_t,
                                    const i2c_device_config_t* config,
                                    i2c_master_dev_handle_t* out) {
  if (rtc_test::regs.init_error) return ESP_FAIL;
  rtc_test::regs.address = config->device_address;
  ++rtc_test::regs.added;
  *out = &device;
  return ESP_OK;
}
esp_err_t i2c_master_bus_rm_device(i2c_master_dev_handle_t) {
  ++rtc_test::regs.removed;
  return ESP_OK;
}
esp_err_t i2c_master_transmit(i2c_master_dev_handle_t, const uint8_t* data,
                              size_t len, int) {
  return rtc_test::regs.write(data, len) ? ESP_OK : ESP_FAIL;
}
esp_err_t i2c_master_receive(i2c_master_dev_handle_t, uint8_t*, size_t, int) {
  return ESP_FAIL;
}
esp_err_t i2c_master_transmit_receive(i2c_master_dev_handle_t,
                                      const uint8_t* tx, size_t tx_len,
                                      uint8_t* rx, size_t rx_len, int) {
  if (tx_len != 1) return ESP_FAIL;
  return rtc_test::regs.read(tx[0], rx, rx_len) ? ESP_OK : ESP_FAIL;
}
esp_err_t spi_bus_add_device(spi_host_device_t,
                             const spi_device_interface_config_t*,
                             spi_device_handle_t* out) {
  if (rtc_test::regs.init_error) return ESP_FAIL;
  ++rtc_test::regs.added;
  *out = &spi;
  return ESP_OK;
}
esp_err_t spi_bus_remove_device(spi_device_handle_t) {
  ++rtc_test::regs.removed;
  return ESP_OK;
}
esp_err_t spi_device_polling_transmit(spi_device_handle_t,
                                      spi_transaction_t* t) {
  const uint8_t* tx = static_cast<const uint8_t*>(t->tx_buffer);
  uint8_t* rx = static_cast<uint8_t*>(t->rx_buffer);
  const size_t len = t->length / 8;
  if ((tx[0] & 0x80) != 0) {
    std::vector<uint8_t> data(tx, tx + len);
    data[0] &= 0x7f;
    return rtc_test::regs.write(data.data(), len) ? ESP_OK : ESP_FAIL;
  }
  return rx != nullptr && rtc_test::regs.read(tx[0], rx + 1, len - 1)
             ? ESP_OK
             : ESP_FAIL;
}
}
