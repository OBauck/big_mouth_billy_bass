
#ifndef CUSTOM_BOARD_H
#define CUSTOM_BOARD_H

#ifdef __cplusplus
extern "C" {
#endif

#include "nrf_gpio.h"

// LEDs definitions for PCA10040
#define LEDS_NUMBER    1

#define LED_START      7
#define LED_1          7
#define LED_STOP       7
   
#define LEDS_ACTIVE_STATE 0

#define LEDS_INV_MASK  LEDS_MASK

#define LEDS_LIST { LED_1 }

#define BSP_LED_0      LED_1

#define BUTTONS_NUMBER 1

#define BUTTON_START   6
#define BUTTON_1       6
#define BUTTON_STOP    6
#define BUTTON_PULL    NRF_GPIO_PIN_PULLUP

#define BUTTONS_ACTIVE_STATE 0

#define BUTTONS_LIST { BUTTON_1 }

#define BSP_BUTTON_0   BUTTON_1

#define SDC_SCK_PIN     29  ///< SDC serial clock (SCK) pin.
#define SDC_MOSI_PIN    30  ///< SDC serial data in (DI) pin.
#define SDC_MISO_PIN    28  ///< SDC serial data out (DO) pin.
#define SDC_CS_PIN      31  ///< SDC chip select (CS) pin.

#define I2S_SCK			2
#define I2S_LRCLK		4
#define I2S_SDOUT		3
#define I2S_SDIN		255
#define I2S_MCK			255

#define BILLY_HEAD_PIN	13
#define BILLY_MOUTH_PIN	11
#define BILLY_TAIL_PIN	12

// Low frequency clock source to be used by the SoftDevice
#define NRF_CLOCK_LFCLKSRC      {.source        = NRF_CLOCK_LF_SRC_XTAL,            \
                                 .rc_ctiv       = 0,                                \
                                 .rc_temp_ctiv  = 0,                                \
                                 .xtal_accuracy = NRF_CLOCK_LF_XTAL_ACCURACY_20_PPM}

#ifdef __cplusplus
}
#endif

#endif //CUSTOM_BOARD_H
