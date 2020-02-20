


#ifndef _COASTER_PINS_
#define _COASTER_PINS_

#include "nrf_gpio.h"

#define my_pin NRF_GPIO_PIN_MAP(0,11)

typedef enum{
    DEEP_SLEEP_EPD,
    POWER_OF_EPD,
}STATES;

#endif