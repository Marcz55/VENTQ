
// Rasmus
// Kartnavigeringsnodvägvalssystem

#include <stdio.h>

#define false uint8_t 0
#define true  uint8_t 1

#define maxNodes 121        // En nod i varje 57cm i en 6x6m bana skulle motsvara 121 stycken noder.

#define corridor uint8_t  0         // Dessa är möjliga tal i whatNode
#define twoWaysCrossing uint8_t 1
#define deadEnd   uint8_t 2
#define Tcrossing uint8_t 3
#define Zcrossing uint8_t 4


//------------------------------------------------ Jocke har definierat dessa i sin kod
#define north uint8_t 1
#define east  uint8_t 2
#define south uint8_t 3
#define west  uint8_t 4
//------------------------------------------------ Ta bort dessa när denna kod integreras

/*
isLeakVisible_g
northSensor_g
northSensor1_g
northSensor2_g
eastSensor_g
eastSensor3_g
eastSensor4_g
... 
             Jocke fixar dessa
*/






// DET SKA FINNAS TVÅ MODES, ETT DÅ ROBOTEN MAPPAR UPP OCH GÖR DENNA ARRAY
// OCH ETT MODE DÅ ROBOTEN LETAR I LISTAN SOM REDAN FINNS, OCH GÅR TILL VALD LÄCKA
// lägger en kommentar ovanför funktionerna för att visa vilka som ska finna i läget då roboten mappar upp kartan (MapMode)






#define maxWallDistance 310 // Roboten bör hållas inom 310 mm från väggen

uint8_t tempNorthAvailible_g = true;
uint8_t tempEastAvailible_g = false;
uint8_t tempSouthAvailible_g = false;
uint8_t tempWestAvailible_g = false;
uint8_t currentNode_g = 0;
uint8_t canMakeNew_g = true;
uint8_t actualLeak_g = 0;
uint8_t validNode_g = true;

struct pathsAndNodes
{
    uint8_t     whatNode;        // Alla typer av noder är definade som siffror
    uint8_t     nodeID;          // Nodens unika id
    uint8_t     wayIn;
    uint8_t     nextDirection;   // Väderstrecken är siffror som är definade
    uint8_t     northAvailible;  // Finns riktningen norr i noden?   Om sant => 1, annars 0
    uint8_t     eastAvailible;
    uint8_t     southAvailible;
    uint8_t     westAvailible;
    uint8_t     containsLeak;    // Finns läcka i "noden", kan bara finnas om det är en korridor
}

// MapMode
void createNewNode()    // Skapar en ny nod och lägger den i arrayen
{
    nodeArray[currentNode_g].whatNode = whatNodeType();
    nodeArray[currentNode_g].nodeID = currentNode;
    nodeArray[currentNode_g].wayIn = whatWayIn();
    nodeArray[currentNode_g].nextDirection = whatsNextDirection();
    nodeArray[currentNode_g].northAvailible = tempNorthAvailible_g;
    nodeArray[currentNode_g].eastAvailible = tempEastAvailible_g;
    nodeArray[currentNode_g].southAvailible = tempSouthAvailible_g;
    nodeArray[currentNode_g].westAvailible = tempWestAvailible_g;
    nodeArray[currentNode_g].containsLeak = false;                // En ny nod kan inte initieras med en läcka
}

// MapMode
uint8_t checkIfNewNode() // "Bool" returnar false om alla noder är desamma och true om någon skiljer och den har skiljt 2 ggr i rad.
{
    if ((nodeArray[currentNode_g].northAvailible == tempNorthAvailible_g) && (nodeArray[currentNode_g].eastAvailible == tempEastAvailible_g) &&
        (nodeArray[currentNode_g].southAvailible == tempSouthAvailible_g) && (nodeArray[currentNode_g].westAvailible == tempWestAvailible_g)) 
    {
        return false;
    }
    else
    {
        if (validNode_g == 2)
        {
            validNode_g = 0;
            return true;
        }
        validNode_g ++;
        return false;
    }
}

// MapMode
uint8_t whatNodeType();
{
    if (tempNorthAvailible_g + tempEastAvailible_g + tempSouthAvailible_g + tempWestAvailible_g == 3)
    {
       return Tcrossing;            // Om det finns 3 vägar så är det en vägvalsnod
    }
    else if (tempNorthAvailible_g + tempEastAvailible_g + tempSouthAvailible_g + tempWestAvailible_g == 2)
    {
        if (tempNorthAvailible_g == tempSouthAvailible_g)
            return corridor;        // Detta måste vara en korridor
        else
            return twoWaysCrossing; // Detta måste vara en 2vägskorsning
    }
    else if (tempNorthAvailible_g + tempEastAvailible_g + tempSouthAvailible_g + tempWestAvailible_g == 1)
    {
        return deadEnd;             // Detta måste vara en återvändsgränd
    }
    else if (tempNorthAvailible_g + tempEastAvailible_g + tempSouthAvailible_g + tempWestAvailible_g == 0)
    {
        return Zcrossing;           // Detta måste vara en Z-sväng (sensornenh. returnerar kortase)
    }
}

// Får finnas i bägge, behövs i MapMode
uint8_t whatWayIn()
{
    return currentDirection;
}

// Får finnas i bägge, behövs i MapMode
uint8_t whatsNextDirection()        // Sätter även currentDirection (behandlar alltså styrbeslut)
{
    if (currentDirection == north)
    {
        if (tempNorthAvailible_g == true)  // Kan jag fortsätta gå rakt på?
            currentDirection == north;
        else if (tempEastAvailible_g == true) // Kan jag svänga öst?
            currentDirection == east;
        else if (tempWestAvailible_g == true) // Kan jag svänga väst?
            currentDirection == west;
        else
            currentDirection == south;        // Detta händer om det är en återvändsgränd (deadEnd)
    }
    else if (currentDirection == east)
    {
        if (tempEastAvailible_g == true)
            currentDirection = east;
        else if (tempSouthAvailible_g == true)
            currentDirection = south;
        else if (tempNorthAvailible_g == true)
            currentDirection = north;
        else
            currentDirection = west;    // deadEnd
    }
    else if (currentDirection == south)
    {
        if (tempSouthAvailible_g == true)
            currentDirection = south;
        else if (tempWestAvailible_g == true)
            currentDirection = west;
        else if (tempEastAvailible_g == true)
            currentDirection = east;
        else
            currentDirection = north;   // deadEnd
    }
    else if (currentDirection == west)
    {
        if (tempWestAvailible_g == true)
            currentDirection = west;
        else if (tempSouthAvailible_g = true)
            currentDirection = south;
        else if (tempNorthAvailible_g = true)
            currentDirection = north;
        else
            currentDirection = east;    // deadEnd
    }
}

// Ska finnas i bägge modes
void updateLeakInfo()
{
    if ((validLeak() == true) &&                              // Faktisk läcka?
        (nodeArray[currentNode_g].containsLeak == false) &&   // Innehåller noden redan en läcka?
        (nodeArray[currentNode_g].whatNode == corridor))      // Läckor får bara finnas i nodtypen "corridor"
    {
        nodeArray[currentNode_g].containsLeak = true;         // Uppdaterar att korridornoden har en läcka sig
    }
}

// Ska finnas i bägge modes
uint8_t validLeak()
{
    if (isLeakVisible_g == true)
    {
        if (actualLeak_g == 3)      // Når variabeln actualLeak_g når 3
        {                           // så är det en faktisk läcka
            return true;
        }
        else
        {
            actualLeak_g ++;
            return false;
        }
    }
    else
    {
        actualLeak_g = 0;
    }

    return false;
}

// Ska bara finnas i MapMode
void updateMakeNew()
{
    if ((nodeArray[currentNode_g].whatNode == deadEnd) && !(nodeArray[currentNode_g].whatNode == Tcrossing))
    {
        canMakeNew_g = false;
    }
    else
    {
        canMakeNew_g = true;
    }
}

// Ska finnas i bägge modes
void updateTempDirections()
{
    if (northSensor_g() > maxWallDistance) // Kan behöva ändra maxWallDistance
        tempNorthAvailible_g = true;
    else
        tempNorthAvailible_g = false;


    if (eastSensor_g() > maxWallDistance)
        tempEastAvailible_g = true;
    else    
        tempEastAvailible_g = false;


    if (southSensor_g() > maxWallDistance)
        tempSouthAvailible_g = true;
    else
        tempSouthAvailible_g = false;


    if (westSensor_g() > maxWallDistance)
        tempWestAvailible_g = true;
    else
        tempWestAvailible_g = false;
}

int main()
{
    struct pathsAndNodes nodeArray[maxNodes];

    // Börjar i en återvändsgränd med norr som frammåt
    nodeArray[0].whatNode = deadEnd;
    nodeArray[0].nodeID = 0;                // Nodens ID börjar på 0, för att vara samma som currentNode_g
    nodeArray[0].wayIn = north;
    nodeArray[0].nextDirection = north;
    nodeArray[0].northAvailible = true;     // Börjar i återvändsgränd med tillgänglig rikt. norr
    nodeArray[0].eastAvailible = false;
    nodeArray[0].southAvailible = false;
    nodeArray[0].westAvailible = false;
    nodeArray[0].containsLeak = false;

    while(1)
    {
        updateTempDirections();
        updateLeakInfo();           // Kollar ifall läcka finns, och lägger till i noden om det fanns
        updateMakeNew();

        // Denna gör nya noder, ska bara finnas i MapMode
        if ((checkIfNewNode() == true) && (canMakeNew_g == true))
        {
            currentNode_g ++;
            createNewNode();
        }
    }
}