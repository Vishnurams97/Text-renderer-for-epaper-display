

#ifndef PROCESSOR_H
#define PROCESSOR_H

#include <stdint.h>
#include <stdbool.h>
#include "epd_big_apis.h"
#include <string.h>
#include "app_timer.h"
#include "gdew075t7.h"

APP_TIMER_DEF(display_timer);
APP_TIMER_DEF(long_press_timer);

void process_message(uint8_t *);

void process_button(uint32_t , uint32_t);

void message_processing_init();

#endif