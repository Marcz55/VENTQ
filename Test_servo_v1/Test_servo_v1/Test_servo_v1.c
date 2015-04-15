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

#define FRONT_LEFT_LEG 1
#define FRONT_RIGHT_LEG 2
#define REAR_LEFT_LEG 3
#define REAR_RIGHT_LEG 4

#define FRONT_LEFT_LEG_X 1
#define FRONT_LEFT_LEG_Y 2
#define FRONT_LEFT_LEG_Z 3
#define FRONT_RIGHT_LEG_X 4
#define FRONT_RIGHT_LEG_Y 5
#define FRONT_RIGHT_LEG_Z 6
#define REAR_LEFT_LEG_X 7
#define REAR_LEFT_LEG_Y 8
#define REAR_LEFT_LEG_Z 9
#define REAR_RIGHT_LEG_X 10
#define REAR_RIGHT_LEG_Y 11
#define REAR_RIGHT_LEG_Z 12

int stepLength_g = 80;
int startPositionX_g = 100;
int startPositionY_g = 100;
int startPositionZ_g = -120;
int standardSpeed_g = 20;

#define INCREMENT_PERIOD_100 100
#define INCREMENT_PERIOD_200 200
#define INCREMENT_PERIOD_300 300
#define INCREMENT_PERIOD_400 400
#define INCREMENT_PERIOD_500 500
//---- Globala variabler ---

 // legIncrementPeriod_g represent the time between ticks in ms. 
 // timerOverflowMax_g 
int legIncrementPeriod_g = 300;
int timerOverflowMax_g = 73;
int timerRemainingTicks_g = 62;



void USART0RecieveMode() 
{
    PORTD = (0<<PORTD2);
}

void USART0SendMode()
{
    PORTD = (1<<PORTD2);
}

volatile uint8_t totOverflow_g;

// klockfrekvensen är 16MHz. 
void timer0Init()
{
    // prescaler 256
    TCCR0B |= (1 << CS02);
    
    // initiera counter
    TCNT0 = 0;
    
    // tillåt timer0 overflow interrupt
    
    TIMSK0 |= (1 << TOIE0);
    
    sei();
    
    totOverflow_g = 0;

}

// timerPeriod ska väljas från fördefinierade tider 
void SetLegIncrementPeriod(int newPeriod)
{
    switch(newPeriod)
    {
        case INCREMENT_PERIOD_100:
        {
            timerOverflowMax_g = 24;
            timerRemainingTicks_g = 106;
            break;
        }
        case INCREMENT_PERIOD_200:
        {
            timerOverflowMax_g = 48;
            timerRemainingTicks_g = 212;
            break;
        }
        case INCREMENT_PERIOD_300:
        {
            timerOverflowMax_g = 73;
            timerRemainingTicks_g = 62;
            break;
        }
        case INCREMENT_PERIOD_400:
        {
            timerOverflowMax_g = 97;
            timerRemainingTicks_g = 168;
            break;
        }
        case INCREMENT_PERIOD_500:
        {
            timerOverflowMax_g = 122;
            timerRemainingTicks_g = 18;
            break;
        }
        default:
            // Här kan man ha någon typ av felhantering om man vill

    }
    return;
}

// calcDynamixelSpeed använder legIncrementPeriod_g och förflyttningssträckan för att beräkna en 
// hastighet som gör att slutpositionen uppnås på periodtiden.
long int calcDynamixelSpeed(long int deltaAngle)
{
    return 1000 * deltaAngle / (6 * legIncrementPeriod_g);
}

ISR(TIMER0_OVF_vect)
{
    // räkna antalet avbrott som skett
    totOverflow_g++;
}

void initUSART()
{
    DDRD = (1<<PORTD2); // Styrsignal för sändning/mottagning, PD2
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

void MoveDynamixel(int ID,long int Angle,long int RevolutionsPerMinute)
{
    if ((Angle <= 300) & (Angle >= 0)) // Tillåtna grader är 0-300
    {
        long int LowGoalPosition = ((Angle*1023)/300) & 0x00FF; // Gör om graden till ett tal mellan 0-1023 och delar upp det i LSB(byte) och MSB(byte)
        long int HighGoalPosition = ((Angle*1023)/300) & 0xFF00;
        HighGoalPosition = (HighGoalPosition >> 8);
    
        long int LowAngleVelocity = 0;
        long int HighAngleVelocity = 0;
    
        if (RevolutionsPerMinute >= 114) // Om RPM över 114 så rör vi oss med snabbaste möjliga hastigheten med spänningen som tillhandahålls
        {
            LowAngleVelocity = 0;
            HighAngleVelocity = 0;
        }
        else
        {
            LowAngleVelocity = ((RevolutionsPerMinute*1023)/114) & 0x00FF;
            HighAngleVelocity = ((RevolutionsPerMinute*1023)/114) & 0xFF00;
            HighAngleVelocity = (HighAngleVelocity >> 8);
        }
    
        USARTSendInstruction5(ID,INST_WRITE,P_GOAL_POSITION_L,LowGoalPosition ,HighGoalPosition, LowAngleVelocity, HighAngleVelocity);
        USARTReadStatusPacket();
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
    long int ActuatorAngle2 =  theta2 + 75;
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
    long int theta1 = atan2f(-x,y)*180/PI;
    long int theta2 = 180/PI*(acosf(-z/sqrt(z*z + (sqrt(x*x + y*y) - a1)*(sqrt(x*x + y*y) - a1))) +
    acosf((z*z + (sqrt(x*x + y*y) - a1)*(sqrt(x*x + y*y) - a1) + a2Square - a3Square)/(2*sqrt(z*z + (sqrt((x*x + y*y) - a1)*(sqrt(x*x + y*y) - a1)))*a2)));
    
    long int theta3 = acosf((a2Square + a3Square - z*z - (sqrt(x*x + y*y) - a1)*(sqrt(x*x + y*y) - a1)) / (2*a2*a3))*180/PI;

    long int ActuatorAngle1 =  theta1 + 193;
    long int ActuatorAngle2 =  theta2 + 75;
    long int ActuatorAngle3 =  theta3 + 3;
    
    
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
    long int theta1 = atan2f(x,-y)*180/PI;
    long int theta2 = 180/PI*(acosf(-z/sqrt(z*z + (sqrt(x*x + y*y) - a1)*(sqrt(x*x + y*y) - a1))) +
    acosf((z*z + (sqrt(x*x + y*y) - a1)*(sqrt(x*x + y*y) - a1) + a2Square - a3Square)/(2*sqrt(z*z + (sqrt((x*x + y*y) - a1)*(sqrt(x*x + y*y) - a1)))*a2)));
    
    long int theta3 = acosf((a2Square + a3Square - z*z - (sqrt(x*x + y*y) - a1)*(sqrt(x*x + y*y) - a1)) / (2*a2*a3))*180/PI;

    long int ActuatorAngle1 =  theta1 + 195;
    long int ActuatorAngle2 =  225 - theta2;
    long int ActuatorAngle3 =  300 - theta3 ;
    
    
    MoveDynamixel(1,ActuatorAngle1,speed);
    USARTReadStatusPacket();
    MoveDynamixel(3,ActuatorAngle2,speed);
    USARTReadStatusPacket();
    MoveDynamixel(5,ActuatorAngle3,speed);
    USARTReadStatusPacket();
    return;
}
void MoveRearRightLeg(float x, float y, float z, int speed)
{
    long int theta1 = atan2f(x,-y)*180/PI;
    long int theta2 = 180/PI*(acosf(-z/sqrt(z*z + (sqrt(x*x + y*y) - a1)*(sqrt(x*x + y*y) - a1))) +
    acosf((z*z + (sqrt(x*x + y*y) - a1)*(sqrt(x*x + y*y) - a1) + a2Square - a3Square)/(2*sqrt(z*z + (sqrt((x*x + y*y) - a1)*(sqrt(x*x + y*y) - a1)))*a2)));
    
    long int theta3 = acosf((a2Square + a3Square - z*z - (sqrt(x*x + y*y) - a1)*(sqrt(x*x + y*y) - a1)) / (2*a2*a3))*180/PI;

    long int ActuatorAngle1 =  theta1 + 105;
    long int ActuatorAngle2 =  225 - theta2;
    long int ActuatorAngle3 =  300 - theta3;
    
    
    MoveDynamixel(7,ActuatorAngle1,speed);
    USARTReadStatusPacket();
    MoveDynamixel(9,ActuatorAngle2,speed);
    USARTReadStatusPacket();
    MoveDynamixel(11,ActuatorAngle3,speed);
    USARTReadStatusPacket();
    return;
}

void MoveToStartPosition()
{
    MoveFrontLeftLeg(-startPositionX_g,startPositionY_g,startPositionZ_g,standardSpeed_g);
    MoveFrontRightLeg(startPositionX_g,startPositionY_g,startPositionZ_g,standardSpeed_g);
    MoveRearLeftLeg(-startPositionX_g,-startPositionY_g,startPositionZ_g,standardSpeed_g);
    MoveRearRightLeg(startPositionX_g,-startPositionY_g,startPositionZ_g,standardSpeed_g);
    return;
}

// Rader är vinklar på servon. Kolumnerna innehåller positioner
long int actuatorPositions_g [12][20];
long int legPositions_g [12][20];

int currentPos_g = 0;
int nextPos_g = 1;
int maxGaitCyclePos_g = 1;

// Den här funktionen hanterar vilken position i gångcykeln roboten är i. Den går runt baserat på 
// antalet positioner som finns i den givna gångstilen. 
void increasePositionIndexes()
{
    if (currentPos_g >= maxGaitCyclePos_g)
    {
        currentPos_g = 0;
    }
    else
    {
        currentPos_g++;
        if (currentPos_g >= maxGaitCyclePos_g)
        {
            nextPos_g = 0;
        }
        else
        {
            nextPos_g = currentPos_g + 1;
        }
    }
    return;
}

typedef struct leg leg;
struct leg {
    int legNumber,
        coxaJoint,
        femurJoint,
        tibiaJoint;
};

leg frontLeftLeg = {FRONT_LEFT_LEG, 2, 4, 6};
leg frontRightLeg = {FRONT_RIGHT_LEG, 8, 10, 12};
leg rearLeftLeg = {REAR_LEFT_LEG, 1, 3, 5};
leg rearRightLeg = {REAR_RIGHT_LEG, 7, 9, 11};


void CalcStraightPath(leg currentLeg, int numberOfPositions, int startIndex, float x1, float y1, float z1, float x2, float y2, float z2)
{
    long int theta1;
    long int theta2;
    long int theta3;
    if ((currentLeg.legNumber == FRONT_LEFT_LEG) | (currentLeg.legNumber == FRONT_RIGHT_LEG))
    {
        x1 *= -1;
        x2 *= -1;
    }
    if ((currentLeg.legNumber == REAR_RIGHT_LEG) | (currentLeg.legNumber == REAR_LEFT_LEG))
    {
        y1 *= -1;
        y2 *= -1;
    }
    float deltaX = (x2 - x1) / numberOfPositions;
    float deltaY = (y2 - y1) / numberOfPositions;
    float deltaZ = (z2 - z1) / numberOfPositions;
    
    float x = x1;
    float y = y1;
    float z = z1;
    
  
    
    
    for (int i = startIndex; i < startIndex + numberOfPositions; i++)
    {
        x = x + deltaX;
        y = y + deltaY;
        z = z + deltaZ;
        
        // lös inverskinematik för lederna.
        theta1 = atan2f(x,y)*180/PI;
        theta2 = 180/PI*(acosf(-z/sqrt(z*z + (sqrt(x*x + y*y) - a1)*(sqrt(x*x + y*y) - a1))) +
        acosf((z*z + (sqrt(x*x + y*y) - a1)*(sqrt(x*x + y*y) - a1) + a2Square - a3Square)/(2*sqrt(z*z + (sqrt((x*x + y*y) - a1)*(sqrt(x*x + y*y) - a1)))*a2)));
        
        theta3 = acosf((a2Square + a3Square - z*z - (sqrt(x*x + y*y) - a1)*(sqrt(x*x + y*y) - a1)) / (2*a2*a3))*180/PI;
        
        // spara resultatet i global array
        switch(currentLeg.legNumber)
        {
            case FRONT_LEFT_LEG:
            {
                actuatorPositions_g[currentLeg.coxaJoint][i] = theta1 + 105;
                actuatorPositions_g[currentLeg.femurJoint][i] =  theta2 + 75;
                actuatorPositions_g[currentLeg.tibiaJoint][i] =  theta3 + 1;
                legPositions_g[FRONT_LEFT_LEG_X][i] = x;
                legPositions_g[FRONT_LEFT_LEG_Y][i] = y;
                legPositions_g[FRONT_LEFT_LEG_Z][i] = z;
                break;
            }
            case FRONT_RIGHT_LEG:
            {
                actuatorPositions_g[currentLeg.coxaJoint][i] = theta1 + 193;
                actuatorPositions_g[currentLeg.femurJoint][i] =  theta2 + 75;
                actuatorPositions_g[currentLeg.tibiaJoint][i] =  theta3 + 3;
                legPositions_g[FRONT_RIGHT_LEG_X][i] = x;
                legPositions_g[FRONT_RIGHT_LEG_Y][i] = y;
                legPositions_g[FRONT_RIGHT_LEG_Z][i] = z;
                break;
            }
            case REAR_LEFT_LEG:
            {
                actuatorPositions_g[currentLeg.coxaJoint][i] = theta1 + 195;
                actuatorPositions_g[currentLeg.femurJoint][i] =  225 - theta2;
                actuatorPositions_g[currentLeg.tibiaJoint][i] =  300 - theta3;
                legPositions_g[REAR_LEFT_LEG_X][i] = x;
                legPositions_g[REAR_LEFT_LEG_Y][i] = y;
                legPositions_g[REAR_LEFT_LEG_Z][i] = z;
                break;
            }
            case REAR_RIGHT_LEG:
            {
                actuatorPositions_g[currentLeg.coxaJoint][i] = theta1 + 105;
                actuatorPositions_g[currentLeg.femurJoint][i] =  225 - theta2;
                actuatorPositions_g[currentLeg.tibiaJoint][i] =  300 - theta3;
                legPositions_g[REAR_RIGHT_LEG_X][i] = x;
                legPositions_g[REAR_RIGHT_LEG_Y][i] = y;
                legPositions_g[REAR_RIGHT_LEG_Z][i] = z;
                break;
            }
        }
        
        
        
        
    }
}

void CalcCurvedPath(leg currentLeg, int numberOfPositions, int startIndex, float x1, float y1, float z1, float x2, float y2, float z2)
{
    int topPosition = startIndex + numberOfPositions / 2 - 1;
    long int theta1;
    long int theta2;
    long int theta3;
    if ((currentLeg.legNumber == FRONT_LEFT_LEG) | (currentLeg.legNumber == FRONT_RIGHT_LEG))
    {
        x1 *= -1;
        x2 *= -1;
    }
    if ((currentLeg.legNumber == REAR_RIGHT_LEG) | (currentLeg.legNumber == REAR_LEFT_LEG))
    {
        y1 *= -1;
        y2 *= -1;
    }
    float deltaX = (x2 - x1) / numberOfPositions;
    float deltaY = (y2 - y1) / numberOfPositions;
    float deltaZBegin = (60 + z2 - z1) / (numberOfPositions / 2); // första halvan av sträckan ska benet röra sig mot en position 4cm över slutpositionen
    float deltaZEnd = (z2 - z1 - 60) / (numberOfPositions / 2); // andra halvan av sträckan ska benet röra sig mot en position 4cm under slutpositionen -> benet får en triangelbana
    float x = x1;
    float y = y1;
    float z = z1;
    
    
    // första halvan av rörelsen
    for (int i = startIndex; i <= topPosition; i++)
    {
        x = x + deltaX;
        y = y + deltaY;
        z = z + deltaZBegin;
        
        // lös inverskinematik för lederna.
        theta1 = atan2f(x,y)*180/PI;
        theta2 = 180/PI*(acosf(-z/sqrt(z*z + (sqrt(x*x + y*y) - a1)*(sqrt(x*x + y*y) - a1))) +
        acosf((z*z + (sqrt(x*x + y*y) - a1)*(sqrt(x*x + y*y) - a1) + a2Square - a3Square)/(2*sqrt(z*z + (sqrt((x*x + y*y) - a1)*(sqrt(x*x + y*y) - a1)))*a2)));
        
        theta3 = acosf((a2Square + a3Square - z*z - (sqrt(x*x + y*y) - a1)*(sqrt(x*x + y*y) - a1)) / (2*a2*a3))*180/PI;
        
        // spara resultatet i global array
        switch(currentLeg.legNumber)
        {
            case FRONT_LEFT_LEG:
            {
                actuatorPositions_g[currentLeg.coxaJoint][i] = theta1 + 105;
                actuatorPositions_g[currentLeg.femurJoint][i] =  theta2 + 75;
                actuatorPositions_g[currentLeg.tibiaJoint][i] =  theta3 + 1;
                legPositions_g[FRONT_LEFT_LEG_X][i] = x;
                legPositions_g[FRONT_LEFT_LEG_Y][i] = y;
                legPositions_g[FRONT_LEFT_LEG_Z][i] = z;
                break;
            }
            case FRONT_RIGHT_LEG:
            {
                actuatorPositions_g[currentLeg.coxaJoint][i] = theta1 + 193;
                actuatorPositions_g[currentLeg.femurJoint][i] =  theta2 + 75;
                actuatorPositions_g[currentLeg.tibiaJoint][i] =  theta3 + 3;
                legPositions_g[FRONT_RIGHT_LEG_X][i] = x;
                legPositions_g[FRONT_RIGHT_LEG_Y][i] = y;
                legPositions_g[FRONT_RIGHT_LEG_Z][i] = z;
                break;
            }
            case REAR_LEFT_LEG:
            {
                actuatorPositions_g[currentLeg.coxaJoint][i] = theta1 + 195;
                actuatorPositions_g[currentLeg.femurJoint][i] =  225 - theta2;
                actuatorPositions_g[currentLeg.tibiaJoint][i] =  300 - theta3;
                legPositions_g[REAR_LEFT_LEG_X][i] = x;
                legPositions_g[REAR_LEFT_LEG_Y][i] = y;
                legPositions_g[REAR_LEFT_LEG_Z][i] = z;
                break;
            }
            case REAR_RIGHT_LEG:
            {
                actuatorPositions_g[currentLeg.coxaJoint][i] = theta1 + 105;
                actuatorPositions_g[currentLeg.femurJoint][i] =  225 - theta2;
                actuatorPositions_g[currentLeg.tibiaJoint][i] =  300 - theta3;
                legPositions_g[REAR_RIGHT_LEG_X][i] = x;
                legPositions_g[REAR_RIGHT_LEG_Y][i] = y;
                legPositions_g[REAR_RIGHT_LEG_Z][i] = z;
                break;
            }
        }
    }
    
    for (int i = topPosition + 1; i < startIndex + numberOfPositions; i++)
    {
        x = x + deltaX;
        y = y + deltaY;
        z = z + deltaZEnd;
        
        // lös inverskinematik för lederna.
        theta1 = atan2f(x,y)*180/PI;
        theta2 = 180/PI*(acosf(-z/sqrt(z*z + (sqrt(x*x + y*y) - a1)*(sqrt(x*x + y*y) - a1))) +
        acosf((z*z + (sqrt(x*x + y*y) - a1)*(sqrt(x*x + y*y) - a1) + a2Square - a3Square)/(2*sqrt(z*z + (sqrt((x*x + y*y) - a1)*(sqrt(x*x + y*y) - a1)))*a2)));
        
        theta3 = acosf((a2Square + a3Square - z*z - (sqrt(x*x + y*y) - a1)*(sqrt(x*x + y*y) - a1)) / (2*a2*a3))*180/PI;
        
        // spara resultatet i global array
       switch(currentLeg.legNumber)
       {
           case FRONT_LEFT_LEG:
           {
               actuatorPositions_g[currentLeg.coxaJoint][i] = theta1 + 105;
               actuatorPositions_g[currentLeg.femurJoint][i] =  theta2 + 75;
               actuatorPositions_g[currentLeg.tibiaJoint][i] =  theta3 + 1;
               legPositions_g[FRONT_LEFT_LEG_X][i] = x;
               legPositions_g[FRONT_LEFT_LEG_Y][i] = y;
               legPositions_g[FRONT_LEFT_LEG_Z][i] = z;
               break;
           }
           case FRONT_RIGHT_LEG:
           {
               actuatorPositions_g[currentLeg.coxaJoint][i] = theta1 + 193;
               actuatorPositions_g[currentLeg.femurJoint][i] =  theta2 + 75;
               actuatorPositions_g[currentLeg.tibiaJoint][i] =  theta3 + 3;
               legPositions_g[FRONT_RIGHT_LEG_X][i] = x;
               legPositions_g[FRONT_RIGHT_LEG_Y][i] = y;
               legPositions_g[FRONT_RIGHT_LEG_Z][i] = z;
               break;
           }
           case REAR_LEFT_LEG:
           {
               actuatorPositions_g[currentLeg.coxaJoint][i] = theta1 + 195;
               actuatorPositions_g[currentLeg.femurJoint][i] =  225 - theta2;
               actuatorPositions_g[currentLeg.tibiaJoint][i] =  300 - theta3;
               legPositions_g[REAR_LEFT_LEG_X][i] = x;
               legPositions_g[REAR_LEFT_LEG_Y][i] = y;
               legPositions_g[REAR_LEFT_LEG_Z][i] = z;
               break;
           }
           case REAR_RIGHT_LEG:
           {
               actuatorPositions_g[currentLeg.coxaJoint][i] = theta1 + 105;
               actuatorPositions_g[currentLeg.femurJoint][i] =  225 - theta2;
               actuatorPositions_g[currentLeg.tibiaJoint][i] =  300 - theta3;
               legPositions_g[REAR_RIGHT_LEG_X][i] = x;
               legPositions_g[REAR_RIGHT_LEG_Y][i] = y;
               legPositions_g[REAR_RIGHT_LEG_Z][i] = z;
               break;
           }
       }
    }
}

// För att testa gången
int Direction = 0;

ISR(INT1_vect)
{   
    MoveDynamixel(frontLeftLeg.coxaJoint, actuatorPositions_g[frontLeftLeg.coxaJoint][currentPos_g],20);
    USARTReadStatusPacket();
    MoveDynamixel(frontLeftLeg.femurJoint, actuatorPositions_g[frontLeftLeg.femurJoint][currentPos_g],20);
    USARTReadStatusPacket();
    MoveDynamixel(frontLeftLeg.tibiaJoint, actuatorPositions_g[frontLeftLeg.tibiaJoint][currentPos_g],20);
    USARTReadStatusPacket();
   
    MoveDynamixel(frontRightLeg.coxaJoint, actuatorPositions_g[frontRightLeg.coxaJoint][currentPos_g],20);
    USARTReadStatusPacket();
    MoveDynamixel(frontRightLeg.femurJoint, actuatorPositions_g[frontRightLeg.femurJoint][currentPos_g],20);
    USARTReadStatusPacket();
    MoveDynamixel(frontRightLeg.tibiaJoint, actuatorPositions_g[frontRightLeg.tibiaJoint][currentPos_g],20);
    USARTReadStatusPacket();
    
    MoveDynamixel(rearRightLeg.coxaJoint, actuatorPositions_g[rearRightLeg.coxaJoint][currentPos_g],20);
    USARTReadStatusPacket();
    MoveDynamixel(rearRightLeg.femurJoint, actuatorPositions_g[rearRightLeg.femurJoint][currentPos_g],20);
    USARTReadStatusPacket();
    MoveDynamixel(rearRightLeg.tibiaJoint, actuatorPositions_g[rearRightLeg.tibiaJoint][currentPos_g],20);
    USARTReadStatusPacket();
    
    MoveDynamixel(rearLeftLeg.coxaJoint, actuatorPositions_g[rearLeftLeg.coxaJoint][currentPos_g],20);
    USARTReadStatusPacket();
    MoveDynamixel(rearLeftLeg.femurJoint, actuatorPositions_g[rearLeftLeg.femurJoint][currentPos_g],20);
    USARTReadStatusPacket();
    MoveDynamixel(rearLeftLeg.tibiaJoint, actuatorPositions_g[rearLeftLeg.tibiaJoint][currentPos_g],20);
    USARTReadStatusPacket();
   
   
   
   /*
   
    if (currentPos_g > 8)
    {
        currentPos_g = 0;
        switch (Direction)
        {
            case 0:
            {
                CalcCurvedPath(frontLeftLeg,10,0,-160,0,-50,-160,140,-50);
                CalcCurvedPath(frontRightLeg,10,0,160,0,-50,160,140,-50);
                CalcCurvedPath(rearLeftLeg,10,0,-160,-140,-50,-160,0,-50);
                CalcCurvedPath(rearRightLeg,10,0,160,-140,-50,160,0,-50);
                
                Direction = 1;
                break;
            }            
            case 1:
            {
                CalcStraightPath(frontLeftLeg,10,0,-160,140,-50,-160,0,-50);
                CalcStraightPath(frontRightLeg,10,0,160,140,-50,160,0,-50);
                CalcStraightPath(rearLeftLeg,10,0,-160,0,-50,-160,-140,-50);
                CalcStraightPath(rearRightLeg,10,0,160,0,-50,160,-140,-50);
                Direction = 0;
                break;
            }
        }
        
    }*/
    currentPos_g++;
    if (currentPos_g > 20)
    {
        currentPos_g = 1;
    }
    return;
} 

void MoveLegToNextPosition(leg leg)
{
    // CoxaJoint
    long int currentAngle = actuatorPositions_g[leg.coxaJoint][currentPos_g];
    long int nextAngle = actuatorPositions_g[leg.coxaJoint][nextPos_g];
    long int RPM = calcDynamixelSpeed(abs(currentAngle - nextAngle));
    MoveDynamixel(leg.coxaJoint, nextAngle, RPM);
    // FemurJoint
    currentAngle = actuatorPositions_g[leg.femurJoint][currentPos_g];
    nextAngle = actuatorPositions_g[leg.femurJoint][nextPos_g];
    RPM = calcDynamixelSpeed(abs(currentAngle - nextAngle));
    MoveDynamixel(leg.femurJoint, nextAngle, RPM);
    // TibiaJoint
    currentAngle = actuatorPositions_g[leg.tibiaJoint][currentPos_g];
    nextAngle = actuatorPositions_g[leg.tibiaJoint][nextPos_g];
    RPM = calcDynamixelSpeed(abs(currentAngle - nextAngle));
    MoveDynamixel(leg.tibiaJoint, nextAngle, RPM);

}

void move()
{
    
    MoveLegToNextPosition(frontLeftLeg);
    MoveLegToNextPosition(frontRightLeg);
    MoveLegToNextPosition(rearLeftLeg);
    MoveLegToNextPosition(rearRightLeg);

    increasePositionIndexes();  
}

MakeTrotGait()
{
    maxGaitCyclePos_g = 19;
    CalcCurvedPath(frontLeftLeg,10,0,-startPositionX_g,startPositionY_g-stepLength_g/2,startPositionZ_g,-startPositionX_g,startPositionY_g+stepLength_g/2,startPositionZ_g);
    CalcStraightPath(frontLeftLeg,10,10,-startPositionX_g,startPositionY_g+stepLength_g/2,startPositionZ_g,-startPositionX_g,startPositionY_g-stepLength_g/2,startPositionZ_g);
    
    CalcCurvedPath(rearRightLeg,10,0,startPositionX_g,-startPositionY_g-stepLength_g/2,startPositionZ_g,startPositionX_g,-startPositionY_g+stepLength_g/2,startPositionZ_g);
    CalcStraightPath(rearRightLeg,10,10,startPositionX_g,-startPositionY_g+stepLength_g/2,startPositionZ_g,startPositionX_g,-startPositionY_g-stepLength_g/2,startPositionZ_g);
    
    CalcStraightPath(rearLeftLeg,10,0,-startPositionX_g,-startPositionY_g+stepLength_g/2,startPositionZ_g,-startPositionX_g,-startPositionY_g-stepLength_g/2,startPositionZ_g);
    CalcCurvedPath(rearLeftLeg,10,10,-startPositionX_g,-startPositionY_g-stepLength_g/2,startPositionZ_g,-startPositionX_g,-startPositionY_g+stepLength_g/2,startPositionZ_g);
    
    CalcStraightPath(frontRightLeg,10,0,startPositionX_g,startPositionY_g+stepLength_g/2,startPositionZ_g,startPositionX_g,startPositionY_g-stepLength_g/2,startPositionZ_g);
    CalcCurvedPath(frontRightLeg,10,10,startPositionX_g,startPositionY_g-stepLength_g/2,startPositionZ_g,startPositionX_g,startPositionY_g+stepLength_g/2,startPositionZ_g);

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
    //MoveFrontRightLeg(150,150,0,30);
    currentPos_g = 5;
    nextPos_g = 6;
    // front right
    /*
    MoveDynamixel(8,193,10);
    USARTReadStatusPacket();
    MoveDynamixel(10, 75 + 45, 10);
    USARTReadStatusPacket();
    MoveDynamixel(12,183 + 45,10);
    USARTReadStatusPacket();
    */
    // front left
    /*
    MoveDynamixel(2,105,10);
    USARTReadStatusPacket();
    MoveDynamixel(4,75 + 45, 10);
    USARTReadStatusPacket();
    MoveDynamixel(6,181 + 45,10);
    USARTReadStatusPacket();
    */
    // rear right
    /*
    MoveDynamixel(7,105,10);
    USARTReadStatusPacket();
    MoveDynamixel(9,225 -45,10);
    USARTReadStatusPacket();
    MoveDynamixel(11,120 -45, 10);
    USARTReadStatusPacket();    
    */
    // rear left
    /*
    MoveDynamixel(1,195,10);
    USARTReadStatusPacket();
    MoveDynamixel(3,225 - 45, 10);
    USARTReadStatusPacket();
    MoveDynamixel(5,120 - 45 ,10);
    USARTReadStatusPacket();
    */
    timer0Init();
    /*
    MoveFrontRightLeg(160,140,-50,10);
    MoveFrontLeftLeg(-160,140,-50,10);
    MoveRearLeftLeg(-160,0,-50,10);
    MoveRearRightLeg(160,0,-50,10);
    
<<<<<<< Updated upstream
	CalcStraightPath(frontLeftLeg,10,0,-160,140,-50,-160,0,-50);
    CalcStraightPath(frontRightLeg,10,0,160,120,-50,160,0,-50);
    CalcStraightPath(rearLeftLeg,10,0,-160,0,-50,-160,-140,-50);
    CalcStraightPath(rearRightLeg,10,0,160,0,-50,160,-140,-50);
	while(1)
    {
    	// kolla om antalet overflows är mer än 52
    	if (totOverflow_g >= 25)
    	{
        	// när detta skett ska timern räkna upp ytterligare 53 tick för att exakt 50ms ska ha passerat
        	if (TCNT0 >= 53)
        	{
            	// xor-tilldelning med en etta gör att biten togglas
            	move();
            	TCNT0 = 0;			// Återställ räknaren
            	totOverflow_g = 0;
            	
        	}
    	}
	}*/
	MakeTrotGait();
    MoveToStartPosition();
    
    while (1)
    {
        // kolla om antalet overflows är mer än 52
    	if (totOverflow_g >= timerOverflowMax_g)
    	{
        	// när detta skett ska timern räkna upp ytterligare 53 tick för att exakt 50ms ska ha passerat
        	if (TCNT0 >= timerRemainingTicks_g)
        	{
            	// xor-tilldelning med en etta gör att biten togglas
            	move();
            	TCNT0 = 0;			// Återställ räknaren
            	totOverflow_g = 0;
            	
        	}
    	}
    }
    
}



