#ifndef __SDCC
#define __sfr
#define __at(X)
#define __interrupt(X)

#define PFS173
#define F_CPU 8000000
#define TARGET_VDD_MV 3300
#define TARGET_VDD 3.3
#endif

#include <pdk/device.h>
#include "auto_sysclock.h"
#include "delay.h"
#include <stdint.h>
#include <stdlib.h>

#define RX_BIT             0

#define SetBit(A,k)     ( A|= 1 << k )
#define GetBit(A,k)    ( (A & (1 << k))!=0 )

#define getRxBit()    (PB & (1 << RX_BIT))


#define INTERVAL 16 // Serial port speed (57600)
#define DIVCONST_2 (int) (INTERVAL/1.7)
#define BIT_SHIFT_COUNT 4 // (~LOG2(INTERVAL))

#define TX_PB // if defined then TX is PB7, else PA7
#define SERIAL_TX_PIN       7


volatile uint16_t txdata;      // Serial data shift register
volatile uint16_t rxdata = 0;  // Serial data shift register
volatile uint8_t counter = 0;  // bit counter

volatile uint8_t byte_needs_processing = 0;

#define CNT_BUF_MAX 10
volatile uint8_t cnt_buf[CNT_BUF_MAX];

char buf [10];

void interrupt(void) __interrupt(0) {
if( INTRQ & INTRQ_TM2 ) //Timer2 interrupt request?
{
  counter = 0;
  byte_needs_processing = 1;
  INTRQ &= ~INTRQ_TM2; //Clear interrupt flag
}

if( INTRQ & INTRQ_PB0 )           //PA0 interrupt request?
{
    counter++;
    if(counter>=CNT_BUF_MAX)
      counter = 0;
    cnt_buf[counter] = TM2CT;
    TM2CT = 0;
    if(getRxBit())
        SetBit(rxdata,counter);
    INTRQ &= ~INTRQ_PB0; //Clear interrupt flag
}
}


void send_bit()
{
    if (txdata) {                                 // Does txdata contains bits to send?
    if (txdata & 0x01)                          // Check bit (1/0) for sending
    #ifdef TX_PB
      __set1(PB, SERIAL_TX_PIN);                // Send 1 on TX Pin
    #else
      __set1(PA, SERIAL_TX_PIN);                // Send 1 on TX Pin
    #endif
    else
    #ifdef TX_PB
      __set0(PB, SERIAL_TX_PIN);                // Send 0 on TX Pin
    #else
      __set0(PA, SERIAL_TX_PIN);                // Send 0 on TX Pin
    #endif
    txdata >>= 1;                               // Shift txdata
  }
}

int prep_char(int c) {
  txdata = (c << 1) | 0x200;                    // Setup txdata with start and stop bit
  return (c);
}


void send_char(char c)
{
  prep_char(c);
  for (uint8_t i = 0; i < 11; i++)
  {
    send_bit();
    _delay_us(INTERVAL);
  }
}

char get_hex(char c)
{
  if(c>9)
		c+=7;
	return '0' + c;
}

void send_char_hex(char c)
{
  send_char('0');
  send_char('x');
  send_char(get_hex(c >> 4));
  send_char(get_hex(0xf & c));
  send_char(' ');
}

void serial_println(char *str, uint8_t len) {
  for (uint8_t i = 0; i < len; i++)
  {
     send_char(str[i]);
  }
}

void reset_buffers()
{
  rxdata = 0;
  for (uint8_t i = 0; i < CNT_BUF_MAX; ++i)
	{
		cnt_buf[i] = 0;
  }
}

// Main program
void main() {
//setup

  TM2C = TM2C_CLK_IHRC;                         //use IHRC -> 16 Mhz
  TM2S = TM2S_PRESCALE_NONE | TM2S_PRESCALE_DIV16;  //no prescale, scale 0.25MHz
  TM2B = 0xff;

  PBDIER |= (1 << RX_BIT);       // Enable RX as digital input
 #ifdef TX_PB
  PBC |= (1 << SERIAL_TX_PIN);
 #else
  PAC |= (1 << SERIAL_TX_PIN);                 // Enable TX Pin as output
 #endif


  txdata = 0xD55F;                              // Setup 2 stop bits, 0x55 char for autobaud, 1 start bit, 5 stop bits
  for (uint8_t i = 0; i < 15; i++)
  {
    send_bit();
    _delay_us(INTERVAL);
  }


  //Setup interrupts
  INTEGS = INTEGS_PB0_BOTH;//Trigger PB0 interrupt on both edges
  INTEN = INTEN_PB0;		//Enable PB0 interrupt
  INTEN |= INTEN_TM2; //Enable Timer2 interrupt
  INTRQ = 0;					//Clear interrupt requests
  __engint();                 //Enable interrupt processing

  serial_println("\nHello, press keys:\n", 20);

  while(1)
  {
    _delay_ms(100);
    uint8_t c = 0;
    if(byte_needs_processing)
    {
      for (uint8_t i = 2; i < CNT_BUF_MAX; ++i)
      {
        uint8_t len = (cnt_buf[i]+DIVCONST_2)>>BIT_SHIFT_COUNT;
        uint8_t bit = GetBit(rxdata,i);
        for (uint8_t j = 0; j < len; ++j)
        {
          c = (c >> 1);
          if(bit == 0)
            c |= 0x80;
        }
      }
      reset_buffers();
      byte_needs_processing = 0;
    }

    if(c)
    {
      send_char(c);
      send_char(' ');
      send_char_hex(c);
      send_char('\n');
    }
  }
}


// Startup code - Setup/calibrate system clock
unsigned char _sdcc_external_startup(void) {

  // Initialize the system clock (CLKMD register) with the IHRC, ILRC, or EOSC clock source and correct divider.
  // The AUTO_INIT_SYSCLOCK() macro uses F_CPU (defined in the Makefile) to choose the IHRC or ILRC clock source and divider.
  // Alternatively, replace this with the more specific PDK_SET_SYSCLOCK(...) macro from pdk/sysclock.h
  AUTO_INIT_SYSCLOCK();

  // Insert placeholder code to tell EasyPdkProg to calibrate the IHRC or ILRC internal oscillator.
  // The AUTO_CALIBRATE_SYSCLOCK(...) macro uses F_CPU (defined in the Makefile) to choose the IHRC or ILRC oscillator.
  // Alternatively, replace this with the more specific EASY_PDK_CALIBRATE_IHRC(...) or EASY_PDK_CALIBRATE_ILRC(...) macro from easy-pdk/calibrate.h
  AUTO_CALIBRATE_SYSCLOCK(TARGET_VDD_MV);

  return 0;   // Return 0 to inform SDCC to continue with normal initialization.
}
