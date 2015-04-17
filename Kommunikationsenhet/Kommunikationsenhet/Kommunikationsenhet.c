/*
 * Kommunikationsenhet.c
 *
 * Created: 3/25/2015 11:26:35 AM
 *  Author: marwa828
 */ 


#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>

#include <stdlib.h> //för länkad lista

#define F_CPU 14745600UL
#include <util/delay.h>

struct node_t* first_p_g;
struct node_t* last_p_g;

struct node_t
{
	unsigned char data_;
	struct node_t* next_;
};

void bluetoothInit()
{
    UBRRL = (0<<UBRR3)|(1<<UBRR2)|(1<<UBRR1)|(1<<UBRR0); //Sätt baud-rate till 115200
    UCSRB = (1<<RXEN)|(1<<TXEN)|(1<<RXCIE); //Sätt på sändare och mottagare, samt sätt på interrupts vid recieve complete respektive tom buffer.
    UCSRC = (1<<URSEL)|(3<<UCSZ0)|(0<<UPM1)|(0<<UPM0); //Sätt 8-bit meddelanden samt ingen paritet
    DDRA = (0<<DDA1)|(1<<DDA0); //Definiera en input och en output
    PORTA = (0<<PORTA0)|(1<<PORTA1); //Skicka ut clear to send
}

void spiInit(void)
{
    DDRB = (1<<PORTB6); //Alla utom MISO ska vara ingångar.
    SPCR = (1<<SPE)|(1<<SPIE); //Sätt på SPI
    //Ska sätta SPIE här med för att fixa avbrottstyrning
    
}

void bluetoothSend(unsigned char data)
{
    while ( !( UCSRA & (1<<UDRE)));
    UDR = data;
}

unsigned char bluetoothReceive(void)
{
    return UDR;
}

unsigned char spiReceive(void)
{
    return SPDR;
}

void spiWrite(unsigned char data)
{
    SPDR = data;
}

void spiReset()
{
    SPDR = 0;
}

void removeFirst ()
{
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
}

void processList()
{
    if (first_p_g != NULL)
    {
        bluetoothSend(first_p_g->data_);
        removeFirst();
    }
}

void appendList (unsigned char data)
{
	// Härifrån skapas en ny nod som sedan initialiseras
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
}

int main(void)
{
	first_p_g = NULL;
	last_p_g = NULL;
	bluetoothInit();
	spiInit();
	sei();
	
	/*while(1)
	{
        while(last_p_g != NULL)   //Gå igenom listan tills den blir tom
        {
          processList();   
        }
        MCUCR = (1<<SE); //Sleep enable
	    sleep_mode(); //Gå in i sleep mode om det inte finns något att göra
	}*/
    
    while(1)
    {
        _delay_ms(1000);
        bluetoothSend(0x44);
    }
}

ISR(USART_RXC_vect) //Inkommet bluetoothmeddelande
{
	MCUCR = (0<<SE);
	spiWrite(bluetoothReceive()); //Information som ska skickas överförs direkt till SPDR, där det är redo att föras över till masterenheten.
}

ISR(SPISTC_vect)//SPI-överföring klar
{
	MCUCR = (0<<SE);
	//Send(SPDR); Lägg in i lista istället!
	appendList(spiReceive());
	spiReset(); //Återställ SPDR.
}
//Kommer att behöva en lista där indata kan sparas tillfälligt.
