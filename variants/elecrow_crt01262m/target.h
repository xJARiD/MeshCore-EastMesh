#pragma once

#define RADIOLIB_STATIC_ONLY 1
#include <SPI.h>
#include <helpers/radiolib/RadioLibWrappers.h>
#include <helpers/radiolib/CustomSX1276Wrapper.h>
#include <helpers/ESP32Board.h>
#include <helpers/AutoDiscoverRTCClock.h>
#include <helpers/sensors/EnvironmentSensorManager.h>
#ifdef DISPLAY_CLASS
  #include <helpers/ui/ST7735Display.h>
  #include <helpers/ui/MomentaryButton.h>
#endif

class CRT01262M_Board : public ESP32Board {
public:
  void begin() {
    SPI.begin(P_LORA_SCLK, P_LORA_MISO, P_LORA_MOSI);
    ESP32Board::begin();
  }
};

extern CRT01262M_Board board;
extern WRAPPER_CLASS radio_driver;
extern AutoDiscoverRTCClock rtc_clock;
extern EnvironmentSensorManager sensors;

#ifdef DISPLAY_CLASS
  extern DISPLAY_CLASS display;
  extern MomentaryButton user_btn;
#endif

bool radio_init();
mesh::LocalIdentity radio_new_identity();
