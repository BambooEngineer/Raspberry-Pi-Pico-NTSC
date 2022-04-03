

#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "stdio.h"
// Our assembled program:
#include "hello.pio.h"
#include "hardware/dma.h"
#include "hardware/irq.h"
#include "hardware/spi.h"
#include "hardware/i2c.h"
#include "pico/multicore.h"
#include "pico/binary_info.h"
#include "hardware/uart.h"
#include "hardware/pwm.h"

#include "graphics.h"


uint32_t pixels[262][8]; // VRAM 

int buffer_chan;
// SNES resolution was 256 by 224 ( 8 : 7 aspect ratio )
// 224 rows of ( 8 x uint32_t values ) 8 x 32 = 256 bits 

#define BUTTONS_NUMBER 12
#define I2C_ADDRESS 0x50 // int 82 = hex 0x52
uint8_t buffer1[6];
uint8_t buffer2[6];
uint8_t val = 0x00;

#define dat 7
#define lat 8
#define clk 9
uint8_t buts[16];

unsigned char sprite[] = { 0x01, 0xC0, 0x03, 0xE0, 0x07, 0xF0, 0x01, 0xC0, // 16 by 16 sprite, 16 sets of 2 chars
0x01, 0xC4, 0x03, 0xE8, 0x02, 0x30, 0x02, 0x20,  
0x02, 0x20, 0x02, 0x20, 0x07, 0xE0, 0x0B, 0x30, 
0x13, 0x30, 0x06, 0x60, 0x04, 0x40, 0x06, 0x60 };
// 510x 8 bits, 10x 8 bits for each line, 80 by 51 
unsigned char picture[] = {0xE0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0xC0, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xC0, 
0x01, 0xFF, 0x80, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x80, 0x07, 0xFF, 0xFF, 0xF0, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x07, 
0xFF, 0xFF, 0xFF, 0xC0, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x0F, 0xFF, 0xFF, 0xFF, 0xF8, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x0F, 0xFF, 
0xFF, 0xFF, 0xFF, 0xC0, 0x03, 0xFF, 0x00, 
0x00, 0x1F, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
0xFF, 0xFF, 0xC0, 0x00, 0x1F, 0xFF, 0xFF, 
0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x80, 0x00, 
0x1F, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
0xFF, 0x80, 0x00, 0x3F, 0xFF, 0xFF, 0xFF, 
0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x7C, 
0x00, 0x03, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
0x00, 0x00, 0xF0, 0x00, 0x00, 0x1F, 0xFF, 
0xFF, 0xFF, 0xFF, 0x00, 0x00, 0xE0, 0xF8, 
0x00, 0x07, 0xF8, 0x00, 0x03, 0xFF, 0x00, 
0x01, 0xFF, 0xFF, 0x00, 0x07, 0xF0, 0x00, 
0x00, 0x1F, 0x00, 0x01, 0xFE, 0x00, 0x00, 
0x07, 0xC0, 0x00, 0x00, 0x0F, 0x00, 0x01, 
0xFC, 0x00, 0x00, 0x0F, 0xE0, 0x00, 0x0F, 
0xC7, 0x00, 0x03, 0xFF, 0x00, 0x00, 0x1F, 
0xE0, 0x00, 0x01, 0xFF, 0x00, 0x03, 0xFF, 
0xFC, 0x00, 0x3F, 0xE0, 0x00, 0x00, 0x7F, 
0x00, 0x03, 0xFF, 0xFF, 0x0F, 0xFF, 0xF0, 
0x00, 0x0C, 0xFE, 0x00, 0x03, 0xFF, 0xFF, 
0xFF, 0xFF, 0xF0, 0x00, 0xFF, 0xFE, 0x00, 
0x81, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
0xFF, 0xFE, 0x00, 0x81, 0xFF, 0xFF, 0xFF, 
0xFF, 0xFF, 0xFF, 0xFF, 0xFE, 0x00, 0xC0, 
0xBF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
0xFE, 0x00, 0xE0, 0xFF, 0xFF, 0x8F, 0xFF, 
0xFF, 0xFF, 0xFF, 0xFC, 0x00, 0xF9, 0xFF, 
0xFD, 0x0F, 0xFF, 0xFE, 0x7F, 0xFF, 0xFC, 
0x20, 0xE3, 0xFF, 0xE0, 0x80, 0x7F, 0xFE, 
0x1F, 0xFF, 0xFC, 0x60, 0xF3, 0xFF, 0x87, 
0xE0, 0x0F, 0x8E, 0x07, 0xFF, 0xFC, 0xE0, 
0xFB, 0xFF, 0x8F, 0xE0, 0x00, 0x1E, 0x00, 
0x7F, 0xFF, 0xE0, 0xFF, 0xFF, 0x9F, 0xC0, 
0x00, 0x3F, 0x00, 0x7F, 0xFF, 0xE0, 0xFF, 
0xFF, 0xFC, 0x00, 0x00, 0x7F, 0x80, 0x7F, 
0xFF, 0xE0, 0xFD, 0xFF, 0xC0, 0x00, 0x00, 
0x03, 0x81, 0xFF, 0xFF, 0xE0, 0xF9, 0x1F, 
0xE0, 0x00, 0x00, 0x00, 0x01, 0xFF, 0xEF, 
0xE0, 0xFC, 0x7F, 0xFE, 0x00, 0x1D, 0xE0, 
0x0F, 0xFF, 0xDF, 0xE0, 0xFE, 0x03, 0xFF, 
0x80, 0x07, 0xFF, 0xFF, 0xFF, 0xFF, 0xE0, 
0xFF, 0x00, 0xFF, 0xF8, 0x00, 0x3F, 0xF9, 
0xFF, 0xFF, 0xE0, 0xFF, 0x80, 0x7F, 0xFE, 
0x03, 0xFF, 0xFD, 0xFF, 0xFF, 0xE0, 0xFF, 
0xC0, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x17, 
0xFF, 0xE0, 0xFF, 0xC0, 0x7F, 0xFF, 0xFF, 
0xFF, 0xFF, 0xC7, 0xFF, 0xE0, 0xFF, 0xC0, 
0x38, 0xFF, 0xFF, 0xFF, 0xFE, 0x3F, 0xFF, 
0xE0, 0xFF, 0xD8, 0x01, 0xFF, 0xFF, 0xFF, 
0xFC, 0x7F, 0xFF, 0xE0, 0xFF, 0xE4, 0x07, 
0xC7, 0xEF, 0xFF, 0x81, 0xFF, 0xFF, 0xE0, 
0xFF, 0xE6, 0x00, 0x00, 0x41, 0xFE, 0x03, 
0xFF, 0xFF, 0xE0, 0xFF, 0xCF, 0x80, 0x00, 
0x00, 0x00, 0x1F, 0xFF, 0xFF, 0xE0, 0xFF, 
0xCB, 0xF0, 0x00, 0x00, 0x00, 0x7F, 0xFF, 
0xFF, 0xE0, 0xFF, 0xCF, 0xF8, 0x00, 0x00, 
0x01, 0xFF, 0xFF, 0xFF, 0xE0, 0xFF, 0xC7, 
0xEF, 0xC0, 0x00, 0x0F, 0xFF, 0xFF, 0xFF, 
0xE0, 0xFF, 0xC3, 0xFF, 0xFE, 0x3F, 0xFF, 
0xFF, 0xFF, 0xFF, 0xE0, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

/////////////////////////////////////////////////////////////////
void sprite16_draw(int x, int y, unsigned char bits[], bool erase){ 

int sprite_y = y;
int sprite_bit = 0;  

int vramx = 0;
uint32_t bit_pos = 0;
uint32_t R = 0;


if(x <=31 && x>0) vramx = 0; 
if(x <=63 && x>31) vramx = 1;     
if(x <=95 && x>63) vramx = 2; 
if(x <=127 && x>95) vramx = 3; 
if(x <=159 && x>127) vramx = 4; 
if(x <=191 && x>159) vramx = 5;  
if(x <=223 && x>191) vramx = 6; 
if(x <=255 && x>223) vramx = 7; 



for(int i=0; i<16; ++i){

  bit_pos = ((bits[sprite_bit]<<8)|bits[sprite_bit+1]) << x%32;
  if(x%32 >= 16) R = ((bits[sprite_bit]<<8)|bits[sprite_bit+1]) >> 32-(x%32); // fixes sprite clipping when moving horizontally
  
  if(!erase) bit_pos = 0x00000000;
  if(!erase) R = 0x00000000;

  pixels[sprite_y][vramx] = bit_pos;
  pixels[sprite_y][vramx+1] = R;
  //if((x%32)>16){
    //pixels[sprite_y][vramx+1] = ((bits[sprite_bit]<<8)|bits[sprite_bit+1]) >> x+16%32; 
  //}


  bit_pos = 0;
  sprite_bit = sprite_bit + 2;
  ++sprite_y; 
}

}
//////////////////////////////////////////////////////////

// 510x 8 bits, 10x 8 bits for each line, 80 by 51 
void draw_picture(int x, int y, int height, unsigned char bits[], bool erase){ 

int sprite_y = y;
int sprite_bit = 0;  

int vramx = 0;
uint32_t bit_pos = 0;
uint32_t bit_pos2 = 0;
uint32_t bit_pos3 = 0;

if(x <=31 && x>0) vramx = 0;    
if(x <=63 && x>31) vramx = 1;
if(x <=95 && x>63) vramx = 2; 
if(x <=127 && x>95) vramx = 3; 
if(x <=159 && x>127) vramx = 4; 
if(x <=191 && x>159) vramx = 5;  
if(x <=223 && x>191) vramx = 6; 
if(x <=255 && x>223) vramx = 7; 

for(int x=0; x<height; ++x){ 

  bit_pos |= (bits[sprite_bit]<<16)|(bits[sprite_bit+1]<<8)|(bits[sprite_bit+2]); // merge 8 bits into 32
  bit_pos2 |= (bits[sprite_bit+3]<<24)|(bits[sprite_bit+4]<<16)|(bits[sprite_bit+5]<<8)|(bits[sprite_bit+6]);
  bit_pos3 |= (bits[sprite_bit+7]<<24)|(bits[sprite_bit+8]<<16)|(bits[sprite_bit+9]<<8);

  
  pixels[sprite_y][vramx] = bit_pos;
  pixels[sprite_y][vramx-1] = bit_pos2;
  pixels[sprite_y][vramx-2] = bit_pos3; // punch them into VRAM

  if(!erase){
    pixels[sprite_y][vramx] = 0x00000000;
    pixels[sprite_y][vramx-1] = 0x00000000;
    pixels[sprite_y][vramx-2] = 0x00000000;  
  }
  

  bit_pos = 0;
  bit_pos2 = 0;
  bit_pos3 = 0;
  sprite_bit = sprite_bit + 10; 
  ++sprite_y; // next line
}

}
//////////////////////////////////////////////////////
void plot_pixel(int x, int y, bool erase){
    int vramx = 0;
    uint32_t bit_pos = 0;
    if(x <=31 && x>0) vramx = 0; // 0
    if(x <=63 && x>31) vramx = 1; // 32
    if(x <=95 && x>63) vramx = 2; // 64
    if(x <=127 && x>95) vramx = 3; // 96
    if(x <=159 && x>127) vramx = 4; // 128
    if(x <=191 && x>159) vramx = 5; // 160
    if(x <=223 && x>191) vramx = 6; // 192
    if(x <=255 && x>223) vramx = 7; // 224
    bit_pos |= 1<<(x%32);
    if(!erase) pixels[y][vramx] = bit_pos;
    else{
      pixels[y][vramx] = 0x00000000;
    }
    
}
////////////////////////////////////////////
void read_Controller(){
   
  gpio_put(lat, 1);
  sleep_us(4);
  gpio_put(clk, 1);
  sleep_us(4);
  gpio_put(lat, 0);
  sleep_us(4);
  for(int i =0;i<16;++i){
    gpio_put(clk, 0);
    sleep_us(4);
    if(!gpio_get(dat)){
      buts[i] = 0;
      //Serial.println(buts[i]);
    }
    else{
      buts[i] = 1;
    }
    gpio_put(clk, 1);
    sleep_us(4);
  }
  gpio_put(lat, 1);
  sleep_us(4);
  
}

////////////////////////////////////////////

int r = 0;
//////////////////////////////////////////////////////////// dma
uint32_t Dma_Line[] = {0x00000000};
uint32_t Dma_Line2[] = {0xFFFFFFFF};
int dchan = 0;
///////// Game variables
int playerx=90;
int playery=100;
int spritePointer=0; // spritesP1[34] && spritesP2[34]
uint64_t refresh=0;
uint64_t currentMillis=0;
uint64_t spriteT=0;
uint32_t clear_screen=0;

static int line=0;
static int Line2=0;
bool hsync = false;

void dma_handler() {
    // Clear the interrupt request.
    dma_hw->ints0 = 1u << dchan; 
    // Give the channel a new addr to read from, and re-trigger it
    
    dma_channel_set_read_addr(dchan, pixels, true); 
    //dma_channel_set_read_addr(dchan, Dma_Line, true); } // dummy bytes beyond the resolution
    //dma_channel_set_read_addr(dchan, Dma_Line2, true);
    
    
}
//////////////////////////////////////////////////////////

void timing_core(){
    PIO pio_S = pio0;  // pio
    uint sync_mem = pio_add_program(pio_S, &NTSC_Sync_program); // program location
    uint sm_S = pio_claim_unused_sm(pio_S, true); // state machine
    NTSC_Sync_program_init(pio_S, sm_S, sync_mem, 0); // start program
////////////////////////////////////////////////////////////////////////////////////////
    PIO pio = pio1; 
    uint pixels_mem = pio_add_program(pio, &pixels_program); 
    uint sm2 = pio_claim_unused_sm(pio, true); 
///////////////////////////////////////////////////
    dchan = dma_claim_unused_channel(true);
    dma_channel_config c = dma_channel_get_default_config(dchan);

    channel_config_set_transfer_data_size(&c, DMA_SIZE_32); // since pio can pull 32 bits
    channel_config_set_read_increment(&c, true);  // address needs to iterate through vram
    channel_config_set_write_increment(&c, false);  // write address needs to stay at pixel PIO
    channel_config_set_dreq(&c, pio_get_dreq(pio, sm2, true)); // DMA pacing is set to the pace of the pixel pio
    

    dma_channel_configure(
        dchan,          // Channel to be configured
        &c,            // The configuration we just created
        &pio->txf[sm2],           // The initial write address      // 
        pixels,           // The initial read address         // PIXELS[LINE]
        2096, // Number of transfers needed;                                
        false           // dont start yet
    );

    // Tell the DMA to raise IRQ line 0 when the channel finishes a block
    dma_channel_set_irq0_enabled(dchan, true);  // give dma channel irq0
    irq_set_exclusive_handler(DMA_IRQ_0, dma_handler); // give irq0 a handler function
    irq_set_enabled(DMA_IRQ_0, true); // enable irq0
    
    //pixels[100][5] = 0xFFFFFFFF;
    draw_picture(100, 100, 51, picture, true);
    pixels_program_init(pio, sm2, pixels_mem, 1, 2);
    dma_handler();
    //////////////////////////////////////////////////////
while(1) {      

    //for(int i = 0; i<262; i++){         // 245 lines visible        ( old pio/multicore handshaking method )
        //sleep_us(4); // sync rise delay
        //pio_sm_put_blocking(pio, sm_S, 0); // go high 
        //sleep_us(59);
        //pio_sm_put_blocking(pio, sm_S, 0);  // tell pio to go low
    //}  // pixels

    read_Controller();

    uint32_t currentMillis = time_us_32(); 
        if(currentMillis - refresh > 16000) { 

          refresh = currentMillis;
          
          if(!buts[6]){
          --playerx;
            if(currentMillis - spriteT > 100000) { // button presses & sprite movement
              spriteT = currentMillis;
              ++spritePointer;
              if(spritePointer >= 8){
                      spritePointer = 0;
                }
            }
          }
          if(!buts[7]){
            ++playerx;
            if(currentMillis - spriteT > 100000) { 
              spriteT = currentMillis;
              ++spritePointer;
              if(spritePointer >= 18 || spritePointer < 10){
                      spritePointer = 10;
                }
            }
          }
          if(!buts[4]) --playery;
          if(!buts[5]) ++playery;
          }

    draw_picture(100, 100, 51, picture, true);
    sprite16_draw(playerx, playery-16, spritesP1[spritePointer], false);
    sprite16_draw(playerx-16, playery, spritesP1[spritePointer], false);
    sprite16_draw(playerx, playery+16, spritesP1[spritePointer], false);
    sprite16_draw(playerx+32, playery, spritesP1[spritePointer], false);
    sprite16_draw(playerx, playery, spritesP1[spritePointer], true);
    // border guys
    sprite16_draw(240, 30, spritesP1[spritePointer], true);
    sprite16_draw(8, 30, spritesP1[spritePointer], true);
    
    
   

    sleep_ms(16);   
    
    
    
  }

}         

int main() {     
    //stdio_init_all();

    gpio_init(lat);
    gpio_init(clk);
    gpio_init(dat);
    gpio_set_dir(lat, 1);
    gpio_set_dir(clk, 1);
    gpio_set_dir(dat, 0);
  
    for(int i=0; i<17; ++i){
      for(int x = 0; x<8; ++x){
        pixels[i+245][x] = 0x00000000; // lines are visible up to 245, the rest of the lines should have no data
      }
    }
    
    multicore_launch_core1(timing_core);


//sprite16_draw(200, 100, sprite);
//draw_picture(100, 30, 51, picture); // ****this function is specific to the picture above*****
    uint32_t game;
    unsigned char buffer[16];

    bool up= true;
    bool down= false;
    bool left= true;
    bool right = false;
    int ballx = 100;
    int bally = 100;

    int paddle1x = 16;
    int paddle1y = 100;
    int paddle2x = 240;
    int paddle2y = 100;

bool paddle1up=false; // !<35 or !>220
bool paddle1down=true;
bool paddle2up=true;
bool paddle2down=false;
bool gameover=false;

bool debug = true;
bool Run_from_cart = false;
bool pong = true;
bool sprite_demo = false; // fix clipping then start sprite scaling 
//double_buffer();



if(debug){ // fill screen to check for blinking or distortion 
    //for(int Ls=0;Ls<245;++Ls){
     // for(int fill=4;fill<5;++fill){
     //   pixels[Ls][fill] = 0xFFFFFFFF;
    //  }
    //}
   // pixels[100][0] = 0x00000800; // bits are displayed from LSB to MSB per scanline 
    //pixels[100][7] = 0x80000000; 
  
    while(1){
      tight_loop_contents();
    }
}

while(!debug) {    
			

  if(Run_from_cart){
		
  tight_loop_contents();
		
      
  }

  
  if(!Run_from_cart){ 

     if(pong){
      while(1){
      
          //read_Controller();
          sleep_ms(15);
          
            plot_pixel(ballx, bally, true);          // Pong ball animation on the pico 

            if(up) --bally;
              
            if(down) ++bally;
              
            if(left) --ballx;
              
            if(right) ++ballx; // paddle1 is on left && paddle2 is on right

            if(ballx>240 && right){
                right = false;
                left = true;
                if(!(bally > paddle2y && bally < paddle2y+20)){
                  ballx = 238;
                  //gameover=true;
                }
            }
            if(ballx<15 && left){
                right = true;
                left = false;
                if(!(bally > paddle1y && bally < paddle1y+20)){
                  ballx = 18;
                  //gameover=true;
                }
            }
            if(bally<35 && up){
              up = false;
              down = true;
            }
            if(bally>230 && down){
              up = true;
              down = false;
            }

            /*for(int line = 0; line < 20; ++line){ // Draw both paddles 
              plot_pixel(paddle1x, paddle1y+line, false);
              plot_pixel(paddle2x, paddle2y+line, false);
            }
            for(int line = 0; line < 1; ++line){
              plot_pixel(paddle1x, paddle1y+20+line, true); 
              plot_pixel(paddle2x, paddle2y+20+line, true); 
              plot_pixel(paddle1x, paddle1y-line, true);
              plot_pixel(paddle2x, paddle2y-line, true);
            }*/

            if(!(paddle1y > 205) && !buts[1]){
              ++paddle1y;
            }
            if(!(paddle1y < 35) && !buts[0]){
              --paddle1y;
            }

            if(!(paddle2y > 205) && !buts[5]){
              ++paddle2y;
            }
            if(!(paddle2y < 35) && !buts[4]){
              --paddle2y;
            }

            //sprite16_draw(paddle1x, paddle1y, sprite);
            //sprite16_draw(paddle2x, paddle2y, sprite);
            plot_pixel(ballx, bally, false);
            if(gameover){
              draw_picture(100, 100, 51, picture, true); 
              sprite16_draw(200, 200, sprite, true);
              sleep_ms(1000);
              draw_picture(100, 100, 51, picture, false);
              sprite16_draw(200, 200, sprite, false);
              gameover = false;
            }
          
          
     
      }
     }



  }

    
    



  }
}


//tight_loop_contents();
 
 
