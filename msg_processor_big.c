
#include "msg_processor_big.h"
#include "nrf52840.h"
#include "nrf_delay.h"

extern uint8_t device_description;

static void display_timer_func(void * p_context){
  
    switch((int)p_context)
    {
        case DEEP_SLEEP_EPD:{
           deep_sleep();
           break;
        }
        case POWER_OF_EPD:{
           power_off_epd();
           break;
        }
    } 
}

void message_processing_init(){
    app_timer_create(&display_timer, APP_TIMER_MODE_SINGLE_SHOT, display_timer_func);  
    app_timer_start(display_timer, APP_TIMER_TICKS(5000), (void *)POWER_OF_EPD);
//    register_callback_function(display_callback);
}

void process_message(uint8_t * data_ptr){
    
    if(*data_ptr == '5'){
        sd_nvic_SystemReset();
    }
    power_on_epd();
    epd_upload(data_ptr,200, PAPER_FULL, false, '0');

    app_timer_start(display_timer, APP_TIMER_TICKS(5000), POWER_OF_EPD);  // making the display enter into power off mode after 5 seconds delay for saving power.

}