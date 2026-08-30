#include <Arduino.h>
#include "target.h"

XiaoS3Board board;

#if defined(P_LORA_SCLK)
  static SPIClass spi;
  RADIO_CLASS radio = new Module(P_LORA_NSS, P_LORA_DIO_1, P_LORA_RESET, P_LORA_BUSY, spi);
#else
  RADIO_CLASS radio = new Module(P_LORA_NSS, P_LORA_DIO_1, P_LORA_RESET, P_LORA_BUSY);
#endif

WRAPPER_CLASS radio_driver(radio, board);

ESP32RTCClock fallback_clock;
AutoDiscoverRTCClock rtc_clock(fallback_clock);
EnvironmentSensorManager sensors;

#ifdef DISPLAY_CLASS
  DISPLAY_CLASS display;
  MomentaryButton user_btn(PIN_USER_BTN, 1000, true);
#endif

#ifdef RF_SWITCH_TABLE
static const uint32_t rfswitch_dios[] = {
  RADIOLIB_LR2021_DIO5,
  RADIOLIB_LR2021_DIO6,
  RADIOLIB_LR2021_DIO7,
  RADIOLIB_LR2021_DIO8,
  RADIOLIB_NC
};

static const Module::RfSwitchMode_t rfswitch_table[] = {
  // Mode                  DIO5  DIO6  DIO7  DIO8
  { LR2021::MODE_STBY,  { LOW,  LOW,  LOW,  LOW  } },
  { LR2021::MODE_RX,    { LOW,  LOW,  LOW,  LOW  } },
  { LR2021::MODE_TX,    { LOW,  HIGH, LOW,  LOW  } },
  { LR2021::MODE_RX_HF, { HIGH, LOW,  HIGH, LOW  } },
  { LR2021::MODE_TX_HF, { LOW,  LOW,  HIGH, HIGH } },
  END_OF_MODE_TABLE,
};
#endif

bool radio_init() {
  fallback_clock.begin();
  rtc_clock.begin(Wire);
  pinMode(21, INPUT);
  pinMode(48, OUTPUT);

#if defined(P_LORA_SCLK)
  int err = radio.std_init(&spi);
  if (err != 1) return err;
#else
  int err = radio.std_init();
  if (err != 1) return err;
#endif

#ifdef RF_SWITCH_TABLE
  radio.setRfSwitchTable(rfswitch_dios, rfswitch_table);
#endif

  return true;
}

mesh::LocalIdentity radio_new_identity() {
  RadioNoiseListener rng(radio);
  return mesh::LocalIdentity(&rng);  // create new random identity
}
