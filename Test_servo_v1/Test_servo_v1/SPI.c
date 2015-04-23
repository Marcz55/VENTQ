/*
 * SPI.c
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
void spiTransmitToSensorUnit(unsigned char data)
{
    PORTA = (1<<PORTA2) | (0<<PORTA1); //Slave select
    SPDR = data;
    while(!(SPSR & (1<<SPIF))); //Vänta på att överföring är klar
    inbuffer = SPDR;
    PORTA = (1<<PORTA2) | (1<<PORTA1); //Slave deselect
}

void spiTransmitToCommUnit(unsigned char data)
{
    PORTA = (1<<PORTA1) | (0<<PORTA2); //Slave select
    SPDR = data;
    while(!(SPSR & (1<<SPIF))); //Vänta på att överföring är klar
    inbuffer = SPDR;
    PORTA = (1<<PORTA1) | (1<<PORTA2); //Slave deselect
}


void transmitDataToCommUnit(int header_, int data)
{
    uint8_t lowDataByte = data;
    uint8_t highDataByte = (data >> 8);
    spiTransmitToCommUnit(header_);
    _delay_us(5);
    spiTransmitToCommUnit(highDataByte);
    _delay_us(5);
    spiTransmitToCommUnit(lowDataByte);
    _delay_us(5);
}

void transmitDataToSensorUnit(int header_, int data)
{
    uint8_t lowDataByte = data;
    uint8_t highDataByte = (data >> 8);
    spiTransmitToSensorUnit(header_);
    _delay_us(5);
    spiTransmitToSensorUnit(highDataByte);
    _delay_us(5);
    spiTransmitToSensorUnit(lowDataByte);
    _delay_us(5);
}


// Ger tillbaka värde motsvarande den headern som skickades.
int fetchDataFromSensorUnit(int header_)
{
    uint8_t lowDataByte;
    uint8_t highDataByte;
    spiTransmitToSensorUnit(header_);
    _delay_us(5);
    spiTransmitToSensorUnit(TRASH);
    highDataByte = inbuffer;
    _delay_us(5);
    spiTransmitToSensorUnit(TRASH);
    lowDataByte = inbuffer;
    return lowDataByte + (highDataByte << 8);
}