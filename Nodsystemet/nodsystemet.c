// Rasmus
// Kartnavigeringsnodvägvalssystem

#include <stdio.h>


#define false 0
#define true  1

#define maxNodes 121        // En nod i varje 57cm i en 6x6m bana skulle motsvara 121 stycken noder.

#define AUTO_MODE      0
#define FINDLEAK_MODE  1    // Dessa behandlar variabeln mode, som bestämmer om labyrinten ska genomsökas,
#define WAIT_FOR_INPUT 2    // hitta vald läcka eller vänta på input

#define corridor  0         // Dessa är möjliga tal i whatNode
#define turn      1
#define deadEnd   2
#define Tcrossing 3
#define Zcrossing 4
#define endOfMaze 5
#define mazeStart 6

//--------------------------------------------------------------------------------------------------------- Jocke har definierat dessa i sin kod
#define north 0
#define east  1
#define south 2
#define west  3
#define noDirection 4
//--------------------------------------------------------------------------------------------------------- Ta bort dessa när denna kod integreras

#define maxTcrossings 10

int pathToLeak[maxTcrossings] = {4, 4, 4, 4, 4, 4, 4, 4, 4, 4}; // noDirection = 4




// ------------------------------------------------------------------------------------------------------------ bara för att testa nedan
int     isLeakVisible_g = 0;
int     northSensor_g = 0;
int     eastSensor_g = 0;
int     southSensor_g = 0;
int     westSensor_g = 0;
//-------------------------------------------------------------------------------------------------------------



#define closeEnoughToWall 280  // Roboten går rakt fram tills den här längden
#define maxWallDistance 570    // Utanför denna längd är det ingen vägg

int tempNorthAvailible_g = true;
int tempEastAvailible_g = false;
int tempSouthAvailible_g = false;
int tempWestAvailible_g = false;
int currentNode_g = 0;
int actualLeak_g = 0;
int validChange_g = 0;
int currentDirection_g = north;
int leaksFound_g = 0;
int currentTcrossing_g = 0;
int TcrossingsFound_g = 0;
int distanceToFrontWall_g = 0;
int leaksToPass_g = 0;
int currentPath = 0;
int mode = FINDLEAK_MODE;

int wantedLeak = 0;

struct node
{
    int     whatNode;           // Alla typer av noder är definade som siffror
    int     nodeID;             // Nodens unika id
    int     pathsExplored;      // Är bägge hållen utforskade är denna 2
    int     wayIn;
    int     nextDirection;      // Väderstrecken är siffror som är definade
    int     northAvailible;     // Finns riktningen norr i noden?   Om sant => 1, annars 0
    int     eastAvailible;
    int     southAvailible;
    int     westAvailible;
    int     containsLeak;       // Finns läcka i "noden", kan bara finnas om det är en korridor
    int     leakID;             // Fanns en läcka får den ett unikt id, annars är denna 0
};

struct node nodeArray[maxNodes];
struct node tempNode_g;

// Ska finnas i bägge modes
void updateTempDirections()
{
    if (currentDirection_g == north)
        distanceToFrontWall_g = northSensor_g;
    else if (currentDirection_g == east)
        distanceToFrontWall_g = eastSensor_g;
    else if (currentDirection_g == south)
        distanceToFrontWall_g = southSensor_g;
    else
        distanceToFrontWall_g = westSensor_g;

    if (northSensor_g > maxWallDistance)    // Kan behöva ändra maxWallDistance
        tempNorthAvailible_g = true;
    else
        tempNorthAvailible_g = false;


    if (eastSensor_g > maxWallDistance)
        tempEastAvailible_g = true;
    else
        tempEastAvailible_g = false;


    if (southSensor_g > maxWallDistance)
        tempSouthAvailible_g = true;
    else
        tempSouthAvailible_g = false;


    if (westSensor_g > maxWallDistance)
        tempWestAvailible_g = true;
    else
        tempWestAvailible_g = false;
}

// Ska finnas i bägge modes
int validLeak()
{
    if (isLeakVisible_g == true)
    {
        if (actualLeak_g == 2)      // Når variabeln actualLeak_g 2
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

// Ska finnas i bägge modes
void updateLeakInfo()
{
    if ((validLeak() == true) &&
    (nodeArray[currentNode_g].containsLeak == false) &&     // Innehåller noden redan en läcka?
    !(nodeArray[currentNode_g].whatNode == Tcrossing) &&    // Läckor får ej finnas i T-korsningar
    !(nodeArray[currentNode_g].whatNode == deadEnd))        // Läckor hamnar innanför en deadEnd, och detta gör att läckor
    {                                                       // inte läggs till på tillbakavägen
        leaksFound_g ++;

        nodeArray[currentNode_g].containsLeak = true;       // Uppdaterar att noden har en läcka i sig
        nodeArray[currentNode_g].leakID = leaksFound_g;     // Läckans id får det nummer som den aktuella läckan har
    }
}

// Ska bara finnas i MapMode
int canMakeNew()
{
    if (currentTcrossing_g == 0)
        return true;
    else if ((nodeArray[currentNode_g].whatNode == deadEnd) &&                                                  // Var senaste noden en återvändsgränd (deadEnd)?
        !(tempNorthAvailible_g + tempEastAvailible_g + tempSouthAvailible_g + tempWestAvailible_g == 3))    // Detta kollar om roboten står i en T-korsning
    {
        return false; // Alltså: om senaste noden var en deadEnd och den nuvarande inte är en T-korsning ska inte nya noder göras
    }
    else if ((nodeArray[currentNode_g].whatNode == Tcrossing) &&                                              // Var senaste noden en Tcrossing
             (nodeArray[currentNode_g].pathsExplored == 2) &&                                                 // Om senaste noden va en Tcrossing, har den utforskats helt?
             !(tempNorthAvailible_g + tempEastAvailible_g + tempSouthAvailible_g + tempWestAvailible_g == 3)) // isåfall ska inte nya noder göras
    {
        if (nodeArray[currentNode_g].nodeID == 1)                   // Om det är första Tcrossing så gör nya noder
        {
            return true;
        }
        return false;
    }
    else
    {
        return true;
    }
}

int isChangeDetected()
{
    if ((tempNode_g.northAvailible == tempNorthAvailible_g) && (tempNode_g.eastAvailible == tempEastAvailible_g) &&
        (tempNode_g.southAvailible == tempSouthAvailible_g) && (tempNode_g.westAvailible == tempWestAvailible_g))
    {
        validChange_g = 0;
        return false;
    }
    else
    {
        if (validChange_g == 1)
        {
            return true;
        }
        validChange_g ++;
        return false;
    }
}

// MapMode
// Denna funktion hanterar konstiga fenomen i Zcrossing, hanteras dock som två st 2vägskorsningar
int checkIfNewNode()
{
    if ((isChangeDetected() == true) && (distanceToFrontWall_g > maxWallDistance))
    {
        return true;    // I detta fall är det en Tcrossing från sidan, eller ut från en korsning, eller en corridor
    }
    else if ((isChangeDetected() == true) && (distanceToFrontWall_g < closeEnoughToWall))
    {
        return true;    // Förändringen va en vägg där fram, nu har roboten gått tillräckligt långt
    }
    else
        return false;
}

// MapMode
int whatNodeType()
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
            return turn; // Detta måste vara en 2vägskorsning
    }
    else if (tempNorthAvailible_g + tempEastAvailible_g + tempSouthAvailible_g + tempWestAvailible_g == 1)
    {
        int i = currentNode_g;
        for (i; i > 0; i--)
        {
            if ((nodeArray[i].nodeID == 1) && (nodeArray[i].pathsExplored == 2))
            {
                return endOfMaze;   // Slutet på labyrinten
            }
        }

        return deadEnd;             // Detta måste vara en återvändsgränd
    }
    else // Här är summan lika med 0, dvs väggar på alla sidor, bör betyda Z-sväng
    {
        return Zcrossing;           // Funkar egentligen inte, en Z-sväng hanteras som två turns
    }
}

int TcrossingID(int whatNode_)
{
    if (whatNode_ == Tcrossing)
    {
        if (currentTcrossing_g == 0)    // Första T-korsningen
        {
            currentTcrossing_g = 1;
            TcrossingsFound_g = 1;
            return currentTcrossing_g;
        }
        else if (nodeArray[currentNode_g].whatNode == deadEnd)
        {
            return currentTcrossing_g;
        }
        else if (nodeArray[currentNode_g].whatNode == Tcrossing)
        {
            int j = currentTcrossing_g;
            for (j; j > 0 ; j--)                    // Den här magiska forloopen letar rätt på en Tcrossing från höger
            {
                int i;                              // i arrayen och kollar om den var "fylld" med pathsExplored
                for (i = 120; i>1 ; i--)            // och va den fylld kollar den på den tidigare T-korsningen
                {
                    if (nodeArray[i].nodeID == j - 1)
                    {
                        if(nodeArray[i].pathsExplored == 2)
                        {
                            currentTcrossing_g --;
                            break;
                        }
                        else
                        {
                            currentTcrossing_g --;
                            return currentTcrossing_g;
                        }
                    }
                }
            }
        }
        else
        {
            TcrossingsFound_g ++;
            currentTcrossing_g = TcrossingsFound_g;
            return currentTcrossing_g;
        }
    }
    else
        return 0;
}

int calcPathsExplored(int whatNode_, int nodeID_)
{
    int paths = 0;

    if (whatNode_ == Tcrossing)
    {
        int ID = nodeID_;

        int i;
        for(i = 0; i < currentNode_g; i++)
        {
            if (nodeArray[i].nodeID == ID)
                paths ++;
        }
    }
    return paths;
}

// Får finnas i bägge, behövs i MapMode
int whatWayIn()
{
    return currentDirection_g;
}

// MapMode
void updateTempNode()          // Uppdaterar tempnoden, eg. skapar en ny nod
{
    tempNode_g.whatNode = whatNodeType();
    tempNode_g.nodeID = TcrossingID(tempNode_g.whatNode);                                     // Är alltid 0 om det inte är en Tcrossing
    tempNode_g.pathsExplored = calcPathsExplored(tempNode_g.whatNode, tempNode_g.nodeID);     // Är alltid 0 om det inte är en Tcrossing
    tempNode_g.wayIn = whatWayIn();
    tempNode_g.nextDirection =  noDirection;            // Sätts till noDirection och ändras i styrbeslut
    tempNode_g.northAvailible = tempNorthAvailible_g;
    tempNode_g.eastAvailible = tempEastAvailible_g;
    tempNode_g.southAvailible = tempSouthAvailible_g;
    tempNode_g.westAvailible = tempWestAvailible_g;
    tempNode_g.containsLeak = false;                    // En ny nod kan inte initieras med en läcka
    tempNode_g.leakID = 0;
}

void placeNodeInArray()
{
    nodeArray[currentNode_g].whatNode        = tempNode_g.whatNode;
    nodeArray[currentNode_g].nodeID          = tempNode_g.nodeID;
    nodeArray[currentNode_g].pathsExplored   = tempNode_g.pathsExplored;
    nodeArray[currentNode_g].wayIn           = tempNode_g.wayIn;
    nodeArray[currentNode_g].nextDirection   = tempNode_g.nextDirection;
    nodeArray[currentNode_g].northAvailible  = tempNode_g.northAvailible;
    nodeArray[currentNode_g].eastAvailible   = tempNode_g.eastAvailible;
    nodeArray[currentNode_g].southAvailible  = tempNode_g.southAvailible;
    nodeArray[currentNode_g].westAvailible   = tempNode_g.westAvailible;
    nodeArray[currentNode_g].containsLeak    = tempNode_g.containsLeak;
    nodeArray[currentNode_g].leakID          = tempNode_g.leakID;
}

void makeLeakPath(char wantedLeak_g)
{
    int index = 0;
    int i = 0;
    for (i; i < 121; i++)
    {
        if (nodeArray[i].whatNode == Tcrossing)
        {
            if (nodeArray[i].pathsExplored == 0)
            {
                pathToLeak[index] = nodeArray[i].nextDirection;
                index ++;
            }
            else if (nodeArray[i].pathsExplored == 1)
            {
                pathToLeak[index - 1] = nodeArray[i].nextDirection;
            }
            else if (nodeArray[i].pathsExplored == 2)
            {
                if (nodeArray[i].nodeID == 1)
                    pathToLeak[0] = noDirection;
                else
                {
                    pathToLeak[index] = noDirection;
                    index --;
                }
            }

            leaksToPass_g = 0;
        }
        else if (nodeArray[i].containsLeak == true)
        {
            if (nodeArray[i].leakID == wantedLeak_g)
                break;
            else
                leaksToPass_g ++;
        }
    }
}

int northToNext()
{
    if (tempNode_g.eastAvailible == true)
    {
        return east;
    }
    else if (tempNode_g.northAvailible == true)
    {
        return north;
    }
    else if (tempNode_g.westAvailible == true)
    {
        return west;
    }
    else
    {
        return south;       // I detta fall är det en deadEnd
    }
}


int eastToNext()
{
    if (tempNode_g.southAvailible == true)
    {
        return south;
    }
    else if (tempNode_g.eastAvailible == true)
    {
        return east;
    }
    else if (tempNode_g.northAvailible == true)
    {
        return north;
    }
    else
    {
        return west;
    }
}

int southToNext()
{
    if (tempNode_g.westAvailible == true)
    {
        return west;
    }
    else if (tempNode_g.southAvailible == true)
    {
        return south;
    }
    else if (tempNode_g.eastAvailible == true)
    {
        return east;
    }
    else
    {
        return north;
    }
}

int westToNext()
{
    if (tempNode_g.northAvailible == true)
    {
        return north;
    }
    else if (tempNode_g.westAvailible == true)
    {
        return west;
    }
    else if (tempNode_g.southAvailible == true)
    {
        return south;
    }
    else
    {
        return east;
    }
}

int decideDirection()      // Autonoma läget
{
    if (whatNodeType() == corridor)
    {
        return currentDirection_g;
    }
    else
    {
        if (currentDirection_g == north)
        {
            currentDirection_g = northToNext();
            return currentDirection_g;
        }
        else if (currentDirection_g == east)
        {
            currentDirection_g = eastToNext();
            return currentDirection_g;
        }
        else if (currentDirection_g == south)
        {
            currentDirection_g = southToNext();
            return currentDirection_g;
        }
        else if (currentDirection_g == west)
        {
            currentDirection_g = westToNext();
            return currentDirection_g;
        }
    }
}

int main()
{
    initNodeAndSteering();

    nodesAndControl();
}

void initNodeAndSteering()
{
    // Börjar i en återvändsgränd med norr som frammåt
    nodeArray[0].whatNode = mazeStart;
    nodeArray[0].nodeID = 0;                // Nodens ID initieras som 0, ändras om det är en Tcrossing
    nodeArray[0].pathsExplored = 0;
    nodeArray[0].wayIn = north;
    nodeArray[0].nextDirection = north;
    nodeArray[0].northAvailible = true;     // Börjar i återvändsgränd med tillgänglig rikt. norr
    nodeArray[0].eastAvailible = false;
    nodeArray[0].southAvailible = false;
    nodeArray[0].westAvailible = false;
    nodeArray[0].containsLeak = false;
    nodeArray[0].leakID = 0;
}

void nodesAndControl()
{
    switch(mode)
    {
        case AUTO_MODE  :

            updateTempDirections();
            updateLeakInfo();           // Kollar ifall läcka finns, och lägger till i noden om det fanns

            if (checkIfNewNode() == true)
            {
                updateTempNode();
                currentDirection_g = decideDirection();

                if (canMakeNew() == true)
                {
                    currentNode_g ++;
                    placeNodeInArray();
                    nodeArray[currentNode_g].nextDirection = currentDirection_g;     // lägger in styrbeslut i arrayen
                }
            }

            if (nodeArray[currentNode_g].whatNode == endOfMaze)
            {
                mode = WAIT_FOR_INPUT;
            }
            break;

        case WAIT_FOR_INPUT  :

            switch(currentVentilatorOptionInstruktion_g)
            {
                case 0b11100000  :
                    wantedLeak = 1;
                    makeLeakPath(wantedLeak);
                    mode = FINDLEAK_MODE;
                    break;
                case 0b11100001  :
                    wantedLeak = 2;
                    makeLeakPath(wantedLeak);
                    mode = FINDLEAK_MODE;
                    break;
                case 0b11100010  :
                    wantedLeak = 3;
                    makeLeakPath(wantedLeak);
                    mode = FINDLEAK_MODE;
                    break;
                case 0b11100011  :
                    wantedLeak = 4;
                    makeLeakPath(wantedLeak);
                    mode = FINDLEAK_MODE;
                    break;
                case 0b11100100  :
                    wantedLeak = 5;
                    makeLeakPath(wantedLeak);
                    mode = FINDLEAK_MODE;
                    break;
                default  :
                    break;
            }
            // Stå still -----------------------------------------------------------------------------------------------------------------------------
            break;

        case FINDLEAK_MODE  :

            updateTempDirections();

            if (checkIfNewNode() == true)
            {
                updateTempNode();

                if (tempNode_g.whatNode == Tcrossing)
                {
                    currentDirection_g = pathToLeak[currentPath];
                    currentPath ++;
                }
                else
                {
                    currentDirection_g = decideDirection();
                }
            }

            if (pathToLeak[currentPath] == noDirection)     // Roboten har nu passerat det sista vägvalet
            {
                if (validLeak() == true)
                {
                    if (tempNode_g.containsLeak == false)   // Denna koll gör att samma läcka inte hanteras igen som en ny
                    {
                        tempNode_g.containsLeak = true;

                        if (leaksToPass == 0)
                            // Läckan är nu hittad... ska roboten stå still? Derp ------------------------------------------------------------------------------------
                        else
                            leaksToPass --;
                    }
                }
            }
            break;
    }
}
