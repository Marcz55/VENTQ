/*
 * Kommunikationsenhet.c
 *
 * Created: 3/25/2015 11:26:35 AM
 *  Author: marwa828
 */ 


#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>

#include <stdlib.h> //f�r l�nkad lista

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
    UBRRL = (0<<UBRR3)|(1<<UBRR2)|(1<<UBRR1)|(1<<UBRR0); //S�tt baud-rate till 115200
    UCSRB = (1<<RXEN)|(1<<TXEN)|(1<<RXCIE); //S�tt p� s�ndare och mottagare, samt s�tt p� interrupts vid recieve complete respektive tom buffer.
    UCSRC = (1<<URSEL)|(3<<UCSZ0)|(0<<UPM1)|(0<<UPM0); //S�tt 8-bit meddelanden samt ingen paritet
    DDRA = (0<<DDA1)|(1<<DDA0)|(1<<DDA2); //Definiera en input och en output
    PORTA = (0<<PORTA0)|(1<<PORTA1)|(0<<PORTA2); //Skicka ut clear to send, samt skapa INTE avbrott i styrenhet
}

void spiInit(void)
{
    DDRB = (1<<PORTB6); //Alla utom MISO ska vara ing�ngar.
    SPCR = (1<<SPE)|(1<<SPIE); //S�tt p� SPI  
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
	// H�rifr�n skapas en ny nod som sedan initialiseras
	cli();
    struct node_t* node = malloc(sizeof(struct node_t));
    if (node == NULL)
    {
        return;
    }        
	node->data_ = data;
	node->next_ = NULL; // ...till hit
	
	if (first_p_g == NULL) // Om listan �r tom
	{
		first_p_g = node;
	}
	else // Om listan inte �r tom
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
        while(last_p_g != NULL)   //G� igenom listan tills den blir tom
        {
		  processList();   
        }
        MCUCR = (1<<SE); //Sleep enable
	    sleep_mode(); //G� in i sleep mode om det inte finns n�got att g�ra
	}
   
}

ISR(USART_RXC_vect) //Inkommet bluetoothmeddelande
{
	MCUCR = (0<<SE);
    PORTA = (0<<PORTA2);
    //unsigned char plutt;
    //plutt = bluetoothReceive();
    //bluetoothSend(0x44);
    //bluetoothSend(0x45);
    //bluetoothSend(plutt);
    spiWrite(bluetoothReceive()); //Information som ska skickas �verf�rs direkt till SPDR, d�r det �r redo att f�ras �ver till masterenheten.
    PORTA = (1<<PORTA2); //Generera avbrott i styrenhet
    MCUCR = (1<<SE);
}

ISR(SPISTC_vect)//SPI-�verf�ring klar
{
	MCUCR = (0<<SE);
	//Send(SPDR); L�gg in i lista ist�llet!
	appendList(spiReceive());
	spiReset(); //�terst�ll SPDR.
    MCUCR = (1<<SE);
}
//Kommer att beh�va en lista d�r indata kan sparas tillf�lligt.

