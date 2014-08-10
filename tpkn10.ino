#include <SPI.h>

/**
 * See the README for pins, this uses port magic for speed in the bit-banging parts
 */
#define OE_DECODER_PORT PORTC
#define OE_DECODER_DDR DDRC
#define OE_DECODER_MASK B00000111
#define OE_DECODER_SHIFT 0
#define LATCH_PORT PORTC
#define LATCH_DDR DDRC
#define LATCH_MASK B00011000
#define ROW_LATCH B00010000
#define COLUMN_LATCH B00001000


#define ROWS 7
#define COLUMNS 40
#define COLS_PER_DRV 5

// 7 rows by 40 leds
uint8_t framebuffer[ROWS][COLUMNS/8];

void init_spi()
{
    // Init HW SPI    
    SPI.begin();
    SPI.setDataMode(SPI_MODE0);
    SPI.setBitOrder(MSBFIRST);
    SPI.setClockDivider(SPI_CLOCK_DIV2); 
    //SPI.setClockDivider(SPI_CLOCK_DIV16); // 1Mhz should still work with messy cables
    //SPI.setClockDivider(SPI_CLOCK_DIV32); // 500kHz should still work with messy cables
    //SPI.setClockDivider(SPI_CLOCK_DIV64); // 500kHz should still work with messy cables
}

void init_bitbang()
{
    // Set the OE and latch pins as outputs
    OE_DECODER_DDR |= (0xff & OE_DECODER_MASK);
    LATCH_DDR |= (0xff & LATCH_MASK);
    // And init low
    OE_DECODER_PORT &= (0xff ^ OE_DECODER_MASK);
    LATCH_PORT &= (0xff ^ LATCH_MASK);
}

inline void select_row(uint8_t row)
{
    // Drive low
    LATCH_PORT &= (0xff ^ ROW_LATCH);
    SPI.transfer(1 << row);
    // Drive high (to latch)
    LATCH_PORT |= ROW_LATCH;
}

inline void select_column_drv(uint8_t columndrv)
{
    OE_DECODER_PORT &= (0xff ^ OE_DECODER_MASK);
    OE_DECODER_PORT |= (columndrv << OE_DECODER_SHIFT);
}

inline void send_column_data(uint8_t data)
{
    // Drive low
    LATCH_PORT &= (0xff ^ COLUMN_LATCH);
    SPI.transfer(data);
    // Drive high (to latch)
    LATCH_PORT |= COLUMN_LATCH;
}

void setup()
{
    Serial.begin(115200);
    init_spi();
    init_bitbang();
    
    // Testpattern
    for (uint8_t row=0; row < ROWS; row++)
    {
        for (uint8_t col=0; col < (COLUMNS/8); col++)
        {
            framebuffer[row][col] = 0xff;
        }
        for (uint8_t i=0; i < COLUMNS; i++)
        {
            if (i % ROWS == row)
            {
                framebuffer[row][i/8] ^= 1 << i % 8;
            }
        }
        /*
        // Invert the pattern
        for (uint8_t col=0; 
        col < (COLUMNS/8); col++)
        {
            framebuffer[row][col] ^= 0xff;
        }
        
        */
        /*
        Famebuffer:
        =====
        10000 00100 00001 00000 01000 00010 00000 10000
        01000 00010 00000 10000 00100 00001 00000 01000
        00100 00001 00000 01000 00010 00000 10000 00100
        00010 00000 10000 00100 00001 00000 01000 00010
        00001 00000 01000 00010 00000 10000 00100 00001
        00000 10000 00100 00001 00000 01000 00010 00000
        00000 01000 00010 00000 10000 00100 00001 00000
        =====
        Booted
        */
    }
    
    dump_framebuffer();

    Serial.print(F("Booted"));
}

void dump_framebuffer()
{
    Serial.println(F("Famebuffer:"));
    Serial.println(F("====="));
    // Dump the framebuffer
    for (uint8_t row=0; row < ROWS; row++)
    {
        for (uint8_t col=0; col < COLUMNS/8; col++)
        {
            for (uint8_t bitpos=0; bitpos < 8; bitpos++)
            {
                if (_BV(bitpos) & framebuffer[row][col])
                {
                    Serial.print("1");
                }
                else
                {
                    Serial.print("0");
                }
            }
        }
        Serial.println();
    }
    Serial.println(F("====="));
}

unsigned long last_move_time;
uint8_t coldata;
uint8_t startbit;
void loop()
{

    // Do something (but do not waste time)
    // Refresh the framebuffer
    for (uint8_t row=0; row < ROWS; row++)
    {
        select_row(row);
        for (uint8_t coldrv=0; coldrv < (COLUMNS/COLS_PER_DRV); coldrv++)
        {
            select_column_drv(coldrv);
            switch(coldrv)
            {
                case 7:
                {
                    coldata =  (framebuffer[row][0] & B00011111);
                    break;
                }
                case 6:
                {
                    coldata =  (framebuffer[row][0] & B11100000) >> 5;
                    coldata |= (framebuffer[row][1] & B00000011) << 3;
                    break;
                }
                case 5:
                {
                  
                    coldata =  (framebuffer[row][1] & B01111100) >> 2; 
                    break;
                }
                case 4:
                {
                    coldata =  (framebuffer[row][1] & B10000000) >> 7;
                    coldata |= (framebuffer[row][2] & B00001111) << 1;
                    break;
                }
                case 3:
                {
                    coldata =  (framebuffer[row][2] & B11110000) >> 4;
                    coldata |= (framebuffer[row][3] & B00000001) << 4;
                    break;
                }
                case 2:
                {
                    coldata =  (framebuffer[row][3] & B00111110) >> 1;
                    break;
                }
                case 1:
                {
                    coldata =  (framebuffer[row][3] & B11000000) >> 6;
                    coldata |= (framebuffer[row][4] & B00000111) << 2;
                    break;
                }
                case 0:
                {
                    coldata =  (framebuffer[row][4] & B11111000) >> 3;
                    break;
                }
            }
            send_column_data(coldata);
            delayMicroseconds(200);
        }
    }


     /* Testing 
    for (uint8_t cdrv=0; cdrv<8; cdrv++)
    {
        select_column_drv(cdrv);
        for (uint8_t row=0; row<7; row++)
        {
            select_row(row);
            for (uint8_t column=0; column<5; column++)
            {
                send_column_data(1 << column);
                // So eyes keep up
                delay(25);
            }
        }
    }
    */


    /* Testing 2
    for (uint8_t cdrv=0; cdrv<8; cdrv++)
    {
        select_column_drv(cdrv);
        for (uint8_t row=0; row<7; row++)
        {
            select_row(row);
            send_column_data(B00011111);
            // So eyes keep up
            //delay(25);
        }
    }
    */
}

