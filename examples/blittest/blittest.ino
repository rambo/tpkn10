#include <SPI.h> // This must be included first
#include <tpkn10.h>

void setup()
{
    Serial.begin(115200);
    Serial.println(F("Booting"));
    tpn10_begin();

    // Testpattern
    Serial.println(F("Creating testpattern"));
    for (uint8_t row=0; row < ROWS; row++)
    {
        for (uint8_t col=0; col < (COLUMNS/8); col++)
        {
            (*write_framebuffer)[row][col] = 0xff;
        }
        for (uint8_t i=0; i < COLUMNS; i++)
        {
            if (i % ROWS == row)
            {
                (*write_framebuffer)[row][i/8] ^= 1 << i % 8;
            }
        }
    }
    Serial.println(F("Pattern done"));
    blit();
    // To serialport, for debugging
    dump_active_framebuffer();

    Serial.println(F("Booted"));
}

void loop()
{

    // Do something with the write_framebuffer, assignment is; (*write_framebuffer)[row][col] = value then call blit_on_blank();
    delay(1000);
    // invert the pattern
    for (uint8_t row=0; row < ROWS; row++)
    {
        for (uint8_t col=0; col < (COLUMNS/8); col++)
        {
            (*write_framebuffer)[row][col] ^= 0xff;
        }
    }
    blit_on_blank();

}

