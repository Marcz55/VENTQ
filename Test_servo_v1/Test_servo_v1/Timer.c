/*
 * Timer.c
 *
 * Created: 4/25/2015 2:51:07 PM
 *  Author: isawi527
 */ 


#include <avr/io.h>
#include <avr/interrupt.h>
#define F_CPU 16000000UL
#include <util/delay.h>
#include "Timer.h"
#include "Definitions.h"


int timer0OverflowMax_g = 73;
int timer0RemainingTicks_g = 62;

int timer2OverflowMax_g = 73;
int timer2RemainingTicks_g = 62;




volatile uint8_t timer0TotOverflow_g;
volatile uint8_t timer2TotOverflow_g;

// klockfrekvensen är 16MHz.
void timer0Init()
{
    // prescaler 256
    TCCR0B |= (1 << CS02);
    
    // initiera counter
    TCNT0 = 0;
    
    // tillåt timer0 overflow interrupt
    
    TIMSK0 |= (1 << TOIE0);
    
    
    timer0TotOverflow_g = 0;

}

void timer2Init()
{
    // prescaler 256, denna timer har dock större upplösning här, därav andra värden
    TCCR2B |= (1 << CS22) | (1 << CS21);
    
    // initiera counter
    TCNT2 = 0;
    
    // tillåt timer2 overflow interrupt
    
    TIMSK2 |= (1 << TOIE2);
    
    timer2TotOverflow_g = 0;
}




// timerPeriod ska väljas från fördefinierade tider
void setTimerPeriod(int timer, int newPeriod)
{
    
    int newTimerRemainingTicks = 100;   // default för säkerhets skull
    int newTimerOverflowMax = 50;       // 
    switch(newPeriod)
    {
        case INCREMENT_PERIOD_10:
        {
            newTimerOverflowMax = 2;
            newTimerRemainingTicks = 113;
            break;
        }
        case INCREMENT_PERIOD_20:
        {
            
            newTimerOverflowMax = 4;
            newTimerRemainingTicks = 226;
            break;
        }
        case INCREMENT_PERIOD_30:
        {
            
            newTimerOverflowMax = 7;
            newTimerRemainingTicks = 83;
            break;
        }
        case INCREMENT_PERIOD_40:
        {
            
            newTimerOverflowMax = 9;
            newTimerRemainingTicks = 196;
            break;
        }
        case INCREMENT_PERIOD_50:
        {
            
            newTimerOverflowMax = 12;
            newTimerRemainingTicks = 53;
            break;
        }
        case INCREMENT_PERIOD_60:
        {
            
            newTimerOverflowMax = 14;
            newTimerRemainingTicks = 166;
            break;
        }
        case INCREMENT_PERIOD_70:
        {
            
            newTimerOverflowMax = 17;
            newTimerRemainingTicks = 23;
            break;
        }
        case INCREMENT_PERIOD_80:
        {
            
            newTimerOverflowMax = 19;
            newTimerRemainingTicks = 136;
            break;
        }
        case INCREMENT_PERIOD_90:
        {
            
            newTimerOverflowMax = 21;
            newTimerRemainingTicks = 249;
            break;
        }
        case INCREMENT_PERIOD_100:
        {
            
            newTimerOverflowMax = 24;
            newTimerRemainingTicks = 106;
            break;
        }
        case INCREMENT_PERIOD_200:
        {

            newTimerOverflowMax = 48;
            newTimerRemainingTicks = 212;
            break;
        }
        case INCREMENT_PERIOD_300:
        {

            newTimerOverflowMax = 73;
            newTimerRemainingTicks = 62;
            break;
        }
        case INCREMENT_PERIOD_400:
        {

            newTimerOverflowMax = 97;
            newTimerRemainingTicks = 168;
            break;
        }
        case INCREMENT_PERIOD_500:
        {

            newTimerOverflowMax = 122;
            newTimerRemainingTicks = 18;
            break;
        }
        // default:
        // Här kan man ha någon typ av felhantering om man vill

    }
    switch(timer)
    {
        case TIMER_0:
        {
            gaitResolutionTime_g = newPeriod;
            timer0OverflowMax_g = newTimerOverflowMax;
            timer0RemainingTicks_g = newTimerRemainingTicks;
            break;
        }
        case TIMER_2:
        {
            commUnitUpdatePeriod_g = newPeriod;
            timer2OverflowMax_g = newTimerOverflowMax;
            timer2RemainingTicks_g = newTimerRemainingTicks;
            break;
        }
    }
    return;
}

int legTimerPeriodEnd()
{
    if (timer0TotOverflow_g >= timer0OverflowMax_g)
    {
        if (TCNT0 >= timer0RemainingTicks_g)
        {
            return TRUE;
        }
    }
    return FALSE;
}

int commTimerPeriodEnd()
{
    if (timer2TotOverflow_g >= timer2OverflowMax_g)
    {
        if (TCNT2 >= timer2RemainingTicks_g)
        {
            return TRUE;
        }
    }
    return FALSE;
}

void resetLegTimer()
{
    TCNT0 = 0;
    timer0TotOverflow_g = 0;
}

void resetCommTimer()
{
    TCNT2 = 0;
    timer2TotOverflow_g = 0;
}

ISR(TIMER0_OVF_vect)
{
    // räkna antalet avbrott från timer0  som skett
    timer0TotOverflow_g++;
}

ISR(TIMER2_OVF_vect)
{
    // antalet avbrott från timer2
    timer2TotOverflow_g++;
}
