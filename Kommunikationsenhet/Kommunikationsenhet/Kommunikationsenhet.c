/*
 * Kommunikationsenhet.c
 *
 * Created: 3/25/2015 11:26:35 AM
 *  Author: marwa828, nicgr354
 */ 


#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>

#include <stdlib.h> //för länkad lista

#define F_CPU 14745600UL
#include <util/delay.h>

struct node_t* first_p_g;
struct node_t* last_p_g;

struct node_t // De structs som kommer bygga upp den länkade listan (inbuffern från styrenheten)
{
	unsigned char data_;
	struct node_t* next_;
    
};

void bluetoothInit()
{
    UBRRL = (0<<UBRR3)|(1<<UBRR2)|(1<<UBRR1)|(1<<UBRR0); //Sätt baud-rate till 115200
    UCSRB = (1<<RXEN)|(1<<TXEN)|(1<<RXCIE); //Sätt på sändare och mottagare, samt sätt på interrupts vid recieve complete respektive tom buffer.
    UCSRC = (1<<URSEL)|(3<<UCSZ0)|(0<<UPM1)|(0<<UPM0); //Sätt 8-bit meddelanden samt ingen paritet
    DDRA = (0<<DDA1)|(1<<DDA0)|(1<<DDA2); //Definiera en input och en output
    PORTA = (0<<PORTA0)|(1<<PORTA1)|(0<<PORTA2); //Skicka ut clear to send, samt skapa INTE avbrott i styrenhet
}

void spiInit(void)
{
    DDRB = (1<<PORTB6); //Alla utom MISO ska vara ingångar.
    SPCR = (1<<SPE)|(1<<SPIE); //Sätt på SPI  
}

void bluetoothSend(unsigned char data) // Skicka något till datorn via Bluetooth
{
    while ( !( UCSRA & (1<<UDRE)));
    UDR = data;
}

unsigned char bluetoothReceive(void) // Läs vad som tagits emot via Bluetooth
{
    return UDR;
}

unsigned char spiReceive(void) // Läs vad som skickats 
{
    return SPDR;
}

void spiWrite(unsigned char data) // Skriv någonting via SPI (så att styrenheten kan hämta det vid nästa överföring)
{
    SPDR = data;
}

void spiReset()
{
    SPDR = 0;
}

void removeFirst () // Ta bort det första elementet i inbufferlistan
{
    cli();
	if (first_p_g == NULL)
    {
        return;
    }
    else
    {
        struct node_t* temp_p = first_p_g->next_;
        free (first_p_g);
        first_p_g = temp_p;
    }
	sei();
}

void processList() // Skicka iväg det som står först i inbuffern
{
    if (first_p_g != NULL)
    {
        bluetoothSend(first_p_g->data_);
        removeFirst();
    }
}

void appendList (unsigned char data) // Lägg till ett nytt meddelande i inbuffern
{
	// Härifrån skapas en ny nod som sedan initialiseras
	cli();
    struct node_t* node = malloc(sizeof(struct node_t));
    if (node == NULL)
    {
        return;
    }        
	node->data_ = data;
	node->next_ = NULL; // ...till hit
	
	if (first_p_g == NULL) // Om listan är tom
	{
		first_p_g = node;
	}
	else // Om listan inte är tom
	{
		last_p_g->next_ = node;
	}
	last_p_g = node;
    sei();
}

int main(void)
{
	first_p_g = NULL;
	last_p_g = NULL;
	bluetoothInit();
	spiInit();
	sei();
	
	while(1)
	{
        while(last_p_g != NULL)   //Gå igenom listan tills den blir tom
        {
		  processList();   
        }
        MCUCR = (1<<SE); //Sleep enable
	    sleep_mode(); //Gå in i sleep mode om det inte finns något att göra
	}
   
}

ISR(USART_RXC_vect) //Inkommet bluetoothmeddelande
{
	MCUCR = (0<<SE);
    PORTA = (0<<PORTA2);
    spiWrite(bluetoothReceive()); //Information som ska skickas överförs direkt till SPDR, där det är redo att föras över till masterenheten.
    PORTA = (1<<PORTA2); //Generera avbrott i styrenhet
    MCUCR = (1<<SE);
}

ISR(SPISTC_vect)//SPI-överföring klar
{
	MCUCR = (0<<SE);
	appendList(spiReceive());
	spiReset(); //Återställ SPDR.
    MCUCR = (1<<SE);
}

