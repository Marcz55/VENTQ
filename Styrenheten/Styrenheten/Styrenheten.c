/*
 * Styrenheten.c
 *
 * Created: 3/26/2015 11:50:32 AM
 * Author: Joakim M�rhed
 * Version: 1.0
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

int halfPathWidth_g = 570/2; // Avst�ndet mellan v�ggar
int regulation_g[3]; // array som regleringen sparas i

int closeEnoughToTurn = 340;
int rangeToShortenStepLength_g = 0;	

// Emergency lockdown
#define EMERGENCY_LOCKDOWN_DISTANCE 280
int tooCloseLastTime_g = FALSE;
int emergencyDowntime_g = 0;
int emergencyLockdown_g = FALSE;

int stepLength_g = 70;
int startPositionX_g = 80;
int startPositionY_g = 80;
int startPositionZ_g = -90;
int stepHeight_g =  16;
int gaitResolution_g = 12; // M�STE VARA DELBART MED 4 vid trot, 8 vid creep
int stepLengthRotationAdjust = 60;
int newGaitResolutionTime = INCREMENT_PERIOD_50; // tid i timerloopen f�r benstyrningen i ms


int currentDirectionInstruction = 0; // Nuvarande manuell styrinstruktion
int currentRotationInstruction = 0;
int optionsHasChanged_g = 0;
int posToCalcGait;
int needToCalcGait = 1;

// ------ Globala variabler f�r "sv�ngar" ------
int BlindStepsTaken_g = 0;
//int BlindStepsToTake_g = 5; // Avst�nd fr�n sensor till mitten av robot ~= 8 cm.
// BlindStepsToTake ska vara ungef�r (halfPathWidth - 8)/practicalStepLength
int BlindStepsToTake_g = 2;
int savedSteplength_g;
int hasSavedSteplength_g = FALSE;
#define BLIND_STEP_LENGTH 80

// ------ Inst�llningar f�r robot-datorkommunikation ------
int newCommUnitUpdatePeriod = INCREMENT_PERIOD_40;

// regler koefficienter
float kProportionalTranslation_g = 0.3;
float kProportionalAngle_g = 2.0;

// hanterar om vi har f�rkortat stegl�ngden eller ej
int stepLengthShortened_g = FALSE;

// hanterar fallen d� vi g�r i manuellt l�ge och g�r diagonalt, ser till att vi normerar stegl�ngden
int diagonalMovement_g = FALSE;

// avst�nd ifr�n sensorera till mitten av roboten (mm)
int sensorOffset_g = 60;


int sendLegWidth = FALSE;
int sendStepHeight = FALSE;
int sendRobotHeight = FALSE;
int sendStepLength = FALSE;
int sendKPAngle = FALSE;
int sendKPTrans = FALSE;
int sendGaitResTime = FALSE;

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

/*
 * Skickar robotparametrarna till kommunikationsenheten. 
 */
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


/*
 * calcDynamixelSpeed anv�nder legIncrementPeriod_g och f�rflyttningsstr�ckan f�r att ber�kna en 
 * hastighet som g�r att slutpositionen uppn�s p� periodtiden.
 */
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



/*
 * Flyttar ett specifikt servo till en given vinkel med en specifik hastighet.
 */
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

/*
 * Flyttar benet till en kartesisk punkt med en viss hastighet. 
 * Anledningen till att det finns olika funktioner f�r de olika benen �r att servona �r
 * monterade p� olika h�ll samt att servoaxeln har olika "offset".
 */
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

/*
 * Flyttar benet till en kartesisk punkt med en viss hastighet. 
 * Anledningen till att det finns olika funktioner f�r de olika benen �r att servona �r
 * monterade p� olika h�ll samt att servoaxeln har olika "offset".
 */
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

/*
 * Flyttar benet till en kartesisk punkt med en viss hastighet. 
 * Anledningen till att det finns olika funktioner f�r de olika benen �r att servona �r
 * monterade p� olika h�ll samt att servoaxeln har olika "offset".
 */
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

/*
 * Flyttar benet till en kartesisk punkt med en viss hastighet. 
 * Anledningen till att det finns olika funktioner f�r de olika benen �r att servona �r
 * monterade p� olika h�ll samt att servoaxeln har olika "offset".
 */
void MoveRearRightLeg(float x, float y, float z, int speed)
{
    long int theta1 = atan2f(x,-y)*180/PI;
    long int theta2 = 180/PI*(acosf(-z/sqrt(z*z + (sqrt(x*x + y*y) - a1)*(sqrt(x*x + y*y) - a1))) +
    acosf((z*z + (sqrt(x*x + y*y) - a1)*(sqrt(x*x + y*y) - a1) + a2Square - a3Square)/(2*sqrt(z*z + (sqrt((x*x + y*y) - a1)*(sqrt(x*x + y*y) - a1)))*a2)));
    
    long int theta3 = acosf((a2Square + a3Square - z*z - (sqrt(x*x + y*y) - a1)*(sqrt(x*x + y*y) - a1)) / (2*a2*a3))*180/PI;

    long int ActuatorAngle1 =  theta1 + 86;
    long int ActuatorAngle2 =  225 - theta2;
    long int ActuatorAngle3 =  300 - theta3;
    
    
    MoveDynamixel(rearRightLeg.coxaJoint,ActuatorAngle1,speed);
    MoveDynamixel(rearRightLeg.femurJoint,ActuatorAngle2,speed);
    MoveDynamixel(rearRightLeg.tibiaJoint,ActuatorAngle3,speed);
    return;
}

/*
 * Flyttar alla fyra ben till sina startpositioner med hastigheten som anges av standardSpeed_g.
 */
void MoveToStartPosition()
{
    MoveFrontLeftLeg(-startPositionX_g,startPositionY_g,startPositionZ_g,standardSpeed_g);
    MoveFrontRightLeg(startPositionX_g,startPositionY_g,startPositionZ_g,standardSpeed_g);
    MoveRearLeftLeg(-startPositionX_g,-startPositionY_g,startPositionZ_g,standardSpeed_g);
    MoveRearRightLeg(startPositionX_g,-startPositionY_g,startPositionZ_g,standardSpeed_g);
    return;
}

/* Rader �r vinklar p� servon. Kolumnerna inneh�ller positioner. Allokerar en extra rad minne 
 * h�r i matriserna bara f�r att f� snyggare kod. 
 */
long int actuatorPositions_g [13][20];
float legPositions_g [13][20];
int regulation_g[3];

int currentPos_g = 0;
int nextPos_g = 1;
int maxGaitCyclePos_g = 1;

/* 
 * Den h�r funktionen hanterar vilken position i g�ngcykeln roboten �r i. Den g�r runt baserat p� 
 * antalet positioner som finns i den givna g�ngstilen. 
 */ 
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

/*
 * Den h�r funktionen skapar en linj�r r�relse f�r ett visst ben fr�n punkt 1 till punkt 2.
 * Antalet delpunkter som skapas best�ms av numberOfPositions.
 * Startpositionen i positionsmatrisen anges av startIndex. 
 */
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
        
        // L�s inverskinematik f�r lederna.
        theta1 = atan2f(x,y)*180/PI;
        theta2 = 180/PI*(acosf(-z/sqrt(z*z + (sqrt(x*x + y*y) - a1)*(sqrt(x*x + y*y) - a1))) +
        acosf((z*z + (sqrt(x*x + y*y) - a1)*(sqrt(x*x + y*y) - a1) + a2Square - a3Square)/(2*sqrt(z*z + (sqrt((x*x + y*y) - a1)*(sqrt(x*x + y*y) - a1)))*a2)));
        
        theta3 = acosf((a2Square + a3Square - z*z - (sqrt(x*x + y*y) - a1)*(sqrt(x*x + y*y) - a1)) / (2*a2*a3))*180/PI;
        
        // Spara resultatet i global array. Korrigera s� att koordinaterna som sparas kan anv�ndas f�r nya anrop.
        // Anledningen till olika offset beroende p� vilka ben servon det g�ller �r att de verkar vara olika 
        // monterade.
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
                actuatorPositions_g[currentLeg.coxaJoint][i] = theta1 + 86;
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


/*
 * En alternativ funktion f�r att skapa en annan lyftr�relse. Anv�nds inte p� roboten i version 1.0
 * Den funktion som anv�nds ist�llet �r CalcCurvedPath().
 */
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
        
        // spara resultatet i global array. Korrigera s� att koordinaterna som sparas kan anv�ndas f�r nya anrop.
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
                actuatorPositions_g[currentLeg.coxaJoint][i] = theta1 + 86;
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

/*
 * Den h�r funktionen skapar en triangul�r r�relse f�r ett visst ben fr�n punkt 1 till punkt 2. 
 * Benet kommer att lyftas upp en stegh�jd i mitten av r�relsen. 
 * Antalet delpunkter som skapas best�ms av numberOfPositions.
 * Startpositionen i positionsmatrisen anges av startIndex. 
 */
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
        
        // Spara resultatet i global array. Korrigera s� att koordinaterna som sparas kan anv�ndas f�r nya anrop.
        // Anledningen till olika offset beroende p� vilka ben servon det g�ller �r att de verkar vara olika 
        // monterade.
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
                actuatorPositions_g[currentLeg.coxaJoint][i] = theta1 + 86;
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
               actuatorPositions_g[currentLeg.coxaJoint][i] = theta1 + 86;
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

/*
 * Flyttar ett ben till sin n�sta position i positionsmatrisen actuatorPositions_g. 
 */
void MoveLegToNextPosition(leg Leg)
{
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

/*
 * Flyttar alla ben till sina n�sta positioner i positionsmatrisen actuatorPositions_g. 
 */
void move()
{
    MoveLegToNextPosition(frontLeftLeg);
    MoveLegToNextPosition(frontRightLeg);
    MoveLegToNextPosition(rearLeftLeg);
    MoveLegToNextPosition(rearRightLeg);
    increasePositionIndexes();
}

/*
 * Unders�ker om en instruktion �r en r�relseinstruktion. Instruktionerna som behandlar mottas
 * fr�n PC:n. En r�relseinstruktion k�nnet�cknas av att den b�rjar med tv� nollor. 
 */
int isMovementInstruction(int instruction)
{
	if ((0b11000000 & instruction) == 0b00000000)
	{
		return TRUE;
	}
	return FALSE;
}

/*
 * Unders�ker om en instruktion �r en inst�llningsinstruktion. Instruktionerna som behandlar mottas
 * fr�n PC:n. En inst�llningsinstruktion k�nnet�cknas av att den b�rjar med en etta f�ljd av en nolla. 
 */
int isOptionInstruction(int instruction)
{
	if ((0b11000000 & instruction) == 0b10000000)
	{
		return TRUE;
	}
	return FALSE;
}

/*
 * Unders�ker om en instruktion �r en styrl�gesinstruktion. Instruktionerna som behandlar mottas
 * fr�n PC:n. En styrl�gesinstruktion k�nnet�cknas av att den b�rjar med tv� ettor.
 */
int isControlModeInstruction(int instruction)
{
	if ((0b11000000 & instruction) == 0b11000000)
	{
		return TRUE;
	}
	return FALSE;
}

/*
 * Avbrott fr�n kommunikationsenheten.
 * H�r behandlas kommunikationen mellan styr och kommunikationsenheten. 
 * K�rs kommunikationsenheten vill �verf�ra n�got till styrenheten. Detta tolkas d� som en instruktion.
 */
ISR(INT0_vect) 
{
    
    spiTransmitToCommUnit(TRASH); // Skicka iv�g skr�p f�r att kunna ta emot det som finns i kommunikationsenhetens SPDR
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
				currentOptionInstruction_g = RETURN_TO_LEAK_1;
				break;
			}
			case RETURN_TO_LEAK_2:
			{
				currentOptionInstruction_g = RETURN_TO_LEAK_2;
				break;
			}
			case RETURN_TO_LEAK_3:
			{
				currentOptionInstruction_g = RETURN_TO_LEAK_3;
				break;
			}
			case RETURN_TO_LEAK_4:
			{
				currentOptionInstruction_g = RETURN_TO_LEAK_4;
				break;
			}
			case RETURN_TO_LEAK_5:
			{
				currentOptionInstruction_g = RETURN_TO_LEAK_5;
				break;
			}
		}
	}
}

int toggleMania = FALSE;
int sendStuff = FALSE;
/*
 * Avbrott fr�n tryckknapp. Startar det autonoma l�get.
 * Vid en andra tryckning skickar roboten information till PC:n om noderna den hittat.
 */
ISR(INT1_vect) 
{ 
    
	if (toggleMania)
    {
        
        emergencyLockdown_g = TRUE;
        emergencyDowntime_g = 12;
        emergencyStop();
        
        sendStuff = TRUE;
        toggleMania = FALSE;
    }
    else
    {
        currentControlMode_g = exploration;
        toggleMania = TRUE;
        
    }
} 





/*
 * Skickar alla sensorv�rden till kommunikationsenheten.
 */
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

/*
 * H�mtar alla sensorv�rden fr�n sensorenheten och l�gger dessa i sensorv�rdesmatrisen. 
 */
void updateAllDistanceSensorData()
{
    distanceValue_g[NORTH] = fetchDataFromSensorUnit(DISTANCE_NORTH);
    distanceValue_g[EAST] = fetchDataFromSensorUnit(DISTANCE_EAST);
    distanceValue_g[SOUTH] = fetchDataFromSensorUnit(DISTANCE_SOUTH);
    distanceValue_g[WEST] = fetchDataFromSensorUnit(DISTANCE_WEST);
	
	if(currentDirection_g == north)
	{
		distanceValue_g[FRONT_RIGHT] = fetchDataFromSensorUnit(SENSOR_6);
		distanceValue_g[FRONT_LEFT] = fetchDataFromSensorUnit(SENSOR_5);
	}
	
	else if(currentDirection_g == east)
	{
		distanceValue_g[FRONT_RIGHT] = fetchDataFromSensorUnit(SENSOR_8);
		distanceValue_g[FRONT_LEFT] = fetchDataFromSensorUnit(SENSOR_7);
	}
	
	else if(currentDirection_g == south)
	{
		distanceValue_g[FRONT_RIGHT] = fetchDataFromSensorUnit(SENSOR_2);
		distanceValue_g[FRONT_LEFT] = fetchDataFromSensorUnit(SENSOR_1);
	}
	
	else if(currentDirection_g == west)
	{
		distanceValue_g[FRONT_RIGHT] = fetchDataFromSensorUnit(SENSOR_4);
		distanceValue_g[FRONT_LEFT] = fetchDataFromSensorUnit(SENSOR_3);
	}
}

/*
 * H�mtar totalvinkeln fr�n sensorenheten och l�gger den i vinkelmatrisen.
 */
void updateTotalAngle()
{
    angleValue_g[TOTAL] = fetchDataFromSensorUnit(TOTAL_ANGLE);
}

/*
 * H�mtar endast sensorv�rden fr�n en viss riktning. Anv�nds EJ i version 1.0
 */
void getSensorData(enum direction regulationDirection)
{
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

/*
 * Ber�knar reglering f�r roboten och sparar denna i regleringsmatrisen. 
 * Riktningen �r anger hur m�nga sidor som roboten kan titta p�. Man kan 
 * v�lja om vinkelreglering ska anv�ndas eller ej.
 */
void calcRegulation(enum direction regulationDirection, int useRotateRegulation)
{
    // regulationDirection motsvarar den sida som regleringen ska titta p�.
    /*
    * 
    *
    * F�rsta v�rdet anger hur mycket �t h�ger relativt r�relseriktningen roboten ska ta sig i varje steg.
    * Andra v�rdet anger hur mycket l�ngre steg benen p� v�nstra sidan relativt r�relseriktningen ska ta, de som medf�r en CW-rotation. 
    * Tredje v�rdet anger hur mycket l�ngre steg benen p� h�gra sidan relativt r�relseriktningen ska ta, de som medf�r en CCW-rotation
    */
	

	int translationRight = 0;

	// variablerna vi baserar regleringen p�, skillnaden mellan aktuellt v�rde och �nskat v�rde
	int translationRegulationError = 0; // avser hur l�ngt till v�nster ifr�n mittpunkten av "v�gen" vi �r
	int angleRegulationError = 0; // avser hur m�nga grader vridna i CCW riktning relativt "den raka riktningen" dvs relativt v�ggarna.





		//Reglering vid g�ng i koridor
	switch (currentDirection_g)
	{
		case north: // g�r vi �t norr kan vi reglera mot v�st eller �ster
		{
			switch(regulationDirection)
			{
				case east:
				{
					translationRegulationError = (distanceValue_g[east] + sensorOffset_g) - halfPathWidth_g; // translationRegulationError avser hur l�ngt till v�nster om mittlinjen vi �r 
					break;
				}

				case west:
				{
					translationRegulationError = halfPathWidth_g - (distanceValue_g[west] + sensorOffset_g);
					break;
				}
                case eastWest:
                {
                    translationRegulationError = distanceValue_g[east] - distanceValue_g[west];
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
                
                case northSouth:
                {
                    translationRegulationError = distanceValue_g[south] - distanceValue_g[north];
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

                case eastWest:
                {
                    translationRegulationError = distanceValue_g[west] - distanceValue_g[east];
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
                
                case northSouth:
                {
                    translationRegulationError = distanceValue_g[north] - distanceValue_g[south];
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
	int leftSideStepLengthAdjust = (kProportionalAngle_g * (-angleRegulationError))/20; // om roboten ska rotera �t h�ger s� l�ter vi benen p� v�nster sida ta l�ngre steg och benen p� h�ger sida ta kortare steg
	int rightSideStepLengthAdjust = kProportionalAngle_g * (angleRegulationError)/20; // eftersom angleRegulationError avser hur mycket vridet �t h�ger om mittlinjen roboten �r  
		
	if (translationRight > 30)
	{
		translationRight = 30;
	}
		
		
	if (translationRight < -30)
	{
		translationRight = -30;
	}
		
	if (leftSideStepLengthAdjust > 40)
	{
		leftSideStepLengthAdjust = 40;
	}
	
	if (leftSideStepLengthAdjust < -40)
	{
		leftSideStepLengthAdjust = -40;
	}
	
	if (rightSideStepLengthAdjust > 40)
	{
		rightSideStepLengthAdjust = 40;
	}
	
	if (rightSideStepLengthAdjust < -40)
	{
		rightSideStepLengthAdjust = -40;
	}
		
	regulation_g[0] =  translationRight;
	regulation_g[1] = leftSideStepLengthAdjust;
	regulation_g[2] = rightSideStepLengthAdjust;
		



    // om vi har en v�gg framf�r oss �r det b�st att ta mindre stegl�ngder
    if(distanceValue_g[currentDirection_g] < rangeToShortenStepLength_g)
	{
        if(stepLengthShortened_g == FALSE) // om vi inte redan har kortat stegl�ngden s� g�r vi stegl�ngden till h�lften av den vanliga
        {
           stepLength_g = stepLength_g/2;
           stepLengthShortened_g = TRUE;
       }
    }
    else
    {
        if (stepLengthShortened_g == TRUE) // om vi inte har en fr�mre v�gg men �nd� har f�rkortad stegl�ngd s� l�ter vi stegl�ngden bli den normala igen
        {
            stepLength_g = stepLength_g*2;
            stepLengthShortened_g = FALSE;
        }
    }


	needToCalcGait = 1; // n�r vi har reglerat beh�ver vi r�kna om g�ngstilen
	return;
}


 /*
 / Beslutar utifr�n currentNode_g och currentDirection vilken riktning som vi kan 
 / anv�nda f�r reglering mot. Returnerar enum direction som kan anv�ndas i calcRegulation
*/
int decideRegulationDirection() 
{
    int closeEnoughToRegulate = 300;
    enum direction tempRegulationDirection = noDirection;
    enum direction directionToRight = ((currentDirection_g + 1) % 4);
    enum direction directionToLeft = ((currentDirection_g + 3) % 4);

    if(currentDirection_g != noDirection)
    {
        if(distanceValue_g[directionToRight] < closeEnoughToRegulate && distanceValue_g[directionToLeft] < closeEnoughToRegulate)
            {
                if(directionToRight == east || directionToRight == west)
                {
                    tempRegulationDirection = eastWest;
                }
                else
                {
                    tempRegulationDirection = northSouth;
                }
            } 
        else if(distanceValue_g[directionToRight] < closeEnoughToRegulate)
        {
            tempRegulationDirection = directionToRight;
        }
        else if(distanceValue_g[directionToLeft] < closeEnoughToRegulate)
        {
            tempRegulationDirection = directionToLeft;
        }
    }
    return tempRegulationDirection;
}


// anv�nds som en globalvariabel f�r att veta vilken typ av "action" vi ska g�ra.
enum order{
	noOrder, // inte utf�ra n�got
	turnBlind,
	turnSeeing
};
enum order currentOrder_g = noOrder;

// en funktion som utf�r det currentOrder_g anger
void applyOrder()
{
	if(currentOrder_g == turnBlind)
	{
        if (!hasSavedSteplength_g)
        {
            hasSavedSteplength_g = TRUE;
            savedSteplength_g = stepLength_g;
            stepLength_g = BLIND_STEP_LENGTH;
        }            
		BlindStepsTaken_g = BlindStepsTaken_g + 1;
		if(BlindStepsTaken_g > BlindStepsToTake_g)
		{
            hasSavedSteplength_g = FALSE;
            stepLength_g = savedSteplength_g;
			currentDirection_g = nextDirection_g;
			currentOrder_g = noOrder;
			BlindStepsTaken_g = 0;
		}
	}
	if(currentOrder_g == turnSeeing)	
	{
		//closeEnoughToTurn = (distanceValue_g[currentDirection_g] + sensorOffset_g) < (stepLength_g/2 + halfPathWidth_g);	
		if ((distanceValue_g[currentDirection_g]) < closeEnoughToTurn)
		{
			currentOrder_g = noOrder;
			currentDirection_g = nextDirection_g;
		}
	}
 
	return;
}
/*
 * Flyttar robotens ben till startpositionen fr�n en godtycklig position i ett steg. 
 * L�ser av benens nuvarande position ifr�n undansparade (x,y,z)-koordinater i legpositions_g.
 * En framtida utveckling av denna funktion �r att �ndra s� att man faktiskt l�ser av servonas vinklar genom
 * l�sfunktioner i dem och sedan omvandla detta till koordinater med hj�lp av fram�tkinematik.
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
    // flytta ner alla benen till markniv�
    CalcStraightPath(frontLeftLeg, 4, 0, -copy[FRONT_LEFT_LEG_X], copy[FRONT_LEFT_LEG_Y], copy[FRONT_LEFT_LEG_Z],
                                        -copy[FRONT_LEFT_LEG_X], copy[FRONT_LEFT_LEG_Y], startPositionZ_g);
    CalcStraightPath(frontRightLeg, 4, 0, copy[FRONT_RIGHT_LEG_X], copy[FRONT_RIGHT_LEG_Y], copy[FRONT_RIGHT_LEG_Z],
                                        copy[FRONT_RIGHT_LEG_X], copy[FRONT_RIGHT_LEG_Y], startPositionZ_g);
    CalcStraightPath(rearLeftLeg, 4, 0, -copy[REAR_LEFT_LEG_X], -copy[REAR_LEFT_LEG_Y], copy[REAR_LEFT_LEG_Z],
                                        -copy[REAR_LEFT_LEG_X], -copy[REAR_LEFT_LEG_Y], startPositionZ_g);
    CalcStraightPath(rearRightLeg, 4, 0, copy[REAR_RIGHT_LEG_X], -copy[REAR_RIGHT_LEG_Y], copy[REAR_RIGHT_LEG_Z],
                                        copy[REAR_RIGHT_LEG_X], -copy[REAR_RIGHT_LEG_Y], startPositionZ_g);
    // lyft v�nster fram och h�ger bak till startpositionerna.
    CalcCurvedPath(frontLeftLeg, 4, 4, -copy[FRONT_LEFT_LEG_X], copy[FRONT_LEFT_LEG_Y], startPositionZ_g,
                                        -startPositionX_g, startPositionY_g, startPositionZ_g);
    CalcStraightPath(frontRightLeg, 4, 4, copy[FRONT_RIGHT_LEG_X], copy[FRONT_RIGHT_LEG_Y], startPositionZ_g,
                                         copy[FRONT_RIGHT_LEG_X], copy[FRONT_RIGHT_LEG_Y], startPositionZ_g);
    CalcStraightPath(rearLeftLeg, 4, 4, -copy[REAR_LEFT_LEG_X], -copy[REAR_LEFT_LEG_Y], startPositionZ_g,
                                        -copy[REAR_LEFT_LEG_X], -copy[REAR_LEFT_LEG_Y], startPositionZ_g);
    CalcCurvedPath(rearRightLeg, 4, 4, copy[REAR_RIGHT_LEG_X], -copy[REAR_RIGHT_LEG_Y], startPositionZ_g,
                                        startPositionX_g, -startPositionY_g, startPositionZ_g);
    // lyft h�ger fram och v�nster bak till startpositionerna
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

/*
 * Utf�r en �verg�ng fr�n startpositionen till travg�ngstilens ber�kningspunkt.
 * Uppl�sningen p� denna funktion beror p� den globala uppl�sningen p� g�ngstilen.
 */
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

/*
 * Utf�r en �verg�ng fr�n startpositionen till krypg�ngstilens startposition f�r r�relse norrut. 
 * Anv�nder EJ positionsmatrisen. Detta �r en direkt r�relse. Anv�nds ej p� roboten i version 1.0
 */
void moveToCreepStartPosition()
{
    long int sixth = stepLength_g/6;
    long int half = stepLength_g/2;
    MoveRearRightLeg(startPositionX_g, -startPositionY_g-half, startPositionZ_g, 20);
    MoveFrontRightLeg(startPositionX_g, startPositionY_g-sixth, startPositionZ_g, 20);
    MoveRearLeftLeg(-startPositionX_g, -startPositionY_g+sixth, startPositionZ_g, 20);
    MoveFrontLeftLeg(-startPositionX_g, startPositionY_g+half, startPositionZ_g, 20);
}

/*
 * Krypg�ngstil skapas utifr�n en viss uppl�sning. R�relsen fungerar i alla fyra riktningar men 
 * reglering finns inte �nnu inbyggt h�r. Anv�nds ej p� roboten i version 1.0
 */
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

/*
 * "G�ngstil" som har uppl�sningen 2 och endast inneh�ller startpositionen.
 */
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

/* 
 * G�r r�relsem�nstret f�r travet, beror p� vilken direction som �r satt p� currentDirection_g.
 */
void MakeTrotGait(int cycleResolution)
{
    // cycleResolution �r antaletpunkter p� kurvan som benen f�ljer. M�ste vara j�mnt tal!
    int res = cycleResolution/2;
    maxGaitCyclePos_g = cycleResolution - 1;
    posToCalcGait = (cycleResolution/4 - 1);
    
    
    // om ej autonomt l�ge kommer den manuella styrningen i main-loopen att anv�nda reglerparametrarna till diagonal g�ng och rotation.
    int translationRight = regulation_g[0];
    int leftSideStepLengthAdjust = regulation_g[1];
    int rightSideStepLengthAdjust = regulation_g[2];
        
    int leftSideTotalStepLength = stepLength_g + leftSideStepLengthAdjust; // left h�nvisar till den v�nstra sidan relativt r�relseriktningen
    int rightSideTotalStepLength = stepLength_g + rightSideStepLengthAdjust; // right h�nvisar till den h�gra sidan relativt r�relseriktningen

    switch(currentDirection_g)
    {       
    // r�relsem�nstret finns p� ett papper (frontleft och rearright b�rjar alltid med curved)
    
    // anm�rk att left/right-SideTotalStepLength menar den totala stegl�ngden p� den v�nstra/h�gra sidan relativt r�relseriktningen 
    // medans frontLeftLeg och de andra benen alltid har samma namn och �r namngivna utifr�n r�relseriktningen norr.   
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

/* 
 * funktioner som anv�nds f�r att �ndra g�ngstilsparametrar
 */
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
	if (kProportionalAngle_g < 10)
	{
		kProportionalAngle_g = kProportionalAngle_g + 0.1;
	}
	return;
}

void decreaseKRotation()
{
	if (kProportionalAngle_g > -10)
	{
		kProportionalAngle_g = kProportionalAngle_g - 0.1;
	}
	return;
}


/*
 * Skapar en �verg�ng till en ny g�ngstil fr�n den nuvarande. 
 * Anv�nds ej p� roboten i version 1.0
 */
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

/*
 * Tittar i nuvarande nod och returnerar om det finns en v�gg fram�t eller ej. 
 */
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

/*
 * J�mf�r avst�ndet fram�t med EMERGENCY_LOCKDOWN_DISTANCE
 */
int tooCloseToFrontWall()
{
    if(currentDirection_g == noDirection)
    {
        return FALSE;
    }
    if(distanceValue_g[currentDirection_g]  < EMERGENCY_LOCKDOWN_DISTANCE)
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

/*
 * Tvingar roboten till ett abrupt stop med hj�lp av returnToStartPosition()
 */
void emergencyStop()
{
    emergencyLockdown_g = TRUE;
    emergencyDowntime_g = 12;
    returnToStartPosition();
    currentGait = standStill;
}

/*
 * Hj�lper roboten att vara i emergencyl�ge tillr�ckligt l�nge f�r att stoppet ska kunna utf�ras.
 */
void emergencyController()
{
    if (emergencyDowntime_g > 0)
        emergencyDowntime_g--;
    else
    {
        emergencyLockdown_g = FALSE;
        directionHasChanged = TRUE; // F�r att roboten ska kunna b�rja g� igen
        if (stopInTCrossing) // Roboten ska l�ta applyOrder ta extra steg innan den sv�nger
        {
            stopInTCrossing = FALSE;
        }        
        else // Roboten ska nu direkt g� i n�sta riktning
        {
            currentDirection_g = nextDirection_g; 
			currentOrder_g = noOrder;           
        }
    }
    return;        
}

/*
 * Den huvudsakliga funktionen som hanterar robotens g�ngstil och �verg�ngar mellan olika riktningar. 
 * Beter sig olika i olika control-modes.
 */
void gaitController()
{
    if ((currentPos_g == posToCalcGait) && (currentControlMode_g != manual)) // h�mtar information fr�n sensorenheten varje g�ng det �r dags att ber�kna g�ngen
    {
        
        if (!emergencyLockdown_g)
        {
            applyOrder();
	    }
        //applyOrder();
        calcRegulation(decideRegulationDirection(), currentNode_g.whatNode != T_CROSSING); // Anv�nd ej rotationsreglering i T-korsningar
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
            switch(currentRotationInstruction) // �ndrar rotation om den finns en instruktion f�r att rotera.
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



            switch(currentDirectionInstruction) // V�ljer riktning beroende p� vad anv�ndaren matat in
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

                    // om vi gick diagonalt innan m�ste vi nu s�tta stegl�ngden till r�tt l�ngd igen
                    if(diagonalMovement_g == TRUE)
                    {
                        stepLength_g = stepLength_g*sqrt(2);
                        diagonalMovement_g = FALSE;
                    }
                    break;
                }
                case NORTH_EAST_HEADER:
                {
                    if (currentGait == standStill)
                    {
                        transitionStartToTrot();
                    }
                    // Huvudsaklig r�relseriktning norr. �stlig offset hanteres genom en h�rdkodad reglering �t h�ger
                    currentGait = trotGait;
                    currentDirection_g = north;
                    regulation_g[0] = stepLength_g;
                    
                    // om vi gick rakt innan s� m�ste stegl�ngden f�rminskas s� den effektiva stegl�ngden blir lika l�ng d� den g�r diagonalt
                    if(diagonalMovement_g == FALSE)
                    {
                        stepLength_g = stepLength_g/sqrt(2);
                        diagonalMovement_g = TRUE;
                    }
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

                    if(diagonalMovement_g == TRUE)
                    {
                        stepLength_g = stepLength_g*sqrt(2);
                        diagonalMovement_g = FALSE;
                    }
                    break;
                    }
                case SOUTH_EAST_HEADER:
                {
                    if (currentGait == standStill)
                    {
                        transitionStartToTrot();
                    }
                    // Huvudsaklig r�relseriktning �ster. Sydlig offset hanteres genom en h�rdkodad reglering �t h�ger
                    currentGait = trotGait;
                    currentDirection_g = east;
                    regulation_g[0] = stepLength_g;
                    if(diagonalMovement_g == FALSE)
                    {
                        stepLength_g = stepLength_g/sqrt(2);
                        diagonalMovement_g = TRUE;
                    }
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
                    if(diagonalMovement_g == TRUE)
                    {
                        stepLength_g = stepLength_g*sqrt(2);
                        diagonalMovement_g = FALSE;
                    }
                    break;
                }
                case SOUTH_WEST_HEADER:
                {
                    if (currentGait == standStill)
                    {
                        transitionStartToTrot();
                    }
                    // Huvudsaklig r�relseriktning s�der. V�stlig offset hanteres genom en h�rdkodad reglering �t h�ger
                    currentGait = trotGait;
                    currentDirection_g = south;
                    regulation_g[0] = stepLength_g;
                    if(diagonalMovement_g == FALSE)
                    {
                        stepLength_g = stepLength_g/sqrt(2);
                        diagonalMovement_g = TRUE;
                    }
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
                    if(diagonalMovement_g == TRUE)
                    {
                        stepLength_g = stepLength_g*sqrt(2);
                        diagonalMovement_g = FALSE;
                    }
                    break;
                }
                case NORTH_WEST_HEADER:
                {
                    if (currentGait == standStill)
                    {
                        transitionStartToTrot();
                    }
                    // Huvudsaklig r�relseriktning v�ster. Nordlig offset hanteres genom en h�rdkodad reglering �t h�ger
                    currentGait = trotGait;
                    currentDirection_g = west;
                    regulation_g[0] = stepLength_g;
                    if(diagonalMovement_g == FALSE)
                    {
                        stepLength_g = stepLength_g/sqrt(2);
                        diagonalMovement_g = TRUE;
                    }
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
    else // Controlmode �r n�got annat �n manuellt
    {
        if(directionHasChanged) // nodesAndControl har tagit ett styrbeslut
        {
            directionHasChanged = FALSE;
            needToCalcGait = TRUE;
			if (currentGait == standStill && !emergencyLockdown_g)
			{
				transitionStartToTrot();
                currentGait = trotGait;
			}
			
            if(currentOrder_g == noOrder) // kan bara f� en ny order om vi inte redan har n�gon
		    { 
				if (nextDirection_g == noDirection)
				{
					returnToStartPosition();
					currentGait = standStill;
					currentDirection_g = noDirection;
				}
				else
				{	
					if (nextDirection_g == currentDirection_g || (nextDirection_g == ((currentDirection_g + 2) % 4))) // unders�ker om n�sta riktning �r fram eller bak relativt nuvarande riktning
					{
						currentDirection_g = nextDirection_g; // om fram eller bak s� �r det bar att byta
					}
					else 
					{
						if (frontAvailable()) // om v�nster eller h�ger rel. nuvarande riktning m�ste vi unders�ka om vi ska sv�nga blint eller med syn
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

/*
 * Tittar om n�gon parameter har �ndrats och v�rdet p� denna till kommunikationsenheten i s� fall.
 */
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

/*
 * Fr�gar sensorenheten om en l�cka �r synlig eller ej.
 * Om roboten �r i utforskningsl�ge skickas kommando till sensorenheten om att 
 * t�nda lamporna n�r en l�cka �r synlig och sl�cka dem om den ej �r synlig.
 */
void checkForLeak()
{
	isLeakVisible_g = fetchDataFromSensorUnit(LEAK_HEADER);
    
    if(currentControlMode_g == exploration)
    {
        if (isLeakVisible_g)
        {
			fetchDataFromSensorUnit(0b00010111);
        }
        else
        {
			fetchDataFromSensorUnit(0b00010000);
			newLeak_g = TRUE;
        }
    }      
    
}

int main(void)
{
    newLeak_g = TRUE;
	initUSART();
    spiMasterInit();
    EICRA = 0b1111; // Stigande flank p� INT1/0 genererar avbrott
    EIMSK = (EIMSK | 3); // M�jligg�r externa avbrott p� INT1/0

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
    savedSteplength_g = stepLength_g;
    BlindStepsToTake_g = 2;//(int)((halfPathWidth_g - 8)/stepLength_g + 0.5);
	rangeToShortenStepLength_g = 0; //startPositionX_g + 2*stepLength_g + 50;
	
    int nodeUpdated_g = FALSE;
	int sendDataToPC = 1; // Anv�nds f�r att bara skicka varannan g�ng i commPeriodTimerEnd
	initNodeRingBuffer(); // Fyller buffern med 5 st �terv�ndsgr�nder med �ppet �t norr. 
    timer0Init();
    timer2Init();
    sei();
    MoveToStartPosition();
    _delay_ms(1000);
    standStillGait();

    setTimerPeriod(TIMER_0, newGaitResolutionTime);
    setTimerPeriod(TIMER_2, newCommUnitUpdatePeriod);
    
	sendAllRobotParameters();
    
    // Sl�ck lamporna p� sensorenheten
    fetchDataFromSensorUnit(0b00010000);
    
    int i = 0;

    // ---- Main-Loop ----
    while (1)
    {    
    	if (legTimerPeriodEnd())
    	{
    	    resetLegTimer();
            if (emergencyLockdown_g)
                emergencyController();
            move();
            gaitController();
    	}
    	if (commTimerPeriodEnd())
    	{
            resetCommTimer();
	        updateAllDistanceSensorData();            
            updateTotalAngle();
            
            if (tooCloseToFrontWall() && currentControlMode_g != manual && !emergencyLockdown_g)
            {
                if (tooCloseLastTime_g)
                {
                    emergencyStop();
                }
                else
                {
                    tooCloseLastTime_g = TRUE;
                }
            }
            else
            {
                tooCloseLastTime_g = FALSE;
            }
            
            
            /*
            nodesAndControl s�tter nextDirection och directionHasChanged om ett styrbeslut tas.
            Kan dessutom �ndra p� controlMode.
            */
            if (currentControlMode_g != manual)
            {
                checkForLeak();
                nodeUpdated_g = nodesAndControl();
            }
            sendChangedRobotParameters();
            if (nodeUpdated_g)
            {
                transmitDataToCommUnit(NODE_INFO, makeNodeData(&currentNode_g));
                transmitDataToCommUnit(CONTROL_DECISION,nextDirection_g);
            }
            if (sendDataToPC >= 10)
	        {
                if (sendStuff && i < 120)
                {
                    transmitDataToCommUnit(NODE_INFO, makeNodeData(&nodeArray[i])); 
                    i++;
                }
                else
	            {
                    transmitAllDataToCommUnit();
	            }
			    sendDataToPC = 1;
	        }
	        else
	        {
			    sendDataToPC++;
	        } 
    	} 
    }
}



 
