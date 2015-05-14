// Rasmus
// Kartnavigeringsnodvägvalssystem

#include <stdio.h>
#include "Definitions.h"
#include "nodsystemet.h"

#define MAX_T_CROSSINGS 10

int pathToLeak[MAX_T_CROSSINGS] = {4, 4, 4, 4, 4, 4, 4, 4, 4, 4}; // noDirection = 4

// ----------------------------------------------------------------------------------------------------- fixa...


#define CLOSE_ENOUGH_TO_WALL 270  // Roboten går rakt fram tills den här längden
#define MAX_WALL_DISTANCE 440    // Utanför denna längd är det ingen vägg

int tempNorthAvailible_g = TRUE;
int tempEastAvailible_g = FALSE;
int tempSouthAvailible_g = FALSE;
int tempWestAvailible_g = FALSE;
int lastAddedNodeIndex_g = 0;
int actualLeak_g = 0;
int validChange_g = 0;
//int currentDirection_g = north;
int leaksFound_g = 0;
int currentTcrossing_g = 0;
int TcrossingsFound_g = 0;
int distanceToFrontWall_g = 0;
int leaksToPass_g = 0;
int currentPath = 0;

int wantedLeak = 0;

int makeNodeData(node* nodeToSend)
{
    int data = 0;
    data |= (nodeToSend->northAvailible << 3);
    data |= (nodeToSend->eastAvailible << 2);
    data |= (nodeToSend->southAvailible << 1);
    data |= (nodeToSend->westAvailible << 0);
    // Placera riktningen spridda över två databytes så att noDirection = 0b00000001 0b00000000
    data |= ((nodeToSend->nextDirection & 3) << 4);
    data |= ((nodeToSend->nextDirection & 4) << 8);
    data |= ((nodeToSend->nodeID & 0b00011111) << 9);
    return data;
}

// Ska finnas i bägge modes
void updateTempDirections()
{
    if (currentDirection_g == north)
        distanceToFrontWall_g = distanceValue_g[NORTH];
    else if (currentDirection_g == east)
        distanceToFrontWall_g = distanceValue_g[EAST];
    else if (currentDirection_g == south)
        distanceToFrontWall_g = distanceValue_g[SOUTH];
    else
        distanceToFrontWall_g = distanceValue_g[WEST];

    if (distanceValue_g[NORTH] > MAX_WALL_DISTANCE)    // Kan behöva ändra MAX_WALL_DISTANCE
        tempNorthAvailible_g = TRUE;
    else
        tempNorthAvailible_g = FALSE;


    if (distanceValue_g[EAST] > MAX_WALL_DISTANCE)
        tempEastAvailible_g = TRUE;
    else
        tempEastAvailible_g = FALSE;


    if (distanceValue_g[SOUTH] > MAX_WALL_DISTANCE)
        tempSouthAvailible_g = TRUE;
    else
        tempSouthAvailible_g = FALSE;


    if (distanceValue_g[WEST] > MAX_WALL_DISTANCE)
        tempWestAvailible_g = TRUE;
    else
        tempWestAvailible_g = FALSE;
}

// Ska finnas i bägge modes
int validLeak()
{
    if (isLeakVisible_g == TRUE)
    {
        if (actualLeak_g == 2)      // Når variabeln actualLeak_g 2
        {                           // så är det en faktisk läcka
            return TRUE;
        }
        else
        {
            actualLeak_g ++;
            return FALSE;
        }
    }
    else
    {
        actualLeak_g = 0;
    }

    return FALSE;
}

// Ska finnas i bägge modes
void updateLeakInfo()
{
    if ((validLeak() == TRUE) &&
    (nodeArray[lastAddedNodeIndex_g].containsLeak == FALSE) &&     // Innehåller noden redan en läcka?
    !(nodeArray[lastAddedNodeIndex_g].whatNode == T_CROSSING) &&    // Läckor får ej finnas i T-korsningar
    !(nodeArray[lastAddedNodeIndex_g].whatNode == DEAD_END))        // Läckor hamnar innanför en DEAD_END, och detta gör att läckor
    {                                                       // inte läggs till på tillbakavägen
        leaksFound_g ++;

        nodeArray[lastAddedNodeIndex_g].containsLeak = TRUE;       // Uppdaterar att noden har en läcka i sig
        nodeArray[lastAddedNodeIndex_g].leakID = leaksFound_g;     // Läckans id får det nummer som den aktuella läckan har
    }
}

// Ska bara finnas i MapMode
int canMakeNew()
{
    if (currentTcrossing_g == 0)
        return TRUE;
    else if ((nodeArray[lastAddedNodeIndex_g].whatNode == DEAD_END) &&                                                  // Var senaste noden en återvändsgränd (DEAD_END)?
        !(tempNorthAvailible_g + tempEastAvailible_g + tempSouthAvailible_g + tempWestAvailible_g == 3))    // Detta kollar om roboten står i en T-korsning
    {
        return FALSE; // Alltså: om senaste noden var en DEAD_END och den nuvarande inte är en T-korsning ska inte nya noder göras
    }
    else if ((nodeArray[lastAddedNodeIndex_g].whatNode == T_CROSSING) &&                                              // Var senaste noden en T_CROSSING
             (nodeArray[lastAddedNodeIndex_g].pathsExplored == 2) &&                                                 // Om senaste noden va en T_CROSSING, har den utforskats helt?
             !(tempNorthAvailible_g + tempEastAvailible_g + tempSouthAvailible_g + tempWestAvailible_g == 3)) // isåfall ska inte nya noder göras
    {
        if ((nodeArray[lastAddedNodeIndex_g].nodeID == 1) && (currentNode_g.whatNode == END_OF_MAZE))                  // Om det är första T_CROSSING så gör nya noder
        {
            return TRUE;
        }
        return FALSE;
    }
    else
    {
        return TRUE;
    }
}

int isChangeDetected()
{
    if ((currentNode_g.northAvailible == tempNorthAvailible_g) && (currentNode_g.eastAvailible == tempEastAvailible_g) &&
        (currentNode_g.southAvailible == tempSouthAvailible_g) && (currentNode_g.westAvailible == tempWestAvailible_g))
    {
        validChange_g = 0;
        return FALSE;
    }
    else
    {
        if (validChange_g == 1)
        {
            return TRUE;
        }
        validChange_g ++;
        return FALSE;
    }
}

void chooseAndSetFrontSensors()
{
	// bestämma vilka sensorer som tempDirAvalible ska sättas till
	if (currentDirection_g == north)
	{
		if (distanceValue_g[FRONT_RIGHT] > MAX_WALL_DISTANCE)
			tempEastAvailible_g = TRUE;
		else
			tempEastAvailible_g = FALSE;
		
		if (distanceValue_g[FRONT_LEFT] > MAX_WALL_DISTANCE)
			tempWestAvailible_g = TRUE;
		else
			tempWestAvailible_g = FALSE;		
	}
	
	else if (currentDirection_g == east)
	{
		if (distanceValue_g[FRONT_RIGHT] > MAX_WALL_DISTANCE)
			tempSouthAvailible_g = TRUE;
		else
			tempSouthAvailible_g = FALSE;
		
		if (distanceValue_g[FRONT_LEFT] > MAX_WALL_DISTANCE)
			tempNorthAvailible_g = TRUE;
		else
			tempNorthAvailible_g = FALSE;
	}
	
	else if (currentDirection_g == south)
	{
		if (distanceValue_g[FRONT_RIGHT] > MAX_WALL_DISTANCE)
			tempWestAvailible_g = TRUE;
		else
			tempWestAvailible_g = FALSE;

		if (distanceValue_g[FRONT_LEFT] > MAX_WALL_DISTANCE)
			tempEastAvailible_g = TRUE;
		else
			tempEastAvailible_g = FALSE;
	}
	
	else if (currentDirection_g == west)
	{
		if (distanceValue_g[FRONT_RIGHT] > MAX_WALL_DISTANCE)
			tempNorthAvailible_g = TRUE;
		else
			tempNorthAvailible_g = FALSE;
			
		if (distanceValue_g[FRONT_LEFT] > MAX_WALL_DISTANCE)
			tempSouthAvailible_g = TRUE;
		else
			tempSouthAvailible_g = FALSE;
	}
}

int isDeadEnd()
{
	if (tempNorthAvailible_g + tempEastAvailible_g + tempSouthAvailible_g + tempWestAvailible_g == 1)
		return TRUE;
	else
		return FALSE;
}

// MapMode
// Denna funktion hanterar konstiga fenomen i Z_CROSSING, hanteras dock som två st 2vägskorsningar
int checkIfNewNode()
{
	if ((nodeArray[lastAddedNodeIndex_g].whatNode == TURN) || (nodeArray[lastAddedNodeIndex_g].whatNode == T_CROSSING))
	{
		if ((currentDirection_g == north) && (distanceValue_g[SOUTH] < MAX_WALL_DISTANCE + 80))
			return FALSE;
		else if ((currentDirection_g == east) && (distanceValue_g[WEST] < MAX_WALL_DISTANCE + 80))
			return FALSE;
		else if ((currentDirection_g == south) && (distanceValue_g[NORTH] < MAX_WALL_DISTANCE + 80))
			return FALSE;
		else if ((currentDirection_g == west) && (distanceValue_g[EAST] < MAX_WALL_DISTANCE + 80))
			return FALSE;
	}
	/*
    if ((isChangeDetected() == TRUE) && (distanceToFrontWall_g > MAX_WALL_DISTANCE))
    {
        return TRUE;    // I detta fall är det en T_CROSSING från sidan, eller ut från en korsning, eller en CORRIDOR
    }
	
	
    else if ((isChangeDetected() == TRUE) && (distanceToFrontWall_g < CLOSE_ENOUGH_TO_WALL))
    {
        return TRUE;    // Förändringen va en vägg där fram, nu har roboten gått tillräckligt långt
    }
	
	
	else if (isChangeDetected() == TRUE) //----------------------------------------------------------------------------testitesti
	{
		chooseAndSetFrontSensors();
		return TRUE;
	} //---------------------------------------------------------------------------------------------------------------
	*/

	if (isChangeDetected() == TRUE)
	{
		if ((isDeadEnd() == TRUE) && (lastAddedNodeIndex_g > 1) && (currentNode_g.whatNode != DEAD_END) && (currentNode_g.whatNode != END_OF_MAZE))
		{
			if (distanceToFrontWall_g < CLOSE_ENOUGH_TO_WALL)
				return TRUE;
			else
				return FALSE;
		}
		return TRUE;
	}
    else
        return FALSE;
}

// MapMode
int whatNodeType()
{
    if (tempNorthAvailible_g + tempEastAvailible_g + tempSouthAvailible_g + tempWestAvailible_g == 3)
    {
        return T_CROSSING;            // Om det finns 3 vägar så är det en vägvalsnod
    }
    else if (tempNorthAvailible_g + tempEastAvailible_g + tempSouthAvailible_g + tempWestAvailible_g == 2)
    {
        if (tempNorthAvailible_g == tempSouthAvailible_g)
            return CORRIDOR;        // Detta måste vara en korridor
        else
            return TURN; // Detta måste vara en 2vägskorsning
    }
    else if (tempNorthAvailible_g + tempEastAvailible_g + tempSouthAvailible_g + tempWestAvailible_g == 1)
    {
        int i = lastAddedNodeIndex_g;
        for (i; i > 0; i--)
        {
            if ((nodeArray[i].nodeID == 1) && (nodeArray[i].pathsExplored == 2))
            {
                return END_OF_MAZE;   // Slutet på labyrinten
            }
        }
        
		return DEAD_END;             // Detta måste vara en återvändsgränd
    }
    else // Här är summan lika med 0, dvs väggar på alla sidor, bör betyda Z-sväng
    {
        return Z_CROSSING;           // Funkar egentligen inte, en Z-sväng hanteras som två turns
    }
}

int TcrossingID(int whatNode_)
{
    if (whatNode_ == T_CROSSING)
    {
        if (currentTcrossing_g == 0)    // Första T-korsningen
        {
            currentTcrossing_g = 1;
            TcrossingsFound_g = 1;
            return currentTcrossing_g;
        }
        else if (nodeArray[lastAddedNodeIndex_g].whatNode == DEAD_END)
        {
            return currentTcrossing_g;
        }
        else if (nodeArray[lastAddedNodeIndex_g].whatNode == T_CROSSING)
        {
            int j = currentTcrossing_g;
            for (j; j > 0 ; j--)                    // Den här magiska forloopen letar rätt på en T_CROSSING från höger
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

    if (whatNode_ == T_CROSSING)
    {
        int ID = nodeID_;

        int i;
        for(i = 0; i < lastAddedNodeIndex_g; i++)
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
void updateCurrentNode()          // Uppdaterar tempnoden, eg. skapar en ny nod
{
    currentNode_g.whatNode = whatNodeType();
    currentNode_g.nodeID = TcrossingID(currentNode_g.whatNode);                                     // Är alltid 0 om det inte är en T_CROSSING
    currentNode_g.pathsExplored = calcPathsExplored(currentNode_g.whatNode, currentNode_g.nodeID);     // Är alltid 0 om det inte är en T_CROSSING
    currentNode_g.wayIn = whatWayIn();
    currentNode_g.nextDirection =  noDirection;            // Sätts till noDirection och ändras i styrbeslut
    currentNode_g.northAvailible = tempNorthAvailible_g;
    currentNode_g.eastAvailible = tempEastAvailible_g;
    currentNode_g.southAvailible = tempSouthAvailible_g;
    currentNode_g.westAvailible = tempWestAvailible_g;
    currentNode_g.containsLeak = FALSE;                    // En ny nod kan inte initieras med en läcka
    currentNode_g.leakID = 0;
}

void placeNodeInArray()
{
    nodeArray[lastAddedNodeIndex_g].whatNode        = currentNode_g.whatNode;
    nodeArray[lastAddedNodeIndex_g].nodeID          = currentNode_g.nodeID;
    nodeArray[lastAddedNodeIndex_g].pathsExplored   = currentNode_g.pathsExplored;
    nodeArray[lastAddedNodeIndex_g].wayIn           = currentNode_g.wayIn;
    nodeArray[lastAddedNodeIndex_g].nextDirection   = currentNode_g.nextDirection;
    nodeArray[lastAddedNodeIndex_g].northAvailible  = currentNode_g.northAvailible;
    nodeArray[lastAddedNodeIndex_g].eastAvailible   = currentNode_g.eastAvailible;
    nodeArray[lastAddedNodeIndex_g].southAvailible  = currentNode_g.southAvailible;
    nodeArray[lastAddedNodeIndex_g].westAvailible   = currentNode_g.westAvailible;
    nodeArray[lastAddedNodeIndex_g].containsLeak    = currentNode_g.containsLeak;
    nodeArray[lastAddedNodeIndex_g].leakID          = currentNode_g.leakID;
}

void makeLeakPath(char wantedLeak_g)
{
    int index = 0;
    int i = 0;
    for (i; i < 121; i++)
    {
        if (nodeArray[i].whatNode == T_CROSSING)
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
        else if (nodeArray[i].containsLeak == TRUE)
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
    if (currentNode_g.eastAvailible == TRUE)
    {
        return east;
    }
    else if (currentNode_g.northAvailible == TRUE)
    {
        return north;
    }
    else if (currentNode_g.westAvailible == TRUE)
    {
        return west;
    }
    else
    {
        return south;       // I detta fall är det en DEAD_END
    }
}


int eastToNext()
{
    if (currentNode_g.southAvailible == TRUE)
    {
        return south;
    }
    else if (currentNode_g.eastAvailible == TRUE)
    {
        return east;
    }
    else if (currentNode_g.northAvailible == TRUE)
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
    if (currentNode_g.westAvailible == TRUE)
    {
        return west;
    }
    else if (currentNode_g.southAvailible == TRUE)
    {
        return south;
    }
    else if (currentNode_g.eastAvailible == TRUE)
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
    if (currentNode_g.northAvailible == TRUE)
    {
        return north;
    }
    else if (currentNode_g.westAvailible == TRUE)
    {
        return west;
    }
    else if (currentNode_g.southAvailible == TRUE)
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
    enum direction tempDirection = currentDirection_g;
    if (currentNode_g.whatNode == CORRIDOR)
    {
        return currentDirection_g;
    }
    else
    {
        if (currentDirection_g == north)
        {
            tempDirection = northToNext();
            return tempDirection;
        }
        else if (currentDirection_g == east)
        {
            tempDirection = eastToNext();
            return tempDirection;
        }
        else if (currentDirection_g == south)
        {
            tempDirection = southToNext();
            return tempDirection;
        }
        else if (currentDirection_g == west)
        {
            tempDirection = westToNext();
            return tempDirection;
        }
    }
}

void initNodeAndSteering()
{
    // Börjar i en återvändsgränd med norr som frammåt
    nodeArray[0].whatNode = MAZE_START;
    nodeArray[0].nodeID = 0;                // Nodens ID initieras som 0, ändras om det är en T_CROSSING
    nodeArray[0].pathsExplored = 0;
    nodeArray[0].wayIn = north;
    nodeArray[0].nextDirection = north;
    nodeArray[0].northAvailible = TRUE;     // Börjar i återvändsgränd med tillgänglig rikt. norr
    nodeArray[0].eastAvailible = FALSE;
    nodeArray[0].southAvailible = FALSE;
    nodeArray[0].westAvailible = FALSE;
    nodeArray[0].containsLeak = FALSE;
    nodeArray[0].leakID = 0;
}

int nodesAndControl()
{
	int newNodeAdded = FALSE;
    switch(currentControlMode_g)
    {
        case exploration  :

            updateTempDirections();
			
			if (distanceToFrontWall_g < MAX_WALL_DISTANCE - 40)
			{
				chooseAndSetFrontSensors();
			}
			
            updateLeakInfo();           // Kollar ifall läcka finns, och lägger till i noden om det fanns

            if (checkIfNewNode() == TRUE)
            {
                updateCurrentNode();
                directionHasChanged = TRUE;
                nextDirection_g = decideDirection();
                
                if (canMakeNew() == TRUE)
                {
                    lastAddedNodeIndex_g ++;
                    placeNodeInArray();
                    nodeArray[lastAddedNodeIndex_g].nextDirection = nextDirection_g;     // lägger in styrbeslut i arrayen
					newNodeAdded = TRUE;
                }
            }

            if (nodeArray[lastAddedNodeIndex_g].whatNode == END_OF_MAZE)
            {
                currentControlMode_g = waitForInput;
            }
            break;

        case waitForInput  :

            switch(currentOptionInstruction_g)
            {
                case 0b11100000  :
                    wantedLeak = 1;
                    makeLeakPath(wantedLeak);
                    currentControlMode_g = returnToLeak;
                    break;
                case 0b11100001  :
                    wantedLeak = 2;
                    makeLeakPath(wantedLeak);
                    currentControlMode_g = returnToLeak;
                    break;
                case 0b11100010  :
                    wantedLeak = 3;
                    makeLeakPath(wantedLeak);
                    currentControlMode_g = returnToLeak;
                    break;
                case 0b11100011  :
                    wantedLeak = 4;
                    makeLeakPath(wantedLeak);
                    currentControlMode_g = returnToLeak;
                    break;
                case 0b11100100  :
                    wantedLeak = 5;
                    makeLeakPath(wantedLeak);
                    currentControlMode_g = returnToLeak;
                    break;
                default  :
                    break;
            }
            
            if (currentDirection_g != noDirection)
            {
                directionHasChanged = TRUE;
                nextDirection_g = noDirection;
                // skicka info om detta till datorn
            }
            break;

        case returnToLeak  :
			{
			updateTempDirections();

			if (checkIfNewNode() == TRUE)
			{
				updateCurrentNode();

				if (currentNode_g.whatNode == T_CROSSING)
				{
					directionHasChanged = TRUE;
					nextDirection_g = pathToLeak[currentPath];
					currentPath ++;
					// skicka info om detta till datorn
				}
				else
				{
					nextDirection_g = decideDirection();
					directionHasChanged = TRUE;
					// Skicka info om detta till datorn
				}
			}

			if (pathToLeak[currentPath] == noDirection)     // Roboten har nu passerat det sista vägvalet
			{
				if (validLeak() == TRUE)
				{
					if (currentNode_g.containsLeak == FALSE)   // Denna koll gör att samma läcka inte hanteras igen som en ny
					{
						currentNode_g.containsLeak = TRUE;

						if (leaksToPass_g == 0)
						{
							if (currentDirection_g != noDirection)
							{
								directionHasChanged = TRUE;     // Läckan är nu hittad
								nextDirection_g = noDirection;
							}
						}
						else
						leaksToPass_g --;
					}
				}
			}
			break;
			}
        default:
			break;
    }
	return newNodeAdded;
}
