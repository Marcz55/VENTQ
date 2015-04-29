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
#include "Timer.h"
#include "Definitions.h"
#include <stdlib.h>


/*
#define TXD0_READY bit_is_set(UCSR0A,5)
#define TXD0_FINISHED bit_is_set(UCSR0A,6)
#define RXD0_READY bit_is_set(UCSR0A,7)
#define TXD0_DATA (UDR0)
#define RXD0_DATA (UDR0)
*/



int stepLength_g = 160;
int startPositionX_g = 80;
int startPositionY_g = 80;
int startPositionZ_g = -120;
int stepHeight_g =  40;
int gaitResolution_g = 8; // MÅSTE VARA DELBART MED 4 vid trot, 8 vid creep
int stepLengthRotationAdjust = 30;
int newGaitResolutionTime = INCREMENT_PERIOD_200; // tid i timerloopen för benstyrningen i ms
int currentDirectionInstruction = 0; // Nuvarande manuell styrinstruktion
int currentRotationInstruction = 0;
int currentVentilatorOptionInstruction_g = 0; // nuvarande instruktion för inställningar av ventilators egenskaper
int ventilatorOptionsHasChanged_g = 0;
int posToCalcGait;
int needToCalcGait = 1;

// ------ Inställningar för robot-datorkommunikation ------
int newCommUnitUpdatePeriod = INCREMENT_PERIOD_500;



/*
// Joakims coola gångstil,
int stepLength_g = 40;
int startPositionX_g = 20;
int startPositionY_g = 140;
int startPositionZ_g = -110;
int stepHeight_g = 20;
int gaitResolution_g = 12;
int gaitResolutionTime_g = INCREMENT_PERIOD_60;
*/
enum controlMode{
    manual,
    autonomous
};
enum controlMode currentControlMode = manual;

enum direction{
    north,
    east,
    south,
    west,
    none
    }; 
enum direction currentDirection = none;

enum gait{
    standStill,
    trotGait,
    creepGait
    };

enum gait currentGait = standStill;
enum gait gaitToUse = trotGait; // ska skrivas funktionalitet för att kunna byta mellan creep och trot här.
int standardSpeed_g = 20;
int statusPackEnabled = 0;




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
        return calculatedSpeed;
    }
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
float legPositions_g [13][20];
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

typdef struct legCoordinates legCoordinates;
struct legCoordinates {
        long int xCoord, 
                yCoord, 
                zCoord;
    };
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
        
        // spara resultatet i global array. Korrigera så att koordinaterna som sparas kan användas för nya anrop.
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
                legPositions_g[FRONT_RIGHT_LEG_X][i] = -x;
                legPositions_g[FRONT_RIGHT_LEG_Y][i] = y;
                legPositions_g[FRONT_RIGHT_LEG_Z][i] = z;
                break;
            }
            case REAR_LEFT_LEG:
            {
                actuatorPositions_g[currentLeg.coxaJoint][i] = theta1 + 195;
                actuatorPositions_g[currentLeg.femurJoint][i] =  225 - theta2;
                actuatorPositions_g[currentLeg.tibiaJoint][i] =  300 - theta3;
                legPositions_g[REAR_LEFT_LEG_X][i] = -x;
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
        
        // spara resultatet i global array. Korrigera så att koordinaterna som sparas kan användas för nya anrop.
        switch(currentLeg.legNumber)
        {
            case FRONT_LEFT_LEG:
            {
                actuatorPositions_g[currentLeg.coxaJoint][i] = theta1 + 105;
                actuatorPositions_g[currentLeg.femurJoint][i] =  theta2 + 75;
                actuatorPositions_g[currentLeg.tibiaJoint][i] =  theta3 + 1;
                legPositions_g[FRONT_LEFT_LEG_X][i] = -x;
                legPositions_g[FRONT_LEFT_LEG_Y][i] = y;
                legPositions_g[FRONT_LEFT_LEG_Z][i] = z;
                break;
            }
            case FRONT_RIGHT_LEG:
            {
                actuatorPositions_g[currentLeg.coxaJoint][i] = theta1 + 193;
                actuatorPositions_g[currentLeg.femurJoint][i] =  theta2 + 75;
                actuatorPositions_g[currentLeg.tibiaJoint][i] =  theta3 + 3;
                legPositions_g[FRONT_RIGHT_LEG_X][i] = -x;
                legPositions_g[FRONT_RIGHT_LEG_Y][i] = y;
                legPositions_g[FRONT_RIGHT_LEG_Z][i] = z;
                break;
            }
            case REAR_LEFT_LEG:
            {
                actuatorPositions_g[currentLeg.coxaJoint][i] = theta1 + 195;
                actuatorPositions_g[currentLeg.femurJoint][i] =  225 - theta2;
                actuatorPositions_g[currentLeg.tibiaJoint][i] =  300 - theta3;
                legPositions_g[REAR_LEFT_LEG_X][i] = -x;
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
        
        // spara resultatet i global array. Korrigera så att koordinaterna som sparas kan användas för nya anrop.
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
                legPositions_g[FRONT_RIGHT_LEG_X][i] = -x;
                legPositions_g[FRONT_RIGHT_LEG_Y][i] = y;
                legPositions_g[FRONT_RIGHT_LEG_Z][i] = z;
                break;
            }
            case REAR_LEFT_LEG:
            {
                actuatorPositions_g[currentLeg.coxaJoint][i] = theta1 + 195;
                actuatorPositions_g[currentLeg.femurJoint][i] =  225 - theta2;
                actuatorPositions_g[currentLeg.tibiaJoint][i] =  300 - theta3;
                legPositions_g[REAR_LEFT_LEG_X][i] = -x;
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
               legPositions_g[FRONT_RIGHT_LEG_X][i] = -x;
               legPositions_g[FRONT_RIGHT_LEG_Y][i] = y;
               legPositions_g[FRONT_RIGHT_LEG_Z][i] = z;
               break;
           }
           case REAR_LEFT_LEG:
           {
               actuatorPositions_g[currentLeg.coxaJoint][i] = theta1 + 195;
               actuatorPositions_g[currentLeg.femurJoint][i] =  225 - theta2;
               actuatorPositions_g[currentLeg.tibiaJoint][i] =  300 - theta3;
               legPositions_g[REAR_LEFT_LEG_X][i] = -x;
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

int isMovementInstruction(int instruction)
{
	if ((0b10000000 & instruction) == 0b10000000)
	{
		return FALSE;
	}
	return TRUE;
}

ISR(INT0_vect) // avbrott från kommunikationsenheten
{
    
    spiTransmitToCommUnit(TRASH); // Skicka iväg skräp för att kunna ta emot det som finns i kommunikationsenhetens SPDR
    int instruction = inbuffer;
	if(isMovementInstruction(instruction))
	{
		currentDirectionInstruction = (instruction & 0b00001111);
		currentRotationInstruction = (instruction & 0b11110000) >> 4;
		directionHasChanged = 1;
	}
	else
	{
		ventilatorOptionsHasChanged_g = 1;
		currentVentilatorOptionInstruction_g = instruction;
	}
}



ISR(INT1_vect) 
{   
    transmitDataToCommUnit(DISTANCE_NORTH,fetchDataFromSensorUnit(DISTANCE_NORTH));
    transmitDataToCommUnit(DISTANCE_EAST, fetchDataFromSensorUnit(DISTANCE_EAST));
    transmitDataToCommUnit(DISTANCE_SOUTH, fetchDataFromSensorUnit(DISTANCE_SOUTH));
    transmitDataToCommUnit(DISTANCE_WEST, fetchDataFromSensorUnit(DISTANCE_WEST));
    
    
    directionHasChanged = 1;
    switch(currentDirection)
    {
        case north:
        {
           currentDirectionInstruction = EAST_HEADER;
           break;
        }
        case east:
        {
            currentDirectionInstruction = SOUTH_HEADER;
            break;
        }
        case south:
        {
            currentDirectionInstruction = WEST_HEADER;
            break;
        }
        case west:
        {
            currentDirectionInstruction = NORTH_HEADER;
            break;
        }
        case none:
        {
            currentDirectionInstruction = NORTH_HEADER;
            break;
        }
    }
    
    
    
    
} 

void transmitAllDataToCommUnit()
{
    transmitDataToCommUnit(DISTANCE_NORTH, fetchDataFromSensorUnit(DISTANCE_NORTH));
    transmitDataToCommUnit(DISTANCE_EAST, fetchDataFromSensorUnit(DISTANCE_EAST));
    transmitDataToCommUnit(DISTANCE_SOUTH, fetchDataFromSensorUnit(DISTANCE_SOUTH));
    transmitDataToCommUnit(DISTANCE_WEST, fetchDataFromSensorUnit(DISTANCE_WEST));
    transmitDataToCommUnit(ANGLE_NORTH, fetchDataFromSensorUnit(ANGLE_NORTH));
    transmitDataToCommUnit(ANGLE_EAST, fetchDataFromSensorUnit(ANGLE_EAST));
    transmitDataToCommUnit(ANGLE_SOUTH, fetchDataFromSensorUnit(ANGLE_SOUTH));
    transmitDataToCommUnit(ANGLE_WEST, fetchDataFromSensorUnit(ANGLE_WEST));
    transmitDataToCommUnit(LEAK_HEADER, fetchDataFromSensorUnit(LEAK_HEADER));
    transmitDataToCommUnit(TOTAL_ANGLE, fetchDataFromSensorUnit(TOTAL_ANGLE));
    return;
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

// arrays som värden hämtade ifrån sensorenheten skall ligga i
int distanceValue_g[4]; // innehåller avstånden från de olika sidorna till väggarna
int angleValue_g[4]; // innehåller vinkeln relativt de olika väggarna, vinkelvärdet är antalet grader som roboten är vriden i CCW riktning relativt var och en av väggarna.
int halfPathWidth_g = 570/2; // Avståndet mellan väggar
int regulation_g[3]; // array som regleringen sparas i


#define TRUE 1
#define FALSE 0
void calcRegulation(enum direction regulationDirection, int useRotateRegulation)
{
    //static int  regulation[3]; // skapar en array som innehåller hur roboten ska reglera
    /*
    * 
    *
    * Första värdet anger hur mycket åt höger relativt rörelseriktningen roboten ska ta sig i varje steg.
    * Andra värdet anger hur mycket längre steg benen på vänstra sidan relativt rörelseriktningen ska ta, de som medför en CW-rotation. 
    * Tredje värdet anger hur mycket längre steg benen på högra sidan relativt rörelseriktningen ska ta, de som medför en CCW-rotation
    */

    int translationRight = 0;
    int sensorOffset = 370;// Avstånd ifrån sensorera till mitten av roboten (mm)

    // variablerna vi baserar regleringen på, skillnaden mellan aktuellt värde och önskat värde
    int translationRegulationError = 0; // avser hur långt till vänster ifrån mittpunkten av "vägen" vi är
    int angleRegulationError = 0; // avser hur många grader vridna i CCW riktning relativt "den raka riktningen" dvs relativt väggarna.

    //Regleringskoefficienter
    int kProportionalTranslation = 1;
    int kProportionalAngle = 1;



    //Reglering vid gång i kooridor
    switch (currentDirection)
    {
        case north: // går vi åt norr kan vi reglera mot väst eller öster
        {
            switch(regulationDirection)
            {
                case east:
                {
                    translationRegulationError = (distanceValue_g[east] + sensorOffset) - halfPathWidth_g; // translationRegulationError avser hur långt till vänster om mittlinjen vi är 
                    break;
                }

                case west:
                {
                    translationRegulationError = halfPathWidth_g - (distanceValue_g[west] + sensorOffset);
                    break;
                }
                case none:
                {
                    translationRegulationError = 0;
                    angleRegulationError = 0;
                    break;
                }
            }
            break;
        }

        case east:
        {
            switch(regulationDirection)
            {
                case south:
                {
                    translationRegulationError = (distanceValue_g[south] + sensorOffset) - halfPathWidth_g;
                    break;
                }

                case north:
                {
                    translationRegulationError = halfPathWidth_g - (distanceValue_g[north] + sensorOffset);
                    break;
                }

                case none:
                {
                    translationRegulationError = 0;
                    angleRegulationError = 0;
                    break;
                }
            }
            break;
        }

        case south:
        {
            switch(regulationDirection)
            {
                case west:
                {
                    translationRegulationError = (distanceValue_g[west] + sensorOffset) - halfPathWidth_g;
                    break;
                }

                case east:
                {
                    translationRegulationError = halfPathWidth_g - (distanceValue_g[east] + sensorOffset);
                    break;
                }

                case none:
                {
                    translationRegulationError = 0;
                    angleRegulationError = 0;
                    break;
                }
            }
            break;
        } 

        case west:
        {
            switch(regulationDirection)
            {
                case north:
                {
                    translationRegulationError = (distanceValue_g[north] + sensorOffset) - halfPathWidth_g;
                    break;
                }

                case south:
                {
                    translationRegulationError = halfPathWidth_g - (distanceValue_g[south] + sensorOffset);
                    break;
                }

                case none:
                {
                    translationRegulationError = 0;
                    angleRegulationError = 0;
                    break;
                }
            }
            break;
        }
    }

    if (useRotateRegulation)
    {
        angleRegulationError = angleValue_g[regulationDirection];
    }
    else
    {
        angleRegulationError = 0;
    }



    translationRight = kProportionalTranslation * translationRegulationError;
    int leftSideStepLengthAdjust = (kProportionalAngle * angleRegulationError)/2; // om roboten ska rotera åt höger så låter vi benen på vänster sida ta längre steg och benen på höger sida ta kortare steg
    int rightSideStepLengthAdjust = kProportionalAngle * (-angleRegulationError)/2; // eftersom angleRegulationError avser hur mycket vridet åt vänster om mittlinjen roboten är  
    regulation_g[0] =  translationRight;
    regulation_g[1] = leftSideStepLengthAdjust;
    regulation_g[2] = rightSideStepLengthAdjust;
   // return;

// vid nuläget har vi inte sensorenheten inkopplad så värdena i distancevalue_g m.m är odefinierade
    regulation_g[0] =  0;
    regulation_g[1] = 0;
    regulation_g[2] = 0;
    return; 

}




// används som en globalvariabel för att veta vilken typ av "action" vi ska göra.
enum action{
	noAction, // inte utföra något
	turnRight, // svänga höger med möjlighet att reglera mot en vägg framför roboten
	turnLeft, // svänga vänster -"-
	turnRightBlind, // svänga höger utan möjlighet att reglera mot någon vägg framför roboten
	turnLeftBlind, // svänga vänster -"-
};
enum action currentAction_g = noAction;

// en funktion som utför det currentAction_g anger
void applyAction()
{
	switch(currentAction_g)
	{
		case turnRight:
		{
			// vi ska svänga åt höger när väggen framför oss är ungefär en halv korridorsbredd framför oss
			if(distanceValue_g[currentDirection] < stepLength_g/2 + halfPathWidth_g)
			{
				currentDirection = (currentDirection + 1) % 4; // ökar currentDirection med ett vilket medför att vi svänger åt höger
				currentAction_g = noAction; 
			}
			break;
		}

		case turnLeft:
		{
			// vi ska svänga åt vänster när väggen framför oss är ungefär en halv korridorsbredd framför oss
			if(distanceValue_g[currentDirection] < stepLength_g/2 + halfPathWidth_g)
			{
				currentDirection = (abs(currentDirection - 1)) % 4; // minskar currentDirection med ett vilket medför att vi svänger åt vänster
				currentAction_g = noAction;
			}        
			break;
		}

		case turnRightBlind:
		{
			
		}
	}
}
/*
/ 
/ ej klar och fungerar ej just nu. Kolla upp vad som finns i legPositions
/
*/
void returnToStartPosition()
{
    int tempRes = 12;
    float copy[13];
    maxGaitCyclePos_g = tempRes - 1;
    posToCalcGait = maxGaitCyclePos_g;
    for (int i = 0; i < 13; i++)
    {
        copy[i] = legPositions_g[i][currentPos_g];
    }
    // flytta ner alla benen till marknivå
    CalcStraightPath(frontLeftLeg, 4, 0, -copy[FRONT_LEFT_LEG_X],copy[FRONT_LEFT_LEG_Y], copy[FRONT_LEFT_LEG_Z],
                                        -copy[FRONT_LEFT_LEG_X], copy[FRONT_LEFT_LEG_Y], startPositionZ_g);
    CalcStraightPath(frontRightLeg, 4, 0, copy[FRONT_RIGHT_LEG_X], copy[FRONT_RIGHT_LEG_Y], copy[FRONT_RIGHT_LEG_Z],
                                        copy[FRONT_RIGHT_LEG_X], copy[FRONT_RIGHT_LEG_Y], startPositionZ_g);
    CalcStraightPath(rearLeftLeg, 4, 0, -copy[REAR_LEFT_LEG_X], -copy[REAR_LEFT_LEG_Y], copy[REAR_LEFT_LEG_Z],
                                        -copy[REAR_LEFT_LEG_X], -copy[REAR_LEFT_LEG_Y], startPositionZ_g);
    CalcStraightPath(rearRightLeg, 4, 0, copy[REAR_RIGHT_LEG_X], -copy[REAR_RIGHT_LEG_Y], copy[REAR_RIGHT_LEG_Z],
                                        copy[REAR_RIGHT_LEG_X], -copy[REAR_RIGHT_LEG_Y], startPositionZ_g);
    // lyft vänster fram och höger bak till startpositionerna.
    CalcCurvedPath(frontLeftLeg, 4, 4, -copy[FRONT_LEFT_LEG_X], copy[FRONT_LEFT_LEG_Y], startPositionZ_g,
                                        -startPositionX_g, startPositionY_g, startPositionZ_g);
    CalcStraightPath(frontRightLeg, 4, 4, copy[FRONT_RIGHT_LEG_X], copy[FRONT_RIGHT_LEG_Y], startPositionZ_g,
                                         copy[FRONT_RIGHT_LEG_X], copy[FRONT_RIGHT_LEG_Y], startPositionZ_g);
    CalcStraightPath(rearLeftLeg, 4, 4, -copy[REAR_LEFT_LEG_X], -copy[REAR_LEFT_LEG_Y], startPositionZ_g,
                                        -copy[REAR_LEFT_LEG_X], -copy[REAR_LEFT_LEG_Y], startPositionZ_g);
    CalcCurvedPath(rearRightLeg, 4, 4, copy[REAR_RIGHT_LEG_X], -copy[REAR_RIGHT_LEG_Y], startPositionZ_g,
                                        startPositionX_g, -startPositionY_g, startPositionZ_g);
    // lyft höger fram och vänster bak till startpositionerna
    CalcStraightPath(frontLeftLeg, 4, 8, -startPositionX_g, startPositionY_g, startPositionZ_g, 
                                        -startPositionX_g, startPositionY_g, startPositionZ_g);
    CalcCurvedPath(frontRightLeg, 4, 8, copy[FRONT_RIGHT_LEG_X], copy[FRONT_RIGHT_LEG_Y], startPositionZ_g, 
                                        startPositionX_g, startPositionY_g, startPositionZ_g);
    CalcCurvedPath(rearLeftLeg, 4, 8, -copy[REAR_LEFT_LEG_X], -copy[REAR_LEFT_LEG_Y], startPositionZ_g, 
                                        -startPositionX_g, -startPositionY_g, startPositionZ_g);
    CalcStraightPath(rearRightLeg, 4, 8, startPositionX_g, -startPositionY_g, startPositionZ_g, 
                                        startPositionX_g, -startPositionY_g, startPositionZ_g);
    currentPos_g = 0;
    nextPos_g = 1;
}

void transitionStartToTrot()
{
    int gaitRes = gaitResolution_g/4;
    maxGaitCyclePos_g = gaitResolution_g - 1;
    posToCalcGait = (gaitRes - 1);
    
    CalcStraightPath(frontLeftLeg,gaitRes,0,-startPositionX_g,startPositionY_g,startPositionZ_g,-startPositionX_g,startPositionY_g,startPositionZ_g+stepHeight_g);
    CalcStraightPath(frontRightLeg,gaitRes,0,startPositionX_g,startPositionY_g,startPositionZ_g,startPositionX_g,startPositionY_g,startPositionZ_g);
    CalcStraightPath(rearLeftLeg,gaitRes,0,-startPositionX_g,-startPositionY_g,startPositionZ_g,-startPositionX_g,-startPositionY_g,startPositionZ_g);
    CalcStraightPath(rearRightLeg,gaitRes,0,startPositionX_g,-startPositionY_g,startPositionZ_g,startPositionX_g,-startPositionY_g,startPositionZ_g+stepHeight_g);
    currentPos_g = 0;
    nextPos_g = 1;
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

transitionToCreep()
{
    legCoordinates frontRightLegCoordinates {0,0,startPositionZ_g};
    legCoordinates frontLeftLegCoordinates {0,0,startPositionZ_g};
    legCoordinates rearLeftLegCoordinates {0,0,startPositionZ_g};
    legCoordinates rearRightLegCoordinates {0,0,startPositionZ_g};

    int translationRight = regulation_g[0];
    int leftSideStepLengthAdjust = regulation_g[1];
    int rightSideStepLengthAdjust = regulation_g[2];
        
    int leftSideTotalStepLength = stepLength_g + leftSideStepLengthAdjust; // left hänvisar till den vänstra sidan relativt rörelseriktningen
    int rightSideTotalStepLength = stepLength_g + rightSideStepLengthAdjust; // right hänvisar till den högra sidan relativt rörelseriktningen

    int transHalf = translationRight/2;
    int transSixth = translationRight/6;
    long int sixthRight = rightSideTotalStepLength/6;
    long int halfRight = rightSideTotalStepLength/2;

    long int sixthLeft = leftSideTotalStepLength/6;
    long int halfLeft = leftSideTotalStepLength/2;
    // Startpositionerna ser olika ut beroende på vilken riktning roboten ska börja gå åt.
    switch(currentDirection)
    {
        case north:
        {            
            frontRightLegCoordinates.xCoord = startPositionX_g-transSixth;
            frontRightLegCoordinates.yCoord = startPositionY_g-sixthRight;

            frontLeftLegCoordinates.xCoord = -startPositionX_g+transHalf;
            frontLeftLegCoordinates.yCoord = startPositionY_g+halfLeft;

            rearRightLegCoordinates.xCoord = startPositionX_g-transHalf;
            rearRightLegCoordinates.yCoord = -startPositionY_g-halfRight;

            rearLeftLegCoordinates.xCoord = -startPositionX_g+transSixth;
            rearLeftLegCoordinates.yCoord = -startPositionY_g+sixthLeft;
            break;
        }
        case east:
        {
            frontRightLegCoordinates.xCoord = startPositionX_g+halfLeft;
            frontRightLegCoordinates.yCoord = startPositionY_g+transHalf;

            frontLeftLegCoordinates.xCoord = -startPositionX_g+sixthLeft;
            frontLeftLegCoordinates.yCoord = startPositionY_g+transSixth;

            rearRightLegCoordinates.xCoord = startPositionX_g-sixthRight;
            rearRightLegCoordinates.yCoord = -startPositionY_g-transSixth;

            rearLeftLegCoordinates.xCoord = -startPositionX_g - halfRight;
            rearLeftLegCoordinates.yCoord =  -startPositionY_g-transHalf;
            break;
        }
        case south:
        {
            frontRightLegCoordinates.xCoord = startPositionX_g-transSixth; 
            frontRightLegCoordinates.yCoord = startPositionY_g-sixthLeft;

            frontLeftLegCoordinates.xCoord = -startPositionX_g+transHalf; 
            frontLeftLegCoordinates.yCoord = startPositionY_g+halfRight;

            rearRightLegCoordinates.xCoord = startPositionX_g-transHalf; 
            rearRightLegCoordinates.yCoord = -startPositionY_g-halfLeft;

            rearLeftLegCoordinates.xCoord = -startPositionX_g+transSixth;
            rearLeftLegCoordinates.yCoord =  -startPositionY_g+sixthRight;
            break;
        }
        case west:
        {
            frontRightLegCoordinates.xCoord = startPositionX_g + halfRight; 
            frontRightLegCoordinates.yCoord = startPositionY_g+transHalf;

            frontLeftLegCoordinates.xCoord = -startPositionX_g+sixthRight; 
            frontLeftLegCoordinates.yCoord = startPositionY_g+transSixth;

            rearRightLegCoordinates.xCoord = startPositionX_g-sixthLeft;
            rearRightLegCoordinates.yCoord = -startPositionY_g-transSixth; 

            rearLeftLegCoordinates.xCoord = -startPositionX_g-halfLeft;
            rearLeftLegCoordinates.yCoord = -startPositionY_g-transHalf;
            break;
        }
        case none:
        {
            frontRightLegCoordinates.xCoord = startPositionX_g;
            frontRightLegCoordinates.yCoord = startPositionY_g;

            frontLeftLegCoordinates.xCoord = -startPositionX_g;
            frontLeftLegCoordinates.yCoord = startPositionY_g;

            rearRightLegCoordinates.xCoord = startPositionX_g;
            rearRightLegCoordinates.yCoord = -startPositionY_g; 

            rearLeftLegCoordinates.xCoord = -startPositionX_g;
            rearLeftLegCoordinates.yCoord = -startPositionY_g;
            break;
        }

    }
    int tempRes = 12;
    float copy[13];
    maxGaitCyclePos_g = tempRes - 1;
    posToCalcGait = maxGaitCyclePos_g;
    for (int i = 0; i < 13; i++)
    {
        copy[i] = legPositions_g[i][currentPos_g];
    }
    // flytta ner alla benen till marknivå
    CalcStraightPath(frontLeftLeg, 4, 0, -copy[FRONT_LEFT_LEG_X],copy[FRONT_LEFT_LEG_Y], copy[FRONT_LEFT_LEG_Z],
                                        -copy[FRONT_LEFT_LEG_X], copy[FRONT_LEFT_LEG_Y], startPositionZ_g);
    CalcStraightPath(frontRightLeg, 4, 0, copy[FRONT_RIGHT_LEG_X], copy[FRONT_RIGHT_LEG_Y], copy[FRONT_RIGHT_LEG_Z],
                                        copy[FRONT_RIGHT_LEG_X], copy[FRONT_RIGHT_LEG_Y], startPositionZ_g);
    CalcStraightPath(rearLeftLeg, 4, 0, -copy[REAR_LEFT_LEG_X], -copy[REAR_LEFT_LEG_Y], copy[REAR_LEFT_LEG_Z],
                                        -copy[REAR_LEFT_LEG_X], -copy[REAR_LEFT_LEG_Y], startPositionZ_g);
    CalcStraightPath(rearRightLeg, 4, 0, copy[REAR_RIGHT_LEG_X], -copy[REAR_RIGHT_LEG_Y], copy[REAR_RIGHT_LEG_Z],
                                        copy[REAR_RIGHT_LEG_X], -copy[REAR_RIGHT_LEG_Y], startPositionZ_g);
    // lyft vänster fram och höger bak till startpositionerna för creep.
    CalcCurvedPath(frontLeftLeg, 4, 4, -copy[FRONT_LEFT_LEG_X], copy[FRONT_LEFT_LEG_Y], startPositionZ_g,
                                        frontLeftLegCoordinates.xCoord, frontLeftLegCoordinates.yCoord, startPositionZ_g);
    CalcStraightPath(frontRightLeg, 4, 4, copy[FRONT_RIGHT_LEG_X], copy[FRONT_RIGHT_LEG_Y], startPositionZ_g,
                                         copy[FRONT_RIGHT_LEG_X], copy[FRONT_RIGHT_LEG_Y], startPositionZ_g);
    CalcStraightPath(rearLeftLeg, 4, 4, -copy[REAR_LEFT_LEG_X], -copy[REAR_LEFT_LEG_Y], startPositionZ_g,
                                        -copy[REAR_LEFT_LEG_X], -copy[REAR_LEFT_LEG_Y], startPositionZ_g);
    CalcCurvedPath(rearRightLeg, 4, 4, copy[REAR_RIGHT_LEG_X], -copy[REAR_RIGHT_LEG_Y], startPositionZ_g,
                                        rearRightLegCoordinates.xCoord, rearRightLegCoordinates.yCoord, startPositionZ_g);
    // lyft höger fram och vänster bak till startpositionerna för creep.
    CalcStraightPath(frontLeftLeg, 4, 8, frontLeftLegCoordinates.xCoord, frontLeftLegCoordinates.yCoord, startPositionZ_g, startPositionZ_g, 
                                        frontLeftLegCoordinates.xCoord, frontLeftLegCoordinates.yCoord, startPositionZ_g, startPositionZ_g);
    CalcCurvedPath(frontRightLeg, 4, 8, copy[FRONT_RIGHT_LEG_X], copy[FRONT_RIGHT_LEG_Y], startPositionZ_g, 
                                        frontRightLegCoordinates.xCoord, frontRightLegCoordinates.yCoord, startPositionZ_g);
    CalcCurvedPath(rearLeftLeg, 4, 8, -copy[REAR_LEFT_LEG_X], -copy[REAR_LEFT_LEG_Y], startPositionZ_g, 
                                        rearLeftLegCoordinates.xCoord, rearLeftLegCoordinates.yCoord, startPositionZ_g);
    CalcStraightPath(rearRightLeg, 4, 8, rearRightLegCoordinates.xCoord, rearRightLegCoordinates.yCoord, startPositionZ_g, 
                                        rearRightLegCoordinates.xCoord, rearRightLegCoordinates.yCoord, startPositionZ_g);
    currentPos_g = 0;
    nextPos_g = 1;
}

// cycleResolution måste vara delbart med 8
void makeCreepGait(int cycleResolution)
{
    int res = cycleResolution/4;
    maxGaitCyclePos_g = cycleResolution -1;
    posToCalcGait = maxGaitCyclePos_g;
    
    currentPos_g = 0;
    nextPos_g = 1;

    int translationRight = regulation_g[0];
    int leftSideStepLengthAdjust = regulation_g[1];
    int rightSideStepLengthAdjust = regulation_g[2];
        
    int leftSideTotalStepLength = stepLength_g + leftSideStepLengthAdjust; // left hänvisar till den vänstra sidan relativt rörelseriktningen
    int rightSideTotalStepLength = stepLength_g + rightSideStepLengthAdjust; // right hänvisar till den högra sidan relativt rörelseriktningen

    int transHalf = translationRight/2;
    int transSixth = translationRight/6;
    long int sixthRight = rightSideTotalStepLength/6;
    long int halfRight = rightSideTotalStepLength/2;

    long int sixthLeft = leftSideTotalStepLength/6;
    long int halfLeft = leftSideTotalStepLength/2;
    switch(currentDirection)
    {
        case north:
        {
            CalcCurvedPath(rearRightLeg, res, 0, startPositionX_g-transHalf, -startPositionY_g-halfRight, startPositionZ_g, startPositionX_g+transHalf, -startPositionY_g+halfRight, startPositionZ_g);
            CalcStraightPath(rearRightLeg, res*3, res, startPositionX_g+transHalf, -startPositionY_g+halfRight, startPositionZ_g, startPositionX_g-transHalf, -startPositionY_g-halfRight, startPositionZ_g);

            CalcStraightPath(frontRightLeg, res, 0, startPositionX_g-transSixth, startPositionY_g-sixthRight, startPositionZ_g, startPositionX_g-transHalf, startPositionY_g-halfRight, startPositionZ_g);
            CalcCurvedPath(frontRightLeg, res, res, startPositionX_g-transHalf, startPositionY_g-halfRight, startPositionZ_g, startPositionX_g+transHalf, startPositionY_g+halfRight, startPositionZ_g);
            CalcStraightPath(frontRightLeg, 2*res, 2*res, startPositionX_g+transHalf, startPositionY_g+halfRight, startPositionZ_g, startPositionX_g-transSixth, startPositionY_g-sixthRight, startPositionZ_g);

            CalcStraightPath(rearLeftLeg, 2*res, 0, -startPositionX_g+transSixth, -startPositionY_g+sixthLeft, startPositionZ_g, -startPositionX_g-transHalf, -startPositionY_g-halfLeft, startPositionZ_g);
            CalcCurvedPath(rearLeftLeg, res, 2*res, -startPositionX_g-transHalf, -startPositionY_g-halfLeft, startPositionZ_g, -startPositionX_g+transHalf, -startPositionY_g+halfLeft, startPositionZ_g);
            CalcStraightPath(rearLeftLeg, res, 3*res, -startPositionX_g+transHalf, -startPositionY_g+halfLeft, startPositionZ_g, -startPositionX_g+transSixth, -startPositionY_g+sixthLeft, startPositionZ_g);

            CalcStraightPath(frontLeftLeg, 3*res, 0, -startPositionX_g+transHalf, startPositionY_g+halfLeft, startPositionZ_g, -startPositionX_g-transHalf, startPositionY_g-halfLeft, startPositionZ_g);
            CalcCurvedPath(frontLeftLeg, res, 3*res, -startPositionX_g-transHalf, startPositionY_g-halfLeft, startPositionZ_g, -startPositionX_g+transHalf, startPositionY_g+halfLeft, startPositionZ_g);
            break;
        }
        case east:
        {
            CalcCurvedPath(rearLeftLeg, res, 0, -startPositionX_g - halfRight, -startPositionY_g-transHalf, startPositionZ_g, -startPositionX_g + halfRight, -startPositionY_g+transHalf, startPositionZ_g);
            CalcStraightPath(rearLeftLeg, res*3, res, -startPositionX_g + halfRight, -startPositionY_g+transHalf, startPositionZ_g, -startPositionX_g - halfRight, -startPositionY_g-transHalf, startPositionZ_g);

            CalcStraightPath(rearRightLeg, res, 0, startPositionX_g-sixthRight, -startPositionY_g-transSixth, startPositionZ_g, startPositionX_g-halfRight, -startPositionY_g-transHalf, startPositionZ_g);
            CalcCurvedPath(rearRightLeg, res, res, startPositionX_g-halfRight, -startPositionY_g-transHalf, startPositionZ_g, startPositionX_g+halfRight, -startPositionY_g+transHalf, startPositionZ_g);
            CalcStraightPath(rearRightLeg, 2*res, 2*res, startPositionX_g+halfRight, -startPositionY_g+transHalf, startPositionZ_g, startPositionX_g-sixthRight, -startPositionY_g-transSixth, startPositionZ_g);

            CalcStraightPath(frontLeftLeg, 2*res, 0, -startPositionX_g+sixthLeft, startPositionY_g+transSixth, startPositionZ_g, -startPositionX_g-halfLeft, startPositionY_g-transHalf, startPositionZ_g);
            CalcCurvedPath(frontLeftLeg, res, 2*res, -startPositionX_g-halfLeft, startPositionY_g-transHalf, startPositionZ_g, -startPositionX_g+halfLeft, startPositionY_g+transHalf, startPositionZ_g);
            CalcStraightPath(frontLeftLeg, res, 3*res, -startPositionX_g+halfLeft, startPositionY_g+transHalf, startPositionZ_g, -startPositionX_g+sixthLeft, startPositionY_g+transSixth, startPositionZ_g);

            CalcStraightPath(frontRightLeg, 3*res, 0, startPositionX_g+halfLeft, startPositionY_g+transHalf, startPositionZ_g, startPositionX_g-halfLeft, startPositionY_g-transHalf, startPositionZ_g);
            CalcCurvedPath(frontRightLeg, res, 3*res, startPositionX_g-halfLeft, startPositionY_g-transHalf, startPositionZ_g, startPositionX_g+halfLeft, startPositionY_g+transHalf, startPositionZ_g);
            break;
        }
        case south:
        {
            CalcCurvedPath(frontLeftLeg, res, 0, -startPositionX_g+transHalf, startPositionY_g+halfRight, startPositionZ_g, -startPositionX_g-transHalf, startPositionY_g-halfRight, startPositionZ_g);
            CalcStraightPath(frontLeftLeg, res*3, res, -startPositionX_g-transHalf, startPositionY_g-halfRight, startPositionZ_g, -startPositionX_g+transHalf, startPositionY_g+halfRight, startPositionZ_g);

            CalcStraightPath(rearLeftLeg, res, 0, -startPositionX_g+transSixth, -startPositionY_g+sixthRight, startPositionZ_g, -startPositionX_g+transHalf, -startPositionY_g+halfRight, startPositionZ_g);
            CalcCurvedPath(rearLeftLeg, res, res, -startPositionX_g+transHalf, -startPositionY_g+halfRight, startPositionZ_g, -startPositionX_g-transHalf, -startPositionY_g-halfRight, startPositionZ_g);
            CalcStraightPath(rearLeftLeg, 2*res, 2*res, -startPositionX_g-transHalf, -startPositionY_g-halfRight, startPositionZ_g, -startPositionX_g+transSixth, -startPositionY_g+sixthRight, startPositionZ_g);

            CalcStraightPath(frontRightLeg, 2*res, 0, startPositionX_g-transSixth, startPositionY_g-sixthLeft, startPositionZ_g, startPositionX_g+transHalf, startPositionY_g+halfLeft, startPositionZ_g);
            CalcCurvedPath(frontRightLeg, res, 2*res, startPositionX_g+transHalf, startPositionY_g+halfLeft, startPositionZ_g, startPositionX_g-transHalf, startPositionY_g-halfLeft, startPositionZ_g);
            CalcStraightPath(frontRightLeg, res, 3*res, startPositionX_g-transHalf, startPositionY_g-halfLeft, startPositionZ_g, startPositionX_g-transSixth, startPositionY_g-sixthLeft, startPositionZ_g);

            CalcStraightPath(rearRightLeg, 3*res, 0, startPositionX_g-transHalf, -startPositionY_g-halfLeft, startPositionZ_g, startPositionX_g+transHalf, -startPositionY_g+halfLeft, startPositionZ_g);
            CalcCurvedPath(rearRightLeg, res, 3*res, startPositionX_g+transHalf, -startPositionY_g+halfLeft, startPositionZ_g, startPositionX_g-transHalf, -startPositionY_g-halfLeft, startPositionZ_g);
            break;
        }
        case west:
        {
            CalcCurvedPath(frontRightLeg, res, 0, startPositionX_g + halfRight, startPositionY_g+transHalf, startPositionZ_g, startPositionX_g - halfRight, startPositionY_g-transHalf, startPositionZ_g);
            CalcStraightPath(frontRightLeg, res*3, res, startPositionX_g - halfRight, startPositionY_g-transHalf, startPositionZ_g, startPositionX_g + halfRight, startPositionY_g+transHalf, startPositionZ_g);

            CalcStraightPath(frontLeftLeg, res, 0, -startPositionX_g+sixthRight, startPositionY_g+transSixth, startPositionZ_g, -startPositionX_g+halfRight, startPositionY_g+transHalf, startPositionZ_g);
            CalcCurvedPath(frontLeftLeg, res, res, -startPositionX_g+halfRight, startPositionY_g+transHalf, startPositionZ_g, -startPositionX_g-halfRight, startPositionY_g-transHalf, startPositionZ_g);
            CalcStraightPath(frontLeftLeg, 2*res, 2*res, -startPositionX_g-halfRight, startPositionY_g-transHalf, startPositionZ_g, -startPositionX_g+sixthRight, startPositionY_g+transSixth, startPositionZ_g);

            CalcStraightPath(rearRightLeg, 2*res, 0, startPositionX_g-sixthLeft, -startPositionY_g-transSixth, startPositionZ_g, startPositionX_g+halfLeft, -startPositionY_g+transHalf, startPositionZ_g);
            CalcCurvedPath(rearRightLeg, res, 2*res, startPositionX_g+halfLeft, -startPositionY_g+transHalf, startPositionZ_g, startPositionX_g-halfLeft, -startPositionY_g-transHalf, startPositionZ_g);
            CalcStraightPath(rearRightLeg, res, 3*res, startPositionX_g-halfLeft, -startPositionY_g-transHalf, startPositionZ_g, startPositionX_g-sixthLeft, -startPositionY_g+transSixth, startPositionZ_g);

            CalcStraightPath(rearLeftLeg, 3*res, 0, -startPositionX_g-halfLeft, -startPositionY_g-transHalf, startPositionZ_g, -startPositionX_g+halfLeft, -startPositionY_g+transHalf, startPositionZ_g);
            CalcCurvedPath(rearLeftLeg, res, 3*res, -startPositionX_g+halfLeft, -startPositionY_g+transHalf, startPositionZ_g, -startPositionX_g-halfLeft, -startPositionY_g-transHalf, startPositionZ_g);
            break;
        }
    }
    
}


void standStillGait()
{
    maxGaitCyclePos_g = 2;
    posToCalcGait = 2;
    CalcStraightPath(frontLeftLeg,3,0,-startPositionX_g,startPositionY_g,startPositionZ_g,-startPositionX_g,startPositionY_g,startPositionZ_g);
    CalcStraightPath(frontRightLeg,3,0,startPositionX_g,startPositionY_g,startPositionZ_g,startPositionX_g,startPositionY_g,startPositionZ_g);
    CalcStraightPath(rearLeftLeg,3,0,-startPositionX_g,-startPositionY_g,startPositionZ_g,-startPositionX_g,-startPositionY_g,startPositionZ_g);
    CalcStraightPath(rearRightLeg,3,0,startPositionX_g,-startPositionY_g,startPositionZ_g,startPositionX_g,-startPositionY_g,startPositionZ_g);
    currentPos_g = 0;
    nextPos_g = 1;

}

// gör rörelsemönstret för travet, beror på vilken direction som är satt på currentDirection
void MakeTrotGait(int cycleResolution)
{
    // cycleResolution är antaletpunkter på kurvan som benen följer. Måste vara jämnt tal!
    int res = cycleResolution/2;
    maxGaitCyclePos_g = cycleResolution - 1;
    posToCalcGait = (cycleResolution/4 - 1);
    
    
    // om ej autonomt läge kommer den manuella styrningen i main-loopen att använda reglerparametrarna till diagonal gång och rotation.
    int translationRight = regulation_g[0];
    int leftSideStepLengthAdjust = regulation_g[1];
    int rightSideStepLengthAdjust = regulation_g[2];
        
    int leftSideTotalStepLength = stepLength_g + leftSideStepLengthAdjust; // left hänvisar till den vänstra sidan relativt rörelseriktningen
    int rightSideTotalStepLength = stepLength_g + rightSideStepLengthAdjust; // right hänvisar till den högra sidan relativt rörelseriktningen

    directionHasChanged = 0;
    switch(currentDirection)
    {       
    // rörelsemönstret finns på ett papper (frontleft och rearright börjar alltid med curved)
    
    // anmärk att left/right-SideTotalStepLength menar den totala steglängden på den vänstra/högra sidan relativt rörelseriktningen 
    // medans frontLeftLeg och de andra benen alltid har samma namn och är namngivna utifrån rörelseriktningen norr.   
        case north:
        {   
            CalcCurvedPath(frontLeftLeg,res,0,-startPositionX_g-translationRight/2,startPositionY_g-leftSideTotalStepLength/2,startPositionZ_g,-startPositionX_g+translationRight/2,startPositionY_g+leftSideTotalStepLength/2,startPositionZ_g);
            CalcStraightPath(frontLeftLeg,res,res,-startPositionX_g+translationRight/2,startPositionY_g+leftSideTotalStepLength/2,startPositionZ_g,-startPositionX_g-translationRight/2,startPositionY_g-leftSideTotalStepLength/2,startPositionZ_g);
  
            CalcCurvedPath(rearRightLeg,res,0,startPositionX_g-translationRight/2,-startPositionY_g-rightSideTotalStepLength/2,startPositionZ_g,startPositionX_g+translationRight/2,-startPositionY_g+rightSideTotalStepLength/2,startPositionZ_g);
            CalcStraightPath(rearRightLeg,res,res,startPositionX_g+translationRight/2,-startPositionY_g+rightSideTotalStepLength/2,startPositionZ_g,startPositionX_g-translationRight/2,-startPositionY_g-rightSideTotalStepLength/2,startPositionZ_g);
    
            CalcStraightPath(rearLeftLeg,res,0,-startPositionX_g+translationRight/2,-startPositionY_g+leftSideTotalStepLength/2,startPositionZ_g,-startPositionX_g-translationRight/2,-startPositionY_g-leftSideTotalStepLength/2,startPositionZ_g);
            CalcCurvedPath(rearLeftLeg,res,res,-startPositionX_g-translationRight/2,-startPositionY_g-leftSideTotalStepLength/2,startPositionZ_g,-startPositionX_g+translationRight/2,-startPositionY_g+leftSideTotalStepLength/2,startPositionZ_g);
    
            CalcStraightPath(frontRightLeg,res,0,startPositionX_g+translationRight/2,startPositionY_g+rightSideTotalStepLength/2,startPositionZ_g,startPositionX_g-translationRight/2,startPositionY_g-rightSideTotalStepLength/2,startPositionZ_g);
            CalcCurvedPath(frontRightLeg,res,res,startPositionX_g-translationRight/2,startPositionY_g-rightSideTotalStepLength/2,startPositionZ_g,startPositionX_g+translationRight/2,startPositionY_g+rightSideTotalStepLength/2,startPositionZ_g);
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

// funktioner som används för att optimera gångstilen
void increaseLegWidth()
{
	if ((startPositionX_g < 150) && (startPositionY_g < 150))
	{
		startPositionX_g = startPositionX_g + 2;
		startPositionY_g = startPositionY_g + 2;
	}
	return;
}

void decreaseLegWidth()
{
	if ((startPositionX_g > 0) && (startPositionY_g > 0))
	{
		startPositionX_g = startPositionX_g - 2;
		startPositionY_g = startPositionY_g - 2;
	}
	return;
}

void decreaseLegHeight()
{
	if (startPositionZ_g < -60)
	{
		startPositionZ_g = startPositionZ_g + 2;
	}
	return;
}

void increaseLegHeight()
{
	if (startPositionZ_g > -160)
	{
		startPositionZ_g = startPositionZ_g - 2;
	}
	return;
}

void increaseStepLength()
{
	if (stepLength_g < 100)
	{
		stepLength_g = stepLength_g + 2;
	}
	return;
}

void decreaseStepLength()
{
	if (stepLength_g > 10)
	{
		stepLength_g = stepLength_g - 2;
	}
	return;
}

void increaseGaitResolutionTime()
{
	if (newGaitResolutionTime < 100)
	{
		newGaitResolutionTime = newGaitResolutionTime + 10;
		setTimerPeriod(TIMER_0, newGaitResolutionTime);
	}
	return;
}

void decreaseGaitResolutionTime()
{
	if (newGaitResolutionTime > 30)
	{
		newGaitResolutionTime = newGaitResolutionTime - 10;
		setTimerPeriod(TIMER_0, newGaitResolutionTime);
	}
	return;
}

void increaseStepHeight()
{
	if (stepHeight_g < 80)
	{
		stepHeight_g = stepHeight_g + 2;
	}
	return;
}

void decreaseStepHeight()
{
	if (stepHeight_g > 10)
	{
		stepHeight_g = stepHeight_g - 2;
	}
	return;
}



void makeGaitTransition(enum gait newGait)
{
    switch(newGait)
    {
        case standStill:
        {
            returnToStartPosition();
            currentGait = standStill;
            break;
        }
        case trotGait:
        {
            transitionStartToTrot();
            currentGait = trotGait;
            break;
        }
    }
}
void gaitController()
{
    if((currentPos_g == posToCalcGait) && (needToCalcGait))
    {
        //calcRegulation();
        switch(currentGait)
        {
            case standStill:
            {
                standStillGait();
                break;
            }
            case trotGait:
            {
                if (gaitToUse == creepGait)
                {
                    transitionToCreep();
                    currentGait = creepGait;
                }
                else
                {
                    MakeTrotGait(gaitResolution_g);
                }
                break;
            }
            case creepGait:
            {
                makeCreepGait(gaitResolution_g);
                break;
            }
        }
        needToCalcGait = 0;
    }
    if (currentControlMode == manual)
    {
        if (directionHasChanged)
        {
            directionHasChanged = 0;
            needToCalcGait = 1;
            switch(currentRotationInstruction) // Ändrar rotation om den finns en instruktion för att rotera.
            {
                case CW_ROTATION:
                {
                    if (currentGait == standStill)
                    {
                        transitionStartToTrot();
                        currentGait = trotGait;
                    }
                    regulation_g[1] = stepLengthRotationAdjust;
                    regulation_g[2] = -stepLengthRotationAdjust;
                    break;
                }
                case CCW_ROTATION:
                {
                    if (currentGait == standStill)
                    {
                        transitionStartToTrot();
                        currentGait = trotGait;
                    }
                    regulation_g[1] = -stepLengthRotationAdjust;
                    regulation_g[2] = stepLengthRotationAdjust;
                    break;
                }
                case NO_ROTATION:
                {
                    if ((currentDirection == none) && (currentGait != standStill))
                    {
                        returnToStartPosition();
                        currentGait = standStill;
                    }
                    regulation_g[1] = 0;
                    regulation_g[2] = 0;
                    break;
                }
            }
            switch(currentDirectionInstruction) // Väljer riktning beroende på vad användaren matat in
            {
                case NORTH_HEADER:
                {
                    currentDirection = north;
                    regulation_g[0] = 0;
                    if (currentGait == standStill)
                    {
                        if (gaitToUse == trotGait)
                        {
                            transitionStartToTrot();
                            currentGait = trotGait;   
                        }
                        else
                        {
                            transitionToCreep();
                            currentGait = creepGait;
                        }
                    }
                    break;
                }
                case NORTH_EAST_HEADER:
                {
                    currentDirection = north;
                    regulation_g[0] = stepLength_g;
                    if (currentGait == standStill)
                    {
                        if (gaitToUse == trotGait)
                        {
                            transitionStartToTrot();
                            currentGait = trotGait;   
                        }
                        else
                        {
                            transitionToCreep();
                            currentGait = creepGait;
                        }
                    }
                    // Huvudsaklig rörelseriktning norr. Östlig offset hanteres genom en hårdkodad reglering åt höger
                    break;
                }
                case EAST_HEADER:
                {
                    currentDirection = east;
                    regulation_g[0] = 0;
                    if (currentGait == standStill)
                    {
                        if (gaitToUse == trotGait)
                        {
                            transitionStartToTrot();
                            currentGait = trotGait;   
                        }
                        else
                        {
                            transitionToCreep();
                            currentGait = creepGait;
                        }
                    }
                    break;
                    }
                case SOUTH_EAST_HEADER:
                {
                    currentDirection = east;
                    regulation_g[0] = stepLength_g;
                    if (currentGait == standStill)
                    {
                        if (gaitToUse == trotGait)
                        {
                            transitionStartToTrot();
                            currentGait = trotGait;   
                        }
                        else
                        {
                            transitionToCreep();
                            currentGait = creepGait;
                        }
                    }
                    // Huvudsaklig rörelseriktning öster. Sydlig offset hanteres genom en hårdkodad reglering åt höger
                    break;
                }
                case SOUTH_HEADER:
                {
                    currentDirection = south;
                    regulation_g[0] = 0;
                    if (currentGait == standStill)
                    {
                        if (gaitToUse == trotGait)
                        {
                            transitionStartToTrot();
                            currentGait = trotGait;   
                        }
                        else
                        {
                            transitionToCreep();
                            currentGait = creepGait;
                        }
                    }
                    break;
                }
                case SOUTH_WEST_HEADER:
                {
                    currentDirection = south;
                    regulation_g[0] = stepLength_g;
                    if (currentGait == standStill)
                    {
                        if (gaitToUse == trotGait)
                        {
                            transitionStartToTrot();
                            currentGait = trotGait;   
                        }
                        else
                        {
                            transitionToCreep();
                            currentGait = creepGait;
                        }
                    }
                    // Huvudsaklig rörelseriktning söder. Västlig offset hanteres genom en hårdkodad reglering åt höger
                    break;
                }
                case WEST_HEADER:
                {
                    currentDirection = west;
                    regulation_g[0] = 0;
                    if (currentGait == standStill)
                    {
                        if (gaitToUse == trotGait)
                        {
                            transitionStartToTrot();
                            currentGait = trotGait;   
                        }
                        else
                        {
                            transitionToCreep();
                            currentGait = creepGait;
                        }
                    }
                    break;
                }
                case NORTH_WEST_HEADER:
                {
                    currentDirection = west;
                    regulation_g[0] = stepLength_g;
                    if (currentGait == standStill)
                    {
                        if (gaitToUse == trotGait)
                        {
                            transitionStartToTrot();
                            currentGait = trotGait;   
                        }
                        else
                        {
                            transitionToCreep();
                            currentGait = creepGait;
                        }
                    }
                    // Huvudsaklig rörelseriktning väster. Nordlig offset hanteres genom en hårdkodad reglering åt höger
                    break;
                }
                case NO_MOVEMENT_DIRECTION_HEADER:
                {
                    if(currentRotationInstruction == NO_ROTATION)
                    {
                        returnToStartPosition();
                        currentGait = standStill;
                    }
                    currentDirection = none;
                    regulation_g[0] = 0;
                    break;
                }
            }
        }
		if(ventilatorOptionsHasChanged_g == 1)
		{
			ventilatorOptionsHasChanged_g = 0;
            needToCalcGait = 1;
			switch (currentVentilatorOptionInstruction_g)
			{
				case INCREASE_LEG_WIDTH:
				{
					increaseLegWidth();
					break;
				}
				
				case DECREASE_LEG_WIDTH:
				{
					decreaseLegWidth();
					break;
				}
				
				case INCREASE_LEG_HEIGHT:
				{
					increaseLegHeight();
					break;
				}
				
				case DECREASE_LEG_HEIGHT:
				{
					decreaseLegHeight();
					break;
				}
				
				case INCREASE_STEP_LENGTH:
				{
					increaseStepLength();
					break;
				}
				
				case DECREASE_STEP_LENGTH:
				{
					decreaseStepLength();
					break;
				}
				
				case INCREASE_STEP_HEIGHT:
				{
					increaseStepHeight();
					break;
				}
				
				case DECREASE_STEP_HEIGHT:
				{
					decreaseStepHeight();
					break;
				}
				
				case INCREASE_GAIT_RESOLUTION_TIME:
				{
					increaseGaitResolutionTime();
					break;
				}
				
				case DECREASE_GAIT_RESOLUTION_TIME:
				{
					decreaseGaitResolutionTime();
					break;
				}
			}
		}
	}
}



int main(void)
{
    
    initUSART();
    spiMasterInit();
    EICRA = 0b1111; // Stigande flank på INT1/0 genererar avbrott
    EIMSK = (EIMSK | 3); // Möjliggör externa avbrott på INT1/0

    currentPos_g = 0;
    nextPos_g = 1;
   
    timer0Init();
    timer2Init();
    sei();
    MoveToStartPosition();
    //moveToCreepStartPosition();
    _delay_ms(1000);
    //currentDirection = east;
    //makeCreepGait(gaitResolution_g);
    standStillGait();
    //currentGait = creepGait;
    
    
    //moveToCreepStartPosition();
    //makeCreepGait(gaitResolution_g);
    setTimerPeriod(TIMER_0, newGaitResolutionTime);
    setTimerPeriod(TIMER_2, newCommUnitUpdatePeriod);
    
    
    

    // ---- Main-Loop ----
    while (1)
    {
	
    	if (legTimerPeriodEnd())
    	{
    	    move();
    	    resetLegTimer();
    	}
    	if (commTimerPeriodEnd())
    	{
            /*
            transmitDataToCommUnit(ANGLE_NORTH,readCurrentTemperatureFromActuator(3));
            transmitDataToCommUnit(ANGLE_EAST, readCurrentTemperatureFromActuator(4));
            transmitDataToCommUnit(ANGLE_SOUTH, readCurrentTemperatureFromActuator(9));
            transmitDataToCommUnit(ANGLE_WEST, readCurrentTemperatureFromActuator(10));
            transmitDataToCommUnit(ANGLE_WEST, ReadTemperatureLimitFromActuator(10));
            transmitDataToCommUnit(TOTAL_ANGLE, readAlarmShutdownStatus(3));
            */
            transmitAllDataToCommUnit();
            resetCommTimer();
    	}
        gaitController();
    }
}



 
