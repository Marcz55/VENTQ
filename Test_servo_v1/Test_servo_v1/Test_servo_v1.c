/*
 * Test_servo_v1.c
 *
 * Created: 3/26/2015 11:50:32 AM
 *  Author: joamo950
 */ 

#include <avr/io.h>
#include <avr/interrupt.h>
#define F_CPU 16000000UL
#include <util/delay.h>
#include "SPI.h"
#include "USART.h"
#include <stdlib.h>

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

#define INCREMENT_PERIOD_10 10
#define INCREMENT_PERIOD_20 20
#define INCREMENT_PERIOD_30 30
#define INCREMENT_PERIOD_40 40
#define INCREMENT_PERIOD_50 50
#define INCREMENT_PERIOD_60 60
#define INCREMENT_PERIOD_70 70
#define INCREMENT_PERIOD_80 80
#define INCREMENT_PERIOD_90 90
#define INCREMENT_PERIOD_100 100
#define INCREMENT_PERIOD_200 200
#define INCREMENT_PERIOD_300 300
#define INCREMENT_PERIOD_400 400
#define INCREMENT_PERIOD_500 500


int stepLength_g = 60;
int startPositionX_g = 100;
int startPositionY_g = 100;
int startPositionZ_g = -120;
int stepHeight_g =  40;
int gaitResolution_g = 12; // MÅSTE VARA DELBART MED 4 vid trot, 8 vid creep
int speedMultiplier_g = 1;
int gaitResolutionTime_g = INCREMENT_PERIOD_40;
int currentInstruction = 0; // Nuvarande manuell styrinstruktion

/*
// Joakims coola gångstil,
int stepLength_g = 40;
int startPositionX_g = 20;
int startPositionY_g = 140;
int startPositionZ_g = -110;
int stepHeight_g = 20;
int gaitResolution_g = 12;
int gaitResolutionTime_g = INCREMENT_PERIOD_60;
int speedMultiplier_g = 1;
*/


enum direction{
    north = 1,
    east,
    south,
    west
    }; 
enum direction currentDirection = north;


int standardSpeed_g = 20;
int statusPackEnabled = 0;


//---- Globala variabler ---

 // gaitResolutionTime_g represent the time between ticks in ms. 
 // timerOverflowMax_g 
int timerOverflowMax_g = 73;
int timerRemainingTicks_g = 62;





void USART0RecieveMode() 
{
    PORTD = (0<<PORTD4);
}

void USART0SendMode()
{
    PORTD = (1<<PORTD4);
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
void SetGaitResolutionPeriod(int newPeriod)
{
    switch(newPeriod)
    {
        case INCREMENT_PERIOD_10:
        {
            gaitResolutionTime_g = newPeriod;
            timerOverflowMax_g = 2;
            timerRemainingTicks_g = 113;
            break;
        }
        case INCREMENT_PERIOD_20:
        {
            gaitResolutionTime_g = newPeriod;
            timerOverflowMax_g = 4;
            timerRemainingTicks_g = 226;
            break;
        }
        case INCREMENT_PERIOD_30:
        {
            gaitResolutionTime_g = newPeriod;
            timerOverflowMax_g = 7;
            timerRemainingTicks_g = 83;
            break;
        }
        case INCREMENT_PERIOD_40:
        {
            gaitResolutionTime_g = newPeriod;
            timerOverflowMax_g = 9;
            timerRemainingTicks_g = 196;
            break;
        }
        case INCREMENT_PERIOD_50:
        {
            gaitResolutionTime_g = newPeriod;
            timerOverflowMax_g = 12;
            timerRemainingTicks_g = 53;
            break;
        }
        case INCREMENT_PERIOD_60:
        {
            gaitResolutionTime_g = newPeriod;
            timerOverflowMax_g = 14;
            timerRemainingTicks_g = 166;
            break;
        }
        case INCREMENT_PERIOD_70:
        {
            gaitResolutionTime_g = newPeriod;
            timerOverflowMax_g = 17;
            timerRemainingTicks_g = 23;
            break;
        }
        case INCREMENT_PERIOD_80:
        {
            gaitResolutionTime_g = newPeriod;
            timerOverflowMax_g = 19;
            timerRemainingTicks_g = 136;
            break;
        }
        case INCREMENT_PERIOD_90:
        {
            gaitResolutionTime_g = newPeriod;
            timerOverflowMax_g = 21;
            timerRemainingTicks_g = 249;
            break;
        }
        case INCREMENT_PERIOD_100:
        {
            gaitResolutionTime_g = newPeriod;
            timerOverflowMax_g = 24;
            timerRemainingTicks_g = 106;
            break;
        }
        case INCREMENT_PERIOD_200:
        {
            gaitResolutionTime_g = newPeriod;
            timerOverflowMax_g = 48;
            timerRemainingTicks_g = 212;
            break;
        }
        case INCREMENT_PERIOD_300:
        {
            gaitResolutionTime_g = newPeriod;
            timerOverflowMax_g = 73;
            timerRemainingTicks_g = 62;
            break;
        }
        case INCREMENT_PERIOD_400:
        {
            gaitResolutionTime_g = newPeriod;
            timerOverflowMax_g = 97;
            timerRemainingTicks_g = 168;
            break;
        }
        case INCREMENT_PERIOD_500:
        {
            gaitResolutionTime_g = newPeriod;
            timerOverflowMax_g = 122;
            timerRemainingTicks_g = 18;
            break;
        }
        // default:
            // Här kan man ha någon typ av felhantering om man vill

    }
    return;
}

// calcDynamixelSpeed använder legIncrementPeriod_g och förflyttningssträckan för att beräkna en 
// hastighet som gör att slutpositionen uppnås på periodtiden.
long int calcDynamixelSpeed(long int deltaAngle)
{
    long int calculatedSpeed = (1000 * deltaAngle) / (6 * gaitResolutionTime_g);
    if (calculatedSpeed < 2)
    {
        return 2;
    }
    else
    {
        return speedMultiplier_g * calculatedSpeed;
    }
}

ISR(TIMER0_OVF_vect)
{
    // räkna antalet avbrott som skett
    totOverflow_g++;
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
        //USARTReadStatusPacket();
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
    MoveDynamixel(4,ActuatorAngle2,speed);
    MoveDynamixel(6,ActuatorAngle3,speed);
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
    MoveDynamixel(10,ActuatorAngle2,speed);
    MoveDynamixel(12,ActuatorAngle3,speed);
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
    MoveDynamixel(3,ActuatorAngle2,speed);
    MoveDynamixel(5,ActuatorAngle3,speed);
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
    MoveDynamixel(9,ActuatorAngle2,speed);
    MoveDynamixel(11,ActuatorAngle3,speed);
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

// Rader är vinklar på servon. Kolumnerna innehåller positioner. Allokerar en extra rad minne 
// här i matriserna bara för att få snyggare kod. 
long int actuatorPositions_g [13][20];
long int legPositions_g [13][20];
int regulation_g[3];

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
        nextPos_g = currentPos_g + 1;
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
void CalcParabelPath(leg currentLeg, int numberOfPositions, int startIndex, float x1, float y1, float z1, float x2, float y2, float z2)
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
    float deltaZBegin = (stepHeight_g + z2 - z1) / (numberOfPositions / 2); // första halvan av sträckan ska benet röra sig mot en position 4cm över slutpositionen
    float deltaZEnd = (z2 - z1 - stepHeight_g) / (numberOfPositions / 2); // andra halvan av sträckan ska benet röra sig mot en position 4cm under slutpositionen -> benet får en triangelbana
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

void MoveLegToNextPosition(leg Leg)
{
    // test
    //long int speed = 32;
    // CoxaJoint
    long int currentAngle = actuatorPositions_g[Leg.coxaJoint][currentPos_g];
    long int nextAngle = actuatorPositions_g[Leg.coxaJoint][nextPos_g];
    long int RPM = calcDynamixelSpeed(abs(currentAngle - nextAngle));
    MoveDynamixel(Leg.coxaJoint, nextAngle, RPM);
    // FemurJoint
    currentAngle = actuatorPositions_g[Leg.femurJoint][currentPos_g];
    nextAngle = actuatorPositions_g[Leg.femurJoint][nextPos_g];
    RPM = calcDynamixelSpeed(abs(currentAngle - nextAngle));
    MoveDynamixel(Leg.femurJoint, nextAngle, RPM);
    // TibiaJoint
    currentAngle = actuatorPositions_g[Leg.tibiaJoint][currentPos_g];
    nextAngle = actuatorPositions_g[Leg.tibiaJoint][nextPos_g];
    RPM = calcDynamixelSpeed(abs(currentAngle - nextAngle));
    MoveDynamixel(Leg.tibiaJoint, nextAngle, RPM);
    

}


void move()
{
    
    MoveLegToNextPosition(frontLeftLeg);
    MoveLegToNextPosition(frontRightLeg);
    MoveLegToNextPosition(rearLeftLeg);
    MoveLegToNextPosition(rearRightLeg);

    increasePositionIndexes();
}

int directionHasChanged = 0;

ISR(INT0_vect)
{
    PORTA = (0<<PORTA2);
    spiTransmit(0x44); // Skicka iväg skräp för att kunna ta emot det som finns i kommunikationsenhetens SPDR
    PORTA = (1<<PORTA2);
    currentInstruction = inbuffer;
    directionHasChanged = 1;
}

ISR(INT1_vect)
{   
    directionHasChanged = 1;
    switch(currentDirection)
    {
        case north:
        {
           currentDirection = east;
           break;
        }
        case east:
        {
            currentDirection = south;
            break;
        }
        case south:
        {
            currentDirection = west;
            break;
        }
        case west:
        {
            currentDirection = north;
            break;
        }
    }
   // move();
} 

int regulation_g[3];

void calcRegulation()
{
    
    //static int  regulation[3]; // skapar en array som innehåller hur roboten ska reglera
    /*
    * 
    *
    * Första värdet anger hur mycket åt höger relativt rörelseriktningen roboten ska ta sig i varje steg.
    * Andra värdet anger hur mycket längre steg benen på vänstra sidan relativt rörelseriktningen ska ta, de som medför en CW-rotation. 
    * Tredje värdet anger hur mycket längre steg benen på högra sidan relativt rörelseriktningen ska ta, de som medför en CCW-rotation
    */
    regulation_g[0] =  0;
    regulation_g[1] = 0;
    regulation_g[2] = 0;
    return; 
}

/*
/ 
/ ej klar
/
*/
void returnToStartPosition()
{
    long int currentLegPositionsCopy[13];
    for (int i = 0; i < 13; i++)
    {
        currentLegPositionsCopy[i] = legPositions_g[i][currentPos_g];
    }
    // flytta ner alla benen till marknivå
    //CalcStraightPath(frontLeftLeg, 2, 0, )
}

void moveToCreepStartPosition()
{
    long int sixth = stepLength_g/6;
    long int half = stepLength_g/2;
    MoveRearRightLeg(startPositionX_g, -startPositionY_g-half, startPositionZ_g, 20);
    MoveFrontRightLeg(startPositionX_g, startPositionY_g-sixth, startPositionZ_g, 20);
    MoveRearLeftLeg(-startPositionX_g, -startPositionY_g+sixth, startPositionZ_g, 20);
    MoveFrontLeftLeg(-startPositionX_g, startPositionY_g+half, startPositionZ_g, 20);
}
// cycleResolution måste vara delbart med 8
void makeCreepGait(int cycleResolution)
{
    int res = cycleResolution/4;
    maxGaitCyclePos_g = cycleResolution -1;
    long int sixth = stepLength_g/6;
    long int half = stepLength_g/2;

    CalcCurvedPath(rearRightLeg, res, 0, startPositionX_g, -startPositionY_g-half, startPositionZ_g, startPositionX_g, -startPositionY_g+half, startPositionZ_g);
    CalcStraightPath(rearRightLeg, res*3, res, startPositionX_g, -startPositionY_g+half, startPositionZ_g, startPositionX_g, -startPositionY_g-half, startPositionZ_g);

    CalcStraightPath(frontRightLeg, res, 0, startPositionX_g, startPositionY_g-sixth, startPositionZ_g, startPositionX_g, startPositionY_g-half, startPositionZ_g);
    CalcCurvedPath(frontRightLeg, res, res, startPositionX_g, startPositionY_g-half, startPositionZ_g, startPositionX_g, startPositionY_g+stepLength_g, startPositionZ_g);
    CalcStraightPath(frontRightLeg, 2*res, 2*res, startPositionX_g, startPositionY_g+half, startPositionZ_g, startPositionX_g, startPositionY_g-sixth, startPositionZ_g);

    CalcStraightPath(rearLeftLeg, 2*res, 0, -startPositionX_g, -startPositionY_g+sixth, startPositionZ_g, -startPositionX_g, -startPositionY_g-half, startPositionZ_g);
    CalcCurvedPath(rearLeftLeg, res, 2*res, -startPositionX_g, -startPositionY_g-half, startPositionZ_g, -startPositionX_g, -startPositionY_g+half, startPositionZ_g);
    CalcStraightPath(rearLeftLeg, res, 3*res, -startPositionX_g, -startPositionY_g+half, startPositionZ_g, -startPositionX_g, -startPositionY_g+sixth, startPositionZ_g);

    CalcStraightPath(frontLeftLeg, 3*res, 0, -startPositionX_g, startPositionY_g+half, startPositionZ_g, -startPositionX_g, startPositionY_g-half, startPositionZ_g);
    CalcCurvedPath(frontLeftLeg, res, 3*res, -startPositionX_g, startPositionY_g-half, startPositionZ_g, -startPositionX_g, startPositionY_g+half, startPositionZ_g);

}


// gör rörelsemönstret för travet, beror på vilken direction som är satt på currentDirection
void MakeTrotGait(int cycleResolution)
{
    // cycleResolution är antaletpunkter på kurvan som benen följer. Måste vara jämnt tal!
    int res = cycleResolution/2;
    maxGaitCyclePos_g = cycleResolution - 1;
    
    calcRegulation();
    int translationRight = regulation_g[0];
    int CWRegulation = regulation_g[1];
    int CCWRegulation = regulation_g[2];
        
    int totalStepLengthCW = stepLength_g + CWRegulation;
    int totalStepLengthCCW = stepLength_g + CCWRegulation;

    directionHasChanged = 0;
    switch(currentDirection)
    {       
    // rörelsemönstret finns på ett papper (frontleft och rearright börjar alltid med curved)
        case north:
        {   
            CalcCurvedPath(frontLeftLeg,res,0,-startPositionX_g-translationRight/2,startPositionY_g-totalStepLengthCW/2,startPositionZ_g,-startPositionX_g+translationRight/2,startPositionY_g+totalStepLengthCW/2,startPositionZ_g);
            CalcStraightPath(frontLeftLeg,res,res,-startPositionX_g+translationRight/2,startPositionY_g+totalStepLengthCW/2,startPositionZ_g,-startPositionX_g-translationRight/2,startPositionY_g-totalStepLengthCW/2,startPositionZ_g);
  
            CalcCurvedPath(rearRightLeg,res,0,startPositionX_g-translationRight/2,-startPositionY_g-totalStepLengthCCW/2,startPositionZ_g,startPositionX_g+translationRight/2,-startPositionY_g+totalStepLengthCCW/2,startPositionZ_g);
            CalcStraightPath(rearRightLeg,res,res,startPositionX_g+translationRight/2,-startPositionY_g+totalStepLengthCCW/2,startPositionZ_g,startPositionX_g-translationRight/2,-startPositionY_g-totalStepLengthCCW/2,startPositionZ_g);
    
            CalcStraightPath(rearLeftLeg,res,0,-startPositionX_g+translationRight/2,-startPositionY_g+totalStepLengthCW/2,startPositionZ_g,-startPositionX_g-translationRight/2,-startPositionY_g-totalStepLengthCW/2,startPositionZ_g);
            CalcCurvedPath(rearLeftLeg,res,res,-startPositionX_g-translationRight/2,-startPositionY_g-totalStepLengthCW/2,startPositionZ_g,-startPositionX_g+translationRight/2,-startPositionY_g+totalStepLengthCW/2,startPositionZ_g);
    
            CalcStraightPath(frontRightLeg,res,0,startPositionX_g+translationRight/2,startPositionY_g+totalStepLengthCCW/2,startPositionZ_g,startPositionX_g-translationRight/2,startPositionY_g-totalStepLengthCCW/2,startPositionZ_g);
            CalcCurvedPath(frontRightLeg,res,res,startPositionX_g-translationRight/2,startPositionY_g-totalStepLengthCCW/2,startPositionZ_g,startPositionX_g+translationRight/2,startPositionY_g+totalStepLengthCCW/2,startPositionZ_g);
            break;
        }
    
        case east:
        {
            CalcCurvedPath(frontLeftLeg,res,0,-startPositionX_g-totalStepLengthCW/2,startPositionY_g+translationRight/2,startPositionZ_g,-startPositionX_g+totalStepLengthCW/2,startPositionY_g-translationRight/2,startPositionZ_g);
            CalcStraightPath(frontLeftLeg,res,res,-startPositionX_g+totalStepLengthCW/2,startPositionY_g-translationRight/2,startPositionZ_g,-startPositionX_g-totalStepLengthCW/2,startPositionY_g+translationRight/2,startPositionZ_g);
        
            CalcCurvedPath(rearRightLeg,res,0,startPositionX_g-totalStepLengthCCW/2,-startPositionY_g+translationRight/2,startPositionZ_g,startPositionX_g+totalStepLengthCCW/2,-startPositionY_g-translationRight/2,startPositionZ_g);
            CalcStraightPath(rearRightLeg,res,res,startPositionX_g+totalStepLengthCCW/2,-startPositionY_g-translationRight/2,startPositionZ_g,startPositionX_g-totalStepLengthCCW/2,-startPositionY_g+translationRight/2,startPositionZ_g);
        
            CalcStraightPath(rearLeftLeg,res,0,-startPositionX_g+totalStepLengthCCW/2,-startPositionY_g-translationRight/2,startPositionZ_g,-startPositionX_g-totalStepLengthCCW/2,-startPositionY_g+translationRight/2,startPositionZ_g);
            CalcCurvedPath(rearLeftLeg,res,res,-startPositionX_g-totalStepLengthCCW/2,-startPositionY_g+translationRight/2,startPositionZ_g,-startPositionX_g+totalStepLengthCCW/2,-startPositionY_g-translationRight/2,startPositionZ_g);
        
            CalcStraightPath(frontRightLeg,res,0,startPositionX_g+totalStepLengthCW/2,startPositionY_g-translationRight/2,startPositionZ_g,startPositionX_g-totalStepLengthCW/2,startPositionY_g+translationRight/2,startPositionZ_g);
            CalcCurvedPath(frontRightLeg,res,res,startPositionX_g-totalStepLengthCW/2,startPositionY_g+translationRight/2,startPositionZ_g,startPositionX_g+totalStepLengthCW/2,startPositionY_g-translationRight/2,startPositionZ_g);
            break;
        }
    
        case south:
        {
            CalcCurvedPath(frontLeftLeg,res,0,-startPositionX_g+translationRight/2,startPositionY_g+totalStepLengthCCW/2,startPositionZ_g,-startPositionX_g-translationRight/2,startPositionY_g-totalStepLengthCCW/2,startPositionZ_g);
            CalcStraightPath(frontLeftLeg,res,res,-startPositionX_g-translationRight/2,startPositionY_g-totalStepLengthCCW/2,startPositionZ_g,-startPositionX_g+translationRight/2,startPositionY_g+totalStepLengthCCW/2,startPositionZ_g);
        
            CalcCurvedPath(rearRightLeg,res,0,startPositionX_g+translationRight/2,-startPositionY_g+totalStepLengthCW/2,startPositionZ_g,startPositionX_g-translationRight/2,-startPositionY_g-totalStepLengthCW/2,startPositionZ_g);
            CalcStraightPath(rearRightLeg,res,res,startPositionX_g-translationRight/2,-startPositionY_g-totalStepLengthCW/2,startPositionZ_g,startPositionX_g+translationRight/2,-startPositionY_g+totalStepLengthCW/2,startPositionZ_g);
        
            CalcStraightPath(rearLeftLeg,res,0,-startPositionX_g-translationRight/2,-startPositionY_g-totalStepLengthCCW/2,startPositionZ_g,-startPositionX_g+translationRight/2,-startPositionY_g+totalStepLengthCCW/2,startPositionZ_g);
            CalcCurvedPath(rearLeftLeg,res,res,-startPositionX_g+translationRight/2,-startPositionY_g+totalStepLengthCCW/2,startPositionZ_g,-startPositionX_g-translationRight/2,-startPositionY_g-totalStepLengthCCW/2,startPositionZ_g);
        
            CalcStraightPath(frontRightLeg,res,0,startPositionX_g-translationRight/2,startPositionY_g-totalStepLengthCW/2,startPositionZ_g,startPositionX_g+translationRight/2,startPositionY_g+totalStepLengthCW/2,startPositionZ_g);
            CalcCurvedPath(frontRightLeg,res,res,startPositionX_g+translationRight/2,startPositionY_g+totalStepLengthCW/2,startPositionZ_g,startPositionX_g-translationRight/2,startPositionY_g-totalStepLengthCW/2,startPositionZ_g);
            break;
        }
    
        case west:
        {
            CalcCurvedPath(frontLeftLeg,res,0,-startPositionX_g+totalStepLengthCCW/2,startPositionY_g-translationRight/2,startPositionZ_g,-startPositionX_g-totalStepLengthCCW/2,startPositionY_g+translationRight/2,startPositionZ_g);
            CalcStraightPath(frontLeftLeg,res,res,-startPositionX_g-totalStepLengthCCW/2,startPositionY_g+translationRight/2,startPositionZ_g,-startPositionX_g+totalStepLengthCCW/2,startPositionY_g-translationRight/2,startPositionZ_g);
        
            CalcCurvedPath(rearRightLeg,res,0,startPositionX_g+totalStepLengthCW/2,-startPositionY_g-translationRight/2,startPositionZ_g,startPositionX_g-totalStepLengthCW/2,-startPositionY_g+translationRight/2,startPositionZ_g);
            CalcStraightPath(rearRightLeg,res,res,startPositionX_g-totalStepLengthCW/2,-startPositionY_g+translationRight/2,startPositionZ_g,startPositionX_g+totalStepLengthCW/2,-startPositionY_g-translationRight/2,startPositionZ_g);
        
            CalcStraightPath(rearLeftLeg,res,0,-startPositionX_g-totalStepLengthCW/2,-startPositionY_g+translationRight/2,startPositionZ_g,-startPositionX_g+totalStepLengthCW/2,-startPositionY_g-translationRight/2,startPositionZ_g);
            CalcCurvedPath(rearLeftLeg,res,res,-startPositionX_g+totalStepLengthCW/2,-startPositionY_g-translationRight/2,startPositionZ_g,-startPositionX_g-totalStepLengthCW/2,-startPositionY_g+translationRight/2,startPositionZ_g);
        
            CalcStraightPath(frontRightLeg,res,0,startPositionX_g-totalStepLengthCCW/2,startPositionY_g+translationRight/2,startPositionZ_g,startPositionX_g+totalStepLengthCCW/2,startPositionY_g-translationRight/2,startPositionZ_g);
            CalcCurvedPath(frontRightLeg,res,res,startPositionX_g+totalStepLengthCCW/2,startPositionY_g-translationRight/2,startPositionZ_g,startPositionX_g-totalStepLengthCCW/2,startPositionY_g+translationRight/2,startPositionZ_g);
            break;
        }
    }
}


int main(void)
{
    initUSART();
    spiMasterInit();
    EICRA = 0b1111; // Stigande flank på INT1/0 genererar avbrott
    EIMSK = (EIMSK | 3); // Möjliggör externa avbrott på INT1/0
    //PORTA = 0xff;
    // MCUCR = (MCUCR | (1 << PUD)); Något som testades för att se om det gjorde något
    //PORTA |= (1 << PORTA0);
     // Möjliggör globala avbrott
    sei();
    currentPos_g = 0;
    nextPos_g = 1;
   
    timer0Init();

    MakeTrotGait(gaitResolution_g);
    MoveToStartPosition();
    //moveToCreepStartPosition();
    //makeCreepGait(gaitResolution_g);
    SetGaitResolutionPeriod(gaitResolutionTime_g);

    
    int posToCalcGait = (gaitResolution_g/4 - 1);

    
    // ---- Main-Loop ----
    while (1)
    { 
        
        if (totOverflow_g >= timerOverflowMax_g)
        {
            if (TCNT0 >= timerRemainingTicks_g)
            {
                move();
                TCNT0 = 0;          // Återställ räknaren
                totOverflow_g = 0;
            }
        }
        if ((currentPos_g == posToCalcGait) & directionHasChanged)
        {
            switch(currentInstruction) // Väljer riktning beroende på vad användaren matat in
            {
                case 1:
                {
                    currentDirection = north;
                    transmitDataToCommUnit(DISTANCE_NORTH, 111);
                    transmitDataToCommUnit(DISTANCE_EAST, 100);
                    transmitDataToCommUnit(DISTANCE_SOUTH, 100);
                    transmitDataToCommUnit(DISTANCE_WEST, 100);
                    directionHasChanged = 0;
                    break;
                }
                case 2:
                {
                    currentDirection = east;
                    transmitDataToCommUnit(DISTANCE_NORTH, 100);
                    transmitDataToCommUnit(DISTANCE_EAST, 111);
                    transmitDataToCommUnit(DISTANCE_SOUTH, 100);
                    transmitDataToCommUnit(DISTANCE_WEST, 100);
                    directionHasChanged = 0;
                    break;
                }
                case 3:
                {
                    currentDirection = south;
                    transmitDataToCommUnit(DISTANCE_NORTH, 100);
                    transmitDataToCommUnit(DISTANCE_EAST, 100);
                    transmitDataToCommUnit(DISTANCE_SOUTH, 111);
                    transmitDataToCommUnit(DISTANCE_WEST, 100);
    
                    directionHasChanged = 0;
                    break;
                }
                case 4:
                {
                    currentDirection = west;
                    transmitDataToCommUnit(DISTANCE_NORTH, 100);
                    transmitDataToCommUnit(DISTANCE_EAST, 100);
                    transmitDataToCommUnit(DISTANCE_SOUTH, 100);
                    transmitDataToCommUnit(DISTANCE_WEST, 111);
                    directionHasChanged = 0;
                    break;
                }
                
            }                
            MakeTrotGait(gaitResolution_g);
        }
    }
}



 