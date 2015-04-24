//
//  main.c
//  AVR-test
//
//  Created by Isak Wiberg on 2015-04-09.
//  Copyright (c) 2015 Isak Wiberg. All rights reserved.
//


// Rasmus
// Kartnavigeringsnodvägvalssystem

#include <stdio.h>


#define false (uint8_t)0
#define true  (uint8_t)1

#define maxNodes 121        // En nod i varje 57cm i en 6x6m bana skulle motsvara 121 stycken noder.

#define corridor  (uint8_t)0         // Dessa är möjliga tal i whatNode
#define turn	  (uint8_t)1
#define deadEnd   (uint8_t)2
#define Tcrossing (uint8_t)3
#define Zcrossing (uint8_t)4


//------------------------------------------------ Jocke har definierat dessa i sin kod
#define north (uint8_t)0
#define east  (uint8_t)1
#define south (uint8_t)2
#define west  (uint8_t)3
//------------------------------------------------ Ta bort dessa när denna kod integreras




uint8_t isLeakVisible_g = 0;
int 	northSensor_g = 0;
int 	eastSensor_g = 0;
int 	southSensor_g = 0;
int 	westSensor_g = 0;
int  	HARDCODEDDIRECTION = north;

int 	testHelper = 0;



/*
int northSensor1_g
int northSensor2_g
int eastSensor3_g
int eastSensor4_g
 ...
 Jocke fixar dessa
*/






// DET SKA FINNAS TVÅ MODES, ETT DÅ ROBOTEN MAPPAR UPP OCH GÖR DENNA ARRAY
// OCH ETT MODE DÅ ROBOTEN LETAR I LISTAN SOM REDAN FINNS, OCH GÅR TILL VALD LÄCKA
// lägger en kommentar ovanför funktionerna för att visa vilka som ska finna i läget då roboten mappar upp kartan (MapMode)




#define closeEnoughToWall (uint8_t)350  // Roboten går rakt fram tills den här längden
#define maxWallDistance (uint16_t)570   // Utanför denna längd är det ingen vägg

uint8_t tempNorthAvailible_g = true;
uint8_t tempEastAvailible_g = false;
uint8_t tempSouthAvailible_g = false;
uint8_t tempWestAvailible_g = false;
uint8_t currentNode_g = 0;
uint8_t actualLeak_g = 0;
uint8_t validChange_g = 0;
uint8_t currentDirection_g = north;
uint8_t leaksFound_g = 0;
uint8_t currentTcrossing_g = 0;
uint8_t TcrossingsFound_g = 0;
uint8_t distanceToFrontWall_g = 0;

struct node
{
    uint8_t     whatNode;        	// Alla typer av noder är definade som siffror
    uint8_t     nodeID;          	// Nodens unika id
    uint8_t		waysExplored		// Är bägge hållen utforskade är denna 2
    uint8_t     wayIn;
    uint8_t     nextDirection;   	// Väderstrecken är siffror som är definade
    uint8_t     northAvailible;  	// Finns riktningen norr i noden?   Om sant => 1, annars 0
    uint8_t     eastAvailible;
    uint8_t     southAvailible;
    uint8_t     westAvailible;
    uint8_t     containsLeak;    	// Finns läcka i "noden", kan bara finnas om det är en korridor
    uint8_t		leakID;			 	// Fanns en läcka får den ett unikt id, annars är denna 0
};

struct node nodeArray[maxNodes];


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

	if (northSensor_g > maxWallDistance) // Kan behöva ändra maxWallDistance
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
uint8_t validLeak()
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
	if ((validLeak() == true) &&                              // Faktisk läcka?
	(nodeArray[currentNode_g].containsLeak == false) &&   // Innehåller noden redan en läcka?
	(nodeArray[currentNode_g].whatNode == corridor))      // Läckor får bara finnas i nodtypen "corridor"
	{													  // Kollen ovan gör även att när roboten inte gör nya noder så kan inte läckor läggas till
		leaksFound_g ++;								  // eftersom whatNode i det fallet är Tcrossing eller deadEnd

		nodeArray[currentNode_g].containsLeak = true;         // Uppdaterar att korridornoden har en läcka sig
		nodeArray[currentNode_g].leakID = leaksFound_g;		  // Läckans id får det nummer som den aktuella läckan har
	}
}

// Ska bara finnas i MapMode
uint8_t canMakeNew()
{
	if ((nodeArray[currentNode_g].whatNode == deadEnd) &&                                               	// Var senaste noden en återvändsgränd (deadEnd)?
		!(tempNorthAvailible_g + tempEastAvailible_g + tempSouthAvailible_g + tempWestAvailible_g == 3))    // Detta kollar om roboten står i en T-korsning
	{
		return false; // Alltså: om senaste noden var en deadEnd och den nuvarande inte är en T-korsning ska inte nya noder göras
	}
	else if ((nodeArray[currentNode_g].whatNode == Tcrossing) &&											  // Var senaste noden en Tcrossing
			 (nodeArray[currentNode_g].waysExplored == 2) &&											  	  // Om senaste noden va en Tcrossing, har den untforskats helt?
			 !(tempNorthAvailible_g + tempEastAvailible_g + tempSouthAvailible_g + tempWestAvailible_g == 3)) // isåfall ska inte nya noder göras
	{
		return false;
	}
	else
	{
		return true;
	}

	// Lägg till saker här: på vägen tillbaka efter en Tcrossing ska inte nya noder göras
}

uint8_t isChangeDetected()
{
	if ((nodeArray[currentNode_g].northAvailible == tempNorthAvailible_g) && (nodeArray[currentNode_g].eastAvailible == tempEastAvailible_g) &&
		(nodeArray[currentNode_g].southAvailible == tempSouthAvailible_g) && (nodeArray[currentNode_g].westAvailible == tempWestAvailible_g))
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
uint8_t checkIfNewNode()
{
	if ((isChangeDetected() == true) && (distanceToFrontWall_g > maxWallDistance))
	{
		return true;	// I detta fall är det en Tcrossing från sidan, eller ut från en korsning, eller en corridor
	}
	else if ((isChangeDetected() == true) && (distanceToFrontWall_g < closeEnoughToWall))
	{
		return true;	// Förändringen va en vägg där fram, nu har roboten gått tillräckligt långt
	}
	else
		return false;
}

// MapMode
uint8_t whatNodeType()
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
		return deadEnd;             // Detta måste vara en återvändsgränd
	}
	else // Här är summan lika med 0, dvs väggar på alla sidor, bör betyda Z-sväng
	{
		return Zcrossing;		// Funkar egentligen inte, en Z-sväng hanteras som två turns
	}
}

uint8_t TcrossingID()
{
	if (nodeArray[currentNode_g].whatNode == Tcrossing)
	{
		if (currentTcrossing_g == 0)	// Första T-korsningen
		{
			currentTcrossing_g = 1;
			TcrossingsFound_g = 1;
			return currentTcrossing_g;	
		}
		else if (nodeArray[currentNode_g - 1].whatNode == deadEnd)
		{
			return currentTcrossing_g;
		}
		else if (nodeArray[currentNode_g - 1].whatNode == Tcrossing)
		{
			int j = currentTcrossing_g;
			for (j; j > 0 ; j--)				// Den här magiska forloopen letar rätt på en Tcrossing från höger
			{									// i arrayen och kollar om den var "fylld" med waysExplored
				for (int i = 120; i>1 ; i--)	// och va den fylld kollar den på den tidigare T-korsningen
				{
					if (nodeArray[i].nodeID == j - 1)
					{
						if(nodeArray[i].waysExplored == 2))
						{
							currentTcrossing_g --;
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

uint8_t calcWaysExplored()
{
	int ways = 0;

	if (nodeArray[currentNode_g].whatNode == Tcrossing)
	{
		int ID = nodeArray[currentNode_g].nodeID;

		for(int i = 0; i < currentNode_g; i++)
		{
			if (nodeArray[i].nodeID == ID)
				ways ++;
		}
	}
	return ways;
}

// Får finnas i bägge, behövs i MapMode
uint8_t whatWayIn()
{
	return currentDirection_g;
}

// Får finnas i bägge, behövs i MapMode
uint8_t whatsNextDirection()
{
	return HARDCODEDDIRECTION;
}

// MapMode
void createNewNode()    // Skapar en ny nod och lägger den i arrayen
{
    nodeArray[currentNode_g].whatNode = whatNodeType();
    nodeArray[currentNode_g].nodeID = TcrossingID();		      // Är alltid 0 om det inte är en Tcrossing
    nodeArray[currentNode_g].waysExplored = calcWaysExplored()	  // Är alltid 0 om det inte är en Tcrossing
    nodeArray[currentNode_g].wayIn = whatWayIn();
    nodeArray[currentNode_g].nextDirection = whatsNextDirection();
    nodeArray[currentNode_g].northAvailible = tempNorthAvailible_g;
    nodeArray[currentNode_g].eastAvailible = tempEastAvailible_g;
    nodeArray[currentNode_g].southAvailible = tempSouthAvailible_g;
    nodeArray[currentNode_g].westAvailible = tempWestAvailible_g;
    nodeArray[currentNode_g].containsLeak = false;                // En ny nod kan inte initieras med en läcka
    nodeArray[currentNode_g].leakID = 0;
}


//---------------------------------------------------------------------------------------------------------------------------------------------

void simulatedeadEnd(uint8_t openDir_)
{
	if (openDir_ == north)
	{
		northSensor_g = 800;
		eastSensor_g = 15;
		southSensor_g = 15;
		westSensor_g = 15;
	}
	else if (openDir_ == east)
	{
		northSensor_g = 15;
		eastSensor_g = 800;
		southSensor_g = 15;
		westSensor_g = 15;
	}
	else if (openDir_ == south)
	{
		northSensor_g = 15;
		eastSensor_g = 15;
		southSensor_g = 800;
		westSensor_g = 15;
	}
	else
	{
		northSensor_g = 15;
		eastSensor_g = 15;
		southSensor_g = 15;
		westSensor_g = 800;
	}
}

void simulateTcrossing1()
{
	northSensor_g = 800;
	eastSensor_g = 800;
	southSensor_g = 800;
	westSensor_g = 15;
}

void simulateTcrossing2()
{
	northSensor_g = 15;
	eastSensor_g = 800;
	southSensor_g = 800;
	westSensor_g = 800;
}

void simulateTcrossing3()
{
	northSensor_g = 800;
	eastSensor_g = 800;
	southSensor_g = 15;
	westSensor_g = 800;
}

void simulateCorridorNorth()
{
	northSensor_g = 800;
	eastSensor_g = 15;
	southSensor_g = 800;
	westSensor_g = 15;
}

void simulateCorridorEast()
{
	northSensor_g = 15;
	eastSensor_g = 800;
	southSensor_g = 15;
	westSensor_g = 800;
}

void simulateEastToNorth()
{
	northSensor_g = 800;
	eastSensor_g = 15;
	southSensor_g = 15;
	westSensor_g = 800;
}

void simulateNorthToWest()
{
	northSensor_g = 15;
	eastSensor_g = 800;
	southSensor_g = 800;
	westSensor_g = 15;
}


void simulateTest()
{
	if ((testHelper > 0) && (testHelper < 4))
	{
		simulateCorridorNorth();
		HARDCODEDDIRECTION = north;
	}
	else if ((testHelper > 3) && (testHelper < 7))
	{
		simulateTcrossing1();
		currentDirection_g = north;
		HARDCODEDDIRECTION = east;
	}
	else if ((testHelper > 6) && (testHelper < 10))
	{
		simulateCorridorEast();
		currentDirection_g = east;
		HARDCODEDDIRECTION = east;
	}
	else if ((testHelper > 9) && (testHelper < 13))
	{
		simulateEastToNorth();
		currentDirection_g = east;
		HARDCODEDDIRECTION = north;
	}
	else if ((testHelper > 12) && (testHelper < 16))
	{
		simulateCorridorNorth();
		currentDirection_g = north;
		HARDCODEDDIRECTION = north;
	}
	else if ((testHelper > 15) && (testHelper < 19))
	{
		simulateTcrossing2();
		currentDirection_g = north;
		HARDCODEDDIRECTION = east;
	}
	else if ((testHelper > 18) && (testHelper < 22))
	{
		simulateCorridorEast();
		currentDirection_g = east;
		HARDCODEDDIRECTION = east;
	
	}
	else if ((testHelper > 21) && (testHelper < 25))
	{
		simulatedeadEnd(west);
		currentDirection_g = east;
		HARDCODEDDIRECTION = west;
	}
	else if ((testHelper > 24) && (testHelper < 29))
	{
		simulateCorridorEast();
		currentDirection_g = west;
		HARDCODEDDIRECTION = west;
	}
	else if ((testHelper > 28) && (testHelper < 32))
	{
		simulateTcrossing2();
		currentDirection_g = west;
		HARDCODEDDIRECTION = west;
	}
	else if ((testHelper > 31) && (testHelper < 35))
	{
		simulateCorridorEast();
		currentDirection_g = west;
		HARDCODEDDIRECTION = west;
	}
	else if ((testHelper > 34) && (testHelper < 39))
	{
		simulateTcrossing1();
		currentDirection_g = west;
		HARDCODEDDIRECTION = north;
	}
	else if ((testHelper > 38) && (testHelper < 42))
	{
		simulateCorridorNorth();
		currentDirection_g = north;
		HARDCODEDDIRECTION = north;
	}
	else if ((testHelper > 41) && (testHelper < 45))
	{
		simulatedeadEnd(south);
		currentDirection_g = north;
		HARDCODEDDIRECTION = south;
	}
	else if ((testHelper > 44) && (testHelper < 49))
	{
		simulateCorridorNorth();
		currentDirection_g = south;
		HARDCODEDDIRECTION = south;
	}
	else if ((testHelper > 48) && (testHelper < 52))
	{
		simulateTcrossing1();
		currentDirection_g = south;
		HARDCODEDDIRECTION = south;
	}
	else if ((testHelper > 51) && (testHelper < 55))
	{
		simulateCorridorNorth();
		currentDirection_g = south;
		HARDCODEDDIRECTION = south;
	}
	else if ((testHelper > 54) && (testHelper < 59))
	{
		simulatedeadEnd(north);
		currentDirection_g = south;
		HARDCODEDDIRECTION = north;
	}
	else if ((testHelper > 58) && (testHelper < 62))
	{
		simulateCorridorNorth();
		currentDirection_g = north;
		HARDCODEDDIRECTION = north;
	}
	else if ((testHelper > 61) && (testHelper < 65))
	{
		simulateTcrossing1();
		currentDirection_g = north;
		HARDCODEDDIRECTION = east;
	}
	else if ((testHelper > 64) && (testHelper < 68))
	{
		simulateCorridorEast();
		currentDirection_g = east;
		HARDCODEDDIRECTION = east;
	}
	else if ((testHelper > 67) && (testHelper < 71))
	{
		simulateTcrossing2();
		currentDirection_g = east;
		HARDCODEDDIRECTION = south;
	}
	else if ((testHelper > 70) && (testHelper < 74))
	{
		simulateCorridorNorth();
		currentDirection_g = south;
		HARDCODEDDIRECTION = south;
	}
	else if ((testHelper > 73) && (testHelper < 77))
	{
		simulateEastToNorth();
		currentDirection_g = south;
		HARDCODEDDIRECTION = west;
	}
	else if ((testHelper > 76) && (testHelper < 80))
	{
		simulateCorridorEast();
		currentDirection_g = west;
		HARDCODEDDIRECTION = west;
	}
	else if ((testHelper > 79) && (testHelper < 83))
	{
		simulateTcrossing1();
		currentDirection_g = west;
		HARDCODEDDIRECTION = north;
	}
	else if ((testHelper > 82) && (testHelper < 86))
	{
		simulateCorridorNorth();
		currentDirection_g = north;
		HARDCODEDDIRECTION = north;
	}
	else if ((testHelper > 85) && (testHelper < 89))
	{
		simulateTcrossing2();
		currentDirection_g = north;
		HARDCODEDDIRECTION = east;
	}
	else if ((testHelper > 88) && (testHelper < 92))
	{
		simulateCorridorEast();
		currentDirection_g = east;
		HARDCODEDDIRECTION = east;
	}
	else if ((testHelper > 91) && (testHelper < 95))
	{
		simulatedeadEnd(west);
		currentDirection_g = east;
		HARDCODEDDIRECTION = west;
	}
	else if ((testHelper > 94) && (testHelper < 98))
	{
		simulateCorridorEast();
		currentDirection_g = west;
		HARDCODEDDIRECTION = west;
	}
	else if ((testHelper > 97) && (testHelper < 101))
	{
		simulateTcrossing2();
		currentDirection_g = west;
		HARDCODEDDIRECTION = west;
	}
	else if ((testHelper > 100) && (testHelper < 104))
	{
		simulateCorridorEast();
		currentDirection_g = west;
		HARDCODEDDIRECTION = west;
	}
	else if ((testHelper > 103) && (testHelper < 107))
	{
		simulatedeadEnd(east);
		currentDirection_g = west;
		HARDCODEDDIRECTION = east;
	}
	else if ((testHelper > 106) && (testHelper < 110))
	{
		simulateCorridorEast();
		currentDirection_g = east;
		HARDCODEDDIRECTION = east;
	}
	else if ((testHelper > 109) && (testHelper < 113))
	{
		simulateTcrossing2();
		currentDirection_g = east;
		HARDCODEDDIRECTION = south;
	}
	else if ((testHelper > 112) && (testHelper < 116))
	{
		simulateCorridorNorth();
		currentDirection_g = south;
		HARDCODEDDIRECTION = south;
	}
	else if ((testHelper > 115) && (testHelper < 119))
	{
		simulateTcrossing1();
		currentDirection_g = south;
		HARDCODEDDIRECTION = south;
	}
	else if ((testHelper > 118) && (testHelper < 122))
	{
		simulateCorridorNorth();
		currentDirection_g = south;
		HARDCODEDDIRECTION = south;
	}
	else if ((testHelper > 121) && (testHelper < 125))
	{
		simulatedeadEnd(north);
		currentDirection_g = south;
		HARDCODEDDIRECTION = south;
	}
	
	testHelper ++;
}

//---------------------------------------------------------------------------------------------------------------------------------------------

int main()
{
    // Börjar i en återvändsgränd med norr som frammåt
    nodeArray[0].whatNode = deadEnd;
    nodeArray[0].nodeID = 0;                // Nodens ID initieras som 0, ändras om det är en Tcrossing
    nodeArray[0].waysExplored = 0;
    nodeArray[0].wayIn = north;
    nodeArray[0].nextDirection = north;
    nodeArray[0].northAvailible = true;     // Börjar i återvändsgränd med tillgänglig rikt. norr
    nodeArray[0].eastAvailible = false;
    nodeArray[0].southAvailible = false;
    nodeArray[0].westAvailible = false;
    nodeArray[0].containsLeak = false;
    nodeArray[0].leakID = 0;
    
    while(testHelper < 125)
    {

    	simulateTest();
    	// delayfunktion 

        updateTempDirections();
        updateLeakInfo();           // Kollar ifall läcka finns, och lägger till i noden om det fanns
        
        // Denna gör nya noder, ska bara finnas i MapMode
        if ((canMakeNew() == true) && (checkIfNewNode() == true))
        {
            currentNode_g ++;
            createNewNode();
        }
    }

    // skriv ut sakerna här

}