#include <SPI.h>

/**
 * See the README for pins, this uses port magic for speed in the bit-banging parts
 */
#define OE_DECODER_PORT  PORTC
#define OE_DECODER_DDR   DDRC
#define OE_DECODER_MASK  B00000111
#define OE_DECODER_SHIFT 0
#define LATCH_PORT       PORTC
#define LATCH_DDR        DDRC
#define LATCH_MASK       B00111000
#define ROW_LATCH        B00010000
#define COLUMN_LATCH     B00001000
#define BLANK_SCREEN     B00100000

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
}

void init_bitbang(void)
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

inline void blank_screen(void)
{
    // Drive low
    LATCH_PORT &= (0xff ^ BLANK_SCREEN);
}

inline void enable_screen(void)
{
    // Drive low
    LATCH_PORT |= BLANK_SCREEN;
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

    // Get ready
    blank_screen();
    select_row(0);
    select_column_drv(0);
    send_column_data(0x0);

    //initTimerCounter2();

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

void initTimerCounter2(void)
{
    // Setup for 0.01s delay
    cli();
    TCCR2A = 0;                  //stop the timer
    TCNT2 = 0;                  //zero the timer
    OCR2A = 200;      //set the compare value (this is about 100uSec on 16MHz/8)
    TIMSK2 = _BV(OCIE2A);         //interrupt on Compare Match A
    //start timer, ctc mode, prescaler clk/8
    TCCR2A = _BV(WGM01);
    TCCR2B = _BV(CS20) | _BV(CS21);
    sei();
}

volatile uint8_t current_row;
volatile uint8_t current_column_drv;
const uint8_t coldrvs = (COLUMNS/COLS_PER_DRV);

ISR(TIMER2_COMPA_vect)
{
    // Draw one segment
    blank_screen();
    select_column_drv(current_column_drv);
    

    // Scan columns
    current_column_drv++;
    if (current_column_drv >= coldrvs)
    {
        current_column_drv = 0;
        current_row++;
        // And rows
        if (current_row >= ROWS)
        {
            current_row = 0;
        }
        blank_screen();
        select_row(current_row);
    }
}


inline uint8_t get_column_drv_data(uint8_t coldrv, uint8_t row)
{
    uint8_t coldata;
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
    return coldata;
}



unsigned long last_move_time;
uint8_t coldata;
uint8_t startbit;
void loop()
{

    // Do something (but do not waste time)
    // Refresh the framebuffer, TODO: move to interrupts
    for (uint8_t row=0; row < ROWS; row++)
    {
        blank_screen();
        select_row(row);
        for (uint8_t coldrv=0; coldrv < (COLUMNS/COLS_PER_DRV); coldrv++)
        {
            blank_screen();
            select_column_drv(coldrv);
            send_column_data(get_column_drv_data(coldrv, row));
            enable_screen();
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

