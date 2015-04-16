
// Rasmus
// Kartnavigeringsnodvägvalssystem

#include <stdio.h>

#define false 0
#define true  1

#define maxNodes 121 // En nod i varje 57cm i en 6x6m bana skulle motsvara 121 stycken noder.

#define corridor  0       // Dessa är möjliga tal i whatNode
#define twoWayCrossing 1
#define deadEnd   2
#define Tcrossing 3

#define north 0  // Dessa är möjliga tal i wayIn och nextDirection
#define east  1
#define south 2
#define west  3

#define maxWallDistance 310 // Roboten bör hållas inom 310 mm från väggen

uint8_t tempNorthAvailible = true;
uint8_t tempEastAvailible = false;
uint8_t tempSouthAvailible = false;
uint8_t tempWestAvailible = false;

uint8_t currentNode = 0; // Nuvarande nod i arrayen

struct pathsAndNodes
{
    uint8_t     whatNode;        // 0 är korridor, 1 är tvåvägskorsning, 2 är återvändsgränd och 3 är vägvalsnod
    uint8_t     nodeID;          // Nodens unika id
    uint8_t     wayIn;           // Riktningen in i noden, 0=>norr, 1=>öst, 2=>cyd, 3=>väst
    uint8_t     nextDirection;   // Riktningen ut ur noden 0=>norr, 1=>öst, 2=>cyd, 3=>väst
    uint8_t     northAvailible;  // Finns riktningen norr i noden?   Om sant => 1, annars 0
    uint8_t     eastAvailible;   // Finns riktningen öst i noden?
    uint8_t     southAvailible;  // Finns riktningen cyd i noden?
    uint8_t     westAvailible;   // Finns riktningen väst i noden?
    uint8_t     containsLeak;    // Finns läcka i "noden", kan bara finnas om det är en korridor
}

struct pathsAndNodes nodeArray[maxNodes];   // Bör kanske flyttas in i main

void createNewNode()    // Skapar en ny nod och lägger den i arrayen
{
    nodeArray[currentNode].whatNode = whatNodeType();
    nodeArray[currentNode].nodeID = currentNode;
    nodeArray[currentNode].wayIn = whatWayIn();
    nodeArray[currentNode].nextDirection = whatsNextDirection();
    nodeArray[currentNode].northAvailible = tempNorthAvailible;
    nodeArray[currentNode].eastAvailible = tempEastAvailible;
    nodeArray[currentNode].southAvailible = tempSouthAvailible;
    nodeArray[currentNode].westAvailible = tempWestAvailible;
    nodeArray[currentNode].containsLeak = isLeakFound();
}


uint8_t checkIfNewNode() // "Bool" returnar false om alla noder är desamma och true om någon skiljer
{
    if ((nodeArray[currentNode].northAvailible == tempNorthAvailible) && (nodeArray[currentNode].eastAvailible == tempEastAvailible) &&
        (nodeArray[currentNode].southAvailible == tempSouthAvailible) && (nodeArray[currentNode].westAvailible == tempWestAvailible)) 
    {
        return false;
    }
    else
    {
        return true;
    }
}

uint8_t whatNodeType();
{
    if (tempNorthAvailible + tempEastAvailible + tempSouthAvailible + tempWestAvailible == 3)
    {
       return Tcrossing; // Om det finns 3 vägar så är det en vägvalsnod
    }
    else if (tempNorthAvailible + tempEastAvailible + tempSouthAvailible + tempWestAvailible == 2)
    {
        if (tempNorthAvailible == tempSouthAvailible)
            return corridor; // Detta måste vara en korridor
        else
            return twoWayCrossing; // Detta måste vara en 2vägskorsning
    }
    else if (tempNorthAvailible + tempEastAvailible + tempSouthAvailible + tempWestAvailible == 1)
    {
        return deadEnd;   // Detta måste vara en återvändsgränd
    }
    else 
}

uint8_t whatWayIn()
{
    return currentDirection;
}

uint8_t whatsNextDirection()
{
    // Styrbeslut, väljer höger först om vägval finns (T-korsning)
}

uint8_t isLeakFound()
{
    // Sensordata som berättar om läcka är hittad (true eller false)
}

int16_t getNorthSensor()
{
    // Sensordata som berättar avståndet åt norr (3siffrigt värde)
}

int16_t getEastSensor()
{
    // Sensordata som berättar avståndet åt öst (3siffrigt värde)
}

int16_t getSouthSensor()
{
    // Sensordata som berättar avståndet åt cyd (3siffrigt värde)
}

int16_t getWestSensor()
{
    // Sensordata som berättar avståndet åt väst (3siffrigt värde)
}


void updateTempDirections()
{
    if (getNorthSensor() > maxWallDistance) // Kan behöva ändra maxWallDistance
        tempNorthAvailible = true;
    else
        tempNorthAvailible = false;


    if (getEastSensor() > maxWallDistance)
        tempEastAvailible = true;
    else    
        tempEastAvailible = false;


    if (getSouthSensor() > maxWallDistance)
        tempSouthAvailible = true;
    else
        tempSouthAvailible = false;


    if (getWestSensor() > maxWallDistance)
        tempWestAvailible = true;
    else
        tempWestAvailible = false;
}

int main()
{
    // Börjar i en återvändsgränd med norr som frammåt
    nodeArray[0].whatNode = deadEnd;
    nodeArray[0].nodeID = 0;             // Nodens ID börjar på 0, för att vara samma som currentNode
    nodeArray[0].wayIn = north;
    nodeArray[0].nextDirection = north;
    nodeArray[0].northAvailible = true;
    nodeArray[0].eastAvailible = false;
    nodeArray[0].southAvailible = false;
    nodeArray[0].westAvailible = false;
    nodeArray[0].containsLeak = false;

    while(1)
    {
        updateTempDirections();
        if (checkIfNewNode() == true)
        {
            currentNode ++;
            createNewNode();
        }
    }
}