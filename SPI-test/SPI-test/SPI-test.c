
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#define F_CPU 16000000UL
#include <util/delay.h>

unsigned char inbuffer = 0x44;

void spiInit(void)
{
	DDRA = (1<<PORTA2);
	PORTA = (1<<PORTA2); //Ha ingen slav vald
	DDRB = (1<<PORTB5)|(1<<PORTB7)|(1<<PORTB4); //Definiera outputs
	SPCR = (1<<SPE)|(1<<MSTR)|(1<<SPR0); //Sätt enhet till master, enable spi, klockfrekvens
}

void spiTransmit(unsigned char data)
{
	 SPDR = data;
	 while(!(SPSR & (1<<SPIF))); //Vänta på att överföring är klar
	 inbuffer = SPDR;
}


void transmitData(int header_, int dataByte1, int dataByte2_)
{
    PORTA = (0<<PORTA2); //Slave select
    spiTransmit(header_);
   	PORTA = (1<<PORTA2); //Slave deselect
   	_delay_us(5);
   	PORTA = (0<<PORTA2);
   	spiTransmit(dataByte1);
    PORTA = (1<<PORTA2);
   	_delay_us(5);
   	PORTA = (0<<PORTA2);
   	spiTransmit(dataByte2_);
   	PORTA = (1<<PORTA2);
   	_delay_us(5);
}

int main(void)
{
    int dataOffset = -3;
    int dataIterator = 0;
    int leakTestByte = 0;
    int nodeTestByte1 = 0b00010011;
    int nodeTestByte2 = 0b00001100;
    while(1)
    {
	    spiInit(); 
    	PORTA = (0<<PORTA2); 
    	_delay_us(5);
        
        transmitData(202,0b00000001,0b01110000 + 2 * dataOffset +152); // Sida 1

        transmitData(232,0b10000001,0b00000111 + 2 * dataOffset + 55);// Vinkel 1
        
        
        
        transmitData(208,0b00000001,0b01110000 + dataOffset); // Sida 2

    	transmitData(240,0b10000001,0b00000111 + dataOffset); // Vinkel 2
        
        
        
        transmitData(216,0b00000001,0b01110000 + 2 * dataOffset +152); // Sida 3

        transmitData(248,0b10000001,0b00000111 + 2 * dataOffset + 55);// Vinkel 3
        
        
        
        transmitData(224,0b00000001,0b01110000 + dataOffset); // Sida 4

        transmitData(136,0b10000001,0b00000111 + dataOffset); // Vinkel 4
        
        
        
        transmitData(144,0b10000001,0b00000111 + dataOffset); // Vinkel total
        
        
        transmitData(152,0,leakTestByte);
 
 
    	transmitData(160,nodeTestByte1,nodeTestByte2); // Nod

        switch(dataIterator)
        {
            case 0:
            dataIterator ++;
            dataOffset = 3;
            break;
            case 1:
            dataIterator ++;
            dataOffset = 1;
            break;
            case 2:
            dataIterator ++;
            dataOffset = -2;
            break;
            case 3:
            dataIterator ++;
            dataOffset = 0;
            break;
            case 4:
            dataIterator = 0;
            dataOffset = -3;
            if (leakTestByte == 0)
            {
                leakTestByte = 1;
            }
            else
            {
                leakTestByte = 0;
            }
            if (nodeTestByte1 == 0b00010011)
            {
                nodeTestByte1 = 0b0001100;
                nodeTestByte2 = 0b00101000;
            }
            else
            {
                nodeTestByte1 = 0b00010011;
                nodeTestByte2 = 0b00001100;
            }
            break;
            default:
            dataIterator = 0;
            break;
        }
        _delay_ms(200);
    }    
}
