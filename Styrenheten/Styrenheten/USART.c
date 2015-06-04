/*
 * USART.c
 *
 * Created: 4/15/2015 10:24:52 AM
 *  Author: isawi527
 */ 

#include <avr/io.h>
#include <avr/interrupt.h>
#define F_CPU 16000000UL
#include <util/delay.h>
#include "USART.h"

// -- USART definitioner
#define TXD0_READY bit_is_set(UCSR0A,5)
#define TXD0_FINISHED bit_is_set(UCSR0A,6)
#define RXD0_READY bit_is_set(UCSR0A,7)
#define TXD0_DATA (UDR0)
#define RXD0_DATA (UDR0)

void USART0RecieveMode() 
{
    PORTD = (0<<PORTD4);
}

void USART0SendMode()
{
    PORTD = (1<<PORTD4);
}

void initUSART()
{
    DDRD = (1<<PORTD4); // Styrsignal för sändning/mottagning, PD2
//  DDRA = (0<<PORTA0); // Signal från extern knapp, kan användas till diverse saker
    USART0RecieveMode();
    UBRR0H = 0x00;
    UBRR0L = 0x00; // Sätter baudraten till fosc/16(UBRR0 + 1) = 1Mhz
    
    UCSR0B = (1<<RXEN0)|(1<<TXEN0); // Aktiverar sändare och mottagare
    
//  USBS0 = 0; // En stoppbit
    //UPM01 = 0; // Ingen paritetsbit
    
    UCSR0C = (0<<USBS0) | (3<<UCSZ00) | (0<<UPM01);
    /*
    UCSZ00 = 1;
    UCSZ01 = 1;
    UCSZ02 = 0; // Sätter antalet databitar i varje paket till 8.
    */

    
}
void USARTWriteChar(char data)
{
    // vänta tills sändaren är redo
    while(!TXD0_READY) //UDRE0 sätts till 1 när buffern är tom
    {
        // gör ingenting
    }
    TXD0_DATA = data;
}
void USARTSendInstruction0(int ID, int instruction)
{
    
    // sätt USART till sändläge
    USART0SendMode();
    UCSR0A = UCSR0A | (0 << 6); // Gjorde så att vi kunde skicka en instruktion efter en instruktion/read.
    USARTWriteChar(0xFF);
    USARTWriteChar(0xFF);
    USARTWriteChar(ID);
    USARTWriteChar(2); // Paketets längd. 
    USARTWriteChar(instruction);
    cli(); // Blockera avbrott
    USARTWriteChar(~(ID+2+instruction)); // Checksum
    while(!TXD0_FINISHED) //TXD0 sätts till 1 då all data shiftats ut ifrån usarten
    {
        // Vänta tills den sänt klart det sista
    }
    USART0RecieveMode();
    UDR0 = 0x00; // Läser recieve bufferten så att vi märker av när vi får in data från servot
    sei(); // Tillåt interrupts igen
    
}

void USARTSendInstruction1(int ID, int instruction, int parameter0)
{
    // sätt USART till sändläge
    USART0SendMode();
    UCSR0A = UCSR0A | (0 << 6); // Gjorde så att vi kunde skicka en instruktion efter en instruktion/read.
    USARTWriteChar(0xFF);
    USARTWriteChar(0xFF);
    USARTWriteChar(ID);
    USARTWriteChar(3); // Paketets längd.
    USARTWriteChar(instruction);
    USARTWriteChar(parameter0);
    cli();
    USARTWriteChar(~(ID+3+instruction+parameter0)); // Checksum
    while(!TXD0_FINISHED) //TXD0 sätts till 1 då all data shiftats ut ifrån usarten
    {
        // Vänta tills den sänt klart det sista
    }
    USART0RecieveMode();
    sei(); // Tillåt interrupts igen
    
}

void USARTSendInstruction2(int ID, int instruction, int parameter0, int parameter1)
{
    // sätt USART till sändläge
    USART0SendMode();
    UCSR0A = UCSR0A | (0 << 6); // Gjorde så att vi kunde skicka en instruktion efter en instruktion/read.
    USARTWriteChar(0xFF);
    USARTWriteChar(0xFF);
    USARTWriteChar(ID);
    USARTWriteChar(4); // Paketets längd.
    USARTWriteChar(instruction);
    USARTWriteChar(parameter0);
    USARTWriteChar(parameter1);
    cli();
    USARTWriteChar(~(ID+4+instruction+parameter0+parameter1)); // Checksum
    while(!TXD0_FINISHED) //TXD0 sätts till 1 då all data shiftats ut ifrån usarten
    {
        // Vänta tills den sänt klart det sista
    }
    USART0RecieveMode();
    sei(); // Tillåt interrupts igen
    
}

void USARTSendInstruction3(int ID, int instruction, int parameter0, int parameter1, int parameter2)
{
    // sätt USART till sändläge
    USART0SendMode();
    UCSR0A = UCSR0A | (0 << 6); // Gjorde så att vi kunde skicka en instruktion efter en instruktion/read.
    USARTWriteChar(0xFF);
    USARTWriteChar(0xFF);
    USARTWriteChar(ID);
    USARTWriteChar(5); // Paketets längd. 
    USARTWriteChar(instruction);
    USARTWriteChar(parameter0);
    USARTWriteChar(parameter1);
    USARTWriteChar(parameter2);
    cli();
    USARTWriteChar(~(ID+5+instruction+parameter0+parameter1+parameter2)); // Checksum
    while(!TXD0_FINISHED) //TXD0 sätts till 1 då all data shiftats ut ifrån usarten
    {
        // Vänta tills den sänt klart det sista
    }
    USART0RecieveMode();
    sei(); // Tillåt interrupts igen
    
}

void USARTSendInstruction4(int ID, int instruction, int parameter0, int parameter1, int parameter2, int parameter3)
{
    // sätt USART till sändläge
    USART0SendMode();
    UCSR0A = UCSR0A | (0 << 6); // Gjorde så att vi kunde skicka en instruktion efter en instruktion/read.
    USARTWriteChar(0xFF);
    USARTWriteChar(0xFF);
    USARTWriteChar(ID);
    USARTWriteChar(6); // Paketets längd.
    USARTWriteChar(instruction);
    USARTWriteChar(parameter0);
    USARTWriteChar(parameter1);
    USARTWriteChar(parameter2);
    USARTWriteChar(parameter3);
    cli();
    USARTWriteChar(~(ID+6+instruction+parameter0+parameter1+parameter2+parameter3)); // Checksum
    while(!TXD0_FINISHED) //TXD0 sätts till 1 då all data shiftats ut ifrån usarten
    {
        // Vänta tills den sänt klart det sista
    }
    USART0RecieveMode();
    sei(); // Tillåt interrupts igen
    
}
void USARTSendInstruction5(int ID, int instruction, int parameter0, int parameter1, int parameter2, int parameter3, int parameter4)
{
    // sätt USART till sändläge
    USART0SendMode();
    UCSR0A = UCSR0A | (0 << 6); // Gjorde så att vi kunde skicka en instruktion efter en instruktion/read.
    USARTWriteChar(0xFF);
    USARTWriteChar(0xFF);
    USARTWriteChar(ID);
    USARTWriteChar(7); // Paketets längd.
    USARTWriteChar(instruction);
    USARTWriteChar(parameter0);
    USARTWriteChar(parameter1);
    USARTWriteChar(parameter2);
    USARTWriteChar(parameter3);
    USARTWriteChar(parameter4);
    cli();
    USARTWriteChar(~(ID+7+instruction+parameter0+parameter1+parameter2+parameter3+parameter4)); // Checksum
    while(!TXD0_FINISHED) //TXD0 sätts till 1 då all data shiftats ut ifrån usarten
    {
        // Vänta tills den sänt klart det sista
    }
    ;
    USART0RecieveMode();
    //char test1 = RXD0_DATA; // För att läsa det som är i reciever bufferten, används nu för att readchar ska funka
    sei(); // Tillåt interrupts igen
    
}


char USARTReadChar()
{
    //Vänta tills data är tillgänglig
    while(!RXD0_READY)
    {
        //Gör ingenting
    }
    return RXD0_DATA;
}

int USARTReadStatusPacket()
{   
    int ValueOfParameters = 0;
    //if ((USARTReadChar() == 0xFF) & (USARTReadChar() == 0xFF)) // Kollar om två startbitar
    //{
        //char test = USARTReadChar();
        char Start1 = USARTReadChar();
        char Start2 = USARTReadChar();
        char ID = USARTReadChar();
        char Length = USARTReadChar();
        char Error = USARTReadChar();
        int HelpVariable = 0;
        // Läser av parametervärdena och sparar värdet i ValueOfParameters
        while (Length > 2) 
        {
            ValueOfParameters = ValueOfParameters + (USARTReadChar() << (8*HelpVariable));
            HelpVariable = HelpVariable + 1;
            Length = Length - 1;
        }
        
        char CheckSum = USARTReadChar();
    //}
    return ValueOfParameters;
    
}

void disableStatusPacketsFromActuator(int ID)
{
    USARTSendInstruction2(ID,INST_WRITE,P_RETURN_LEVEL,0x01);
}
void enableStatusPacketsFromActuator(int ID)
{
    USARTSendInstruction2(ID,INST_WRITE,P_RETURN_LEVEL,0x02);
}

int ReadTemperatureLimitFromActuator(int ID)
{
    USARTSendInstruction2(ID,INST_READ,P_LIMIT_TEMPERATURE,0x01);
    return USARTReadStatusPacket();
}

int readCurrentTemperatureFromActuator(int ID)
{
    USARTSendInstruction2(ID,INST_READ,P_PRESENT_TEMPERATURE,0x01);
    return USARTReadStatusPacket();
}

void setMaxTemperatureLimitToActuator(int ID)
{
    USARTSendInstruction2(ID,INST_WRITE,P_LIMIT_TEMPERATURE,85);
}

int readAlarmShutdownStatus(int ID)
{
    USARTSendInstruction2(ID,INST_READ,P_ALARM_SHUTDOWN,0x01);
    return USARTReadStatusPacket();
}

void setNewIDOfActuator (int ID, int newID)
{
    USARTSendInstruction2(ID, INST_WRITE, P_ID, newID);
}