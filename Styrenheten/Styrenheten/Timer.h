/*
 * Timer.h
 *
 * Created: 4/25/2015 2:51:40 PM
 *  Author: Isak Wiberg
 */ 


#ifndef TIMER_H_
#define TIMER_H_


#include <avr/io.h>
#include <avr/interrupt.h>
#define F_CPU 16000000UL
#include <util/delay.h>

//---- Globala variabler ---

int gaitResolutionTime_g;
int commUnitUpdatePeriod_g;

// gaitResolutionTime_g represent the time between ticks in ms.
// timerOverflowMax_g
#define TIMER_0 0
#define TIMER_2 2




void timer0Init();
void timer2Init();
void setTimerPeriod(int timer, int newPeriod);
int legTimerPeriodEnd();
int commTimerPeriodEnd();
void resetLegTimer();
void resetCommTimer();




#endif /* TIMER_H_ */