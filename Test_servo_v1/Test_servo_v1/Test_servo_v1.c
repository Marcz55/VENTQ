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


 #define NORTH 1
 #define NORTH_EAST 5
 #define EAST 4
 #define SOUTH_EAST 6
 #define SOUTH 2
 #define SOUTH_WEST 10
 #define WEST 8
 #define NORTH_WEST 9
 #define NO_MOVEMENT_DIRECTION 0

 #define CW_ROTATION 1
 #define CCW_ROTATION 2
 #define NO_ROTATION 0


int stepLength_g = 70;
int startPositionX_g = 100;
int startPositionY_g = 100;
int startPositionZ_g = -120;
int stepHeight_g =  40;
int gaitResolution_g = 12; // M�STE VARA DELBART MED 4 vid trot, 8 vid creep
int stepLengthRotationAdjust = 30;
int gaitResolutionTime_g = INCREMENT_PERIOD_200;
int currentDirectionInstruction = 0; // Nuvarande manuell styrinstruktion
int currentRotationInstruction = 0;

/*
// Joakims coola g�ngstil,
int stepLength_g = 40;
int startPositionX_g = 20;
int startPositionY_g = 140;
int startPositionZ_g = -110;
int stepHeight_g = 20;
int gaitResolution_g = 12;
int gaitResolutionTime_g = INCREMENT_PERIOD_60;
*/
enum controlMode{
    manuell,
    autonomous
};
enum controlMode currentControlMode = manuell;

enum direction{
    none,
    north,
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





volatile uint8_t totOverflow_g;

// klockfrekvensen �r 16MHz. 
void timer0Init()
{
    // prescaler 256
    TCCR0B |= (1 << CS02);
    
    // initiera counter
    TCNT0 = 0;
    
    // till�t timer0 overflow interrupt
    
    TIMSK0 |= (1 << TOIE0);
    
    sei();
    
    totOverflow_g = 0;

}

// timerPeriod ska v�ljas fr�n f�rdefinierade tider 
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
            // H�r kan man ha n�gon typ av felhantering om man vill

    }
    return;
}

// calcDynamixelSpeed anv�nder legIncrementPeriod_g och f�rflyttningsstr�ckan f�r att ber�kna en 
// hastighet som g�r att slutpositionen uppn�s p� periodtiden.
long int calcDynamixelSpeed(long int deltaAngle)
{
    long int calculatedSpeed = (1000 * deltaAngle) / (6 * gaitResolutionTime_g);
    if (calculatedSpeed < 2)
    {
        return 2;
    }
    else
    {
        return calculatedSpeed;
    }
}

ISR(TIMER0_OVF_vect)
{
    // r�kna antalet avbrott som skett
    totOverflow_g++;
}



void MoveDynamixel(int ID,long int Angle,long int RevolutionsPerMinute)
{
    if ((Angle <= 300) & (Angle >= 0)) // Till�tna grader �r 0-300
    {
        long int LowGoalPosition = ((Angle*1023)/300) & 0x00FF; // G�r om graden till ett tal mellan 0-1023 och delar upp det i LSB(byte) och MSB(byte)
        long int HighGoalPosition = ((Angle*1023)/300) & 0xFF00;
        HighGoalPosition = (HighGoalPosition >> 8);
    
        long int LowAngleVelocity = 0;
        long int HighAngleVelocity = 0;
    
        if (RevolutionsPerMinute >= 114) // Om RPM �ver 114 s� r�r vi oss med snabbaste m�jliga hastigheten med sp�nningen som tillhandah�lls
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

// Rader �r vinklar p� servon. Kolumnerna inneh�ller positioner. Allokerar en extra rad minne 
// h�r i matriserna bara f�r att f� snyggare kod. 
long int actuatorPositions_g [13][20];
long int legPositions_g [13][20];
int regulation_g[3];

int currentPos_g = 0;
int nextPos_g = 1;
int maxGaitCyclePos_g = 1;

// Den h�r funktionen hanterar vilken position i g�ngcykeln roboten �r i. Den g�r runt baserat p� 
// antalet positioner som finns i den givna g�ngstilen. 
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
        
        // l�s inverskinematik f�r lederna.
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
        
        // l�s inverskinematik f�r lederna.
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
    float deltaZBegin = (stepHeight_g + z2 - z1) / (numberOfPositions / 2); // f�rsta halvan av str�ckan ska benet r�ra sig mot en position 4cm �ver slutpositionen
    float deltaZEnd = (z2 - z1 - stepHeight_g) / (numberOfPositions / 2); // andra halvan av str�ckan ska benet r�ra sig mot en position 4cm under slutpositionen -> benet f�r en triangelbana
    float x = x1;
    float y = y1;
    float z = z1;
    
    
    // f�rsta halvan av r�relsen
    for (int i = startIndex; i <= topPosition; i++)
    {
        x = x + deltaX;
        y = y + deltaY;
        z = z + deltaZBegin;
        
        // l�s inverskinematik f�r lederna.
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
        
        // l�s inverskinematik f�r lederna.
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

ISR(INT0_vect) // avbrott fr�n kommunikationsenheten
{
    
    PORTA = (0<<PORTA2);
    spiTransmit(0x44); // Skicka iv�g skr�p f�r att kunna ta emot det som finns i kommunikationsenhetens SPDR
    PORTA = (1<<PORTA2);
    int instruction = inbuffer;
    currentDirectionInstruction = (instruction & 0b00001111);
    currentRotationInstruction = (instruction & 0b11110000) >> 4;
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



void getSensorData()
{
    /*
    * Sensordata fyller de globala arrayerna distanceValue_g, angleValue_g och sensorValue_g
    * 
    *
    *
    */
}

// arrays som v�rden h�mtade ifr�n sensorenheten skall ligga i
int distanceValue_g[4]; // inneh�ller avst�nden fr�n de olika sidorna till v�ggarna
int angleValue_g[4]; // inneh�ller vinkeln relativt de olika v�ggarna
int sensorValue_g[8]; // inneh�ller avst�nden fr�n varje separat sensor till v�ggarna
int pathWidth_g = 570; // Avst�ndet mellan v�ggar
int regulation_g[3]; // array som regleringen sparas i
void calcRegulation()
{
    
    //static int  regulation[3]; // skapar en array som inneh�ller hur roboten ska reglera
    /*
    * 
    *
    * F�rsta v�rdet anger hur mycket �t h�ger relativt r�relseriktningen roboten ska ta sig i varje steg.
    * Andra v�rdet anger hur mycket l�ngre steg benen p� v�nstra sidan relativt r�relseriktningen ska ta, de som medf�r en CW-rotation. 
    * Tredje v�rdet anger hur mycket l�ngre steg benen p� h�gra sidan relativt r�relseriktningen ska ta, de som medf�r en CCW-rotation
    */

    int translationRight = 0;
    int sensorOffset = 370;// Avst�nd ifr�n sensorera till mitten av roboten (mm)

    // variablerna vi baserar regleringen p�, skillnaden mellan aktuellt v�rde och �nskat v�rde
    int translationRegulationError = 0; // avser hur l�ngt till v�nster ifr�n mittpunkten av "v�gen" vi �r
    int angleRegulationError = 0; // avser hur m�nga grader vridna i CCW riktning relativt "den raka riktningen" dvs relativt v�ggarna.

    //Regleringskoefficienter
    int kProportionalTranslation = 1;
    int kProportionalAngle = 1;

    //Reglering vid g�ng i kooridor
    switch (currentDirection)
    {
        case north:
        {
            translationRegulationError = (distanceValue_g[east] + sensorOffset) - pathWidth_g;
            angleRegulationError = angleValue_g[east]; 
            break;
        }

        case east:
        {
            translationRegulationError = (distanceValue_g[south] + sensorOffset) - pathWidth_g;
            angleRegulationError = angleValue_g[south];
            break;
        }

        case south:
        {
            translationRegulationError = (distanceValue_g[west] + sensorOffset) - pathWidth_g;
            angleRegulationError = angleValue_g[west];
            break;
        }

        case west:
        {
            translationRegulationError = (distanceValue_g[north] + sensorOffset) - pathWidth_g;
            angleRegulationError = angleValue_g[north];
            break;
        }
    }

    translationRight = kProportionalTranslation * translationRegulationError;
    int leftSideStepLengthAdjust = kProportionalAngle/2 * angleRegulationError; // om roboten ska rotera �t h�ger s� l�ter vi benen p� v�nster sida ta l�ngre steg och benen p� h�ger sida ta kortare steg
    int rightSideStepLengthAdjust = kProportionalAngle/2 * (-angleRegulationError);
    regulation_g[0] =  translationRight;
    regulation_g[1] = leftSideStepLengthAdjust;
    regulation_g[2] = rightSideStepLengthAdjust;
   // return;

// vid nul�get har vi inte sensorenheten inkopplad s� v�rdena i distancevalue_g m.m �r odefinierade
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
    // flytta ner alla benen till markniv�
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
// cycleResolution m�ste vara delbart med 8
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


// g�r r�relsem�nstret f�r travet, beror p� vilken direction som �r satt p� currentDirection
void MakeTrotGait(int cycleResolution)
{
    // cycleResolution �r antaletpunkter p� kurvan som benen f�ljer. M�ste vara j�mnt tal!
    int res = cycleResolution/2;
    maxGaitCyclePos_g = cycleResolution - 1;
    
    if(currentControlMode == autonomous)
    {
        calcRegulation();    
    }
    
    // om ej autonomt l�ge kommer den manuella styrningen i main-loopen att anv�nda reglerparametrarna till diagonal g�ng och rotation.
    int translationRight = regulation_g[0];
    int leftSideStepLengthAdjust = regulation_g[1];
    int rightSideStepLengthAdjust = regulation_g[2];
        
    int leftSideTotalStepLength = stepLength_g + leftSideStepLengthAdjust; // left h�nvisar till den v�nstra sidan relativt r�relseriktningen
    int rightSideTotalStepLength = stepLength_g + rightSideStepLengthAdjust; // right h�nvisar till den h�gra sidan relativt r�relseriktningen


    int newZ = startPositionZ_g + 4*stepHeight_g/gaitResolution_g;
    directionHasChanged = 0;
    switch(currentDirection)
    {       
    // r�relsem�nstret finns p� ett papper (frontleft och rearright b�rjar alltid med curved)
    
    // anm�rk att left/right-SideTotalStepLength menar den totala stegl�ngden p� den v�nstra/h�gra sidan relativt r�relseriktningen 
    // medans frontLeftLeg och de andra benen alltid har samma namn och �r namngivna utifr�n r�relseriktningen norr.   
        case north:
        {   
            
            /// HAR �NDRATS
            CalcStraightPath(frontLeftLeg,1, 0,-startPositionX_g-translationRight/2,startPositionY_g-leftSideTotalStepLength/2,startPositionZ_g,-startPositionX_g-translationRight/2,startPositionY_g-leftSideTotalStepLength/2, newZ);
            CalcCurvedPath(frontLeftLeg,res-2,1,-startPositionX_g-translationRight/2,startPositionY_g-leftSideTotalStepLength/2,newZ,-startPositionX_g+translationRight/2,startPositionY_g+leftSideTotalStepLength/2,newZ);
            CalcStraightPath(frontLeftLeg,1, res-1, -startPositionX_g+translationRight/2,startPositionY_g+leftSideTotalStepLength/2,newZ, -startPositionX_g+translationRight/2,startPositionY_g+leftSideTotalStepLength/2,startPositionZ_g);
           
            CalcStraightPath(frontLeftLeg,res,res,-startPositionX_g+translationRight/2,startPositionY_g+leftSideTotalStepLength/2,startPositionZ_g,-startPositionX_g-translationRight/2,startPositionY_g-leftSideTotalStepLength/2,startPositionZ_g);
  ////////////////////////
            CalcStraightPath(rearRightLeg,1,0, startPositionX_g-translationRight/2,-startPositionY_g-rightSideTotalStepLength/2,startPositionZ_g, startPositionX_g-translationRight/2,-startPositionY_g-rightSideTotalStepLength/2, newZ);
            CalcCurvedPath(rearRightLeg,res-2,1,startPositionX_g-translationRight/2,-startPositionY_g-rightSideTotalStepLength/2,newZ,startPositionX_g+translationRight/2,-startPositionY_g+rightSideTotalStepLength/2,newZ);
            CalcStraightPath(rearRightLeg,1,res-1, startPositionX_g+translationRight/2,-startPositionY_g+rightSideTotalStepLength/2,newZ,startPositionX_g+translationRight/2,-startPositionY_g+rightSideTotalStepLength/2,startPositionZ_g);
            
            CalcStraightPath(rearRightLeg,res,res,startPositionX_g+translationRight/2,-startPositionY_g+rightSideTotalStepLength/2,startPositionZ_g,startPositionX_g-translationRight/2,-startPositionY_g-rightSideTotalStepLength/2,startPositionZ_g);
    //////////////////////////////
            CalcStraightPath(rearLeftLeg,res,0,-startPositionX_g+translationRight/2,-startPositionY_g+leftSideTotalStepLength/2,startPositionZ_g,-startPositionX_g-translationRight/2,-startPositionY_g-leftSideTotalStepLength/2,startPositionZ_g);
            
            CalcStraightPath(rearLeftLeg,1, res,-startPositionX_g-translationRight/2,-startPositionY_g-leftSideTotalStepLength/2,startPositionZ_g, -startPositionX_g-translationRight/2,-startPositionY_g-leftSideTotalStepLength/2,newZ);
            CalcCurvedPath(rearLeftLeg,res-2,res+1,-startPositionX_g-translationRight/2,-startPositionY_g-leftSideTotalStepLength/2,newZ,-startPositionX_g+translationRight/2,-startPositionY_g+leftSideTotalStepLength/2,newZ);
            CalcStraightPath(rearLeftLeg,1,2*res-1,-startPositionX_g+translationRight/2,-startPositionY_g+leftSideTotalStepLength/2,newZ, -startPositionX_g+translationRight/2,-startPositionY_g+leftSideTotalStepLength/2,startPositionZ_g);
    ///////////////////////////
            CalcStraightPath(frontRightLeg,res,0,startPositionX_g+translationRight/2,startPositionY_g+rightSideTotalStepLength/2,startPositionZ_g,startPositionX_g-translationRight/2,startPositionY_g-rightSideTotalStepLength/2,startPositionZ_g);
            
            CalcStraightPath(frontRightLeg,1,res,startPositionX_g-translationRight/2,startPositionY_g-rightSideTotalStepLength/2,startPositionZ_g, startPositionX_g-translationRight/2,startPositionY_g-rightSideTotalStepLength/2,newZ);
            CalcCurvedPath(frontRightLeg,res-2,res+1,startPositionX_g-translationRight/2,startPositionY_g-rightSideTotalStepLength/2,newZ,startPositionX_g+translationRight/2,startPositionY_g+rightSideTotalStepLength/2,newZ);
            CalcStraightPath(frontRightLeg,1,2*res-1, startPositionX_g+translationRight/2,startPositionY_g+rightSideTotalStepLength/2,newZ, startPositionX_g+translationRight/2,startPositionY_g+rightSideTotalStepLength/2,startPositionZ_g);
            break;
        }
    
        case east:
        {
            CalcCurvedPath(frontLeftLeg,res,0,-startPositionX_g-leftSideTotalStepLength/2,startPositionY_g+translationRight/2,startPositionZ_g,-startPositionX_g+leftSideTotalStepLength/2,startPositionY_g-translationRight/2,startPositionZ_g);
            CalcStraightPath(frontLeftLeg,res,res,-startPositionX_g+leftSideTotalStepLength/2,startPositionY_g-translationRight/2,startPositionZ_g,-startPositionX_g-leftSideTotalStepLength/2,startPositionY_g+translationRight/2,startPositionZ_g);
        
            CalcCurvedPath(rearRightLeg,res,0,startPositionX_g-rightSideTotalStepLength/2,-startPositionY_g+translationRight/2,startPositionZ_g,startPositionX_g+rightSideTotalStepLength/2,-startPositionY_g-translationRight/2,startPositionZ_g);
            CalcStraightPath(rearRightLeg,res,res,startPositionX_g+rightSideTotalStepLength/2,-startPositionY_g-translationRight/2,startPositionZ_g,startPositionX_g-rightSideTotalStepLength/2,-startPositionY_g+translationRight/2,startPositionZ_g);
        
            CalcStraightPath(rearLeftLeg,res,0,-startPositionX_g+rightSideTotalStepLength/2,-startPositionY_g-translationRight/2,startPositionZ_g,-startPositionX_g-rightSideTotalStepLength/2,-startPositionY_g+translationRight/2,startPositionZ_g);
            CalcCurvedPath(rearLeftLeg,res,res,-startPositionX_g-rightSideTotalStepLength/2,-startPositionY_g+translationRight/2,startPositionZ_g,-startPositionX_g+rightSideTotalStepLength/2,-startPositionY_g-translationRight/2,startPositionZ_g);
        
            CalcStraightPath(frontRightLeg,res,0,startPositionX_g+leftSideTotalStepLength/2,startPositionY_g-translationRight/2,startPositionZ_g,startPositionX_g-leftSideTotalStepLength/2,startPositionY_g+translationRight/2,startPositionZ_g);
            CalcCurvedPath(frontRightLeg,res,res,startPositionX_g-leftSideTotalStepLength/2,startPositionY_g+translationRight/2,startPositionZ_g,startPositionX_g+leftSideTotalStepLength/2,startPositionY_g-translationRight/2,startPositionZ_g);
            break;
        }
    
        case south:
        {
            CalcCurvedPath(frontLeftLeg,res,0,-startPositionX_g+translationRight/2,startPositionY_g+rightSideTotalStepLength/2,startPositionZ_g,-startPositionX_g-translationRight/2,startPositionY_g-rightSideTotalStepLength/2,startPositionZ_g);
            CalcStraightPath(frontLeftLeg,res,res,-startPositionX_g-translationRight/2,startPositionY_g-rightSideTotalStepLength/2,startPositionZ_g,-startPositionX_g+translationRight/2,startPositionY_g+rightSideTotalStepLength/2,startPositionZ_g);
        
            CalcCurvedPath(rearRightLeg,res,0,startPositionX_g+translationRight/2,-startPositionY_g+leftSideTotalStepLength/2,startPositionZ_g,startPositionX_g-translationRight/2,-startPositionY_g-leftSideTotalStepLength/2,startPositionZ_g);
            CalcStraightPath(rearRightLeg,res,res,startPositionX_g-translationRight/2,-startPositionY_g-leftSideTotalStepLength/2,startPositionZ_g,startPositionX_g+translationRight/2,-startPositionY_g+leftSideTotalStepLength/2,startPositionZ_g);
        
            CalcStraightPath(rearLeftLeg,res,0,-startPositionX_g-translationRight/2,-startPositionY_g-rightSideTotalStepLength/2,startPositionZ_g,-startPositionX_g+translationRight/2,-startPositionY_g+rightSideTotalStepLength/2,startPositionZ_g);
            CalcCurvedPath(rearLeftLeg,res,res,-startPositionX_g+translationRight/2,-startPositionY_g+rightSideTotalStepLength/2,startPositionZ_g,-startPositionX_g-translationRight/2,-startPositionY_g-rightSideTotalStepLength/2,startPositionZ_g);
        
            CalcStraightPath(frontRightLeg,res,0,startPositionX_g-translationRight/2,startPositionY_g-leftSideTotalStepLength/2,startPositionZ_g,startPositionX_g+translationRight/2,startPositionY_g+leftSideTotalStepLength/2,startPositionZ_g);
            CalcCurvedPath(frontRightLeg,res,res,startPositionX_g+translationRight/2,startPositionY_g+leftSideTotalStepLength/2,startPositionZ_g,startPositionX_g-translationRight/2,startPositionY_g-leftSideTotalStepLength/2,startPositionZ_g);
            break;
        }
    
        case west:
        {
            CalcCurvedPath(frontLeftLeg,res,0,-startPositionX_g+rightSideTotalStepLength/2,startPositionY_g-translationRight/2,startPositionZ_g,-startPositionX_g-rightSideTotalStepLength/2,startPositionY_g+translationRight/2,startPositionZ_g);
            CalcStraightPath(frontLeftLeg,res,res,-startPositionX_g-rightSideTotalStepLength/2,startPositionY_g+translationRight/2,startPositionZ_g,-startPositionX_g+rightSideTotalStepLength/2,startPositionY_g-translationRight/2,startPositionZ_g);
        
            CalcCurvedPath(rearRightLeg,res,0,startPositionX_g+leftSideTotalStepLength/2,-startPositionY_g-translationRight/2,startPositionZ_g,startPositionX_g-leftSideTotalStepLength/2,-startPositionY_g+translationRight/2,startPositionZ_g);
            CalcStraightPath(rearRightLeg,res,res,startPositionX_g-leftSideTotalStepLength/2,-startPositionY_g+translationRight/2,startPositionZ_g,startPositionX_g+leftSideTotalStepLength/2,-startPositionY_g-translationRight/2,startPositionZ_g);
        
            CalcStraightPath(rearLeftLeg,res,0,-startPositionX_g-leftSideTotalStepLength/2,-startPositionY_g+translationRight/2,startPositionZ_g,-startPositionX_g+leftSideTotalStepLength/2,-startPositionY_g-translationRight/2,startPositionZ_g);
            CalcCurvedPath(rearLeftLeg,res,res,-startPositionX_g+leftSideTotalStepLength/2,-startPositionY_g-translationRight/2,startPositionZ_g,-startPositionX_g-leftSideTotalStepLength/2,-startPositionY_g+translationRight/2,startPositionZ_g);
        
            CalcStraightPath(frontRightLeg,res,0,startPositionX_g-rightSideTotalStepLength/2,startPositionY_g+translationRight/2,startPositionZ_g,startPositionX_g+rightSideTotalStepLength/2,startPositionY_g-translationRight/2,startPositionZ_g);
            CalcCurvedPath(frontRightLeg,res,res,startPositionX_g+rightSideTotalStepLength/2,startPositionY_g-translationRight/2,startPositionZ_g,startPositionX_g-rightSideTotalStepLength/2,startPositionY_g+translationRight/2,startPositionZ_g);
            break;
        }

        case none:
        {
            CalcCurvedPath(frontLeftLeg,res,0,-startPositionX_g,startPositionY_g-leftSideStepLengthAdjust/2,startPositionZ_g,-startPositionX_g,startPositionY_g+leftSideStepLengthAdjust/2,startPositionZ_g);
            CalcStraightPath(frontLeftLeg,res,res,-startPositionX_g,startPositionY_g+leftSideStepLengthAdjust/2,startPositionZ_g,-startPositionX_g,startPositionY_g-leftSideStepLengthAdjust/2,startPositionZ_g);
        
            CalcCurvedPath(rearRightLeg,res,0,startPositionX_g,-startPositionY_g-rightSideStepLengthAdjust/2,startPositionZ_g,startPositionX_g,-startPositionY_g+rightSideStepLengthAdjust/2,startPositionZ_g);
            CalcStraightPath(rearRightLeg,res,res,startPositionX_g,-startPositionY_g+rightSideStepLengthAdjust/2,startPositionZ_g,startPositionX_g,-startPositionY_g-rightSideStepLengthAdjust/2,startPositionZ_g);
        
            CalcStraightPath(rearLeftLeg,res,0,-startPositionX_g,-startPositionY_g+leftSideStepLengthAdjust/2,startPositionZ_g,-startPositionX_g,-startPositionY_g-leftSideStepLengthAdjust/2,startPositionZ_g);
            CalcCurvedPath(rearLeftLeg,res,res,-startPositionX_g,-startPositionY_g-leftSideStepLengthAdjust/2,startPositionZ_g,-startPositionX_g,-startPositionY_g+leftSideStepLengthAdjust/2,startPositionZ_g);
        
            CalcStraightPath(frontRightLeg,res,0,startPositionX_g,startPositionY_g+rightSideStepLengthAdjust/2,startPositionZ_g,startPositionX_g,startPositionY_g-rightSideStepLengthAdjust/2,startPositionZ_g);
            CalcCurvedPath(frontRightLeg,res,res,startPositionX_g,startPositionY_g-rightSideStepLengthAdjust/2,startPositionZ_g,startPositionX_g,startPositionY_g+rightSideStepLengthAdjust/2,startPositionZ_g);
            break;
        }
    }
}


int main(void)
{
    initUSART();
    spiMasterInit();
    EICRA = 0b1111; // Stigande flank p� INT1/0 genererar avbrott
    EIMSK = (EIMSK | 3); // M�jligg�r externa avbrott p� INT1/0
    //PORTA = 0xff;
    // MCUCR = (MCUCR | (1 << PUD)); N�got som testades f�r att se om det gjorde n�got
    //PORTA |= (1 << PORTA0);
     // M�jligg�r globala avbrott
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
                TCNT0 = 0;          // �terst�ll r�knaren
                totOverflow_g = 0;
            }
        }
        if ((currentPos_g == posToCalcGait) & directionHasChanged)
        {
            switch(currentRotationInstruction) // �ndrar rotation om den finns en instruktion f�r att rotera.
            {
                case CW_ROTATION:
                {
                    regulation_g[1] = stepLengthRotationAdjust;
                    regulation_g[2] = -stepLengthRotationAdjust;
                    directionHasChanged = 0;
                    break;
                }
                case CCW_ROTATION:
                {
                    regulation_g[1] = -stepLengthRotationAdjust;
                    regulation_g[2] = stepLengthRotationAdjust;
                    directionHasChanged = 0;
                    break;
                }
                case NO_ROTATION:
                {
                    regulation_g[1] = 0;
                    regulation_g[2] = 0;
                    directionHasChanged = 0;
                    break;
                }
            }
            switch(currentDirectionInstruction) // V�ljer riktning beroende p� vad anv�ndaren matat in
            {
                case NORTH:
                {
                    currentDirection = north;
                    regulation_g[0] = 0;
                    directionHasChanged = 0;
                    break;
                }
                case NORTH_EAST:
                {
                    // Huvudsaklig r�relseriktning norr. �stlig offset hanteres genom en h�rdkodad reglering �t h�ger
                    currentDirection = north;
                    regulation_g[0] = stepLength_g; 
                    directionHasChanged = 0;
                    break;
                }
                case EAST:
                {
                    currentDirection = east;
                    regulation_g[0] = 0;
                    directionHasChanged = 0;
                    break;
                }
                case SOUTH_EAST:
                {
                    // Huvudsaklig r�relseriktning �ster. Sydlig offset hanteres genom en h�rdkodad reglering �t h�ger
                    currentDirection = east;
                    regulation_g[0] = stepLength_g; 
                    directionHasChanged = 0;
                    break;
                }
                case SOUTH:
                {
                    // Huvudsaklig r�relseriktning �ster. Sydlig offset hanteres genom en h�rdkodad reglering �t h�ger
                    currentDirection = south;
                    regulation_g[0] = 0;
                    directionHasChanged = 0;
                    break;
                }
                case SOUTH_WEST:
                {
                    // Huvudsaklig r�relseriktning s�der. V�stlig offset hanteres genom en h�rdkodad reglering �t h�ger
                    currentDirection = south;
                    regulation_g[0] = stepLength_g;
                    directionHasChanged = 0;
                    break;
                }
                case WEST:
                {
                    currentDirection = west;
                    regulation_g[0] = 0;
                    directionHasChanged = 0;
                    break;
                }
                case NORTH_WEST:
                {
                    // Huvudsaklig r�relseriktning v�ster. Nordlig offset hanteres genom en h�rdkodad reglering �t h�ger
                    currentDirection = west;
                    regulation_g[0] = stepLength_g;
                    directionHasChanged = 0;
                    break;
                }
                case NO_MOVEMENT_DIRECTION:
                {
                    currentDirection = none;
                    regulation_g[0] = 0;
                    directionHasChanged = 0;
                    break;
                }
                
            }                
            MakeTrotGait(gaitResolution_g);
        }
    }
}



 
