/*
 * nodsystemet.c
 *
 * Skapad: 4/20/2015 
 * Author: Rasmus Vilhelmsson
 * Version: 1.0
 *
 */ 

#include <stdio.h>
#include "Definitions.h"
#include "nodsystemet.h"

#define MAX_T_CROSSINGS 10

int pathToLeak[MAX_T_CROSSINGS] = {4, 4, 4, 4, 4, 4, 4, 4, 4, 4}; // noDirection = 4
int pathBackHome[MAX_T_CROSSINGS] = {4, 4, 4, 4, 4, 4, 4, 4, 4, 4}; // noDirection = 4

#define CLOSE_ENOUGH_TO_WALL 280  // Roboten går rakt fram tills den här längden

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
int deadEnds = 0;
int currentPath = 0;
int currentPathHome = 0;

int done_g = FALSE;

int wantedLeak = 0;

/*
 * Denna funktion omvandlar ett nodobjekt till en int som kan skickas via bluetooth till 
 * PC:n enligt ett gränssnitt som finns dokumenterat i den tekniska dokumentationen.
 */
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

/*
 * Hämtar ut en specifik nod ur buffern.
 */
simpleNode getNode(NodeRingBuffer* buffer, uint8_t elementID)
{
    return buffer->array[elementID];
}

/*
 * Lägger in en nod i buffern. Observera att det alltid finns exakt 5 noder i buffern. 
 * Den äldsta noden i buffern skrivs alltså över. 
 */
void addNode(NodeRingBuffer* buffer, simpleNode newNode)
{
    buffer->array[buffer->writeIndex] = newNode;
    if (buffer->writeIndex <= 3)
        buffer->writeIndex++;
    else
        buffer->writeIndex = 0;
}

/*
 * Fyller den globalt definierade buffern med återvändsgränder. 
 * Anledningen till detta är att roboten inte skall ta konstiga beslut vid uppstart.
 */
void initNodeRingBuffer()
{
	simpleNode dummyDeadEnd = {1,0,0,0};
	addNode(&nodeRingBuffer, dummyDeadEnd);
	addNode(&nodeRingBuffer, dummyDeadEnd);
	addNode(&nodeRingBuffer, dummyDeadEnd);
	addNode(&nodeRingBuffer, dummyDeadEnd);
	addNode(&nodeRingBuffer, dummyDeadEnd);
}

/*
 * Skapar en simpleNode av den senast avlästa sensordatan och fyller ringbuffern med denna data.
 * Används endast då vi lagt in en ny nod i listan. Syftet med detta är att minsta osäkerheten vid 
 * övergångar mellan två nodtyper.
 */
void fillNodeMemoryWithTemp()
{
	simpleNode simpleTempNode = {tempNorthAvailible_g, tempEastAvailible_g, tempSouthAvailible_g, tempWestAvailible_g};
	addNode(&nodeRingBuffer, simpleTempNode);
	addNode(&nodeRingBuffer, simpleTempNode);
	addNode(&nodeRingBuffer, simpleTempNode);
	addNode(&nodeRingBuffer, simpleTempNode);
	addNode(&nodeRingBuffer, simpleTempNode);
}


/*#define L_DEADEND 400
#define L_SIDE 450
#define L_WIDTH 490 //570
#define L_TWALL 470
#define L_TWALL_LONG 490 //600
#define L_TURN 500*/

#define L_FRONTWALL 350
#define L_DEADEND 280
#define L_SIDE 460
#define L_WIDTH 430 //570
#define L_TWALL 470
#define L_TWALL_LONG 490 //600
#define L_TURN 500
#define MAX_CORRIDOR_DISTANCE 350

/*
* Uppdaterar temporära avstånd och
* returnerar TRUE eller FALSE om en 
* riktning anses vara öppen
*/
void updateTempDirections()
{
	int frontDistance_;
	int backDistance_;
	int rightSide_;
	int leftSide_;
	int rightDistance_ = distanceValue_g[FRONT_RIGHT];
	int leftDistance_ = distanceValue_g[FRONT_LEFT];
	if (currentDirection_g == north)
	{
		distanceToFrontWall_g = distanceValue_g[NORTH];
		frontDistance_ = distanceValue_g[NORTH];
		backDistance_ = distanceValue_g[SOUTH];
		rightSide_ = distanceValue_g[EAST];
		leftSide_ = distanceValue_g[WEST];
	} 
	else if (currentDirection_g == east)
	{
		distanceToFrontWall_g = distanceValue_g[EAST];
		frontDistance_ = distanceValue_g[EAST];
		backDistance_ = distanceValue_g[WEST];
		rightSide_ = distanceValue_g[SOUTH];
		leftSide_ = distanceValue_g[NORTH];
	} 
	else if (currentDirection_g == south)
	{
		distanceToFrontWall_g = distanceValue_g[SOUTH];
		frontDistance_ = distanceValue_g[SOUTH];
		backDistance_ = distanceValue_g[NORTH];
		rightSide_ = distanceValue_g[WEST];
		leftSide_ = distanceValue_g[EAST];
	} 
	else if (currentDirection_g == west)
	{
		distanceToFrontWall_g = distanceValue_g[WEST];
		frontDistance_ = distanceValue_g[WEST];
		backDistance_ = distanceValue_g[EAST];
		rightSide_ = distanceValue_g[NORTH];
		leftSide_ = distanceValue_g[SOUTH];
	}

	if (frontDistance_ < L_FRONTWALL)
	{
		if ((rightSide_ > L_WIDTH - 20) && (leftSide_ > L_WIDTH - 20) &&    (backDistance_ > L_SIDE))
		{
			// In i T-korsning
			if (currentDirection_g == north)
			{
				tempNorthAvailible_g = FALSE;
				tempEastAvailible_g = TRUE;
				tempSouthAvailible_g = TRUE;
				tempWestAvailible_g = TRUE;
		
		
			} else if (currentDirection_g == EAST)
			{
				tempNorthAvailible_g = TRUE;
				tempEastAvailible_g = FALSE;
				tempSouthAvailible_g = TRUE;
				tempWestAvailible_g = TRUE;
		
		
			} else if (currentDirection_g == SOUTH)
			{
				tempNorthAvailible_g = TRUE;
				tempEastAvailible_g = TRUE;
				tempSouthAvailible_g = FALSE;
				tempWestAvailible_g = TRUE;
		
		
			} else if (currentDirection_g == WEST)
			{
				tempNorthAvailible_g = TRUE;
				tempEastAvailible_g = TRUE;
				tempSouthAvailible_g = TRUE;
				tempWestAvailible_g = FALSE;
			}	

		} else if ((rightSide_ < L_SIDE - 50) && (leftSide_ > L_WIDTH))
		{
			// Sväng vänster
			if (currentDirection_g == north)
			{
				tempNorthAvailible_g = FALSE;
				tempEastAvailible_g = FALSE;
				tempSouthAvailible_g = TRUE;
				tempWestAvailible_g = TRUE;
				
				
			} else if (currentDirection_g == EAST)
			{
				tempNorthAvailible_g = TRUE;
				tempEastAvailible_g = FALSE;
				tempSouthAvailible_g = FALSE;
				tempWestAvailible_g = TRUE;
				
				
			} else if (currentDirection_g == SOUTH)
			{
				tempNorthAvailible_g = TRUE;
				tempEastAvailible_g = TRUE;
				tempSouthAvailible_g = FALSE;
				tempWestAvailible_g = FALSE;
				
				
			} else if (currentDirection_g == WEST)
			{
				tempNorthAvailible_g = FALSE;
				tempEastAvailible_g = TRUE;
				tempSouthAvailible_g = TRUE;
				tempWestAvailible_g = FALSE;
			}

		} else if ((rightSide_ > L_WIDTH) && (leftSide_ < L_SIDE - 50))
		{
			// Sväng höger
			if (currentDirection_g == north)
			{
				tempNorthAvailible_g = FALSE;
				tempEastAvailible_g = TRUE;
				tempSouthAvailible_g = TRUE;
				tempWestAvailible_g = FALSE;
				
				
			} else if (currentDirection_g == EAST)
			{
				tempNorthAvailible_g = FALSE;
				tempEastAvailible_g = FALSE;
				tempSouthAvailible_g = TRUE;
				tempWestAvailible_g = TRUE;
				
				
			} else if (currentDirection_g == SOUTH)
			{
				tempNorthAvailible_g = TRUE;
				tempEastAvailible_g = FALSE;
				tempSouthAvailible_g = FALSE;
				tempWestAvailible_g = TRUE;
				
				
			} else if (currentDirection_g == WEST)
			{
				tempNorthAvailible_g = TRUE;
				tempEastAvailible_g = TRUE;
				tempSouthAvailible_g = FALSE;
				tempWestAvailible_g = FALSE;
			}
			
			
			
		} else if ((rightSide_ < L_SIDE) && (leftSide_ < L_SIDE) && (frontDistance_ < L_DEADEND))
		{
		// Återvändsgränd
		// Kolla summa av sidoavstånd + robotbredd, kan vara sväng eller
			if ((rightSide_ + leftSide_ + 140) > 620) // 140 sensorernas avstånd från varandra
			{
			
			} else if (currentDirection_g == north)
			{
				tempNorthAvailible_g = FALSE;
				tempEastAvailible_g = FALSE;
				tempSouthAvailible_g = TRUE;
				tempWestAvailible_g = FALSE;
			
			
			} else if (currentDirection_g == EAST)
			{
				tempNorthAvailible_g = FALSE;
				tempEastAvailible_g = FALSE;
				tempSouthAvailible_g = FALSE;
				tempWestAvailible_g = TRUE;
			
			
			} else if (currentDirection_g == SOUTH)
			{
				tempNorthAvailible_g = TRUE;
				tempEastAvailible_g = FALSE;
				tempSouthAvailible_g = FALSE;
				tempWestAvailible_g = FALSE;
			
			
			} else if (currentDirection_g == WEST)
			{
				tempNorthAvailible_g = FALSE;
				tempEastAvailible_g = TRUE;
				tempSouthAvailible_g = FALSE;
				tempWestAvailible_g = FALSE;
			}

		}
	} else if ((rightSide_ < L_SIDE) && (leftSide_ > L_SIDE))
	{
		if ((frontDistance_ > L_SIDE + 20) && (backDistance_ > L_SIDE + 20))
		{
			// T-korsning vänster
			if (currentDirection_g == north)
			{
				tempNorthAvailible_g = TRUE;
				tempEastAvailible_g = FALSE;
				tempSouthAvailible_g = TRUE;
				tempWestAvailible_g = TRUE;
				
				
			} else if (currentDirection_g == EAST)
			{
				tempNorthAvailible_g = TRUE;
				tempEastAvailible_g = TRUE;
				tempSouthAvailible_g = FALSE;
				tempWestAvailible_g = TRUE;
				
				
			} else if (currentDirection_g == SOUTH)
			{
				tempNorthAvailible_g = TRUE;
				tempEastAvailible_g = TRUE;
				tempSouthAvailible_g = TRUE;
				tempWestAvailible_g = FALSE;
				
				
			} else if (currentDirection_g == WEST)
			{
				tempNorthAvailible_g = FALSE;
				tempEastAvailible_g = TRUE;
				tempSouthAvailible_g = TRUE;
				tempWestAvailible_g = TRUE;
			}
			
			
		}
	} else if ((rightSide_ > L_SIDE) && (leftSide_ < L_SIDE))
	{
		if ((frontDistance_ > L_SIDE + 20) && (backDistance_ > L_SIDE + 20))
		{
			// T-korsning höger
			if (currentDirection_g == north)
			{
				tempNorthAvailible_g = TRUE;
				tempEastAvailible_g = TRUE;
				tempSouthAvailible_g = TRUE;
				tempWestAvailible_g = FALSE;
				
				
			} else if (currentDirection_g == EAST)
			{
				tempNorthAvailible_g = FALSE;
				tempEastAvailible_g = TRUE;
				tempSouthAvailible_g = TRUE;
				tempWestAvailible_g = TRUE;
				
				
			} else if (currentDirection_g == SOUTH)
			{
				tempNorthAvailible_g = TRUE;
				tempEastAvailible_g = FALSE;
				tempSouthAvailible_g = TRUE;
				tempWestAvailible_g = TRUE;
				
				
			} else if (currentDirection_g == WEST)
			{
				tempNorthAvailible_g = TRUE;
				tempEastAvailible_g = TRUE;
				tempSouthAvailible_g = FALSE;
				tempWestAvailible_g = TRUE;
			}
			
			
		}
	} else if ((rightSide_ < L_SIDE - 60) && (leftSide_ < L_SIDE - 60))
	{
		if (frontDistance_ > L_SIDE)
		{
			// Korridor
			if ((currentDirection_g == north) || (currentDirection_g == south))
			{
				tempNorthAvailible_g = TRUE;
				tempEastAvailible_g = FALSE;
				tempSouthAvailible_g = TRUE;
				tempWestAvailible_g = FALSE;
			} else if ((currentDirection_g == west) || (currentDirection_g == east))
			{
				tempNorthAvailible_g = FALSE;
				tempEastAvailible_g = TRUE;
				tempSouthAvailible_g = FALSE;
				tempWestAvailible_g = TRUE;
			}		
		}
	}
}

/*
* Denna funktion gör en rimlighetskoll
* om läckan som är synlig är en faktiskt
* läcka eller bara eventuellt brus.
* Denna ändrades i slutet av projektet till att
* alltid returnera TRUE om det är en synlig läcka,
* men kan enkelt ändras tillbaka genom att ändra 
* "(actualLeak == 0)" till 2 exempelvis
*/
int validLeak()
{
    if (isLeakVisible_g == TRUE)
    {
		if (actualLeak_g == 0)
        {
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

/*
* Uppdaterar en temporär läcka-variabel,
* värdena är TRUE eller FALSE
*/
void updateLeakInfo()
{
    if ((validLeak() == TRUE) &&
    (nodeArray[lastAddedNodeIndex_g].containsLeak == FALSE) &&     // Innehåller noden redan en läcka?
    !(nodeArray[lastAddedNodeIndex_g].whatNode == T_CROSSING) &&    // Läckor får ej finnas i T-korsningar
    !(nodeArray[lastAddedNodeIndex_g].whatNode == DEAD_END) && 	// Läckor hamnar innanför en DEAD_END, och detta gör att läckor inte läggs till på tillbakavägen
    (newLeak_g == TRUE))                                           // Kollar så vi inte redan lagt till denna läcka
    {                                                       
        leaksFound_g ++;
        newLeak_g = FALSE;                                          // Vi har lagt till läckan

        nodeArray[lastAddedNodeIndex_g].containsLeak = TRUE;       // Uppdaterar att noden har en läcka i sig
        nodeArray[lastAddedNodeIndex_g].leakID = leaksFound_g;     // Läckans id får det nummer som den aktuella läckan har
    }
}

/*
* Denna funktion kollar om den nya noden som hittats
* får läggas till i listan, detta för att inte lägga
* in noder på redan utforskad väg
*/
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

// Använder nodeRingBuffer för att ta ett majoritetsbeslut och uppdaterar tempDirections därefter.
void decideChangeFromMajority()
{
    // Skapa och lägg in ny simpleNode i ringbuffern.
    simpleNode newNode = {tempNorthAvailible_g, tempEastAvailible_g, tempSouthAvailible_g, tempWestAvailible_g};
    addNode(&nodeRingBuffer, newNode);
    
    uint8_t votesFor = 0;

    uint8_t resultNorth;
    uint8_t resultEast;
    uint8_t resultSouth;
    uint8_t resultWest;
    
    simpleNode node1 = getNode(&nodeRingBuffer, 0);
    simpleNode node2 = getNode(&nodeRingBuffer, 1);
    simpleNode node3 = getNode(&nodeRingBuffer, 2);
    simpleNode node4 = getNode(&nodeRingBuffer, 3);
    simpleNode node5 = getNode(&nodeRingBuffer, 4);
    
    uint8_t vote1;
    uint8_t vote2;
    uint8_t vote3;
    uint8_t vote4;
    uint8_t vote5;
    
    // Omröstning för northAvailable
    vote1 = node1.northAvailable;
    vote2 = node2.northAvailable;
    vote3 = node3.northAvailable;
    vote4 = node4.northAvailable;
    vote5 = node5.northAvailable;
    votesFor = vote1 + vote2 + vote3 + vote4 + vote5;
    if (votesFor >= 3) // Majoritet öppet
    {
        resultNorth = TRUE;
    }
    else if(votesFor <= 2) // Majoritet stängt
    {
        resultNorth = FALSE;
    }
    
    // Omröstning för eastAvailable
    vote1 = node1.eastAvailable;
    vote2 = node2.eastAvailable;
    vote3 = node3.eastAvailable;
    vote4 = node4.eastAvailable;
    vote5 = node5.eastAvailable;
    votesFor = vote1 + vote2 + vote3 + vote4 + vote5;
    if (votesFor >= 3) // Majoritet öppet
    {
        resultEast = TRUE;
    }
    else if(votesFor <= 2) // Majoritet stängt
    {
        resultEast = FALSE;
    }
    
    // Omröstning för southAvailable
    vote1 = node1.southAvailable;
    vote2 = node2.southAvailable;
    vote3 = node3.southAvailable;
    vote4 = node4.southAvailable;
    vote5 = node5.southAvailable;
    votesFor = vote1 + vote2 + vote3 + vote4 + vote5;
    if (votesFor >= 3) // Majoritet öppet
    {
        resultSouth = TRUE;
    }
    else if(votesFor <= 2) // Majoritet stängt
    {
        resultSouth = FALSE;
    }
    
    // Omröstning för westAvailable
    vote1 = node1.westAvailable;
    vote2 = node2.westAvailable;
    vote3 = node3.westAvailable;
    vote4 = node4.westAvailable;
    vote5 = node5.westAvailable;
    votesFor = vote1 + vote2 + vote3 + vote4 + vote5;
    if (votesFor >= 3) // Majoritet öppet
    {
        resultWest = 1;
    }
    else if(votesFor <= 2) // Majoritet stängt
    {
        resultWest = 0;
    }
    
     // Det finns alltid majoritet i alla beslut
   
    tempNorthAvailible_g  = resultNorth;
    tempEastAvailible_g  = resultEast;
    tempSouthAvailible_g = resultSouth;
    tempWestAvailible_g = resultWest;
}

/*
* Kollar om förändring har skett med temporära riktningar
*/
int isChangeDetected()
{
    if ((currentNode_g.northAvailible == tempNorthAvailible_g) && (currentNode_g.eastAvailible == tempEastAvailible_g) &&
        (currentNode_g.southAvailible == tempSouthAvailible_g) && (currentNode_g.westAvailible == tempWestAvailible_g))
    {
        return FALSE;
    }
    else
    {
        return TRUE;
    }
}

int tempIsDeadEnd()
{
	if (tempNorthAvailible_g + tempEastAvailible_g + tempSouthAvailible_g + tempWestAvailible_g == 1)
		return TRUE;
	else
		return FALSE;
}

int tempIsTCrossing()
{
	if (tempNorthAvailible_g + tempEastAvailible_g + tempSouthAvailible_g + tempWestAvailible_g == 3)
		return TRUE;
	else
		return FALSE;
}

/*
* Denna funktion kollar om roboten har
* hamnat i en ny nod
*/
int checkIfNewNode()
{
    decideChangeFromMajority(); 
	if (isChangeDetected() == TRUE)
	{
		if ((currentNode_g.whatNode == T_CROSSING) && tempIsTCrossing())
			return FALSE;
		return TRUE;
	} 
    else
        return FALSE;
}

/*
* Kollar vilken typ av nod
* som roboten har hamnat i
*/
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
            return TURN; // Detta måste vara en sväng
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
		
		if ((currentTcrossing_g == 0) && (deadEnds == 2))
		{
			return END_OF_MAZE;
			deadEnds = 0;
		}
			
		deadEnds ++;
		return DEAD_END;             // Detta måste vara en återvändsgränd
    }
    else // Här är summan lika med 0, dvs väggar på alla sidor, bör betyda Z-sväng
    {
        return Z_CROSSING;           // Funkar egentligen inte, en Z-sväng hanteras som två turns
    }
}

/*
* Kollar vilket ID som ska tilldelas till
* alla T-korsningar som hittas
*/
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
            {                                       // i arrayen och kollar om den var "fylld" med pathsExplored
                int i;                              // och va den fylld kollar den på den tidigare T-korsningen
                for (i = 120; i>1 ; i--)
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

/*
* Beräknar hur många vägar som har utforskat från
* en T-korsning med ett specifikt ID
*/
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

/*
* Funktion som returnerar den motsatta riktningen
* som funktionen anropas med
*/
int inverseThisDirection(int dir_)
{
	if (dir_ == north)
	return south;
	else if (dir_ == east)
	return west;
	else if (dir_ == south)
	return north;
	else if (dir_ == west)
	return east;
	else
	return noDirection;
}

/*
* Funktion som returnerar vägen in i en nod
*/
int whatWayIn()
{
	return inverseThisDirection(currentDirection_g);
}

/*
* Uppdaterar den temporära noden som roboten hamnar i,
* denna körs även de gånger då nya noder inte får läggas till
*/
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

/*
* Lägger in den temporära noden i arrayen som
* som fungerar som en lista med noder
*/
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

/*
* Ränkar ut kortast väg i labyrinten 
* till efterfrågad läcka
*/
void makePathToLeak(int findThisLeak_)
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
				{
					pathToLeak[0] = noDirection;
				}
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
            if (nodeArray[i].leakID == findThisLeak_)
                break;
            else
                leaksToPass_g ++;
        }
    }
}

/*
* Returnerar det styrbeslut som roboten
* ska ta om den innan gått i riktningen norr
*/
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

/*
* Returnerar det styrbeslut som roboten
* ska ta om den innan gått i riktningen öst
*/
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

/*
* Returnerar det styrbeslut som roboten
* ska ta om den innan gått i riktningen syd
*/
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

/*
* Returnerar det styrbeslut som roboten
* ska ta om den innan gått i riktningen väst
*/
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

/*
* Hanterar styrbeslut i det autonoma läget
*/
int decideDirection()
{
	if (done_g == TRUE)
	{
		return noDirection;
	}
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

/*
* Initiering till nodsystemet för roboten
*/
void initNodeAndSteering()
{
    // Börjar i en återvändsgränd med norr som frammåt
    nodeArray[0].whatNode = MAZE_START;
    nodeArray[0].nodeID = 0;
    nodeArray[0].pathsExplored = 0;
    nodeArray[0].wayIn = north;
    nodeArray[0].nextDirection = north;
    nodeArray[0].northAvailible = TRUE;
    nodeArray[0].eastAvailible = FALSE;
    nodeArray[0].southAvailible = FALSE;
    nodeArray[0].westAvailible = FALSE;
    nodeArray[0].containsLeak = FALSE;
    nodeArray[0].leakID = 0;
	
	currentPathHome = 0;
	currentPath = 0;
}

/*
* Kollar om den temporära noden är öppen
* i den riktning som funktionen anropas med
*/
int currentNodeOpenInDirection(enum direction dir_)
{
    switch (dir_)
    {
        case north:
        {
            return currentNode_g.northAvailible;
        }
        case east:
        {
            return currentNode_g.eastAvailible;
        }
        case south:
        {
            return currentNode_g.southAvailible;
        }
        case west:
        {
            return currentNode_g.westAvailible;
        }
        case noDirection:
        {
            return FALSE;
        }
    }
}

/*
* Simulerar en korridor som passar den riktning
* som roboten står i, detta gör att i läget "returnHome" 
* kan den upptäcka ny nod när den vänder
*/
simulateCorridor(enum direction dirBack_)   // 
{
    if ((dirBack_ == north) || (dirBack_ == south))
        {
            tempNorthAvailible_g = TRUE;
            tempSouthAvailible_g = TRUE;
            tempWestAvailible_g = FALSE;
            tempEastAvailible_g = FALSE;
            updateCurrentNode();
        }
    else
    {
            tempNorthAvailible_g = FALSE;
            tempSouthAvailible_g = FALSE;
            tempWestAvailible_g = TRUE;
            tempEastAvailible_g = TRUE;
            updateCurrentNode();
    }
}

/*
* Funktion som körs kontinuerligt i main-loopen.
* Hanterar: kartläggning av labyrint, väntan på anrop
* att gå till en specifik läcka, att roboten går kortast väg
* till önskad läcka och att gå tillbaka samma väg till start
*/
int nodesAndControl()
{
    int nodeUpdated = FALSE;
    switch(currentControlMode_g)
    {
        case exploration  :

            updateTempDirections();
            updateLeakInfo();           // Kollar ifall läcka finns och lägger till infon i senast noden i listan om det fanns en läcka

            if (checkIfNewNode() == TRUE)
            {
				fillNodeMemoryWithTemp();
                updateCurrentNode();
                directionHasChanged = TRUE;
                nextDirection_g = decideDirection();
                // Detta gör så att roboten stannar så fort den ser en T-korsning där en turnblind ska göras. 
                
                if (currentNode_g.whatNode == T_CROSSING && currentDirection_g != nextDirection_g && currentNodeOpenInDirection(currentDirection_g))
                {
                    stopInTCrossing = TRUE;
                    emergencyStop();
                }
                
                nodeUpdated = TRUE;
                if (canMakeNew() == TRUE)
                {
                    lastAddedNodeIndex_g ++;
                    placeNodeInArray();
                    nodeArray[lastAddedNodeIndex_g].nextDirection = nextDirection_g;     // lägger in styrbeslut i arrayen
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
				
                case 0b11000000  :
					wantedLeak = 1;
                    makePathToLeak(wantedLeak);
                    currentControlMode_g = returnToLeak;
					directionHasChanged = TRUE;
					currentDirection_g = north;
					nextDirection_g = north;
                    break;
                case 0b11000001  :
                    wantedLeak = 2;
                    makePathToLeak(wantedLeak);
                    currentControlMode_g = returnToLeak;
                    directionHasChanged = TRUE;
					currentDirection_g = north;
					nextDirection_g = north;
                    break;
                case 0b11000010  :
                    wantedLeak = 3;
                    makePathToLeak(wantedLeak);
                    currentControlMode_g = returnToLeak;
                    directionHasChanged = TRUE;
					currentDirection_g = north;
					nextDirection_g = north;
                    break;
                case 0b11000011  :
                    wantedLeak = 4;
                    makePathToLeak(wantedLeak);
                    currentControlMode_g = returnToLeak;
                    directionHasChanged = TRUE;
					currentDirection_g = north;
					nextDirection_g = north;
                    break;
                case 0b11000100  :
                    wantedLeak = 5;
                    makePathToLeak(wantedLeak);
                    currentControlMode_g = returnToLeak;
                    directionHasChanged = TRUE;
					currentDirection_g = north;
					nextDirection_g = north;
                    break;
                default  :
                    break;
            }
            
            if ((currentDirection_g != noDirection) && (currentControlMode_g == waitForInput))
            {
                directionHasChanged = TRUE;
                nextDirection_g = noDirection;
            }
            break;

        case returnToLeak  :
		{
				
			updateTempDirections();

			if (checkIfNewNode() == TRUE)
			{
				fillNodeMemoryWithTemp();
				updateCurrentNode();

				if (currentNode_g.whatNode == T_CROSSING)
				{
					pathBackHome[currentPath] = inverseThisDirection(currentDirection_g);
					directionHasChanged = TRUE;
					nextDirection_g = pathToLeak[currentPath];
					currentPath ++;
				}
				else
				{
					nextDirection_g = decideDirection();
					directionHasChanged = TRUE;
				}
				
				if (currentNode_g.whatNode == T_CROSSING && currentDirection_g != nextDirection_g && currentNodeOpenInDirection(currentDirection_g))
				{
					stopInTCrossing = TRUE;
					emergencyStop();
				}
				
				nodeUpdated = TRUE;
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
								nextDirection_g = inverseThisDirection(currentDirection_g);
                                simulateCorridor(nextDirection_g);
                                currentDirection_g = nextDirection_g; 
                                nodeUpdated = TRUE;
								currentPathHome = currentPath - 1;
								currentControlMode_g = returnHome;
                                switch(wantedLeak) // Tänd lampor på roboten
                                {
                                    case 1:
                                    {
                                        fetchDataFromSensorUnit(0b00010001);
                                        break;
                                    }
                                    case 2:
                                    {
                                        fetchDataFromSensorUnit(0b00010010);
                                        break;
                                    }
                                    case 3:
                                    {
                                        fetchDataFromSensorUnit(0b00010100);
                                        break;
                                    }
                                }                                    
							}
						}
						else
							leaksToPass_g --;
					}
				}
			}
			break;
		}
			
		case returnHome  :
		{
			updateTempDirections();
			
			if (checkIfNewNode() == TRUE)
			{
				fillNodeMemoryWithTemp();
				updateCurrentNode();

				if (currentPath == 0)
				{
					if ((currentNode_g.whatNode == DEAD_END) || (currentNode_g.whatNode == END_OF_MAZE))
					{
						done_g = TRUE;
						directionHasChanged = TRUE;
						nextDirection_g = noDirection;
						currentDirection_g = noDirection;
						currentControlMode_g = waitForInput;
					}
				}
				
				if (currentNode_g.whatNode == T_CROSSING)
				{
					directionHasChanged = TRUE;
					nextDirection_g = pathBackHome[currentPathHome];
					currentPathHome --;
					currentPath --;			// Fungerar här som en koll för att roboten veta vilken deadEnd som är home
				}
				else
				{
					directionHasChanged = TRUE;
					nextDirection_g = decideDirection();					
				}
				
				if (currentNode_g.whatNode == T_CROSSING && currentDirection_g != nextDirection_g && currentNodeOpenInDirection(currentDirection_g))
				{
					stopInTCrossing = TRUE;
					emergencyStop();
				}
				
				nodeUpdated = TRUE;
			}
			
			break;	
		}
        default:
			break;
    }
	return nodeUpdated;
}
