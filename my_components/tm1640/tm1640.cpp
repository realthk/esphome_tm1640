#include "tm1640.h"
#include "esphome/core/log.h"
#include "esphome/core/helpers.h"
#include "esphome/core/hal.h"

namespace esphome {
namespace tm1640 {

static const char *const TAG = "display.tm1640";
const uint8_t TM1640_CMD_DATA = 0x40;  //!< Display data command
const uint8_t TM1640_CMD_CTRL = 0x80;  //!< Display control command
const uint8_t TM1640_CMD_ADDR = 0xc0;  //!< Display address command
const uint8_t TM1640_UNKNOWN_CHAR = 0b11111111;

// Data command bits
const uint8_t TM1640_DATA_WRITE = 0x00;          //!< Write data
const uint8_t TM1640_DATA_READ_KEYS = 0x02;      //!< Read keys
const uint8_t TM1640_DATA_AUTO_INC_ADDR = 0x00;  //!< Auto increment address
const uint8_t TM1640_DATA_FIXED_ADDR = 0x04;     //!< Fixed address

//
//      A
//     ---
//  F |   | B
//     -G-
//  E |   | C
//     ---
//      D   X
// XABCDEFG
const uint8_t TM1640_ASCII_TO_RAW[] PROGMEM = {
    0b00000000, // (32)  <space>
    0b10000110, // (33)	!
    0b00100010, // (34)	"
    0b01111110, // (35)	#
    0b01101101, // (36)	$
    0b00000000, // (37)	%
    0b00000000, // (38)	&
    0b00000010, // (39)	'
    0b00110000, // (40)	(
    0b00000110, // (41)	)
    0b01100011, // (42)	*
    0b00000000, // (43)	+
    0b00000100, // (44)	,
    0b01000000, // (45)	-
    0b10000000, // (46)	.
    0b01010010, // (47)	/
    0b00111111, // 0
    0b00000110, // 1
    0b01011011, // 2
    0b01001111, // 3
    0b01100110, // 4
    0b01101101, // 5
    0b01111101, // 6
    0b00000111, // 7
    0b01111111, // 8
    0b01101111, // 9
    0b00000000, // (58)	:
    0b00000000, // (59)	;
    0b00000000, // (60)	<
    0b01001000, // (61)	=
    0b00000000, // (62)	>
    0b01010011, // (63)	?
    0b01011111, // (64)	@
    0b01110111, // (65)	A
    0b01111111, // (66)	B
    0b00111001, // (67)	C
    0b00111111, // (68)	D
    0b01111001, // (69)	E
    0b01110001, // (70)	F
    0b00111101, // (71)	G
    0b01110110, // (72)	H
    0b00000110, // (73)	I
    0b00011110, // (74)	J
    0b01101001, // (75)	K
    0b00111000, // (76)	L
    0b00010101, // (77)	M
    0b00110111, // (78)	N
    0b00111111, // (79)	O
    0b01110011, // (80)	P
    0b01100111, // (81)	Q
    0b00110001, // (82)	R
    0b01101101, // (83)	S
    0b01111000, // (84)	T
    0b00111110, // (85)	U
    0b00101010, // (86)	V
    0b00011101, // (87)	W
    0b01110110, // (88)	X
    0b01101110, // (89)	Y
    0b01011011, // (90)	Z
    0b00111001, // (91)	[
    0b01100100, // (92)	\ (this can't be the last char on a line, even in comment or it'll concat)
    0b00001111, // (93)	]
    0b00000000, // (94)	^
    0b00001000, // (95)	_
    0b00100000, // (96)	`
    0b01011111, // (97)	a
    0b01111100, // (98)	b
    0b01011000, // (99)	c
    0b01011110, // (100)	d
    0b01111011, // (101)	e
    0b00110001, // (102)	f
    0b01101111, // (103)	g
    0b01110100, // (104)	h
    0b00000100, // (105)	i
    0b00001110, // (106)	j
    0b01110101, // (107)	k
    0b00110000, // (108)	l
    0b01010101, // (109)	m
    0b01010100, // (110)	n
    0b01011100, // (111)	o
    0b01110011, // (112)	p
    0b01100111, // (113)	q
    0b01010000, // (114)	r
    0b01101101, // (115)	s
    0b01111000, // (116)	t
    0b00011100, // (117)	u
    0b00101010, // (118)	v
    0b00011101, // (119)	w
    0b01110110, // (120)	x
    0b01101110, // (121)	y
    0b01000111, // (122)	z
    0b01000110, // (123)	{
    0b00000110, // (124)	|
    0b01110000, // (125)	}
    0b00000001, // (126)	~
};
void TM1640Display::setup() {
  ESP_LOGCONFIG(TAG, "Setting up TM1640...");

  this->clk_pin_->setup();               // OUTPUT
  this->clk_pin_->digital_write(false);  // LOW
  this->dio_pin_->setup();               // OUTPUT
  this->dio_pin_->digital_write(false);  // LOW

  this->display();
}
void TM1640Display::dump_config() {
  ESP_LOGCONFIG(TAG, "TM1640:");
  ESP_LOGCONFIG(TAG, "  Intensity: %d", this->intensity_);
  ESP_LOGCONFIG(TAG, "  Inverted: %d", this->inverted_);
  ESP_LOGCONFIG(TAG, "  Length: %d", this->length_);
  LOG_PIN("  CLK Pin: ", this->clk_pin_);
  LOG_PIN("  DIO Pin: ", this->dio_pin_);
  LOG_UPDATE_INTERVAL(this);
}

void TM1640Display::update() {
  for (uint8_t &i : this->buffer_)
    i = 0;
  if (this->writer_.has_value())
    (*this->writer_)(*this);
  this->display();
}

float TM1640Display::get_setup_priority() const { return setup_priority::PROCESSOR; }
void TM1640Display::bit_delay_() { delayMicroseconds(100); }
void TM1640Display::start_() {
  this->dio_pin_->pin_mode(gpio::FLAG_OUTPUT);
  this->bit_delay_();
}

void TM1640Display::stop_() {
  this->dio_pin_->pin_mode(gpio::FLAG_OUTPUT);
  bit_delay_();
  this->clk_pin_->pin_mode(gpio::FLAG_INPUT);
  bit_delay_();
  this->dio_pin_->pin_mode(gpio::FLAG_INPUT);
  bit_delay_();
}

void TM1640Display::display() {
  ESP_LOGVV(TAG, "Display %02X%02X%02X%02X", buffer_[0], buffer_[1], buffer_[2], buffer_[3]);

  // Write DATA CMND
  this->start_();
  this->send_byte_(TM1640_CMD_DATA);
  this->stop_();

  // Write ADDR CMD + first digit address
  this->start_();
  this->send_byte_(TM1640_CMD_ADDR);

  // Write the data bytes
  if (this->inverted_) {
    for (int8_t i = this->length_ - 1; i >= 0; i--) {
      this->send_byte_(this->buffer_[i]);
    }
  } else {
    for (auto b : this->buffer_) {
      this->send_byte_(b);
    }
  }

  this->stop_();

  // Write display CTRL CMND + brightness
  this->start_();
  this->send_byte_(TM1640_CMD_CTRL + ((this->intensity_ & 0x7) | 0x08));
  this->stop_();
}

bool TM1640Display::send_byte_(uint8_t b) {
  uint8_t data = b;
  for (uint8_t i = 0; i < 8; i++) {
    // CLK low
    this->clk_pin_->pin_mode(gpio::FLAG_OUTPUT);
    this->bit_delay_();
    // Set data bit
    if (data & 0x01) {
      this->dio_pin_->pin_mode(gpio::FLAG_INPUT);
    } else {
      this->dio_pin_->pin_mode(gpio::FLAG_OUTPUT);
    }

    this->bit_delay_();
    // CLK high
    this->clk_pin_->pin_mode(gpio::FLAG_INPUT);
    this->bit_delay_();
    data = data >> 1;
  }

  // Wait for acknowledge
  // CLK to zero
  this->clk_pin_->pin_mode(gpio::FLAG_OUTPUT);
  this->dio_pin_->pin_mode(gpio::FLAG_INPUT);
  this->bit_delay_();
  // CLK to high
  this->clk_pin_->pin_mode(gpio::FLAG_INPUT);
  this->bit_delay_();
  uint8_t ack = this->dio_pin_->digital_read();
  if (ack == 0) {
    this->dio_pin_->pin_mode(gpio::FLAG_OUTPUT);
  }

  this->bit_delay_();
  this->clk_pin_->pin_mode(gpio::FLAG_INPUT);
  this->bit_delay_();

  return ack;
}

uint8_t TM1640Display::print(uint8_t start_pos, const char *str) {
  // ESP_LOGV(TAG, "Print at %d: %s", start_pos, str);
  uint8_t pos = start_pos;
  for (; *str != '\0'; str++) {
    uint8_t data = TM1640_UNKNOWN_CHAR;
    if (*str >= ' ' && *str <= '~')
      data = progmem_read_byte(&TM1640_ASCII_TO_RAW[*str - ' ']);

    if (data == TM1640_UNKNOWN_CHAR) {
      ESP_LOGW(TAG, "Encountered character '%c' with no TM1640 representation while translating string!", *str);
    }
    // Remap segments, for compatibility with MAX7219 segment definition which is
    // XABCDEFG, but TM1640 is // XGFEDCBA
    if (this->inverted_) {
      // XABCDEFG > XGCBAFED
      data = ((data & 0x80) ? 0x80 : 0) |  // no move X
             ((data & 0x40) ? 0x8 : 0) |   // A
             ((data & 0x20) ? 0x10 : 0) |  // B
             ((data & 0x10) ? 0x20 : 0) |  // C
             ((data & 0x8) ? 0x1 : 0) |    // D
             ((data & 0x4) ? 0x2 : 0) |    // E
             ((data & 0x2) ? 0x4 : 0) |    // F
             ((data & 0x1) ? 0x40 : 0);    // G
    } else {
      // XABCDEFG > XGFEDCBA
      data = ((data & 0x80) ? 0x80 : 0) |  // no move X
             ((data & 0x40) ? 0x1 : 0) |   // A
             ((data & 0x20) ? 0x2 : 0) |   // B
             ((data & 0x10) ? 0x4 : 0) |   // C
             ((data & 0x8) ? 0x8 : 0) |    // D
             ((data & 0x4) ? 0x10 : 0) |   // E
             ((data & 0x2) ? 0x20 : 0) |   // F
             ((data & 0x1) ? 0x40 : 0);    // G
    }
    if (*str == '.') {
      if (pos != start_pos)
        pos--;
      this->buffer_[pos] |= 0b10000000;
    } else {
      if (pos >= 6) {
        ESP_LOGE(TAG, "String is too long for the display!");
        break;
      }
      this->buffer_[pos] = data;
    }
    pos++;
  }
  return pos - start_pos;
}
uint8_t TM1640Display::print(const char *str) { return this->print(0, str); }
uint8_t TM1640Display::printf(uint8_t pos, const char *format, ...) {
  va_list arg;
  va_start(arg, format);
  char buffer[64];
  int ret = vsnprintf(buffer, sizeof(buffer), format, arg);
  va_end(arg);
  if (ret > 0)
    return this->print(pos, buffer);
  return 0;
}
uint8_t TM1640Display::printf(const char *format, ...) {
  va_list arg;
  va_start(arg, format);
  char buffer[64];
  int ret = vsnprintf(buffer, sizeof(buffer), format, arg);
  va_end(arg);
  if (ret > 0)
    return this->print(buffer);
  return 0;
}

#ifdef USE_TIME
uint8_t TM1640Display::strftime(uint8_t pos, const char *format, time::ESPTime time) {
  char buffer[64];
  size_t ret = time.strftime(buffer, sizeof(buffer), format);
  if (ret > 0)
    return this->print(pos, buffer);
  return 0;
}
uint8_t TM1640Display::strftime(const char *format, time::ESPTime time) { return this->strftime(0, format, time); }
#endif

}  // namespace tm1640
}  // namespace esphome
