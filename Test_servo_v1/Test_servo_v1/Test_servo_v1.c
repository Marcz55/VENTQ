/*
 * Test_servo_v1.c
 *
 * Created: 3/26/2015 11:50:32 AM
 *  Author: joamo950
 */ 

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

#define TXD0_READY bit_is_set(UCSR0A,5)
#define TXD0_FINISHED bit_is_set(UCSR0A,6)
#define RXD0_READY bit_is_set(UCSR0A,7)
#define TXD0_DATA (UDR0)
#define RXD0_DATA (UDR0)

//--- Control Table Address ---
//EEPROM AREA
#define P_MODEL_NUMBER_L 0
#define P_MODOEL_NUMBER_H 1
#define P_VERSION 2
#define P_ID 3
#define P_BAUD_RATE 4
#define P_RETURN_DELAY_TIME 5
#define P_CW_ANGLE_LIMIT_L 6
#define P_CW_ANGLE_LIMIT_H 7
#define P_CCW_ANGLE_LIMIT_L 8
#define P_CCW_ANGLE_LIMIT_H 9
#define P_SYSTEM_DATA2 10
#define P_LIMIT_TEMPERATURE 11
#define P_DOWN_LIMIT_VOLTAGE 12
#define P_UP_LIMIT_VOLTAGE 13
#define P_MAX_TORQUE_L 14
#define P_MAX_TORQUE_H 15
#define P_RETURN_LEVEL 16
#define P_ALARM_LED 17
#define P_ALARM_SHUTDOWN 18
#define P_OPERATING_MODE 19
#define P_DOWN_CALIBRATION_L 20
#define P_DOWN_CALIBRATION_H 21
#define P_UP_CALIBRATION_L 22
#define P_UP_CALIBRATION_H 23
#define P_TORQUE_ENABLE (24)
#define P_LED (25)
#define P_CW_COMPLIANCE_MARGIN (26)
#define P_CCW_COMPLIANCE_MARGIN (27)
#define P_CW_COMPLIANCE_SLOPE (28)
#define P_CCW_COMPLIANCE_SLOPE (29)
#define P_GOAL_POSITION_L (30)
#define P_GOAL_POSITION_H (31)
#define P_GOAL_SPEED_L (32)
#define P_GOAL_SPEED_H (33)
#define P_TORQUE_LIMIT_L (34)
#define P_TORQUE_LIMIT_H (35)
#define P_PRESENT_POSITION_L (36)
#define P_PRESENT_POSITION_H (37)
#define P_PRESENT_SPEED_L (38)
#define P_PRESENT_SPEED_H (39)
#define P_PRESENT_LOAD_L (40)
#define P_PRESENT_LOAD_H (41)
#define P_PRESENT_VOLTAGE (42)
#define P_PRESENT_TEMPERATURE (43)
#define P_REGISTERED_INSTRUCTION (44)
#define P_PAUSE_TIME (45)
#define P_MOVING (46)
#define P_LOCK (47)
#define P_PUNCH_L (48)
#define P_PUNCH_H (49)

//--- Instruction ---
#define INST_PING 0x01
#define INST_READ 0x02
#define INST_WRITE 0x03
#define INST_REG_WRITE 0x04
#define INST_ACTION 0x05
#define INST_RESET 0x06
#define INST_DIGITAL_RESET 0x07
#define INST_SYSTEM_READ 0x0C
#define INST_SYSTEM_WRITE 0x0D
#define INST_SYNC_WRITE 0x83
#define INST_SYNC_REG_WRITE 0x84

void USART0RecieveMode() 
{
	PORTD = (0<<PORTD2);
}

void USART0SendMode()
{
	PORTD = (1<<PORTD2);
}



void initUSART()
{
	DDRD = (1<<PORTD2); // Styrsignal f�r s�ndning/mottagning, PD2
	DDRA = (0<<PORTA0); // Signal fr�n extern knapp, kan anv�ndas till diverse saker
	USART0RecieveMode();
	UBRR0H = 0x00;
	UBRR0L = 0x00; // S�tter baudraten till fosc/16(UBRR0 + 1) = 1Mhz
	
	UCSR0B = (1<<RXEN0)|(1<<TXEN0); // Aktiverar s�ndare och mottagare
	
//	USBS0 = 0; // En stoppbit
	//UPM01 = 0; // Ingen paritetsbit
	
	UCSR0C = (0<<USBS0) | (3<<UCSZ00) | (0<<UPM01);
	/*
	UCSZ00 = 1;
	UCSZ01 = 1;
	UCSZ02 = 0; // S�tter antalet databitar i varje paket till 8.
	*/

	
}
void USARTWriteChar(char data)
{
	// v�nta tills s�ndaren �r redo
	while(!TXD0_READY) //UDRE0 s�tts till 1 n�r buffern �r tom
	{
		// g�r ingenting
	}
	TXD0_DATA = data;
}
void USARTSendInstruction0(int ID, int instruction)
{
	
	// s�tt USART till s�ndl�ge
	USART0SendMode();
	
	USARTWriteChar(0xFF);
	USARTWriteChar(0xFF);
	USARTWriteChar(ID);
	USARTWriteChar(2); // Paketets l�ngd. 
	USARTWriteChar(instruction);
	cli(); // Blockera avbrott
	USARTWriteChar(~(ID+2+instruction)); // Checksum
	while(!TXD0_FINISHED) //TXD0 s�tts till 1 d� all data shiftats ut ifr�n usarten
	{
		// V�nta tills den s�nt klart det sista
	}
	USART0RecieveMode();
	UDR0 = 0x00; // L�ser recieve bufferten s� att vi m�rker av n�r vi f�r in data fr�n servot
	sei(); // Till�t interrupts igen
	
}

void USARTSendInstruction1(int ID, int instruction, int parameter0)
{
	// s�tt USART till s�ndl�ge
	USART0SendMode();
	
	USARTWriteChar(0xFF);
	USARTWriteChar(0xFF);
	USARTWriteChar(ID);
	USARTWriteChar(3); // Paketets l�ngd.
	USARTWriteChar(instruction);
	USARTWriteChar(parameter0);
	cli();
	USARTWriteChar(~(ID+3+instruction+parameter0)); // Checksum
	while(!TXD0_FINISHED) //TXD0 s�tts till 1 d� all data shiftats ut ifr�n usarten
	{
		// V�nta tills den s�nt klart det sista
	}
	USART0RecieveMode();
	sei(); // Till�t interrupts igen
	
}

void USARTSendInstruction2(int ID, int instruction, int parameter0, int parameter1)
{
	// s�tt USART till s�ndl�ge
	USART0SendMode();
	
	USARTWriteChar(0xFF);
	USARTWriteChar(0xFF);
	USARTWriteChar(ID);
	USARTWriteChar(4); // Paketets l�ngd.
	USARTWriteChar(instruction);
	USARTWriteChar(parameter0);
	USARTWriteChar(parameter1);
	cli();
	USARTWriteChar(~(ID+4+instruction+parameter0+parameter1)); // Checksum
	while(!TXD0_FINISHED) //TXD0 s�tts till 1 d� all data shiftats ut ifr�n usarten
	{
		// V�nta tills den s�nt klart det sista
	}
	USART0RecieveMode();
	sei(); // Till�t interrupts igen
	
}

void USARTSendInstruction3(int ID, int instruction, int parameter0, int parameter1, int parameter2)
{
	// s�tt USART till s�ndl�ge
	USART0SendMode();
	
	USARTWriteChar(0xFF);
	USARTWriteChar(0xFF);
	USARTWriteChar(ID);
	USARTWriteChar(5); // Paketets l�ngd.
	USARTWriteChar(instruction);
	USARTWriteChar(parameter0);
	USARTWriteChar(parameter1);
	USARTWriteChar(parameter2);
	cli();
	USARTWriteChar(~(ID+5+instruction+parameter0+parameter1+parameter2)); // Checksum
	while(!TXD0_FINISHED) //TXD0 s�tts till 1 d� all data shiftats ut ifr�n usarten
	{
		// V�nta tills den s�nt klart det sista
	}
	USART0RecieveMode();
	sei(); // Till�t interrupts igen
	
}

void USARTSendInstruction4(int ID, int instruction, int parameter0, int parameter1, int parameter2, int parameter3)
{
	// s�tt USART till s�ndl�ge
	USART0SendMode();
	
	USARTWriteChar(0xFF);
	USARTWriteChar(0xFF);
	USARTWriteChar(ID);
	USARTWriteChar(6); // Paketets l�ngd.
	USARTWriteChar(instruction);
	USARTWriteChar(parameter0);
	USARTWriteChar(parameter1);
	USARTWriteChar(parameter2);
	USARTWriteChar(parameter3);
	cli();
	USARTWriteChar(~(ID+6+instruction+parameter0+parameter1+parameter2+parameter3)); // Checksum
	while(!TXD0_FINISHED) //TXD0 s�tts till 1 d� all data shiftats ut ifr�n usarten
	{
		// V�nta tills den s�nt klart det sista
	}
	USART0RecieveMode();
	sei(); // Till�t interrupts igen
	
}
void USARTSendInstruction5(int ID, int instruction, int parameter0, int parameter1, int parameter2, int parameter3, int parameter4)
{
	// s�tt USART till s�ndl�ge
	USART0SendMode();
	UCSR0A = UCSR0A | (0 << 6); 
	USARTWriteChar(0xFF);
	USARTWriteChar(0xFF);
	USARTWriteChar(ID);
	USARTWriteChar(7); // Paketets l�ngd.
	USARTWriteChar(instruction);
	USARTWriteChar(parameter0);
	USARTWriteChar(parameter1);
	USARTWriteChar(parameter2);
	USARTWriteChar(parameter3);
	USARTWriteChar(parameter4);
	cli();
	USARTWriteChar(~(ID+7+instruction+parameter0+parameter1+parameter2+parameter3+parameter4)); // Checksum
	while(!TXD0_FINISHED) //TXD0 s�tts till 1 d� all data shiftats ut ifr�n usarten
	{
		// V�nta tills den s�nt klart det sista
	}
	;
	USART0RecieveMode();
 	char test1 = RXD0_DATA; // F�r att l�sa det som �r i reciever bufferten, anv�nds nu f�r att readchar ska funka
	sei(); // Till�t interrupts igen
	
}


char USARTReadChar()
{
	//V�nta tills data �r tillg�nglig
	while(!RXD0_READY)
	{
		//G�r ingenting
	}
	return RXD0_DATA;
}

int USARTReadStatusPacket()
{
	int ValueOfParameters = 0;
	//if ((USARTReadChar() == 0xFF) & (USARTReadChar() == 0xFF)) // Kollar om tv� startbitar
	//{
		DDRA = 0xff;
		//char test = USARTReadChar();
		char Start1 = USARTReadChar();
		char Start2 = USARTReadChar();
		char ID = USARTReadChar();
		PORTA = ID;
		char Length = USARTReadChar();
		char Error = USARTReadChar();
		int HelpVariable = 0;
		// L�ser av parameterv�rdena och sparar v�rdet i ValueOfParameters
		while (Length > 2) 
		{
			ValueOfParameters = ValueOfParameters + (USARTReadChar() << (8*HelpVariable));
	//		PORTA = PORTA | (1 << HelpVariable);
			HelpVariable = HelpVariable + 1;
			Length = Length - 1;
		}
		
		char CheckSum = USARTReadChar();
	//}
	return ValueOfParameters;
	
}

void MoveDynamixel(int ID,long int Degree,long int Velocity)
{
	if ((Degree <= 300) & (Degree >= 0)) // Till�tna grader �r 0-300
	{
		long int LowGoalPosition = ((Degree*1023)/300) & 0x00FF; // G�r om graden till ett tal mellan 0-1023 och delar upp det i LSB(byte) och MSB(byte)
		long int HighGoalPosition = ((Degree*1023)/300) & 0xFF00;
		HighGoalPosition = (HighGoalPosition >> 8);
	
		long int LowAngleVelocity = 0;
		long int HighAngleVelocity = 0;
	
		if (Velocity >= 114) // Om RPM �ver 114 s� r�r vi oss med snabbaste m�jliga hastigheten med sp�nningen som tillhandah�lls
		{
			LowAngleVelocity = 0;
			HighAngleVelocity = 0;
		}
		else
		{
			LowAngleVelocity = ((Velocity*1023)/114) & 0x00FF;
			HighAngleVelocity = ((Velocity*1023)/114) & 0xFF00;
			HighAngleVelocity = (HighAngleVelocity >> 8);
		}
	
		USARTSendInstruction5(ID,INST_WRITE,P_GOAL_POSITION_L,LowGoalPosition ,HighGoalPosition, LowAngleVelocity, HighAngleVelocity);
	}
}

int main(void)
{
	initUSART();
	//USARTSendInstruction5(5,INST_WRITE,P_GOAL_POSITION_L,0x54,0x02,0x00,0x01);
	//USARTWriteChar(0xFF);
	/*while(1)
	{
		if (PINA & (1<<PINA0) ) // knapp nedtryckt
		{*/
			/*
			USART0SendMode();
			USARTWriteChar(0x11);
			while(!TXD0_READY) //UDRE0 s�tts till 1 n�r buffern �r tom
			{
				// V�nta tills den s�nt klart det sista
			}
			USART0RecieveMode();
			*/
			
		//	USARTSendInstruction5(5,INST_WRITE,P_GOAL_POSITION_L, 0x03, 0x03, 0x00, 0x02);//ID, instruction, parameters
	
			//USARTSendInstruction2(3,INST_READ,0x12,0x01);
			//USARTSendInstruction0(1,INST_PING);//ID, instruction, parameters
			//USARTSendInstruction2(5,INST_READ,0x12,0x01);
			//DDRB = 0xFF;
			//PORTB = USARTReadStatusPacket();
			MoveDynamixel(6,200,15);
			USARTReadStatusPacket();
			MoveDynamixel(12,200,15); 
			USARTReadStatusPacket();
			MoveDynamixel(5,100,15);
			USARTReadStatusPacket();
			MoveDynamixel(11,100,15);
			USARTReadStatusPacket();
			MoveDynamixel(3,200,15);
			USARTReadStatusPacket();
			MoveDynamixel(4,100,15);
			USARTReadStatusPacket();
			MoveDynamixel(10,100,15);
			USARTReadStatusPacket();
			MoveDynamixel(9,200,15);
			USARTReadStatusPacket();
			_delay_ms(10000);
			MoveDynamixel(1,100,15);
			USARTReadStatusPacket();
			MoveDynamixel(2,100,15);
			USARTReadStatusPacket();
			MoveDynamixel(7,100,15);
			USARTReadStatusPacket();
			MoveDynamixel(8,100,15);
			USARTReadStatusPacket();
		/*}
		else 
		{
			send = 1;
		}
	}*/
}



