#include <Arduino.h>
#include "target.h"

static RADIO_CLASS radio = new Module(P_LORA_NSS, P_LORA_DIO_0, P_LORA_RESET, P_LORA_DIO_1, SPI);

CRT01262M_Board         board;
WRAPPER_CLASS           radio_driver(radio, board);

ESP32RTCClock           fallback_clock;
AutoDiscoverRTCClock    rtc_clock(fallback_clock);
EnvironmentSensorManager sensors;

#ifdef DISPLAY_CLASS
  DISPLAY_CLASS       display;
  MomentaryButton     user_btn(PIN_USER_BTN);
#endif

bool radio_init() {
#ifdef P_LORA_TX_LED
    pinMode(P_LORA_TX_LED, OUTPUT);
    digitalWrite(P_LORA_TX_LED, LOW);
#endif
    fallback_clock.begin();
    rtc_clock.begin(Wire);
    return radio.std_init(&SPI);
}

mesh::LocalIdentity radio_new_identity() {
    RadioNoiseListener rng(radio);
    return mesh::LocalIdentity(&rng);
}
