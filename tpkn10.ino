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
typedef uint8_t Framebuffer[ROWS][COLUMNS/8];

Framebuffer framebuffer_one;
Framebuffer framebuffer_two;
Framebuffer *active_framebuffer = &framebuffer_one;


volatile boolean change_row = false;
volatile int8_t current_row = -1;
volatile int8_t current_column_drv = -1;
const uint8_t coldrvs = (COLUMNS/COLS_PER_DRV);


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

inline void select_column_drv(uint8_t columndrv)
{
    OE_DECODER_PORT &= (0xff ^ OE_DECODER_MASK);
    OE_DECODER_PORT |= (columndrv << OE_DECODER_SHIFT);
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
    Serial.println(F("Booting"));
    pinMode(10, OUTPUT); // aka PB2
    init_spi();
    init_bitbang();
    
    // Testpattern
    Serial.println(F("Creating testpattern"));
    for (uint8_t row=0; row < ROWS; row++)
    {
        for (uint8_t col=0; col < (COLUMNS/8); col++)
        {
            (*active_framebuffer)[row][col] = 0xff;
        }
        for (uint8_t i=0; i < COLUMNS; i++)
        {
            if (i % ROWS == row)
            {
                (*active_framebuffer)[row][i/8] ^= 1 << i % 8;
            }
        }
    }
    Serial.println(F("Pattern done"));
    // To serialport, for debugging
    dump_active_framebuffer();

    // Get ready
    blank_screen();
    // We must start with a row change
    change_row = true;
    // Used for refreshing the screen
    Serial.println(F("Init timer"));
    initTimerCounter2();

    Serial.println(F("Booted"));
}

// To serialport, for debugging
void dump_active_framebuffer()
{
    Serial.println(F("Famebuffer:"));
    Serial.println(F("====="));
    // Dump the framebuffer_one
    for (uint8_t row=0; row < ROWS; row++)
    {
        for (uint8_t col=0; col < COLUMNS/8; col++)
        {
            for (uint8_t bitpos=0; bitpos < 8; bitpos++)
            {
                if (_BV(bitpos) & (*active_framebuffer)[row][col])
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

// This determines the refesh frequency, we must keep each column driver on for some amount of time so the LEDs have time to turn on.
void initTimerCounter2(void)
{
    // Setup for 50uSec delay (or so)
    cli();
    TCCR2A = 0;                  //stop the timer
    TCNT2 = 0;                  //zero the timer
    OCR2A = 100;      //set the compare value
    TIMSK2 = _BV(OCIE2A);         //interrupt on Compare Match A
    //start timer, ctc mode, prescaler clk/8
    TCCR2A = _BV(WGM01);
    TCCR2B = _BV(CS20) | _BV(CS21);
    sei();
}

// This handles the refreshing
ISR(TIMER2_COMPA_vect)
{
    PORTB |= _BV(2); // Turn pin 10 on (for debugging)
    // Blank for redraw
    blank_screen();
    // Enable SPI interrupts
    SPCR |= _BV(SPIE);

    // Handle row-change
    if (change_row)
    {
        current_row++;        
        if (current_row >= ROWS)
        {
            // This probably causes extra wait at end of rows
            current_row = -1;
            // Aaand recurse
            TIMER2_COMPA_vect();
            return;
        }
        // Drive low to "chip-select"
        LATCH_PORT &= (0xff ^ ROW_LATCH);
        SPDR = 1 << current_row;
        return;
    }

    // Scan columns (0-7)
    current_column_drv++;
    if (current_column_drv >= coldrvs)
    {
        current_column_drv = -1;
        change_row = true;
        // And recurse
        TIMER2_COMPA_vect();
        return;
    }

    select_column_drv(current_column_drv);
    // Drive low ("chip-select")
    LATCH_PORT &= (0xff ^ COLUMN_LATCH);

    SPDR = get_column_drv_data(current_column_drv, current_row);

    TCNT2 = 0; //zero the timer
}

// Called when transfer is done
ISR(SPI_STC_vect)
{
    // Disable the interrupt
    SPCR &= ~_BV(SPIE);
    PORTB &= ~_BV(2); // Turn pin 10 off  (for debugging)
    if (change_row)
    {
        // Drive high (to latch)
        LATCH_PORT |= ROW_LATCH;
        change_row = false;
        // And call the timer vector to handle the column.
        TIMER2_COMPA_vect();
        return;
    }
    // Drive high (to latch)
    LATCH_PORT |= COLUMN_LATCH;
    // All data is sent, re-enable the screen.
    enable_screen();
}

// This convers the simple framebuffer_one we have to correct data format for the column drivers (which are funky)
inline uint8_t get_column_drv_data(uint8_t coldrv, uint8_t row)
{
    uint8_t coldata;
    switch(coldrv)
    {
        case 7:
        {
            coldata =  ((*active_framebuffer)[row][0] & B00011111);
            break;
        }
        case 6:
        {
            coldata =  ((*active_framebuffer)[row][0] & B11100000) >> 5;
            coldata |= ((*active_framebuffer)[row][1] & B00000011) << 3;
            break;
        }
        case 5:
        {
          
            coldata =  ((*active_framebuffer)[row][1] & B01111100) >> 2; 
            break;
        }
        case 4:
        {
            coldata =  ((*active_framebuffer)[row][1] & B10000000) >> 7;
            coldata |= ((*active_framebuffer)[row][2] & B00001111) << 1;
            break;
        }
        case 3:
        {
            coldata =  ((*active_framebuffer)[row][2] & B11110000) >> 4;
            coldata |= ((*active_framebuffer)[row][3] & B00000001) << 4;
            break;
        }
        case 2:
        {
            coldata =  ((*active_framebuffer)[row][3] & B00111110) >> 1;
            break;
        }
        case 1:
        {
            coldata =  ((*active_framebuffer)[row][3] & B11000000) >> 6;
            coldata |= ((*active_framebuffer)[row][4] & B00000111) << 2;
            break;
        }
        case 0:
        {
            coldata =  ((*active_framebuffer)[row][4] & B11111000) >> 3;
            break;
        }
    }
    return coldata;
}

void loop()
{

    // Do something with the frambuffer (TODO: Double-buffer it)

}

