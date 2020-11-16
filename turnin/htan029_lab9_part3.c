/*	Author: Heng Tan
 *  Partner(s) Name: 
 *	Lab Section: 024
 *	Assignment: Lab 9 Exercise 3
 *	Exercise Description: [optional - include for your own benefit]
 *
 *	I acknowledge all content contained herein, excluding template or example
 *	code, is my own original work.
 *
 *  Demo Link: https://youtu.be/KFkRXH-4X5M
 */

#include <avr/io.h>
#include <avr/interrupt.h>
#ifdef _SIMULATE_
#include "simAVRHeader.h"
#endif

static unsigned char global_out;
static unsigned char sound;
static unsigned char read;

////////////// Fixed time /////////////////////////

const unsigned char tasksNum = 4;
const unsigned long tasksPeriodGCD = 2;
const unsigned long periodBlinkLED = 1000;
const unsigned long periodThreeLEDs = 300;
const unsigned long periodOutLED = 2;
const unsigned long periodSound = 2;

////////////////////////////////////////////////////
///////////////// task struct //////////////////////

typedef struct task {
  int state; // Current state of the task
  unsigned long period; // Rate at which the task should tick
  unsigned long elapsedTime; // Time since task's previous tick
  int (*TickFct)(int); // Function to call for task's tick
} task;

task tasks[4];

enum BL_States { BL_SMStart, BL_s1 };
int TickFct_BlinkLED(int state);

enum TL_States { TL_SMStart, TL_s1, TL_s2, TL_s3 };
int TickFct_ThreeLEDs(int state);

enum OUT_States { OUT_SMStart, OUT};
int TickFct_OutLED(int state);

enum SOUND_States { SOUND_SMStart, wait, on};
int TickFct_Sound(int state);


/////////////// task struct end ////////////////////
////////////////////////////////////////////////////

////////////////////////////////////////////////////
///////////////////// Timer ////////////////////////

volatile unsigned char TimerFlag = 0;

unsigned long _avr_timer_M = 1;
unsigned long _avr_timer_cntcurr = 0;

void TimerOn(){
    TCCR1B = 0x0B;
    OCR1A = 125;
    TIMSK1 = 0x02;
    TCNT1 = 0;
    _avr_timer_cntcurr = _avr_timer_M;
    SREG |= 0x80;
}

void TimerOff(){
    TCCR1B = 0x00;
}

void TimerISR(){
    unsigned char i;
    for (i = 0; i < tasksNum; ++i) { // Heart of the scheduler code
        if ( tasks[i].elapsedTime >= tasks[i].period ) { // Ready
            tasks[i].state = tasks[i].TickFct(tasks[i].state);
            tasks[i].elapsedTime = 0;
        }
        tasks[i].elapsedTime += tasksPeriodGCD;
    }
}

ISR(TIMER1_COMPA_vect){
    _avr_timer_cntcurr--;
    if(_avr_timer_cntcurr == 0){
        TimerISR();
        _avr_timer_cntcurr = _avr_timer_M;
    }
}

void TimerSet(unsigned long M){
    _avr_timer_M = M;
    _avr_timer_cntcurr = _avr_timer_M;
}

/////////////////// Timer end //////////////////////
////////////////////////////////////////////////////

int main(void) {
    DDRA = 0x00; PORTA = 0xFF;
    DDRB = 0xFF; PORTB = 0x00;

    global_out = 0;

    unsigned char i=0;
    tasks[i].state = BL_SMStart;
    tasks[i].period = periodBlinkLED;
    tasks[i].elapsedTime = tasks[i].period;
    tasks[i].TickFct = &TickFct_BlinkLED;
    ++i;
    tasks[i].state = TL_SMStart;
    tasks[i].period = periodThreeLEDs;
    tasks[i].elapsedTime = tasks[i].period;
    tasks[i].TickFct = &TickFct_ThreeLEDs;
    ++i;
    tasks[i].state = OUT_SMStart;
    tasks[i].period = periodOutLED;
    tasks[i].elapsedTime = tasks[i].period;
    tasks[i].TickFct = &TickFct_OutLED;
    ++i;
    tasks[i].state = SOUND_SMStart;
    tasks[i].period = periodSound;
    tasks[i].elapsedTime = tasks[i].period;
    tasks[i].TickFct = &TickFct_Sound;

    TimerSet(tasksPeriodGCD);
    TimerOn();
    
    while(1) {
        read = (~PINA)  & 0x04;
    }
    return 0;
}



int TickFct_BlinkLED(int state) {
    static unsigned char tmp;
    switch(state) { // Transitions
        case BL_SMStart: // Initial transition
            tmp = 0; // Initialization behavior
            state = BL_s1;
            break;
        case BL_s1:
            state = BL_s1;
            break;
        default:
            state = BL_SMStart;
    } // Transitions

    switch(state) { // State actions
        case BL_s1:
            tmp = !tmp;
            if(tmp) global_out = global_out | 0x08;
            else global_out = global_out & 0xF7;
            break;
        default:
            break;
    } // State actions
    return state;
}

int TickFct_ThreeLEDs(int state) {
    switch(state) { // Transitions
        case TL_SMStart: // Initial transition
            state = TL_s1;
            break;
        case TL_s1:
            state = TL_s2;
            break;
        case TL_s2:
            state = TL_s3;
            break;
        case TL_s3:
            state = TL_s1;
            break;
        default:
            state = TL_SMStart;
    } // Transitions

    switch(state) { // State actions
        case TL_s1:
            global_out = global_out & 0xF8;
            global_out = global_out | 0x01;
            break;
        case TL_s2:
            global_out = global_out & 0xF8;
            global_out = global_out | 0x02;
            break;
        case TL_s3:
            global_out = global_out & 0xF8;
            global_out = global_out | 0x04;
            break;
        default:
            break;
    } // State actions
    return state;
}

int TickFct_OutLED(int state){
    static unsigned char tmp;
    switch(state) { // Transitions
        case BL_SMStart: // Initial transition
            PORTB = tmp = 0; // Initialization behavior
            state = BL_s1;
            break;
        case BL_s1:
            state = BL_s1;
            break;
        default:
            state = BL_SMStart;
    } // Transitions

    switch(state) { // State actions
        case BL_s1:
            if(read) PORTB = global_out | sound;
            else PORTB = global_out;
            break;
        default:
            break;
    } // State actions
    return state;
}

int TickFct_Sound(int state){
    
    switch (state) {
		case SOUND_SMStart:
			state = wait;
			break;
		case on:
			state = wait;
			break;
		case wait:
			if (read) { state = on;}
			else state = wait;
			break;
		default: 
			state = SOUND_SMStart;
			break;
	}
	switch (state) {
		case SOUND_SMStart: break;
		case on:
			sound = 0x10; 
			break;
		case wait:
			sound = 0x00;
			break;
		default: break;
	}
	return state;
}