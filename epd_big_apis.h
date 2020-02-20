
#ifndef GxEPD_H
#define GxEPD_H


#include "app_error.h"
#include "nrf_gpio.h"

//nrf52 EPD interfaces
#define BUSY_N_PIN NRF_GPIO_PIN_MAP(1,4)
#define RST_N      NRF_GPIO_PIN_MAP(0,20)
#define D_OR_C_N   NRF_GPIO_PIN_MAP(0,12)
#define CS_N       NRF_GPIO_PIN_MAP(0,22)
#define SCL_        NRF_GPIO_PIN_MAP(0,15)
#define SDA_        NRF_GPIO_PIN_MAP(0,24)

#define SCREEN_WIDTH_PX 480
#define SCREEN_HEIGHT_PX 800

#define SCREEN_WIDTH  100  // 0 - 99
#define SCREEN_HEIGHT 480

/*
screen_y_top
|screen_x_left            |screen_x_right
|                         |
|                         |
|                         |
|                         |
|                         |
|                         |
|                         |
|                         |
|                         |
|                         |
screen_y_bottom
*/

typedef struct {
  unsigned int screen_width_x;
  unsigned int screen_height_y;
  unsigned int hex_buffer;
}active_area_t;

#define MIN_X 0
#define MAX_X 99  //8 bit frame

#define MIN_Y 0
#define MAX_Y 479

#define LINE_PADDING 120

#define FONT_HEIGHT 95  // 0 -> 95
#define FRONT_PADDING 2  // 0 -> 95

enum Display_modes
{
    EPD_PARTIAL_DISPLAY_MODE,
    EPD_FULL_DISPLAY_MODE,
    EPD_RED_TEXT,
    EPD_BLACK_TEXT
};

typedef enum{
  PAPER_HEADER,
  PAPER_FOOTER,
  PAPER_BODY,
  PAPER_FULL
}pr_area;

typedef enum{
  EPD_WHITE,
  EPD_RED,
  EPD_BLACK = 255,
}epd_background;

typedef  void  (* display_callback)(void);

typedef bool consider_old_buffer;

void epd_write_cmd(unsigned char );

void epd_write_data(unsigned char );

static uint8_t default_screen[] = "ABCDEFGHIJKLMNPQRSTUVWXYZabcdefghihklmnopqrstu{1234}56890-^>";

//static uint8_t old_data_buffer[48000];

static uint8_t e_paper_image[48000];

//Essential APIs
void epd_reset();

static void spi_write_(uint8_t value);

void pins_init();

static void lut_update(uint8_t mode);

void enter_partial_mode();

void exit_partial_mode();

ret_code_t epd_mode_init(uint8_t mode);

ret_code_t epd_upload(uint8_t * data_ptr, int len, pr_area, consider_old_buffer, uint8_t);

static void full_image_display();

static void partial_display();

static void upload_data();

static void push_data(uint8_t * old_, unsigned int len1, uint8_t * new_, unsigned int len2);

void clear_full_screen(uint8_t );

void clear_window(uint8_t x_start, uint8_t x_end, uint8_t y_start, uint8_t y_end);

static void check_status();

void deep_sleep();

void build_epic_bmp(uint8_t * ptr , int len, epd_background);

void define_partial_window(uint8_t x_start, uint8_t x_end, uint8_t y_start, uint8_t y_end);

void apply_back_color(epd_background );

void apply_text_color(epd_background );

void power_off_epd();

void EPD_init___(void);

void epd_init();
//void register_callback_function( * display_callback);

void power_on_epd();

#endif