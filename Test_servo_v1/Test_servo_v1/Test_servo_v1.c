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
#include "nodsystemet.h"
#include <stdlib.h>

int halfPathWidth_g = 570/2; // Avståndet mellan väggar
int regulation_g[3]; // array som regleringen sparas i

int closeEnoughToTurn_g = 0;
		


int stepLength_g = 80;
int startPositionX_g = 80;
int startPositionY_g = 80;
int startPositionZ_g = -80;
int stepHeight_g =  20;
int gaitResolution_g = 12; // MÅSTE VARA DELBART MED 4 vid trot, 8 vid creep
int stepLengthRotationAdjust = 30;
int newGaitResolutionTime = INCREMENT_PERIOD_60; // tid i timerloopen för benstyrningen i ms


int currentDirectionInstruction = 0; // Nuvarande manuell styrinstruktion
int currentRotationInstruction = 0;
int optionsHasChanged_g = 0;
int posToCalcGait;
int needToCalcGait = 1;
int sensorOffset_g = 60;// Avstånd ifrån sensorera till mitten av roboten (mm)
// ------ Globala variabler för "svängar" ------
int BlindStepsTaken_g = 0;
//int BlindStepsToTake_g = 5; // Avstånd från sensor till mitten av robot ~= 8 cm.
// BlindStepsToTake ska vara ungefär (halfPathWidth - 8)/practicalStepLength
int BlindStepsToTake_g = 0;

// ------ Inställningar för robot-datorkommunikation ------
int newCommUnitUpdatePeriod = INCREMENT_PERIOD_200;

// regler koefficienter
float kProportionalTranslation_g = 0.3;
float kProportionalAngle_g = 0.4;



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

int sendLegWidth = FALSE;
int sendStepHeight = FALSE;
int sendRobotHeight = FALSE;
int sendStepLength = FALSE;
int sendKPAngle = FALSE;
int sendKPTrans = FALSE;
int sendGaitResTime = FALSE;

// currentDirection_g-deklaration samt controlMode flyttade till Definitions.h

typedef struct leg leg;
struct leg {
    int legNumber,
    coxaJoint,
    femurJoint,
    tibiaJoint;
};

leg frontLeftLeg = {FRONT_LEFT_LEG, 2, 4, 6};
leg frontRightLeg = {FRONT_RIGHT_LEG, 8, 10, 12};
leg rearLeftLeg = {REAR_LEFT_LEG, 1, 3, 18};
leg rearRightLeg = {REAR_RIGHT_LEG, 7, 9, 11};


enum gait currentGait = standStill;

int standardSpeed_g = 20;
int statusPackEnabled = 0;


void sendAllRobotParameters()
{
    transmitDataToCommUnit(STEP_INCREMENT_TIME_HEADER, gaitResolutionTime_g);
    transmitDataToCommUnit(STEP_LENGTH_HEADER, stepLength_g);
    transmitDataToCommUnit(STEP_HEIGHT_HEADER, stepHeight_g);
    transmitDataToCommUnit(LEG_DISTANCE_HEADER, startPositionX_g);
    transmitDataToCommUnit(ROBOT_HEIGHT_HEADER, -startPositionZ_g);
    transmitDataToCommUnit(K_P_ANGLE_HEADER, 100*kProportionalAngle_g);
    transmitDataToCommUnit(K_P_TRANS_HEADER, 100*kProportionalTranslation_g);
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
    
    
    MoveDynamixel(frontLeftLeg.coxaJoint,ActuatorAngle1,speed);
    MoveDynamixel(frontLeftLeg.femurJoint,ActuatorAngle2,speed);
    MoveDynamixel(frontLeftLeg.tibiaJoint,ActuatorAngle3,speed);
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
    
    
    MoveDynamixel(frontRightLeg.coxaJoint,ActuatorAngle1,speed);
    MoveDynamixel(frontRightLeg.femurJoint,ActuatorAngle2,speed);
    MoveDynamixel(frontRightLeg.tibiaJoint,ActuatorAngle3,speed);
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
    
    
    MoveDynamixel(rearLeftLeg.coxaJoint,ActuatorAngle1,speed);
    MoveDynamixel(rearLeftLeg.femurJoint,ActuatorAngle2,speed);
    MoveDynamixel(rearLeftLeg.tibiaJoint,ActuatorAngle3,speed);
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
    
    
    MoveDynamixel(rearRightLeg.coxaJoint,ActuatorAngle1,speed);
    MoveDynamixel(rearRightLeg.femurJoint,ActuatorAngle2,speed);
    MoveDynamixel(rearRightLeg.tibiaJoint,ActuatorAngle3,speed);
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


int isMovementInstruction(int instruction)
{
	if ((0b11000000 & instruction) == 0b00000000)
	{
		return TRUE;
	}
	return FALSE;
}

int isOptionInstruction(int instruction)
{
	if ((0b11000000 & instruction) == 0b10000000)
	{
		return TRUE;
	}
	return FALSE;
}

int isControlModeInstruction(int instruction)
{
	if ((0b11000000 & instruction) == 0b11000000)
	{
		return TRUE;
	}
	return FALSE;
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
	else if (isOptionInstruction(instruction))
	{
		optionsHasChanged_g = 1;
		currentOptionInstruction_g = instruction;
	}
	else if (isControlModeInstruction(instruction))
	{
		switch(instruction)
		{
			case EXPLORATION_MODE_INSTRUCTION:
			{
				currentControlMode_g = exploration;
				break;
			}
			case MANUAL_MODE_INSTRUCTION:
			{
				currentControlMode_g = manual;
				break;
			}
			case RETURN_TO_LEAK_1:
			{
				break;
			}
			case RETURN_TO_LEAK_2:
			{
				break;
			}
			case RETURN_TO_LEAK_3:
			{
				break;
			}
			case RETURN_TO_LEAK_4:
			{
				break;
			}
			case RETURN_TO_LEAK_5:
			{
				break;
			}
		}
	}
}

int toggleMania = FALSE;
int sendStuff = FALSE;
ISR(INT1_vect) 
{ 
    
	if (toggleMania)
    {
        currentControlMode_g = manual;
        sendStuff = TRUE;
        toggleMania = FALSE;
    }
    else
    {
        currentControlMode_g = exploration;
        toggleMania = TRUE;
        
    }
	
	/*
    directionHasChanged = 1;
    switch(currentDirection_g)
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
        case noDirection:
        {
            currentDirectionInstruction = NORTH_HEADER;
            break;
        }
    }*/
    
    
    
    
} 






void transmitAllDataToCommUnit()
{
    transmitDataToCommUnit(DISTANCE_NORTH, distanceValue_g[NORTH]);
    transmitDataToCommUnit(DISTANCE_EAST, distanceValue_g[EAST]);
    transmitDataToCommUnit(DISTANCE_SOUTH, distanceValue_g[SOUTH]);
    transmitDataToCommUnit(DISTANCE_WEST, distanceValue_g[WEST]);
    transmitDataToCommUnit(ANGLE_NORTH, angleValue_g[NORTH]);
    transmitDataToCommUnit(ANGLE_EAST, angleValue_g[EAST]);
    transmitDataToCommUnit(ANGLE_SOUTH, angleValue_g[SOUTH]);
    transmitDataToCommUnit(ANGLE_WEST, angleValue_g[WEST]);
    transmitDataToCommUnit(LEAK_HEADER, fetchDataFromSensorUnit(LEAK_HEADER));
    transmitDataToCommUnit(TOTAL_ANGLE, fetchDataFromSensorUnit(TOTAL_ANGLE));
    return;
}

void updateAllDistanceSensorData()
{
    distanceValue_g[NORTH] = fetchDataFromSensorUnit(DISTANCE_NORTH);
    distanceValue_g[EAST] = fetchDataFromSensorUnit(DISTANCE_EAST);
    distanceValue_g[SOUTH] = fetchDataFromSensorUnit(DISTANCE_SOUTH);
    distanceValue_g[WEST] = fetchDataFromSensorUnit(DISTANCE_WEST);
}

void updateTotalAngle()
{
    angleValue_g[TOTAL] = fetchDataFromSensorUnit(TOTAL_ANGLE);
}

void getSensorData(enum direction regulationDirection)
{
    /*
    * Sensordata fyller de globala arrayerna distanceValue_g, angleValue_g
    */
    /* Kommenterar ut för att testa om det är fel på datahämtningen eller ej
    distanceValue_g[NORTH] = fetchDataFromSensorUnit(DISTANCE_NORTH);
    distanceValue_g[EAST] = fetchDataFromSensorUnit(DISTANCE_EAST);
    distanceValue_g[SOUTH] = fetchDataFromSensorUnit(DISTANCE_SOUTH);
    distanceValue_g[WEST] = fetchDataFromSensorUnit(DISTANCE_WEST);

    angleValue_g[NORTH] = fetchDataFromSensorUnit(ANGLE_NORTH);
    angleValue_g[EAST] = fetchDataFromSensorUnit(ANGLE_EAST);
    angleValue_g[SOUTH] = fetchDataFromSensorUnit(ANGLE_SOUTH);
    angleValue_g[WEST] = fetchDataFromSensorUnit(ANGLE_WEST);
	*/
	
	
	// test för att inte hämta så mycket sensordata
	switch(regulationDirection)
	{
		case north:
		{
			distanceValue_g[regulationDirection] = fetchDataFromSensorUnit(DISTANCE_NORTH);
			angleValue_g[regulationDirection] = fetchDataFromSensorUnit(ANGLE_NORTH);
			break;
		}
		
		case east:
		{
			distanceValue_g[regulationDirection] = fetchDataFromSensorUnit(DISTANCE_EAST);
			angleValue_g[regulationDirection] = fetchDataFromSensorUnit(ANGLE_EAST);
			break;
		}
		
		case south:
		{
			distanceValue_g[regulationDirection] = fetchDataFromSensorUnit(DISTANCE_SOUTH);
			angleValue_g[regulationDirection] = fetchDataFromSensorUnit(ANGLE_SOUTH);
			break;
		}
		
		case west:
		{
			distanceValue_g[regulationDirection] = fetchDataFromSensorUnit(DISTANCE_WEST);
			angleValue_g[regulationDirection] = fetchDataFromSensorUnit(ANGLE_WEST);
			break;
		}
	}
	return;
}


void calcRegulation(enum direction regulationDirection, int useRotateRegulation)
{
    // regulationDirection motsvarar den sida som regleringen ska titta på.
    /*
    * 
    *
    * Första värdet anger hur mycket åt höger relativt rörelseriktningen roboten ska ta sig i varje steg.
    * Andra värdet anger hur mycket längre steg benen på vänstra sidan relativt rörelseriktningen ska ta, de som medför en CW-rotation. 
    * Tredje värdet anger hur mycket längre steg benen på högra sidan relativt rörelseriktningen ska ta, de som medför en CCW-rotation
    */
	

	int translationRight = 0;
	

	// variablerna vi baserar regleringen på, skillnaden mellan aktuellt värde och önskat värde
	int translationRegulationError = 0; // avser hur långt till vänster ifrån mittpunkten av "vägen" vi är
	int angleRegulationError = 0; // avser hur många grader vridna i CCW riktning relativt "den raka riktningen" dvs relativt väggarna.





		//Reglering vid gång i koridor
	switch (currentDirection_g)
	{
		case north: // går vi åt norr kan vi reglera mot väst eller öster
		{
			switch(regulationDirection)
			{
				case east:
				{
					translationRegulationError = (distanceValue_g[east] + sensorOffset_g) - halfPathWidth_g; // translationRegulationError avser hur långt till vänster om mittlinjen vi är 
					break;
				}

				case west:
				{
					translationRegulationError = halfPathWidth_g - (distanceValue_g[west] + sensorOffset_g);
					break;
				}
				case noDirection:
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
					translationRegulationError = (distanceValue_g[south] + sensorOffset_g) - halfPathWidth_g;
					break;
				}

				case north:
				{
					translationRegulationError = halfPathWidth_g - (distanceValue_g[north] + sensorOffset_g);
					break;
				}

				case noDirection:
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
					translationRegulationError = (distanceValue_g[west] + sensorOffset_g) - halfPathWidth_g;
					break;
				}

				case east:
				{
					translationRegulationError = halfPathWidth_g - (distanceValue_g[east] + sensorOffset_g);
					break;
				}

				case noDirection:
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
					translationRegulationError = (distanceValue_g[north] + sensorOffset_g) - halfPathWidth_g;
					break;
				}

				case south:
				{
					translationRegulationError = halfPathWidth_g - (distanceValue_g[south] + sensorOffset_g);
					break;
				}

				case noDirection:
				{
					translationRegulationError = 0;
					angleRegulationError = 0;
	    			break;
				}
			}
			break;
		}
        case noDirection:
        {
            translationRegulationError = 0;
            angleRegulationError = 0;
            break;
        }            
	}

	if (useRotateRegulation)
	{
		angleRegulationError = angleValue_g[TOTAL];
	}
	else
	{
		angleRegulationError = 0;
	}

	//Regleringskoefficienter


	translationRight = kProportionalTranslation_g * translationRegulationError;
	int leftSideStepLengthAdjust = (kProportionalAngle_g * angleRegulationError)/2; // om roboten ska rotera åt höger så låter vi benen på vänster sida ta längre steg och benen på höger sida ta kortare steg
	int rightSideStepLengthAdjust = kProportionalAngle_g * (angleRegulationError)/2; // eftersom angleRegulationError avser hur mycket vridet åt vänster om mittlinjen roboten är  
		
	if (translationRight > 60)
	{
		translationRight = 60;
	}
		
		
	if (translationRight < -60)
	{
		translationRight = -60;
	}
		
	if (leftSideStepLengthAdjust > 60)
	{
		leftSideStepLengthAdjust = 60;
	}
	
	if (leftSideStepLengthAdjust < -60)
	{
		leftSideStepLengthAdjust = -60;
	}
	
	if (rightSideStepLengthAdjust > 60)
	{
		rightSideStepLengthAdjust = 60;
	}
	
	if (rightSideStepLengthAdjust < -60)
	{
		rightSideStepLengthAdjust = -60;
	}
		
		
		
		
	regulation_g[0] =  translationRight;
	regulation_g[1] = leftSideStepLengthAdjust;
	regulation_g[2] = rightSideStepLengthAdjust;
		
	needToCalcGait = 1; // när vi har reglerat behöver vi räkna om gångstilen
	return;
}


 /*
 / Beslutar utifrån currentNode_g och currentDirection vilken riktning som vi kan 
 / använda för reglering mot. Returnerar enum direction som kan användas i calcRegulation
*/
int decideRegulationDirection() 
{
    enum direction tempRegulationDirection = noDirection;
    int northAvailable = currentNode_g.northAvailible;
    int eastAvailable = currentNode_g.eastAvailible;
    int southAvailable = currentNode_g.southAvailible;
    int westAvailable = currentNode_g.westAvailible;
    switch(currentDirection_g)
    {
        case north:
        {
            if (!eastAvailable) // det finns en vägg åt öster
            {
                tempRegulationDirection = east;
            }                
            else if (!westAvailable)
            {
                tempRegulationDirection = west;
            }              
            break;
        }
        case east:
        {
            if (!southAvailable)
            {
                tempRegulationDirection = south;
            }
            else if (!northAvailable)
            {
                tempRegulationDirection = north;
            }
            break;
        }
        case south:
        {
            if (!eastAvailable)
            {
                tempRegulationDirection = east;
            }
            else if (!westAvailable)
            {
                tempRegulationDirection = west;
            }
            break;
        }
        case west:
        {
            if (!northAvailable)
            {
                tempRegulationDirection = north;
            }
            else if (!southAvailable)
            {
                tempRegulationDirection = south;
            }
            break;
        }
        case noDirection:
        {
            tempRegulationDirection = noDirection;
            break;
        }
    }
    return tempRegulationDirection;
}


// används som en globalvariabel för att veta vilken typ av "action" vi ska göra.
enum order{
	noOrder, // inte utföra något
	turnBlind,
	turnSeeing
};
enum order currentOrder_g = noOrder;

// en funktion som utför det currentAction_g anger
void applyOrder()
{
	if(currentOrder_g == turnBlind)
	{
		BlindStepsTaken_g = BlindStepsTaken_g + 1;
		if(BlindStepsTaken_g >= BlindStepsToTake_g)
		{
			currentDirection_g = nextDirection_g;
			currentOrder_g = noOrder;
			BlindStepsTaken_g = 0;
		}
	}
	if(currentOrder_g == turnSeeing)	
	{
		closeEnoughToTurn_g = distanceValue_g[currentDirection_g] < (stepLength_g/2 + halfPathWidth_g);	
		if (closeEnoughToTurn_g)
		{
			currentOrder_g = noOrder;
			currentDirection_g = nextDirection_g;
		}
	}
	return;
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

// cycleResolution måste vara delbart med 8
void makeCreepGait(int cycleResolution)
{
    int res = cycleResolution/4;
    maxGaitCyclePos_g = cycleResolution -1;
    posToCalcGait = maxGaitCyclePos_g;
    long int sixth = stepLength_g/6;
    long int half = stepLength_g/2;
    switch(currentDirection_g)
    {
        case north:
        {
            CalcCurvedPath(rearRightLeg, res, 0, startPositionX_g, -startPositionY_g-half, startPositionZ_g, startPositionX_g, -startPositionY_g+half, startPositionZ_g);
            CalcStraightPath(rearRightLeg, res*3, res, startPositionX_g, -startPositionY_g+half, startPositionZ_g, startPositionX_g, -startPositionY_g-half, startPositionZ_g);

            CalcStraightPath(frontRightLeg, res, 0, startPositionX_g, startPositionY_g-sixth, startPositionZ_g, startPositionX_g, startPositionY_g-half, startPositionZ_g);
            CalcCurvedPath(frontRightLeg, res, res, startPositionX_g, startPositionY_g-half, startPositionZ_g, startPositionX_g, startPositionY_g+half, startPositionZ_g);
            CalcStraightPath(frontRightLeg, 2*res, 2*res, startPositionX_g, startPositionY_g+half, startPositionZ_g, startPositionX_g, startPositionY_g-sixth, startPositionZ_g);

            CalcStraightPath(rearLeftLeg, 2*res, 0, -startPositionX_g, -startPositionY_g+sixth, startPositionZ_g, -startPositionX_g, -startPositionY_g-half, startPositionZ_g);
            CalcCurvedPath(rearLeftLeg, res, 2*res, -startPositionX_g, -startPositionY_g-half, startPositionZ_g, -startPositionX_g, -startPositionY_g+half, startPositionZ_g);
            CalcStraightPath(rearLeftLeg, res, 3*res, -startPositionX_g, -startPositionY_g+half, startPositionZ_g, -startPositionX_g, -startPositionY_g+sixth, startPositionZ_g);

            CalcStraightPath(frontLeftLeg, 3*res, 0, -startPositionX_g, startPositionY_g+half, startPositionZ_g, -startPositionX_g, startPositionY_g-half, startPositionZ_g);
            CalcCurvedPath(frontLeftLeg, res, 3*res, -startPositionX_g, startPositionY_g-half, startPositionZ_g, -startPositionX_g, startPositionY_g+half, startPositionZ_g);
            break;
        }
        case east:
        {
            CalcCurvedPath(rearLeftLeg, res, 0, -startPositionX_g - half, -startPositionY_g, startPositionZ_g, -startPositionX_g + half, -startPositionY_g, startPositionZ_g);
            CalcStraightPath(rearLeftLeg, res*3, res, -startPositionX_g + half, -startPositionY_g, startPositionZ_g, -startPositionX_g - half, -startPositionY_g, startPositionZ_g);

            CalcStraightPath(rearRightLeg, res, 0, startPositionX_g-sixth, -startPositionY_g, startPositionZ_g, startPositionX_g-half, -startPositionY_g, startPositionZ_g);
            CalcCurvedPath(rearRightLeg, res, res, startPositionX_g-half, -startPositionY_g, startPositionZ_g, startPositionX_g+half, -startPositionY_g, startPositionZ_g);
            CalcStraightPath(rearRightLeg, 2*res, 2*res, startPositionX_g+half, -startPositionY_g, startPositionZ_g, startPositionX_g-sixth, -startPositionY_g, startPositionZ_g);

            CalcStraightPath(frontLeftLeg, 2*res, 0, -startPositionX_g+sixth, startPositionY_g, startPositionZ_g, -startPositionX_g-half, startPositionY_g, startPositionZ_g);
            CalcCurvedPath(frontLeftLeg, res, 2*res, -startPositionX_g-half, startPositionY_g, startPositionZ_g, -startPositionX_g+half, startPositionY_g, startPositionZ_g);
            CalcStraightPath(frontLeftLeg, res, 3*res, -startPositionX_g+half, startPositionY_g, startPositionZ_g, -startPositionX_g+sixth, startPositionY_g, startPositionZ_g);

            CalcStraightPath(frontRightLeg, 3*res, 0, startPositionX_g+half, startPositionY_g, startPositionZ_g, startPositionX_g-half, startPositionY_g, startPositionZ_g);
            CalcCurvedPath(frontRightLeg, res, 3*res, startPositionX_g-half, startPositionY_g, startPositionZ_g, startPositionX_g+half, startPositionY_g, startPositionZ_g);
            break;
        }
        case south:
        {
            CalcCurvedPath(frontLeftLeg, res, 0, -startPositionX_g, startPositionY_g+half, startPositionZ_g, -startPositionX_g, startPositionY_g-half, startPositionZ_g);
            CalcStraightPath(frontLeftLeg, res*3, res, -startPositionX_g, startPositionY_g-half, startPositionZ_g, -startPositionX_g, startPositionY_g+half, startPositionZ_g);

            CalcStraightPath(rearLeftLeg, res, 0, -startPositionX_g, -startPositionY_g+sixth, startPositionZ_g, -startPositionX_g, -startPositionY_g+half, startPositionZ_g);
            CalcCurvedPath(rearLeftLeg, res, res, -startPositionX_g, -startPositionY_g+half, startPositionZ_g, -startPositionX_g, -startPositionY_g-half, startPositionZ_g);
            CalcStraightPath(rearLeftLeg, 2*res, 2*res, -startPositionX_g, -startPositionY_g-half, startPositionZ_g, -startPositionX_g, -startPositionY_g+sixth, startPositionZ_g);

            CalcStraightPath(frontRightLeg, 2*res, 0, startPositionX_g, startPositionY_g-sixth, startPositionZ_g, startPositionX_g, startPositionY_g+half, startPositionZ_g);
            CalcCurvedPath(frontRightLeg, res, 2*res, startPositionX_g, startPositionY_g+half, startPositionZ_g, startPositionX_g, startPositionY_g-half, startPositionZ_g);
            CalcStraightPath(frontRightLeg, res, 3*res, startPositionX_g, startPositionY_g-half, startPositionZ_g, startPositionX_g, startPositionY_g-sixth, startPositionZ_g);

            CalcStraightPath(rearRightLeg, 3*res, 0, startPositionX_g, -startPositionY_g-half, startPositionZ_g, startPositionX_g, -startPositionY_g+half, startPositionZ_g);
            CalcCurvedPath(rearRightLeg, res, 3*res, startPositionX_g, -startPositionY_g+half, startPositionZ_g, startPositionX_g, -startPositionY_g-half, startPositionZ_g);
            break;
        }
        case west:
        {
            CalcCurvedPath(frontRightLeg, res, 0, startPositionX_g + half, startPositionY_g, startPositionZ_g, startPositionX_g - half, startPositionY_g, startPositionZ_g);
            CalcStraightPath(frontRightLeg, res*3, res, startPositionX_g - half, startPositionY_g, startPositionZ_g, startPositionX_g + half, startPositionY_g, startPositionZ_g);

            CalcStraightPath(frontLeftLeg, res, 0, -startPositionX_g+sixth, startPositionY_g, startPositionZ_g, -startPositionX_g+half, startPositionY_g, startPositionZ_g);
            CalcCurvedPath(frontLeftLeg, res, res, -startPositionX_g+half, startPositionY_g, startPositionZ_g, -startPositionX_g-half, startPositionY_g, startPositionZ_g);
            CalcStraightPath(frontLeftLeg, 2*res, 2*res, -startPositionX_g-half, startPositionY_g, startPositionZ_g, -startPositionX_g+sixth, startPositionY_g, startPositionZ_g);

            CalcStraightPath(rearRightLeg, 2*res, 0, startPositionX_g-sixth, -startPositionY_g, startPositionZ_g, startPositionX_g+half, -startPositionY_g, startPositionZ_g);
            CalcCurvedPath(rearRightLeg, res, 2*res, startPositionX_g+half, -startPositionY_g, startPositionZ_g, startPositionX_g-half, -startPositionY_g, startPositionZ_g);
            CalcStraightPath(rearRightLeg, res, 3*res, startPositionX_g-half, -startPositionY_g, startPositionZ_g, startPositionX_g-sixth, -startPositionY_g, startPositionZ_g);

            CalcStraightPath(rearLeftLeg, 3*res, 0, -startPositionX_g-half, -startPositionY_g, startPositionZ_g, -startPositionX_g+half, -startPositionY_g, startPositionZ_g);
            CalcCurvedPath(rearLeftLeg, res, 3*res, -startPositionX_g+half, -startPositionY_g, startPositionZ_g, -startPositionX_g-half, -startPositionY_g, startPositionZ_g);
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

// gör rörelsemönstret för travet, beror på vilken direction som är satt på currentDirection_g
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

    switch(currentDirection_g)
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

        case noDirection:
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
	if (startPositionZ_g > -180)
	{
		startPositionZ_g = startPositionZ_g - 2;
	}
	return;
}

void increaseStepLength()
{
	if (stepLength_g < 160)
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
	if ((newGaitResolutionTime < 100) && (newGaitResolutionTime >= 30))
	{
		newGaitResolutionTime = newGaitResolutionTime + 10;
		setTimerPeriod(TIMER_0, newGaitResolutionTime);
	}
    else if((newGaitResolutionTime >= 100) && (newGaitResolutionTime < 500))
    {
        newGaitResolutionTime = newGaitResolutionTime + 100;
        setTimerPeriod(TIMER_0, newGaitResolutionTime);
    }
	return;
}

void decreaseGaitResolutionTime()
{
	if ((newGaitResolutionTime > 30) && (newGaitResolutionTime <= 100))
	{
		newGaitResolutionTime = newGaitResolutionTime - 10;
		setTimerPeriod(TIMER_0, newGaitResolutionTime);
	}
    else if((newGaitResolutionTime > 100) && (newGaitResolutionTime <= 500))
    {
		
        newGaitResolutionTime = newGaitResolutionTime - 100;
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

void increaseKTranslation()
{
	if (kProportionalTranslation_g < 1)
	{
		kProportionalTranslation_g = kProportionalTranslation_g + 0.1;
	}
	return;
}

void decreaseKTranslation()
{
	if (kProportionalTranslation_g > 0.1)
	{
		kProportionalTranslation_g = kProportionalTranslation_g - 0.1;
	}
	return;
}

void increaseKRotation()
{
	if (kProportionalAngle_g < 1)
	{
		kProportionalAngle_g = kProportionalAngle_g + 0.1;
	}
	return;
}

void decreaseKRotation()
{
	if (kProportionalAngle_g > -1)
	{
		kProportionalAngle_g = kProportionalAngle_g - 0.1;
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

int frontAvailable()
{
	int northAvailable = currentNode_g.northAvailible;
	int eastAvailable = currentNode_g.eastAvailible;
	int southAvailable = currentNode_g.southAvailible;
	int westAvailable = currentNode_g.westAvailible;
	
	switch (currentDirection_g)
	{
		case north:
		{
			return northAvailable;
			break;
		}
		
		case east:
		{
			return eastAvailable;
			break;
		}
		
		case south:
		{
			return southAvailable;
			break;
		}
		
		case west:
		{
			return westAvailable;
			break;
		}
		
		case noDirection:
		{
			return 0;
			break;
		}
	}
}


void gaitController()
{
	
	// tillfälligt eftersom dessa var lokala i en annan funktion, borde flyttas ut och göras globala och uppdateras bra
	
	
	if ((currentPos_g == posToCalcGait) && (currentControlMode_g != manual)) // hämtar information från sensorenheten varje gång det är dags att beräkna gången
	{
		calcRegulation(decideRegulationDirection(), TRUE);
		applyOrder();
	}

    if((currentPos_g == posToCalcGait) && (needToCalcGait))
    {
       
        switch(currentGait)
        {
            case standStill:
            {
                standStillGait();
                break;
            }
            case trotGait:
            {
    
                MakeTrotGait(gaitResolution_g);
                break;
            }
            case creepGait:
            {
                makeCreepGait(gaitResolution_g);
                break;
            }
        }
        needToCalcGait = FALSE;
    }
    if (currentControlMode_g == manual)
    {
        if (directionHasChanged)
        {
            directionHasChanged = FALSE;
            needToCalcGait = TRUE;
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
                    if ((currentDirection_g == noDirection) && (currentGait == trotGait))
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
                    if (currentGait == standStill)
                    {
                        transitionStartToTrot();
                    }
                    currentGait = trotGait;
                    currentDirection_g = north;
                    regulation_g[0] = 0;
                    break;
                }
                case NORTH_EAST_HEADER:
                {
                    if (currentGait == standStill)
                    {
                        transitionStartToTrot();
                    }
                    // Huvudsaklig rörelseriktning norr. Östlig offset hanteres genom en hårdkodad reglering åt höger
                    currentGait = trotGait;
                    currentDirection_g = north;
                    regulation_g[0] = stepLength_g;
                    break;
                }
                case EAST_HEADER:
                {
                    if (currentGait == standStill)
                    {
                        transitionStartToTrot();
                    }
                    currentGait = trotGait;
                    currentDirection_g = east;
                    regulation_g[0] = 0;
                    break;
                    }
                case SOUTH_EAST_HEADER:
                {
                    if (currentGait == standStill)
                    {
                        transitionStartToTrot();
                    }
                    // Huvudsaklig rörelseriktning öster. Sydlig offset hanteres genom en hårdkodad reglering åt höger
                    currentGait = trotGait;
                    currentDirection_g = east;
                    regulation_g[0] = stepLength_g;
                    break;
                }
                case SOUTH_HEADER:
                {
                    if (currentGait == standStill)
                    {
                        transitionStartToTrot();
                    }
                    currentGait = trotGait;
                    currentDirection_g = south;
                    regulation_g[0] = 0;
                    break;
                }
                case SOUTH_WEST_HEADER:
                {
                    if (currentGait == standStill)
                    {
                        transitionStartToTrot();
                    }
                    // Huvudsaklig rörelseriktning söder. Västlig offset hanteres genom en hårdkodad reglering åt höger
                    currentGait = trotGait;
                    currentDirection_g = south;
                    regulation_g[0] = stepLength_g;
                    break;
                }
                case WEST_HEADER:
                {
                    if (currentGait == standStill)
                    {
                        transitionStartToTrot();
                    }
                    currentGait = trotGait;
                    currentDirection_g = west;
                    regulation_g[0] = 0;
                    break;
                }
                case NORTH_WEST_HEADER:
                {
                    if (currentGait == standStill)
                    {
                        transitionStartToTrot();
                    }
                    // Huvudsaklig rörelseriktning väster. Nordlig offset hanteres genom en hårdkodad reglering åt höger
                    currentGait = trotGait;
                    currentDirection_g = west;
                    regulation_g[0] = stepLength_g;
                    break;
                }
                case NO_MOVEMENT_DIRECTION_HEADER:
                {
                    if(currentRotationInstruction == NO_ROTATION)
                    {
                        returnToStartPosition();
                        currentGait = standStill;
                    }
                    currentDirection_g = noDirection;
                    regulation_g[0] = 0;
                    break;
                }
            }
        }
	}
    else // Controlmode är något annat än manuellt
    {
        if(directionHasChanged) // nodesAndControl har tagit ett styrbeslut
        {
            directionHasChanged = FALSE;
            needToCalcGait = TRUE;
			if (currentGait == standStill)
			{
				transitionStartToTrot();
			}
			currentGait = trotGait;
            if(currentOrder_g == noOrder) // kan bara få en ny order om vi inte redan har någon
		    {
				if (nextDirection_g == noDirection)
				{
					returnToStartPosition();
					currentGait = standStill;
					currentDirection_g = noDirection;
				}
				else
				{	
					if (nextDirection_g == currentDirection_g || (nextDirection_g == ((currentDirection_g + 2) % 4))) // undersöker om nästa riktning är fram eller bak relativt nuvarande riktning
					{
						currentDirection_g = nextDirection_g; // om fram eller bak så är det bar att byta
					}
					else 
					{
						if (frontAvailable()) // om vänster eller höger rel. nuvarande riktning måste vi undersöka om vi ska svänga blint eller med syn
						{
							currentOrder_g = turnBlind;
						}
						else
						{
							currentOrder_g = turnSeeing;
						}
					}
				}
			}
        }
    }
    if(optionsHasChanged_g == 1)
    {
        optionsHasChanged_g = FALSE;
        needToCalcGait = TRUE;
        switch (currentOptionInstruction_g)
        {
            case INCREASE_LEG_WIDTH:
            {
                increaseLegWidth();
                sendLegWidth = TRUE;
                break;
            }
            
            case DECREASE_LEG_WIDTH:
            {
                decreaseLegWidth();
                sendLegWidth = TRUE;
                break;
            }
            
            case INCREASE_LEG_HEIGHT:
            {
                increaseLegHeight();
                sendRobotHeight = TRUE;
                break;
            }
            
            case DECREASE_LEG_HEIGHT:
            {
                decreaseLegHeight();
                sendRobotHeight = TRUE;
                break;
            }
            
            case INCREASE_STEP_LENGTH:
            {
                increaseStepLength();
                sendStepLength = TRUE;
                break;
            }
            
            case DECREASE_STEP_LENGTH:
            {
                decreaseStepLength();
                sendStepLength = TRUE;
                break;
            }
            
            case INCREASE_STEP_HEIGHT:
            {
                increaseStepHeight();
                sendStepHeight = TRUE;
                break;
            }
            
            case DECREASE_STEP_HEIGHT:
            {
                decreaseStepHeight();
                sendStepHeight = TRUE;
                break;
            }
            
            case INCREASE_GAIT_RESOLUTION_TIME:
            {
                increaseGaitResolutionTime();
                sendGaitResTime = TRUE;
                break;
            }
            
            case DECREASE_GAIT_RESOLUTION_TIME:
            {
                decreaseGaitResolutionTime();
                sendGaitResTime = TRUE;
                break;
            }
            
            case INCREASE_K_TRANSLATION:
            {
                increaseKTranslation();
                sendKPTrans = TRUE;
                break;
            }
            
            case DECREASE_K_TRANSLATION:
            {
                decreaseKTranslation();
                sendKPTrans = TRUE;
                break;
            }
            
            case INCREASE_K_ROTATION:
            {
                increaseKRotation();
                sendKPAngle = TRUE;
                break;
            }
            
            case DECREASE_K_ROTATION:
            {
                decreaseKRotation();
                sendKPAngle = TRUE;
                break;
            }
        }
    }
}

void sendChangedRobotParameters()
{
    if (sendLegWidth)
    {
        transmitDataToCommUnit(LEG_DISTANCE_HEADER, startPositionX_g);
        sendLegWidth = FALSE;
    }        
    if (sendStepHeight)
    {
        transmitDataToCommUnit(STEP_HEIGHT_HEADER, stepHeight_g);
        sendStepHeight = FALSE;
    }
    if (sendRobotHeight)
    {
        transmitDataToCommUnit(ROBOT_HEIGHT_HEADER, -startPositionZ_g);
        sendRobotHeight = FALSE;
    }
    if (sendStepLength) 
    {
        transmitDataToCommUnit(STEP_LENGTH_HEADER, stepLength_g);
        sendStepLength = FALSE;
    }
    if (sendKPAngle) 
    {
        transmitDataToCommUnit(K_P_ANGLE_HEADER, 100*kProportionalAngle_g);
        sendKPAngle = FALSE;
    }
    if (sendKPTrans) 
    {
        transmitDataToCommUnit(K_P_TRANS_HEADER, 100*kProportionalTranslation_g);
        sendKPTrans = FALSE;
    }
    if (sendGaitResTime) 
    {
        transmitDataToCommUnit(STEP_INCREMENT_TIME_HEADER, gaitResolutionTime_g);
        sendGaitResTime = FALSE;
    }
}

void checkForLeak()
{
	isLeakVisible_g = fetchDataFromSensorUnit(LEAK_HEADER);
}
int tooCloseToFrontWall()
{
	if(currentDirection_g == noDirection)
	{
		return 0;
	}
	if(distanceValue_g[currentDirection_g] + sensorOffset_g < 285)//(stepLength_g/2 + halfPathWidth_g))
	{
		return 1;
	}
	else
	{
		return 0;
	}
}
void emergencyStop()
{
	transitionStartToTrot();
	cli();
	_delay_ms(4000);
	sei();
}
int main(void)
{
	initUSART();
    spiMasterInit();
    EICRA = 0b1111; // Stigande flank på INT1/0 genererar avbrott
    EIMSK = (EIMSK | 3); // Möjliggör externa avbrott på INT1/0

    // initiering av globala variabler
    currentPos_g = 0;
    nextPos_g = 1;
    currentControlMode_g = manual;
    currentDirection_g = north;
	nextDirection_g = north;
    currentOptionInstruction_g = 0;
    initNodeAndSteering();
    directionHasChanged = FALSE;
    currentGait = standStill;
    optionsHasChanged_g = 0;
    BlindStepsToTake_g = (int)((halfPathWidth_g - 8)/stepLength_g + 0.5);
	
    int nodeAdded = FALSE;
	int sendDataToPC = FALSE; // Används för att bara skicka varannan gång i commPeriodTimerEnd
    timer0Init();
    timer2Init();
    sei();
    MoveToStartPosition();
    //moveToCreepStartPosition();
    _delay_ms(1000);
    //currentDirection_g = east;
    //makeCreepGait(gaitResolution_g);
    standStillGait();

    //currentGait = creepGait;

    //moveToCreepStartPosition();
    //makeCreepGait(gaitResolution_g);
    setTimerPeriod(TIMER_0, newGaitResolutionTime);
    setTimerPeriod(TIMER_2, newCommUnitUpdatePeriod);
    
	sendAllRobotParameters();


    // ---- Main-Loop ----
    while (1)
    {
	    
    	if (legTimerPeriodEnd())
    	{
			move();
    		resetLegTimer(); 
			gaitController();
		}
    	if (commTimerPeriodEnd())
    	{
			updateAllDistanceSensorData();
			if(tooCloseToFrontWall() && currentControlMode_g == exploration) // Kollar om roboten kommit för nära väggen framåt och avbryter rörelsen framåt
			{
				emergencyStop();
			}
            updateTotalAngle();
			checkForLeak();
            sendChangedRobotParameters();
            if (sendDataToPC)
			{
				transmitDataToCommUnit(NODE_INFO, makeNodeData(&currentNode_g));
				transmitDataToCommUnit(CONTROL_DECISION,nextDirection_g);
				transmitAllDataToCommUnit();
				sendDataToPC = FALSE;
			}
			else
			{
				sendDataToPC = TRUE;
			}
            /*
			if (sendStuff && i < 120)
            {
                transmitDataToCommUnit(NODE_INFO, makeNodeData(&nodeArray[i]));
                i++;
                         
            }
            else
                transmitDataToCommUnit(NODE_INFO, makeNodeData(&currentNode_g));
				*/
            resetCommTimer();
    	}
        /*
        nodesAndControl sätter nextDirection och directionHasChanged om ett styrbeslut tas.
        Kan dessutom ändra på controlMode.
        */
        
        if (currentControlMode_g != manual)
        {
            //if (nodesAndControl()) // Funktionen returnerar TRUE om en ny nod lagts till.
              //  nodeAdded = TRUE;
            nodesAndControl();
        }
               
    }
}



 
