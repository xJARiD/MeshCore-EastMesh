#pragma once

#include "DisplayDriver.h"
#include <Wire.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <helpers/RefCountedDigitalPin.h>

// Subclass that adds clearFullRAM() and setOffsets() needed by panels whose
// controller RAM is larger than the visible area (e.g. ST7735S 132×162 RAM,
// 128×160 visible).  clearFullRAM() uses public spiWrite/writeCommand to zero
// the full controller RAM before init; setOffsets() directly patches the
// _xstart/_ystart members used by setAddrWindow().
class ExtendedST7735 : public Adafruit_ST7735 {
public:
  using Adafruit_ST7735::Adafruit_ST7735;
  void clearFullRAM();
  void setOffsets(int16_t col, int16_t row) { _xstart = col; _ystart = row; }
};

class ST7735Display : public DisplayDriver {
  ExtendedST7735 display;
  bool _isOn;
  uint16_t _color;
  RefCountedDigitalPin* _peripher_power;

  bool i2c_probe(TwoWire& wire, uint8_t addr);
public:
#ifdef USE_PIN_TFT
  ST7735Display(RefCountedDigitalPin* peripher_power=NULL) : DisplayDriver(128, 64),
      display(PIN_TFT_CS, PIN_TFT_DC, PIN_TFT_SDA, PIN_TFT_SCL, PIN_TFT_RST),
      _peripher_power(peripher_power)
  {
    _isOn = false;
  }
#elif defined(USE_VSPI)
  ST7735Display(RefCountedDigitalPin* peripher_power=NULL) : DisplayDriver(128, 64),
      display(&SPI, PIN_TFT_CS, PIN_TFT_DC, PIN_TFT_RST),
      _peripher_power(peripher_power)
  {
    _isOn = false;
  }
#else
  ST7735Display(RefCountedDigitalPin* peripher_power=NULL) : DisplayDriver(128, 64),
      display(&SPI1, PIN_TFT_CS, PIN_TFT_DC, PIN_TFT_RST),
      _peripher_power(peripher_power)
  {
    _isOn = false;
  }
#endif
  bool begin();

  bool isOn() override { return _isOn; }
  void turnOn() override;
  void turnOff() override;
  void clear() override;
  void startFrame(Color bkg = DARK) override;
  void setTextSize(int sz) override;
  void setColor(Color c) override;
  void setCursor(int x, int y) override;
  void print(const char* str) override;
  void fillRect(int x, int y, int w, int h) override;
  void drawRect(int x, int y, int w, int h) override;
  void drawXbm(int x, int y, const uint8_t* bits, int w, int h) override;
  uint16_t getTextWidth(const char* str) override;
  void endFrame() override;
};
