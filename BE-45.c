/* 
Name: Blinking Eyes
Author: Brad Blumenthal
Copyright: 2012 Brad Blumenthal
License: GPLv3
 */

#define F_CPU 128000UL

#include <avr/io.h>      
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdlib.h>
#include <inttypes.h>

uint8_t eyes_open;
volatile uint8_t blink_count;
volatile uint8_t blink_flag;
volatile uint8_t tick_flag;
volatile uint8_t getting_brighter;
const uint8_t min_bright=16;
const uint8_t max_bright=128;
volatile uint8_t brightness;
uint8_t lfsr; // Linear Feedback Shift Register
const uint8_t min_blink = 32u; // don't blink more than once every 3 secs or so


ISR (TIMER1_OVF_vect) {
  TIMSK &= ~_BV(TOIE1); // I have this thing about shutting off interrupts
  sei(); // ... but only the ones I'm servicing

  tick_flag = 1;
  blink_count--;
  if (!blink_count) {
    blink_flag = 1;
  }
  TIMSK |= _BV(TOIE1);
}

void init_timer0_PWM(void) {
  DDRB |= (1<<DDB0) | (1<<DDB1); // Data direction even for PWM
  // Setting up the Timer / Counter Prescalar and Clock Source (TTCR0A/B)
  TCCR0A |= (1<<WGM01) | (1<<WGM00); // Fast PWM
  TCCR0A |= (1<<COM0A1) | (1<<COM0B1); // Clear on match; set at BOTTOM
  TCCR0B |= (1<<CS00); // No pre-scaler

  brightness = max_bright;
  OCR0A = brightness; // OCR0A/B are the registers that control PWM on 
  OCR0B = brightness; // pins 5 and 6 (OC0A/B)
  getting_brighter = 0;

}

int main(void) {

  // Timer1 set to CK/64 or about 256 counts * ~10 (8) hZ at 128KhZ clock rate
  TCCR1 |= _BV(CS12) | _BV(CS11) | _BV(CS10); 
  TIMSK |= _BV(TOIE1);  // Enable Timer/Counter1 Overflow Interrupt

  //  Turn on the eyes
  eyes_open = 1;
  blink_flag = 0;
  init_timer0_PWM();
  lfsr = 23; // pick your favorite magic number to seed (just not 0)
  blink_count = (lfsr & 0x3F) | min_blink; 
  // ^^ more efficient than max(blink_count, min_blink)
  lfsr = (lfsr >> 1) ^ (-(lfsr & 1u) & 0xF0u);

  sei(); // Release the hounds (enable interrupts)

  while(1){

    if (tick_flag) {
      tick_flag = 0;
      if (blink_flag) {
	blink_flag = 0;
	if (eyes_open) {
	  eyes_open = 0;
	  // Turn eyes off by setting compare output mode to normal operation
	  TCCR0A &= ~(1<<COM0A1) & ~(1<<COM0B1);
	  blink_count = (lfsr & 0x01) + 1; // off for 1-2 ticks
	  	}
	else {
	  eyes_open = 1;
	  // Turn eyes on by setting mode to fast PWM.  (Eeek!)
	  TCCR0A |= (1<<COM0A1) | (1<<COM0B1);
	  blink_count = (lfsr & 0x3F) | min_blink;
	  lfsr = (lfsr >> 1) ^ (-(lfsr & 1u) & 0xF0u);
	}
      }
      else { // One "tick,"  but we didn't blink...
	if (getting_brighter) {
	  brightness++;
	  brightness++;
	  OCR0A = brightness;
	  OCR0B = brightness;
	  if (brightness >= max_bright) 
	    getting_brighter = 0;
	} else {
	  brightness--;
	  brightness--;
	  OCR0A = brightness;
	  OCR0B = brightness;
	  if (brightness <= min_bright)
	    getting_brighter = 1;
	}
      }
    }
  }
  return(0);
}
