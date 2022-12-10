#pragma once

#include "esphome/core/component.h"
#include "esphome/core/defines.h"
#include "esphome/core/hal.h"

namespace esphome {
namespace tm1640 {

class TM1640Display;

using tm1640_writer_t = std::function<void(TM1640Display &)>;

class TM1640Display : public PollingComponent {
 public:
  void set_writer(tm1640_writer_t &&writer) { this->writer_ = writer; }

  void setup() override;

  void dump_config() override;

  void set_clk_pin(GPIOPin *pin) { clk_pin_ = pin; }
  void set_dio_pin(GPIOPin *pin) { dio_pin_ = pin; }

  float get_setup_priority() const override;

  void update() override;

  /// Evaluate the printf-format and print the result at the given position.
  uint8_t printf(uint8_t pos, const char *format, ...) __attribute__((format(printf, 3, 4)));
  /// Evaluate the printf-format and print the result at position 0.
  uint8_t printf(const char *format, ...) __attribute__((format(printf, 2, 3)));

  /// Print `str` at the given position.
  uint8_t print(uint8_t pos, const char *str);
  /// Print `str` at position 0.
  uint8_t print(const char *str);

  void set_intensity(uint8_t intensity) { this->intensity_ = intensity; }
  void set_length(uint8_t length) { this->length_ = length; }

  void display();

 protected:
  void bit_delay_();
  void setup_pins_();
  void send_byte_(uint8_t b);
  uint8_t read_byte_();
  void start_();
  void stop_();

  GPIOPin *dio_pin_;
  GPIOPin *clk_pin_;
  uint8_t intensity_;
  uint8_t length_;
  optional<tm1640_writer_t> writer_{};
  uint8_t buffer_[16] = {0};
};

}  // namespace tm1640
}  // namespace esphome
