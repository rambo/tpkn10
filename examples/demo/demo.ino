#include "sintable.h"
#include <SPI.h> // This must be included first
#include <tpkn10.h>

#define COLUMNS 40
#define ROWS 7

void setup()
{
    Serial.begin(115200);
    Serial.println(F("Booting"));
    tpn10_begin();

    Serial.println(F("Booted"));
}

// Set a pixel
void pset(byte x, byte y, byte color)
{
    if (color!=0)
    {
        (*write_framebuffer)[y][x/8] |= 1<<(7-(x%8));
    } else {
        (*write_framebuffer)[y][x/8] &= ~(1<<(7-(x%8)));
    }
}

// Set a whole byte
inline void bset(byte x, byte y, byte value)
{
    (*write_framebuffer)[y][x] = value;
}


// Trying to adap the effect code from https://github.com/tanelikaivola/led_matrix_60x16/blob/master/led_matrix_60x16.ino for this smaller display, not having much luck..
// the loop routine runs over and over again forever:
void loop_effect()
{
  static byte y = 0;
  static byte t = 0;
  static unsigned int time = 0;
  static byte xstretch = 40;
  static byte ystretch = 40;
  static unsigned int v = 0;
  
  for(byte x = 0; x < (COLUMNS/8)+1; x++)
  {
      byte pix = 0;
      for(byte p = 0; p < 8; p++) {
        float color = sint((((x<<3)+p)*xstretch+t) & 0xFF);
        color += sint((y*ystretch+v) & 0xFF);
        pix = (pix << 1) + (color>sint((time>>1)&0xFF));
      }
      bset(x, y, pix);
  }

  y++;
  if (y > ROWS)
  {
      y = 0;
  }
  
  if(y==0) {
    blit_on_blank();
    memcpy(write_framebuffer, active_framebuffer, ROWS*(COLUMNS/8));
    time++;
    t+=10;
    
    v = sint(time) * 1600;
    xstretch = sint(time/8) * 20 + 30;
    ystretch = sint((time+0x100)/4) * 20 + 30;
    delay(10);
  }
}

void loop()
{
    loop_effect();
}
