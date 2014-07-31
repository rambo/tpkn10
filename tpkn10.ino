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
#define LATCH_SHIFT 3

void init_spi()
{
    // Init HW SPI    
    SPI.begin();
    SPI.setDataMode(SPI_MODE0);
    SPI.setBitOrder(MSBFIRST);
    SPI.setClockDivider(SPI_CLOCK_DIV16); // 1Mhz should still work with messy cables
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

void setup()
{
    init_spi();
    init_bitbang();
}

void loop()
{
}

