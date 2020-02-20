/* 
    The file contains necessary Api definitions for GDEW0213C38 2.13 inch display.
*/

#include "epd_big_apis.h"
#include "fontdatabase_big.h"
#include "nrf_gpio.h"
#include "nrf_delay.h"
#include "nrf_log.h"
#include "nrf_drv_spi.h"
#include <stdlib.h>
#include "app_timer.h"
#include "nrf_drv_gpiote.h"
#include "nrf_gpiote.h"

#define SPI_INSTANCE  0 /**< SPI instance index. */

APP_TIMER_DEF(DISPLAY_TIMEOUT_TIMER);

static const nrf_drv_spi_t spi = NRF_DRV_SPI_INSTANCE(SPI_INSTANCE);  /**< SPI instance. */
static volatile bool spi_xfer_done;  /**< Flag used to indicate that SPI instance completed the transfer. */
//static volatile bool busy_flag;

bool header_present_;
bool negate_flag;
bool background_negate;
static volatile bool display_timeout_ = false;

static uint8_t tx_buffer[1];
bool lut_partial_present = false;
static volatile active_area_t present_area;

//static const nrfx_gpiote_in_config_t epd_inconf = NRFX_GPIOTE_CONFIG_IN_SENSE_LOTOHI(1);

//void epd_event_handler(nrfx_gpiote_pin_t pin, nrf_gpiote_polarity_t action){
//    busy_flag = false;
//}

void spi_event_handler(nrf_drv_spi_evt_t const * p_event,
                       void *                    p_context)
{
    spi_xfer_done = true;
}

void display_timeout(void * p_context){
    if(display_timeout_ == true){
        sd_nvic_SystemReset();
    }
    printf("timer called no panic");
}

static void spi_write_(unsigned char value){

    unsigned char i;
    
    tx_buffer[0] = value;

    spi_xfer_done = false;

    APP_ERROR_CHECK(nrf_drv_spi_transfer(&spi, tx_buffer, sizeof(tx_buffer), NULL, NULL));

    while(!spi_xfer_done){
       sd_app_evt_wait();
    }
}

void epd_reset(){
//    LCD RESET
    nrf_gpio_pin_clear(RST_N);
    nrf_delay_ms(100);
    nrf_gpio_pin_set(RST_N);
    nrf_delay_ms(100);
}

void epd_write_cmd(unsigned char command){
    nrf_gpio_pin_clear(D_OR_C_N);
    spi_write_(command);
}

void epd_write_data(unsigned char command){
    nrf_gpio_pin_set(D_OR_C_N);
    spi_write_(command);
}

static void lut_update(uint8_t mode){

    unsigned int count;
    
    if(mode == EPD_FULL_DISPLAY_MODE){
        lut_partial_present = false;
        epd_write_cmd(0x20);
        for(count=0;count<44;count++)	     
                {epd_write_data(lut_vcomDC[count]);}

        epd_write_cmd(0x21);
        for(count=0;count<42;count++)	     
                {epd_write_data(lut_ww[count]);}   
  
        epd_write_cmd(0x22);
        for(count=0;count<42;count++)	     
                {epd_write_data(lut_bw[count]);} 

        epd_write_cmd(0x23);
        for(count=0;count<42;count++)	     
                {epd_write_data(lut_wb[count]);} 

        epd_write_cmd(0x24);
        for(count=0;count<42;count++)	     
                {epd_write_data(lut_bb[count]);}     
    }
    else if (mode == EPD_PARTIAL_DISPLAY_MODE){
        if(lut_partial_present != true){
            lut_partial_present = true;
            epd_write_cmd(0x20);
            for(count=0;count<44;count++)	     
                {epd_write_data(lut_vcom1[count]);}

            epd_write_cmd(0x21);
            for(count=0;count<42;count++)	     
                    {epd_write_data(lut_ww1[count]);}   
  
            epd_write_cmd(0x22);
            for(count=0;count<42;count++)	     
                    {epd_write_data(lut_bw1[count]);} 

            epd_write_cmd(0x24);
            for(count=0;count<42;count++)	     
                    {epd_write_data(lut_bb1[count]);} 
        }
    }
    else if(mode == EPD_RED_TEXT){
        epd_write_cmd(0x23);
        for(int count=0;count<42;count++)	     
                {epd_write_data(lut_wr1[count]);} 
    }

    else if(mode == EPD_BLACK_TEXT){
        epd_write_cmd(0x23);
        for(int count=0;count<42;count++)	     
                {epd_write_data(lut_wb1[count]);} 
    }

}

void pins_init(){

    nrf_drv_spi_config_t spi_config = NRF_DRV_SPI_DEFAULT_CONFIG;
    spi_config.ss_pin   = CS_N;
    spi_config.miso_pin = 0xff;
    spi_config.mosi_pin = SDA_;
    spi_config.sck_pin  = SCL_;
    APP_ERROR_CHECK(nrf_drv_spi_init(&spi, &spi_config, spi_event_handler, NULL));
    ret_code_t err_code;
    app_timer_create(&DISPLAY_TIMEOUT_TIMER, APP_TIMER_MODE_SINGLE_SHOT, display_timeout);
//    err_code = nrf_drv_gpiote_init();
//    APP_ERROR_CHECK(err_code);
//    nrfx_gpiote_init(); 
//    nrfx_gpiote_in_init(BUSY_N_PIN,  &epd_inconf, epd_event_handler);
//    nrfx_gpiote_in_event_enable(BUSY_N_PIN, true);

    nrf_gpio_cfg_input(BUSY_N_PIN, NRF_GPIO_PIN_NOPULL);
    nrf_gpio_cfg_output(RST_N);
    nrf_gpio_cfg_output(D_OR_C_N);
//    nrf_gpio_cfg_output(CS_N);
//    nrf_gpio_cfg_output(SCL_);
//    nrf_gpio_cfg_output(SDA_);

}

void define_partial_window(uint8_t x_start, uint8_t x_end, uint8_t y_start, uint8_t y_end){

    epd_write_cmd(0x91);		//This command makes the display enter partial mode
    epd_write_cmd(0x90);		//resolution setting
    epd_write_data (x_start);   //x-start     
    epd_write_data (x_end-1);	 //x-end	

    epd_write_data (y_start/256);
    epd_write_data (y_start%256);   //y-start    

    epd_write_data (y_end/256);		
    epd_write_data (y_end%256-1);  //y-end
    epd_write_data (0x28);	
}

static void check_status(){
    
    display_timeout_ = true;
    app_timer_stop(DISPLAY_TIMEOUT_TIMER);
    app_timer_start(DISPLAY_TIMEOUT_TIMER, APP_TIMER_TICKS(10000), NULL);
    while(true)
    {
        if(nrf_gpio_pin_read(BUSY_N_PIN)==1){
            break;
        }
        else{
        }
        nrf_delay_ms(500);
    }   
    app_timer_stop(DISPLAY_TIMEOUT_TIMER);
    display_timeout_ = false;
}

void apply_text_color(epd_background color){
    if(color == EPD_RED){
        epd_write_cmd(0x23);
        for(int count=0;count<42;count++)	     
                {epd_write_data(lut_wr1[count]);} 
    }
    else{
        epd_write_cmd(0x23);
        for(int count=0;count<42;count++)	     
                {epd_write_data(lut_wb1[count]);} 
    }
}

void apply_back_color(epd_background color){
    if(color == EPD_BLACK){
//        epd_write_cmd(0x00);			//panel setting
//        epd_write_data(0x23);		//LUT from OTP£¬128x296 --23 invert color
        negate_flag = true;
    }
    else if (color == EPD_WHITE){
//        epd_write_cmd(0x00);			//panel setting
//        epd_write_data(0xb3);		//LUT from OTP£¬128x296 --23 invert color
        negate_flag = false;
    }
}

void power_on_epd(){
    epd_write_cmd(0x04);  
    check_status();
}

void epd_init(){
     unsigned char HRES_byte1=0x03;			//800
     unsigned char HRES_byte2=0x20;
     unsigned char VRES_byte1=0x01;			//480
     unsigned char VRES_byte2=0xE0;
    
    epd_reset();			//Electronic paper IC reset	

    epd_write_cmd(0x01);			//POWER SETTING
    epd_write_data (0x07);
    epd_write_data (0x07);                       //VGH=20V,VGL=-20V
    epd_write_data (0x3f);		         //VDH=15V
    epd_write_data (0x3f);		        //VDL=-15V

    epd_write_cmd(0x04);                        //POWER ON
    nrf_delay_us(100);  
    check_status();                             //waiting for the electronic paper IC to release the idle signal
    
    epd_write_cmd(0X00);			//PANNEL SETTING
    epd_write_data(0x1F);                       //KW-3f   KWR-2F	BWROTP 0f	BWOTP 1f

    epd_write_cmd(0x61);        	        //tres			
    epd_write_data (HRES_byte1);		//source 800
    epd_write_data (HRES_byte2);
    epd_write_data (VRES_byte1);		//gate 480
    epd_write_data (VRES_byte2);
    
//    lut_update(EPD_FULL_DISPLAY_MODE);
    
    epd_write_cmd(0X15);		
    epd_write_data(0x00);		

    epd_write_cmd(0X50);			//VCOM AND DATA INTERVAL SETTING
    epd_write_data(0x10);
    epd_write_data(0x07);

    epd_write_cmd(0X60);			//TCON SETTING
    epd_write_data(0x22);

    epd_upload(default_screen,140, PAPER_FULL, true, '1');

    deep_sleep();
    power_off_epd();
}

ret_code_t epd_mode_init(uint8_t mode)
{
    if(mode == EPD_FULL_DISPLAY_MODE){

      epd_write_cmd(0x82);			//vcom_DC setting  	
      epd_write_data (0x28);	

      epd_write_cmd(0X50);			//VCOM AND DATA INTERVAL SETTING			
      epd_write_data(0x97);		//WBmode:VBDF 17|D7 VBDW 97 VBDB 57		WBRmode:VBDF F7 VBDW 77 VBDB 37  VBDR B7
    
    }
    else{
        epd_write_cmd(0x82);			//vcom_DC setting  	
        epd_write_data (0x08);	
        epd_write_cmd(0X50);
        epd_write_data(0x47);
    }
        
    lut_update(mode);
}

static void full_image_display(){

}

static void partial_display(){

}

static void upload_data(){
    epd_write_cmd(0x12);			//DISPLAY REFRESH 	
    nrf_delay_us(300);	    //!!!The delay here is necessary, 200uS at least!!!     
    check_status();
}

static void push_data(uint8_t * old_, unsigned int len1, uint8_t * new_, unsigned int len2){

    unsigned int i;

    epd_write_cmd(0x10);
    for(i=0;i<len1;i++)	     
    {
        epd_write_data(*old_++);  
    }  
    nrf_delay_ms(2);

    epd_write_cmd(0x13);
    for(i=0;i<len2;i++)	     
    {
        epd_write_data(*new_++);  
    }  
    nrf_delay_ms(2);
}

void PIC_display(unsigned char* picData)
{
  uint32_t i;
  epd_write_cmd(0x10);	     
  for(i=0;i<48000;i++)	     
  {
      epd_write_data(0x00);  
  }
  epd_write_cmd(0x13);	     
  for(i=0;i<48000;i++)	     
  {
    epd_write_data(*picData);  
    picData++;
          
  }

}

void clear_big_screen()
{
  uint32_t i;
  epd_write_cmd(0x10);	     
  for(i=0;i<48000;i++)	     
  {
      epd_write_data(0x00);  
  }
  epd_write_cmd(0x13);	     
  for(i=0;i<48000;i++)	     
  {
    epd_write_data(0xff);
          
  }

}

void clear_full_screen(uint8_t var){
    
    epd_mode_init(EPD_FULL_DISPLAY_MODE);

    unsigned int i;
    epd_write_cmd(0x10);
    for(i=0;i<2756;i++)	     
    {
        epd_write_data(0x00);  
    }  
    nrf_delay_ms(2);
  
    epd_write_cmd(0x13);
    if(var == 0x00){
        for(i=0;i<2756;i++)	     
        {
            epd_write_data(device_details[i]);
        }
    }
    else{
        for(i=0;i<2756;i++)	     
        {  
            epd_write_data(255);  
        }
    }
    nrf_delay_ms(2);
    upload_data();
}

void clear_window(uint8_t x_start, uint8_t x_end, uint8_t y_start, uint8_t y_end){

    define_partial_window(x_start, x_end, y_start, y_end);
    unsigned int no_of_pixels = x_end * y_end / 8, i;

    for(i=0;i<no_of_pixels;i++)	     
    {
        epd_write_data(0xff);  
    }  
    nrf_delay_ms(2);
  
    epd_write_cmd(0x13);
    for(i=0;i<no_of_pixels;i++)	     
    {
        epd_write_data(0x00);  
//        epd_write_data(255);  
    }
    nrf_delay_ms(2);
    upload_data();   
    check_status();
}

void enter_partial_mode(){
    epd_write_cmd(0x91);
}

void exit_partial_mode(){
    epd_write_cmd(0x92);
}

ret_code_t epd_upload(uint8_t * data_ptr, int len, pr_area area, consider_old_buffer csdr, uint8_t type){
    
    if(area == PAPER_HEADER){
//        lut_update(EPD_RED_TEXT);        
        lut_update(EPD_BLACK_TEXT);
        apply_back_color(EPD_BLACK);
        present_area.screen_width_x = 2;
        present_area.screen_height_y = 212;
        present_area.hex_buffer = 212 * 2;
        define_partial_window(SCREEN_WIDTH_PX - 16, SCREEN_WIDTH_PX , 0, 212);
        build_epic_bmp(data_ptr, len, EPD_BLACK);
    }

    else if(area == PAPER_BODY){
        lut_update(EPD_BLACK_TEXT);
        apply_back_color(EPD_WHITE);
        present_area.screen_width_x = 11;
        present_area.screen_height_y = 212;
        present_area.hex_buffer = 212 * 11;
        define_partial_window(0, SCREEN_WIDTH_PX - 16, 0, 212);
        build_epic_bmp(data_ptr, len, EPD_WHITE);
    }

    else if(area == PAPER_FOOTER){
       define_partial_window(0, 16 , 0, 212);
       build_epic_bmp(data_ptr, len, EPD_BLACK);
    }

    else if(area == PAPER_FULL){
//       lut_update(EPD_BLACK_TEXT);
       present_area.screen_width_x = 60;
       present_area.screen_height_y = 798;
       present_area.hex_buffer = 800 * 60;
//       define_partial_window(0, 104, 0, 212);
       build_epic_bmp(data_ptr, len, EPD_WHITE);
    }

    PIC_display(e_paper_image);

    upload_data();
    
    free(e_paper_image);

    return NRF_SUCCESS;
}

void deep_sleep(){
    check_status();       
    epd_write_cmd(0X07);  	//deep sleep
    check_status();
//    epd_write_data(0xA5);
}

void power_off_epd(){

//    epd_write_cmd(0X50);  	//VCOM_AND_DATA_INTERVAL_SETTING
//    epd_write_data(0X17);  	  
//    epd_write_cmd(0X82);  	//VCM_DC_SETTING_REGISTER
//    epd_write_data(0X00); 
//
//    epd_write_cmd(0X01);  
//    epd_write_data(0X02);      //gate switch to external
//    epd_write_data(0X00);
//    epd_write_data(0X00);
//    epd_write_data(0X00);
    check_status();        //waiting for the electronic paper IC to release the idle signal
    epd_write_cmd(0X02); 
}

void build_epic_bmp(uint8_t * ptr, int len, epd_background color){
    
//    bool go_to_next_line = false;
    int i,j,string_length;
    int index, space = 0;
    int m;
    int vst = 0;
    int ved = 0;
    int hst, hed;
    int jump = 0;
    int front_gap = 0;
    int top_gap = 0;
    negate_flag = false;
//    if(color==EPD_BLACK){
//        negate_flag = false;
//        background_negate = false;
//    }
//    e_paper_image = (uint8_t *)malloc(present_area.hex_buffer * sizeof(uint8_t));

    memset(e_paper_image, 0, present_area.hex_buffer);

    bool loop_break = false;
    int count = 0;
    int current_line_px = 0;
    int next_ltr_width ;
    for(string_length=0;string_length<len;string_length++){
        index = 0;
        m=1;
        if(loop_break){
            break;
        }
        switch(*ptr++){
            
            case 'A':
            {     
                 for(i = 0; i < FONT_HEIGHT; i++){
                    index = (i + current_line_px) * SCREEN_WIDTH + front_gap+FRONT_PADDING;
                    for(j = 1; j <= alpha_A[0]; j++){
                          e_paper_image[index++] = (negate_flag) ? ~alpha_A[m++] : alpha_A[m++];
                    }
                 }
                 front_gap += alpha_A[0];
            }
            break;
            case 'B':
            {     
                 for(i = 0; i < FONT_HEIGHT; i++){
                    index = (i + current_line_px) * SCREEN_WIDTH + front_gap+FRONT_PADDING;
                    for(j = 1; j <= alpha_B[0]; j++){
                          e_paper_image[index++] = (negate_flag) ? ~alpha_B[m++] : alpha_B[m++];
                    }
                 }
                 front_gap += alpha_B[0];
            }
            break;
            case 'C':
            {     
                 for(i = 0; i < FONT_HEIGHT; i++){
                    index = (i + current_line_px) * SCREEN_WIDTH + front_gap+FRONT_PADDING;
                    for(j = 1; j <= alpha_C[0]; j++){
                          e_paper_image[index++] = (negate_flag) ? ~alpha_C[m++] : alpha_C[m++];
                    }
                 }
                 front_gap += alpha_C[0];
            }
            break;
            case 'D':
            {     
                 for(i = 0; i < FONT_HEIGHT; i++){
                    index = (i + current_line_px) * SCREEN_WIDTH + front_gap+FRONT_PADDING;
                    for(j = 1; j <= alpha_D[0]; j++){
                          e_paper_image[index++] = (negate_flag) ? ~alpha_D[m++] : alpha_D[m++];
                    }
                 }
                 front_gap += alpha_D[0];
            }
            break;
            case 'E':
            {     
                 for(i = 0; i < FONT_HEIGHT; i++){
                    index = (i + current_line_px) * SCREEN_WIDTH + front_gap+FRONT_PADDING;
                    for(j = 1; j <= alpha_E[0]; j++){
                          e_paper_image[index++] = (negate_flag) ? ~alpha_E[m++] : alpha_E[m++];
                    }
                 }
                 front_gap += alpha_E[0];
            }
            break;
            case 'F':
            {     
                 for(i = 0; i < FONT_HEIGHT; i++){
                    index = (i + current_line_px) * SCREEN_WIDTH + front_gap+FRONT_PADDING;
                    for(j = 1; j <= alpha_F[0]; j++){
                          e_paper_image[index++] = (negate_flag) ? ~alpha_F[m++] : alpha_F[m++];
                    }
                 }
                 front_gap += alpha_F[0];
            }
            break;
            case 'G':
            {     
                 for(i = 0; i < FONT_HEIGHT; i++){
                    index = (i + current_line_px) * SCREEN_WIDTH + front_gap+FRONT_PADDING;
                    for(j = 1; j <= alpha_G[0]; j++){
                          e_paper_image[index++] = (negate_flag) ? ~alpha_G[m++] : alpha_G[m++];
                    }
                 }
                 front_gap += alpha_G[0];
            }
            break;
            case 'H':
            {     
                 for(i = 0; i < FONT_HEIGHT; i++){
                    index = (i + current_line_px) * SCREEN_WIDTH + front_gap+FRONT_PADDING;
                    for(j = 1; j <= alpha_H[0]; j++){
                          e_paper_image[index++] = (negate_flag) ? ~alpha_H[m++] : alpha_H[m++];
                    }
                 }
                 front_gap += alpha_H[0];
            }
            break;
            case 'I':
            {     
                 for(i = 0; i < FONT_HEIGHT; i++){
                    index = (i + current_line_px) * SCREEN_WIDTH + front_gap+FRONT_PADDING;
                    for(j = 1; j <= alpha_I[0]; j++){
                          e_paper_image[index++] = (negate_flag) ? ~alpha_I[m++] : alpha_I[m++];
                    }
                 }
                 front_gap += alpha_I[0];
            }
            break;
            case 'J':
            {     
                 for(i = 0; i < FONT_HEIGHT; i++){
                    index = (i + current_line_px) * SCREEN_WIDTH + front_gap+FRONT_PADDING;
                    for(j = 1; j <= alpha_J[0]; j++){
                          e_paper_image[index++] = (negate_flag) ? ~alpha_J[m++] : alpha_J[m++];
                    }
                 }
                 front_gap += alpha_J[0];
            }
            break;
            case 'K':
            {     
                 for(i = 0; i < FONT_HEIGHT; i++){
                    index = (i + current_line_px) * SCREEN_WIDTH + front_gap+FRONT_PADDING;
                    for(j = 1; j <= alpha_K[0]; j++){
                          e_paper_image[index++] = (negate_flag) ? ~alpha_K[m++] : alpha_K[m++];
                    }
                 }
                 front_gap += alpha_K[0];
            }
            break;
            case 'L':
            {     
                 for(i = 0; i < FONT_HEIGHT; i++){
                    index = (i + current_line_px) * SCREEN_WIDTH + front_gap+FRONT_PADDING;
                    for(j = 1; j <= alpha_L[0]; j++){
                          e_paper_image[index++] = (negate_flag) ? ~alpha_L[m++] : alpha_L[m++];
                    }
                 }
                 front_gap += alpha_L[0];
            }
            break;
            case 'M':
            {     
                 for(i = 0; i < FONT_HEIGHT; i++){
                    index = (i + current_line_px) * SCREEN_WIDTH + front_gap+FRONT_PADDING;
                    for(j = 1; j <= alpha_M[0]; j++){
                          e_paper_image[index++] = (negate_flag) ? ~alpha_M[m++] : alpha_M[m++];
                    }
                 }
                 front_gap += alpha_M[0];
            }
            break;
            case 'N':
            {     
                 for(i = 0; i < FONT_HEIGHT; i++){
                    index = (i + current_line_px) * SCREEN_WIDTH + front_gap+FRONT_PADDING;
                    for(j = 1; j <= alpha_N[0]; j++){
                          e_paper_image[index++] = (negate_flag) ? ~alpha_N[m++] : alpha_N[m++];
                    }
                 }
                 front_gap += alpha_N[0];
            }
            break;
            case 'O':
            {     
                 for(i = 0; i < FONT_HEIGHT; i++){
                    index = (i + current_line_px) * SCREEN_WIDTH + front_gap+FRONT_PADDING;
                    for(j = 1; j <= alpha_O[0]; j++){
                          e_paper_image[index++] = (negate_flag) ? ~alpha_O[m++] : alpha_O[m++];
                    }
                 }
                 front_gap += alpha_O[0];
            }
            break;
            case 'P':
            {     
                 for(i = 0; i < FONT_HEIGHT; i++){
                    index = (i + current_line_px) * SCREEN_WIDTH + front_gap+FRONT_PADDING;
                    for(j = 1; j <= alpha_P[0]; j++){
                          e_paper_image[index++] = (negate_flag) ? ~alpha_P[m++] : alpha_P[m++];
                    }
                 }
                 front_gap += alpha_P[0];
            }
            break;
            case 'Q':
            {     
                 for(i = 0; i < FONT_HEIGHT; i++){
                    index = (i + current_line_px) * SCREEN_WIDTH + front_gap+FRONT_PADDING;
                    for(j = 1; j <= alpha_Q[0]; j++){
                          e_paper_image[index++] = (negate_flag) ? ~alpha_Q[m++] : alpha_Q[m++];
                    }
                 }
                 front_gap += alpha_Q[0];
            }
            break;
            case 'R':
            {     
                 for(i = 0; i < FONT_HEIGHT; i++){
                    index = (i + current_line_px) * SCREEN_WIDTH + front_gap+FRONT_PADDING;
                    for(j = 1; j <= alpha_R[0]; j++){
                          e_paper_image[index++] = (negate_flag) ? ~alpha_R[m++] : alpha_R[m++];
                    }
                 }
                 front_gap += alpha_R[0];
            }
            break;
            case 'S':
            {     
                 for(i = 0; i < FONT_HEIGHT; i++){
                    index = (i + current_line_px) * SCREEN_WIDTH + front_gap+FRONT_PADDING;
                    for(j = 1; j <= alpha_S[0]; j++){
                          e_paper_image[index++] = (negate_flag) ? ~alpha_S[m++] : alpha_S[m++];
                    }
                 }
                 front_gap += alpha_S[0];
            }
            break;
            case 'T':
            {     
                 for(i = 0; i < FONT_HEIGHT; i++){
                    index = (i + current_line_px) * SCREEN_WIDTH + front_gap+FRONT_PADDING;
                    for(j = 1; j <= alpha_T[0]; j++){
                          e_paper_image[index++] = (negate_flag) ? ~alpha_T[m++] : alpha_T[m++];
                    }
                 }
                 front_gap += alpha_T[0];
            }
            break;
            case 'U':
            {     
                 for(i = 0; i < FONT_HEIGHT; i++){
                    index = (i + current_line_px) * SCREEN_WIDTH + front_gap+FRONT_PADDING;
                    for(j = 1; j <= alpha_U[0]; j++){
                          e_paper_image[index++] = (negate_flag) ? ~alpha_U[m++] : alpha_U[m++];
                    }
                 }
                 front_gap += alpha_U[0];
            }
            break;
            case 'V':
            {     
                 for(i = 0; i < FONT_HEIGHT; i++){
                    index = (i + current_line_px) * SCREEN_WIDTH + front_gap+FRONT_PADDING;
                    for(j = 1; j <= alpha_V[0]; j++){
                          e_paper_image[index++] = (negate_flag) ? ~alpha_V[m++] : alpha_V[m++];
                    }
                 }
                 front_gap += alpha_V[0];
            }
            break;
            case 'W':
            {     
                 for(i = 0; i < FONT_HEIGHT; i++){
                    index = (i + current_line_px) * SCREEN_WIDTH + front_gap+FRONT_PADDING;
                    for(j = 1; j <= alpha_W[0]; j++){
                          e_paper_image[index++] = (negate_flag) ? ~alpha_W[m++] : alpha_W[m++];
                    }
                 }
                 front_gap += alpha_W[0];
            }
            break;
            case 'X':
            {     
                 for(i = 0; i < FONT_HEIGHT; i++){
                    index = (i + current_line_px) * SCREEN_WIDTH + front_gap+FRONT_PADDING;
                    for(j = 1; j <= alpha_X[0]; j++){
                          e_paper_image[index++] = (negate_flag) ? ~alpha_X[m++] : alpha_X[m++];
                    }
                 }
                 front_gap += alpha_X[0];
            }
            break;
            case 'Y':
            {     
                 for(i = 0; i < FONT_HEIGHT; i++){
                    index = (i + current_line_px) * SCREEN_WIDTH + front_gap+FRONT_PADDING;
                    for(j = 1; j <= alpha_Y[0]; j++){
                          e_paper_image[index++] = (negate_flag) ? ~alpha_Y[m++] : alpha_Y[m++];
                    }
                 }
                 front_gap += alpha_Y[0];
            }
            break;
            case 'Z':
            {     
                 for(i = 0; i < FONT_HEIGHT; i++){
                    index = (i + current_line_px) * SCREEN_WIDTH + front_gap+FRONT_PADDING;
                    for(j = 1; j <= alpha_Z[0]; j++){
                          e_paper_image[index++] = (negate_flag) ? ~alpha_Z[m++] : alpha_Z[m++];
                    }
                 }
                 front_gap += alpha_Z[0];
            }
            break;

// small letters
            case 'a':
            {     
                 for(i = 0; i < FONT_HEIGHT; i++){
                    index = (i + current_line_px) * SCREEN_WIDTH + front_gap+FRONT_PADDING;
                    for(j = 1; j <= alpha_a[0]; j++){
                          e_paper_image[index++] = (negate_flag) ? ~alpha_a[m++] : alpha_a[m++];
                    }
                 }
                 front_gap += alpha_a[0];
            }
            break;
            case 'b':
            {     
                 for(i = 0; i < FONT_HEIGHT; i++){
                    index = (i + current_line_px) * SCREEN_WIDTH + front_gap+FRONT_PADDING;
                    for(j = 1; j <= alpha_b[0]; j++){
                          e_paper_image[index++] = (negate_flag) ? ~alpha_b[m++] : alpha_b[m++];
                    }
                 }
                 front_gap += alpha_b[0];
            }
            break;
            case 'c':
            {     
                 for(i = 0; i < FONT_HEIGHT; i++){
                    index = (i + current_line_px) * SCREEN_WIDTH + front_gap+FRONT_PADDING;
                    for(j = 1; j <= alpha_c[0]; j++){
                          e_paper_image[index++] = (negate_flag) ? ~alpha_c[m++] : alpha_c[m++];
                    }
                 }
                 front_gap += alpha_c[0];
            }
            break;
            case 'd':
            {     
                 for(i = 0; i < FONT_HEIGHT; i++){
                    index = (i + current_line_px) * SCREEN_WIDTH + front_gap+FRONT_PADDING;
                    for(j = 1; j <= alpha_d[0]; j++){
                          e_paper_image[index++] = (negate_flag) ? ~alpha_d[m++] : alpha_d[m++];
                    }
                 }
                 front_gap += alpha_d[0];
            }
            break;
            case 'e':
            {     
                 for(i = 0; i < FONT_HEIGHT; i++){
                    index = (i + current_line_px) * SCREEN_WIDTH + front_gap+FRONT_PADDING;
                    for(j = 1; j <= alpha_e[0]; j++){
                          e_paper_image[index++] = (negate_flag) ? ~alpha_e[m++] : alpha_e[m++];
                    }
                 }
                 front_gap += alpha_e[0];
            }
            break;
            case 'f':
            {     
                 for(i = 0; i < FONT_HEIGHT; i++){
                    index = (i + current_line_px) * SCREEN_WIDTH + front_gap+FRONT_PADDING;
                    for(j = 1; j <= alpha_f[0]; j++){
                          e_paper_image[index++] = (negate_flag) ? ~alpha_f[m++] : alpha_f[m++];
                    }
                 }
                 front_gap += alpha_f[0];
            }
            break;
            case 'g':
            {     
                 for(i = 0; i < FONT_HEIGHT; i++){
                    index = (i + current_line_px) * SCREEN_WIDTH + front_gap+FRONT_PADDING;
                    for(j = 1; j <= alpha_g[0]; j++){
                          e_paper_image[index++] = (negate_flag) ? ~alpha_g[m++] : alpha_g[m++];
                    }
                 }
                 front_gap += alpha_g[0];
            }
            break;
            case 'h':
            {     
                 for(i = 0; i < FONT_HEIGHT; i++){
                    index = (i + current_line_px) * SCREEN_WIDTH + front_gap+FRONT_PADDING;
                    for(j = 1; j <= alpha_h[0]; j++){
                          e_paper_image[index++] = (negate_flag) ? ~alpha_h[m++] : alpha_h[m++];
                    }
                 }
                 front_gap += alpha_h[0];
            }
            break;
            case 'i':
            {             
                 for(i = 0; i < FONT_HEIGHT; i++){
                    index = (i + current_line_px) * SCREEN_WIDTH + front_gap+FRONT_PADDING;
                    for(j = 1; j <= alpha_i[0]; j++){
                          e_paper_image[index++] = (negate_flag) ? ~alpha_i[m++] : alpha_i[m++];
                    }
                 }
                 front_gap += alpha_i[0];
            }
            break;
            case 'j':
            {     
                 for(i = 0; i < FONT_HEIGHT; i++){
                    index = (i + current_line_px) * SCREEN_WIDTH + front_gap+FRONT_PADDING;
                    for(j = 1; j <= alpha_j[0]; j++){
                          e_paper_image[index++] = (negate_flag) ? ~alpha_j[m++] : alpha_j[m++];
                    }
                 }
                 front_gap += alpha_j[0];
            }
            break;
            case 'k':
            {     
                 for(i = 0; i < FONT_HEIGHT; i++){
                    index = (i + current_line_px) * SCREEN_WIDTH + front_gap+FRONT_PADDING;
                    for(j = 1; j <= alpha_k[0]; j++){
                          e_paper_image[index++] = (negate_flag) ? ~alpha_k[m++] : alpha_k[m++];
                    }
                 }
                 front_gap += alpha_k[0];
            }
            break;
            case 'l':
            {     
                 for(i = 0; i < FONT_HEIGHT; i++){
                    index = (i + current_line_px) * SCREEN_WIDTH + front_gap+FRONT_PADDING;
                    for(j = 1; j <= alpha_l[0]; j++){
                          e_paper_image[index++] = (negate_flag) ? ~alpha_l[m++] : alpha_l[m++];
                    }
                 }
                 front_gap += alpha_l[0];
            }
            break;
            case 'm':
            {     
                 for(i = 0; i < FONT_HEIGHT; i++){
                    index = (i + current_line_px) * SCREEN_WIDTH + front_gap+FRONT_PADDING;
                    for(j = 1; j <= alpha_m[0]; j++){
                          e_paper_image[index++] = (negate_flag) ? ~alpha_m[m++] : alpha_m[m++];
                    }
                 }
                 front_gap += alpha_m[0];
            }
            break;
            case 'n':
            {     
                 for(i = 0; i < FONT_HEIGHT; i++){
                    index = (i + current_line_px) * SCREEN_WIDTH + front_gap+FRONT_PADDING;
                    for(j = 1; j <= alpha_n[0]; j++){
                          e_paper_image[index++] = (negate_flag) ? ~alpha_n[m++] : alpha_n[m++];
                    }
                 }
                 front_gap += alpha_n[0];
            }
            break;
            case 'o':
            {     
                 for(i = 0; i < FONT_HEIGHT; i++){
                    index = (i + current_line_px) * SCREEN_WIDTH + front_gap+FRONT_PADDING;
                    for(j = 1; j <= alpha_o[0]; j++){
                          e_paper_image[index++] = (negate_flag) ? ~alpha_o[m++] : alpha_o[m++];
                    }
                 }
                 front_gap += alpha_o[0];
            }
            break;
            case 'p':
            {     
                 for(i = 0; i < FONT_HEIGHT; i++){
                    index = (i + current_line_px) * SCREEN_WIDTH + front_gap+FRONT_PADDING;
                    for(j = 1; j <= alpha_p[0]; j++){
                          e_paper_image[index++] = (negate_flag) ? ~alpha_p[m++] : alpha_p[m++];
                    }
                 }
                 front_gap += alpha_p[0];
            }
            break;
            case 'q':
            {     
                 for(i = 0; i < FONT_HEIGHT; i++){
                    index = (i + current_line_px) * SCREEN_WIDTH + front_gap+FRONT_PADDING;
                    for(j = 1; j <= alpha_q[0]; j++){
                          e_paper_image[index++] = (negate_flag) ? ~alpha_q[m++] : alpha_q[m++];
                    }
                 }
                 front_gap += alpha_q[0];
            }
            break;
            case 'r':
            {     
                 for(i = 0; i < FONT_HEIGHT; i++){
                    index = (i + current_line_px) * SCREEN_WIDTH + front_gap+FRONT_PADDING;
                    for(j = 1; j <= alpha_r[0]; j++){
                          e_paper_image[index++] = (negate_flag) ? ~alpha_r[m++] : alpha_r[m++];
                    }
                 }
                 front_gap += alpha_r[0];
            }
            break;
            case 's':
            {     
                 for(i = 0; i < FONT_HEIGHT; i++){
                    index = (i + current_line_px) * SCREEN_WIDTH + front_gap+FRONT_PADDING;
                    for(j = 1; j <= alpha_s[0]; j++){
                          e_paper_image[index++] = (negate_flag) ? ~alpha_s[m++] : alpha_s[m++];
                    }
                 }
                 front_gap += alpha_s[0];
            }
            break;
            case 't':
            {     
                 for(i = 0; i < FONT_HEIGHT; i++){
                    index = (i + current_line_px) * SCREEN_WIDTH + front_gap+FRONT_PADDING;
                    for(j = 1; j <= alpha_t[0]; j++){
                          e_paper_image[index++] = (negate_flag) ? ~alpha_t[m++] : alpha_t[m++];
                    }
                 }
                 front_gap += alpha_t[0];
            }
            break;
            case 'u':
            {     
                 for(i = 0; i < FONT_HEIGHT; i++){
                    index = (i + current_line_px) * SCREEN_WIDTH + front_gap+FRONT_PADDING;
                    for(j = 1; j <= alpha_u[0]; j++){
                          e_paper_image[index++] = (negate_flag) ? ~alpha_u[m++] : alpha_u[m++];
                    }
                 }
                 front_gap += alpha_u[0];
            }
            break;
            case 'v':
            {     
                 for(i = 0; i < FONT_HEIGHT; i++){
                    index = (i + current_line_px) * SCREEN_WIDTH + front_gap+FRONT_PADDING;
                    for(j = 1; j <= alpha_v[0]; j++){
                          e_paper_image[index++] = (negate_flag) ? ~alpha_v[m++] : alpha_v[m++];
                    }
                 }
                 front_gap += alpha_v[0];
            }
            break;
            case 'w':
            {     
                 for(i = 0; i < FONT_HEIGHT; i++){
                    index = (i + current_line_px) * SCREEN_WIDTH + front_gap+FRONT_PADDING;
                    for(j = 1; j <= alpha_w[0]; j++){
                          e_paper_image[index++] = (negate_flag) ? ~alpha_w[m++] : alpha_w[m++];
                    }
                 }
                 front_gap += alpha_w[0];
            }
            break;
            case 'x':
            {     
                 for(i = 0; i < FONT_HEIGHT; i++){
                    index = (i + current_line_px) * SCREEN_WIDTH + front_gap+FRONT_PADDING;
                    for(j = 1; j <= alpha_x[0]; j++){
                          e_paper_image[index++] = (negate_flag) ? ~alpha_x[m++] : alpha_x[m++];
                    }
                 }
                 front_gap += alpha_x[0];
            }
            break;
            case 'y':
            {     
                 for(i = 0; i < FONT_HEIGHT; i++){
                    index = (i + current_line_px) * SCREEN_WIDTH + front_gap+FRONT_PADDING;
                    for(j = 1; j <= alpha_y[0]; j++){
                          e_paper_image[index++] = (negate_flag) ? ~alpha_y[m++] : alpha_y[m++];
                    }
                 }
                 front_gap += alpha_y[0];
            }
            break;
            case 'z':
            {     
                 for(i = 0; i < FONT_HEIGHT; i++){
                    index = (i + current_line_px) * SCREEN_WIDTH + front_gap+FRONT_PADDING;
                    for(j = 1; j <= alpha_z[0]; j++){
                          e_paper_image[index++] = (negate_flag) ? ~alpha_z[m++] : alpha_z[m++];
                    }
                 }
                 front_gap += alpha_z[0];
            }
            break;

// numbers
            case '0':
            {     
                 for(i = 0; i < FONT_HEIGHT; i++){
                    index = (i + current_line_px) * SCREEN_WIDTH + front_gap+FRONT_PADDING;
                    for(j = 1; j <= num_0[0]; j++){
                          e_paper_image[index++] = (negate_flag) ? ~num_0[m++] : num_0[m++];
                    }
                 }
                 front_gap += num_0[0];
            }
            break;
            case '1':
            {     
                 for(i = 0; i < FONT_HEIGHT; i++){
                    index = (i + current_line_px) * SCREEN_WIDTH + front_gap+FRONT_PADDING;
                    for(j = 1; j <= num_1[0]; j++){
                          e_paper_image[index++] = (negate_flag) ? ~num_1[m++] : num_1[m++];
                    }
                 }
                 front_gap += num_1[0];
            }
            break;
            case '2':
            {     
                 for(i = 0; i < FONT_HEIGHT; i++){
                    index = (i + current_line_px) * SCREEN_WIDTH + front_gap+FRONT_PADDING;
                    for(j = 1; j <= num_2[0]; j++){
                          e_paper_image[index++] = (negate_flag) ? ~num_2[m++] : num_2[m++];
                    }
                 }
                 front_gap += num_2[0];
            }
            break;
            case '3':
            {     
                 for(i = 0; i < FONT_HEIGHT; i++){
                    index = (i + current_line_px) * SCREEN_WIDTH + front_gap+FRONT_PADDING;
                    for(j = 1; j <= num_1[0]; j++){
                          e_paper_image[index++] = (negate_flag) ? ~num_3[m++] : num_3[m++];
                    }
                 }
                 front_gap += num_3[0];
            }
            break;
            case '4':
            {     
                 for(i = 0; i < FONT_HEIGHT; i++){
                    index = (i + current_line_px) * SCREEN_WIDTH + front_gap+FRONT_PADDING;
                    for(j = 1; j <= num_4[0]; j++){
                          e_paper_image[index++] = (negate_flag) ? ~num_4[m++] : num_4[m++];
                    }
                 }
                 front_gap += num_4[0];
            }
            break;
            case '5':
            {     
                 for(i = 0; i < FONT_HEIGHT; i++){
                    index = (i + current_line_px) * SCREEN_WIDTH + front_gap+FRONT_PADDING;
                    for(j = 1; j <= num_5[0]; j++){
                          e_paper_image[index++] = (negate_flag) ? ~num_5[m++] : num_5[m++];
                    }
                 }
                 front_gap += num_5[0];
            }
            break;
            case '6':
            {     
                 for(i = 0; i < FONT_HEIGHT; i++){
                    index = (i + current_line_px) * SCREEN_WIDTH + front_gap+FRONT_PADDING;
                    for(j = 1; j <= num_6[0]; j++){
                          e_paper_image[index++] = (negate_flag) ? ~num_6[m++] : num_6[m++];
                    }
                 }
                 front_gap += num_6[0];
            }
            break;
            case '7':
            {     
                 for(i = 0; i < FONT_HEIGHT; i++){
                    index = (i + current_line_px) * SCREEN_WIDTH + front_gap+FRONT_PADDING;
                    for(j = 1; j <= num_7[0]; j++){
                          e_paper_image[index++] = (negate_flag) ? ~num_7[m++] : num_7[m++];
                    }
                 }
                 front_gap += num_7[0];
            }
            break;
            case '8':
            {     
                 for(i = 0; i < FONT_HEIGHT; i++){
                    index = (i + current_line_px) * SCREEN_WIDTH + front_gap+FRONT_PADDING;
                    for(j = 1; j <= num_8[0]; j++){
                          e_paper_image[index++] = (negate_flag) ? ~num_8[m++] : num_8[m++];
                    }
                 }
                 front_gap += num_8[0];
            }
            break;
            case '9':
            {     
                 for(i = 0; i < FONT_HEIGHT; i++){
                    index = (i + current_line_px) * SCREEN_WIDTH + front_gap+FRONT_PADDING;
                    for(j = 1; j <= num_9[0]; j++){
                          e_paper_image[index++] = (negate_flag) ? ~num_9[m++] : num_9[m++];
                    }
                 }
                 front_gap += num_9[0];
            }
            break;

//punctuations

            case ',':
                 ved = vst;
                 vst = vst - comma[0];
                 for(i = 1; i <=present_area.screen_height_y; i++){
                    for(j=1;j<=present_area.screen_width_x ;j++){
                      if(i>=vst  && i<ved && j>=hst && j<=hed){
                          e_paper_image[index] = (negate_flag) ? ~comma[m++] : comma[m++];
                          index=index+1;
                      }
                      else{
                          //e_paper_image[index] = (background_negate) ? 255 : 0;
                          index=index+1;
                      }
                  }
               }
               break;
            case '.':
            {     
                 for(i = 0; i < FONT_HEIGHT; i++){
                    index = (i + current_line_px) * SCREEN_WIDTH + front_gap+FRONT_PADDING;
                    for(j = 1; j <= dot[0]; j++){
                          e_paper_image[index++] = (negate_flag) ? ~dot[m++] : dot[m++];
                    }
                 }
                 front_gap += dot[0];
            }
            break;
            case ':':
            {     
                 for(i = 0; i < FONT_HEIGHT; i++){
                    index = (i + current_line_px) * SCREEN_WIDTH + front_gap+FRONT_PADDING;
                    for(j = 1; j <= colon[0]; j++){
                          e_paper_image[index++] = (negate_flag) ? ~colon[m++] : colon[m++];
                    }
                 }
                 front_gap += colon[0];
            }
            case '#':
            {     
                 for(i = 0; i < FONT_HEIGHT; i++){
                    index = (i + current_line_px) * SCREEN_WIDTH + front_gap+FRONT_PADDING;
                    for(j = 1; j <= hash[0]; j++){
                          e_paper_image[index++] = (negate_flag) ? ~hash[m++] : hash[m++];
                    }
                 }
                 front_gap += hash[0];
            }
            break;
            case '-':
            {     
                 for(i = 0; i < FONT_HEIGHT; i++){
                    index = (i + current_line_px) * SCREEN_WIDTH + front_gap+FRONT_PADDING;
                    for(j = 1; j <= hiphen[0]; j++){
                          e_paper_image[index++] = (negate_flag) ? ~hiphen[m++] : hiphen[m++];
                    }
                 }
                 front_gap += hiphen[0];
            }
            break;
            case '^':
            {     
                 for(i = 0; i < FONT_HEIGHT; i++){
                    index = (i + current_line_px) * SCREEN_WIDTH + front_gap+FRONT_PADDING;
                    for(j = 1; j <= clock_icon[0]; j++){
                          e_paper_image[index++] = (negate_flag) ? ~clock_icon[m++] : clock_icon[m++];
                    }
                 }
                 front_gap += clock_icon[0];
            }
            break;
            case '~':
                 ved = vst;
                 vst = vst - appr[0];
                 for(i = 1; i <=present_area.screen_height_y; i++){
                    for(j=1;j<=present_area.screen_width_x ;j++){
                      if(i>=vst  && i<ved && j>=hst && j<=hed){
                          e_paper_image[index] = (negate_flag) ? ~appr[m++] : appr[m++];
                          index=index+1;
                      }
                      else{
                          //e_paper_image[index] = (background_negate) ? 255 : 0;
                          index=index+1;
                      }
                  }
               }
               break;
            case '<':
                 ved = vst;
                 vst = vst - female_icon[0];
                 for(i = 1; i <=present_area.screen_height_y; i++){
                    for(j=1;j<=present_area.screen_width_x ;j++){
                      if(i>=vst  && i<ved && j>=hst && j<=hed){
                          e_paper_image[index] = (negate_flag) ? ~female_icon[m++] : female_icon[m++];
                          index=index+1;
                      }
                      else{
                          //e_paper_image[index] = (background_negate) ? 255 : 0;
                          index=index+1;
                      }
                  }
               }
               break;
            case '`':
            {
                 for(i = 0; i < FONT_HEIGHT; i++){
                    index = (i + current_line_px) * SCREEN_WIDTH + front_gap+FRONT_PADDING;
                    for(j = 1; j <= logo[0]; j++){
                          e_paper_image[index++] = (negate_flag) ? ~logo[m++] : logo[m++];
                    }
                 }
                 front_gap += logo[0];
            }
            break;
            case ' ':
            {
                 for(i = 0; i < FONT_HEIGHT; i++){
                    index = (i + current_line_px) * SCREEN_WIDTH + front_gap+FRONT_PADDING;
                    for(j = 1; j <= 2; j++){
                          e_paper_image[index++] = (negate_flag) ? 0xff : 0x00 ;
                    }
                 }
                 front_gap += 2;
            }
            break;
            case '[':
            {
                 for(i = 0; i < FONT_HEIGHT; i++){
                    index = (i + current_line_px) * SCREEN_WIDTH + front_gap+FRONT_PADDING;
                    for(j = 1; j <= NEXT_IMAGE[0]; j++){
                          e_paper_image[index++] = (negate_flag) ? ~NEXT_IMAGE[m++] : NEXT_IMAGE[m++];
                    }
                 }
                 front_gap += NEXT_IMAGE[0];
            }
            break;
            case ']':
            {
                 for(i = 0; i < FONT_HEIGHT; i++){
                    index = (i + current_line_px) * SCREEN_WIDTH + front_gap+FRONT_PADDING;
                    for(j = 1; j <= NOW_IMAGE[0]; j++){
                          e_paper_image[index++] = (negate_flag) ? ~NOW_IMAGE[m++] : NOW_IMAGE[m++];
                    }
                 }
                 front_gap += NOW_IMAGE[0];
            }
            break;
            case '_':            
              current_line_px+=LINE_PADDING;
              front_gap = 0;
              break;
            case '*':
              vst = MAX_Y;
              ved = 0;
              hst = hed + 2;
              hed = hst + 1;
              break;
            case '|':
               ved = vst;
               char spc[2];
               spc[0] = *ptr++;
               spc[1] = *ptr++;
               space = atoi(spc);
               vst = vst - space;
              break;
            case '$':
               ved = vst;
               char spc_[3];
               spc_[0] = *ptr++;
               spc_[1] = *ptr++;
               spc_[2] = *ptr++;
               space = atoi(spc_);
               vst = vst - space;
               break;
            case '{':
               negate_flag = true;
               break;
            case '}':
               negate_flag = false;
               break;
            case '>':
              loop_break = true;
              break;
       }
       if(front_gap+FRONT_PADDING > 90){
            current_line_px += LINE_PADDING;
            front_gap = 0;
       }
       if(current_line_px > 384){
            loop_break = true;
       }
      
    }
}
