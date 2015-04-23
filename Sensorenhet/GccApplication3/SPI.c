/*
 * CFile1.c
 *
 * Created: 4/20/2015 11:26:24 AM
 *  Author: marwa828
 */ 


#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#define F_CPU 16000000UL
#include <util/delay.h>
#include "SPI.h"

void spiMasterInit(void)
{
    DDRA = (1<<PORTA2)|(1<<PORTA1); // Denna port skickar slave-select till en annan processor.
    PORTA = (1<<PORTA2); //Ha ingen slav vald
    DDRB = (1<<PORTB5)|(1<<PORTB7)|(0<<PORTB4); //Definiera outputs. PortB4 skall hållas hög på master.
    SPCR = (1<<SPE)|(1<<MSTR)|(1<<SPR0); //Sätt enhet till master, enable spi, klockfrekvens
}

void spiSlaveInit()
{
    DDRB = (1<<PORTB6); //Alla utom MISO ska vara ingångar.
    SPCR = (1<<SPE)|(1<<SPIE); //Sätt på SPI    
}    
void spiTransmit(unsigned char data)
{
    SPDR = data;
    while(!(SPSR & (1<<SPIF))); //Vänta på att överföring är klar
    inbuffer = SPDR;
}


void transmitDataToCommUnit(int header_, int data)
{
    uint8_t lowDataByte = data;
    uint8_t highDataByte = (data >> 8);
    PORTA = (0<<PORTA2); //Slave select
    spiTransmit(header_);
    PORTA = (1<<PORTA2); //Slave deselect
    _delay_us(5);
    PORTA = (0<<PORTA2);
    spiTransmit(highDataByte);
    PORTA = (1<<PORTA2);
    _delay_us(5);
    PORTA = (0<<PORTA2);
    spiTransmit(lowDataByte);
    PORTA = (1<<PORTA2);
    _delay_us(5);
}

void transmitDataToSensorUnit(int header_, int data)
{
    uint8_t lowDataByte = data;
    uint8_t highDataByte = (data >> 8);
    PORTA = (0<<PORTA1); //Slave select
    spiTransmit(header_);
    PORTA = (1<<PORTA1); //Slave deselect
    _delay_us(5);
    PORTA = (0<<PORTA1);
    spiTransmit(highDataByte);
    PORTA = (1<<PORTA1);
    _delay_us(5);
    PORTA = (0<<PORTA1);
    spiTransmit(lowDataByte);
    PORTA = (1<<PORTA1);
    _delay_us(5);
}

