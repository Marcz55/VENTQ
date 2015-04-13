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


//----Konstanter-Inverskinematik----
#define a1 50 
#define a2 67
#define a3 130
#define a1Square 2500 
#define a2Square 4489
#define a3Square 16900
#define PI 3.141592

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
	DDRD = (1<<PORTD2); // Styrsignal för sändning/mottagning, PD2
//	DDRA = (0<<PORTA0); // Signal från extern knapp, kan användas till diverse saker
	USART0RecieveMode();
	UBRR0H = 0x00;
	UBRR0L = 0x00; // Sätter baudraten till fosc/16(UBRR0 + 1) = 1Mhz
	
	UCSR0B = (1<<RXEN0)|(1<<TXEN0); // Aktiverar sändare och mottagare
	
//	USBS0 = 0; // En stoppbit
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

void MoveDynamixel(int ID,long int Degree,long int Velocity)
{
	if ((Degree <= 300) & (Degree >= 0)) // Tillåtna grader är 0-300
	{
		long int LowGoalPosition = ((Degree*1023)/300) & 0x00FF; // Gör om graden till ett tal mellan 0-1023 och delar upp det i LSB(byte) och MSB(byte)
		long int HighGoalPosition = ((Degree*1023)/300) & 0xFF00;
		HighGoalPosition = (HighGoalPosition >> 8);
	
		long int LowAngleVelocity = 0;
		long int HighAngleVelocity = 0;
	
		if (Velocity >= 114) // Om RPM över 114 så rör vi oss med snabbaste möjliga hastigheten med spänningen som tillhandahålls
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
	return;
}

void MoveFrontLeftLeg(float x, float y, float z, int speed)
{
	long int theta1 = atan2f(-x,y)*180/PI;
	long int theta2 = 180/PI*(acosf(-z/sqrt(z*z + (sqrt(x*x + y*y) - a1)*(sqrt(x*x + y*y) - a1))) + 
		acosf((z*z + (sqrt(x*x + y*y) - a1)*(sqrt(x*x + y*y) - a1) + a2Square - a3Square)/(2*sqrt(z*z + (sqrt((x*x + y*y) - a1)*(sqrt(x*x + y*y) - a1)))*a2)));
	
	long int theta3 = acosf((a2Square + a3Square - z*z - (sqrt(x*x + y*y) - a1)*(sqrt(x*x + y*y) - a1)) / (2*a2*a3))*180/PI;

	long int ActuatorAngle1 =  theta1 + 105;
	long int ActuatorAngle2 =  theta2 + 60;
	long int ActuatorAngle3 =  theta3 + 1;
	
	
	MoveDynamixel(2,ActuatorAngle1,speed);
	USARTReadStatusPacket();
	MoveDynamixel(4,ActuatorAngle2,speed);
	USARTReadStatusPacket();
	MoveDynamixel(6,ActuatorAngle3,speed);
	USARTReadStatusPacket();
	return;
}

void MoveFrontRightLeg(float x, float y, float z, int speed)
{
	long int theta1 = atan2f(x,y)*180/PI;
	long int theta2 = 180/PI*(acosf(-z/sqrt(z*z + (sqrt(x*x + y*y) - a1)*(sqrt(x*x + y*y) - a1))) +
	acosf((z*z + (sqrt(x*x + y*y) - a1)*(sqrt(x*x + y*y) - a1) + a2Square - a3Square)/(2*sqrt(z*z + (sqrt((x*x + y*y) - a1)*(sqrt(x*x + y*y) - a1)))*a2)));
	
	long int theta3 = acosf((a2Square + a3Square - z*z - (sqrt(x*x + y*y) - a1)*(sqrt(x*x + y*y) - a1)) / (2*a2*a3))*180/PI;

	long int ActuatorAngle1 =  theta1 + 105;
	long int ActuatorAngle2 =  theta2 + 60;
	long int ActuatorAngle3 =  theta3 + 1;
	
	
	MoveDynamixel(8,ActuatorAngle1,speed);
	USARTReadStatusPacket();
	MoveDynamixel(10,ActuatorAngle2,speed);
	USARTReadStatusPacket();
	MoveDynamixel(12,ActuatorAngle3,speed);
	USARTReadStatusPacket();	
	return;
}
void MoveRearLeftLeg(float x, float y, float z, int speed)
{
	long int theta1 = atan2f(-x,y)*180/PI;
	long int theta2 = 180/PI*(acosf(-z/sqrt(z*z + (sqrt(x*x + y*y) - a1)*(sqrt(x*x + y*y) - a1))) +
	acosf((z*z + (sqrt(x*x + y*y) - a1)*(sqrt(x*x + y*y) - a1) + a2Square - a3Square)/(2*sqrt(z*z + (sqrt((x*x + y*y) - a1)*(sqrt(x*x + y*y) - a1)))*a2)));
	
	long int theta3 = acosf((a2Square + a3Square - z*z - (sqrt(x*x + y*y) - a1)*(sqrt(x*x + y*y) - a1)) / (2*a2*a3))*180/PI;

	long int ActuatorAngle1 =  theta1 + 105;
	long int ActuatorAngle2 =  theta2 + 60;
	long int ActuatorAngle3 =  theta3 + 1;
	
	
	MoveDynamixel(1,ActuatorAngle1,speed);
	USARTReadStatusPacket();
	MoveDynamixel(5,ActuatorAngle3,speed);
	USARTReadStatusPacket();
	MoveDynamixel(3,ActuatorAngle2,speed);
	USARTReadStatusPacket();
	return;
}
void MoveRearRightLeg(float x, float y, float z, int speed)
{
	long int theta1 = atan2f(x,y)*180/PI;
	long int theta2 = 180/PI*(acosf(-z/sqrt(z*z + (sqrt(x*x + y*y) - a1)*(sqrt(x*x + y*y) - a1))) +
	acosf((z*z + (sqrt(x*x + y*y) - a1)*(sqrt(x*x + y*y) - a1) + a2Square - a3Square)/(2*sqrt(z*z + (sqrt((x*x + y*y) - a1)*(sqrt(x*x + y*y) - a1)))*a2)));
	
	long int theta3 = acosf((a2Square + a3Square - z*z - (sqrt(x*x + y*y) - a1)*(sqrt(x*x + y*y) - a1)) / (2*a2*a3))*180/PI;

	long int ActuatorAngle1 =  theta1 + 105;
	long int ActuatorAngle2 =  theta2 + 60;
	long int ActuatorAngle3 =  theta3 + 1;
	
	
	MoveDynamixel(1,ActuatorAngle1,speed);
	USARTReadStatusPacket();
	MoveDynamixel(5,ActuatorAngle3,speed);
	USARTReadStatusPacket();
	MoveDynamixel(3,ActuatorAngle2,speed);
	USARTReadStatusPacket();
	return;
}

ISR(INT1_vect)
{	
	MoveFrontLeftLeg(-120,120,0,10);
}



int main(void)
{
	initUSART();
	cli();
	EICRA = 0b1100; // Stigande flank på INT1 genererar avbrott
	EIMSK = (EIMSK | 2); // Möjliggör externa avbrott på INT0, pinne 40  
	DDRA = 0;
	// MCUCR = (MCUCR | (1 << PUD)); Något som testades för att se om det gjorde något
	//PORTA |= (1 << PORTA0);
	 // Möjliggör globala avbrott
	sei();
	MoveFrontLeftLeg(-120,120,-100,30);
	while(1)
	{
		
	}
	
//	MoveDynamixel(6,90,10);
}



