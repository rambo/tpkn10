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

void init_spi()
{
    // Init HW SPI    
    SPI.begin();
    SPI.setDataMode(SPI_MODE0);
    SPI.setBitOrder(MSBFIRST);
//    SPI.setClockDivider(SPI_CLOCK_DIV16); // 1Mhz should still work with messy cables
    SPI.setClockDivider(SPI_CLOCK_DIV32); // 500kHz should still work with messy cables
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
    /*
    Serial.print(F("select_row port value B"));
    Serial.println(LATCH_PORT, BIN);
    */
    SPI.transfer(1 << row);
    // Drive high (to latch)
    LATCH_PORT |= ROW_LATCH;
    /*
    Serial.print(F("select_row port value B"));
    Serial.println(LATCH_PORT, BIN);
    */
}

inline void select_column_drv(uint8_t columndrv)
{
    Serial.print(F("select_column_drv called with "));
    Serial.println(columndrv, DEC);
    // Make sure the value is in valid range
    columndrv &= OE_DECODER_MASK;
    OE_DECODER_PORT &= (0xff ^ OE_DECODER_MASK);
    OE_DECODER_PORT |= (columndrv << OE_DECODER_SHIFT);
    Serial.print(F("select_column_drv port value B"));
    Serial.println(OE_DECODER_PORT, BIN);
}

inline void send_column_data(uint8_t data)
{
    // Drive low
    LATCH_PORT &= (0xff ^ COLUMN_LATCH);
    /*
    Serial.print(F("send_column_data port value B"));
    Serial.println(LATCH_PORT, BIN);
    */
    SPI.transfer(data);
    // Drive high (to latch)
    LATCH_PORT |= COLUMN_LATCH;
    /*
    Serial.print(F("send_column_data port value B"));
    Serial.println(LATCH_PORT, BIN);
    */
}

void setup()
{
    Serial.begin(115200);
    init_spi();
    init_bitbang();
    Serial.print(F("Booted"));
}

uint16_t iteration;
void loop()
{
    iteration++;
    Serial.print(F("Iteration "));
    Serial.println(iteration, DEC);
    for (uint8_t cdrv=1; cdrv<=8; cdrv++)
    {
        select_column_drv(cdrv);
        for (uint8_t row=0; row<7; row++)
        {
            select_row(row);
            for (uint8_t column=0; column<5; column++)
            {
                Serial.print(F("cdrv="));
                Serial.println(cdrv, DEC);
                Serial.print(F("row="));
                Serial.println(row, DEC);
                Serial.print(F("column="));
                Serial.println(column, DEC);
                send_column_data(1 << column);
                // So eyes keep up
                delay(200);
            }
        }
    }
    //select_column_drv(0); // This disables all

    // And wait a while until next round
    delay(1500);
}

