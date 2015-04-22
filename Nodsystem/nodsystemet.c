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
#define north (uint8_t)1
#define east  (uint8_t)2
#define south (uint8_t)3
#define west  (uint8_t)4
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

struct node
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
    uint8_t		leakID;			 // Fanns en läcka får den ett unikt id, annars är denna 0
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

// Ska finnas i bägge modes
void updateLeakInfo()
{
	if ((validLeak() == true) &&                              // Faktisk läcka?
	(nodeArray[currentNode_g].containsLeak == false) &&   // Innehåller noden redan en läcka?
	(nodeArray[currentNode_g].whatNode == corridor))      // Läckor får bara finnas i nodtypen "corridor"
	{
		leaksFound_g ++;

		nodeArray[currentNode_g].containsLeak = true;         // Uppdaterar att korridornoden har en läcka sig
		nodeArray[currentNode_g].leakID = leaksFound_g;		  // Läckans id får det nummer som den aktuella läckan har
	}
}

// Ska bara finnas i MapMode
uint8_t canMakeNew()
{
	if ((nodeArray[currentNode_g].whatNode == deadEnd) &&                                               // Var senaste noden en återvändsgränd (deadEnd)?
	!(tempNorthAvailible_g + tempEastAvailible_g + tempSouthAvailible_g + tempWestAvailible_g == 3))    // Detta kollar om roboten står i en T-korsning
	{
		return false; // Alltså: om senaste noden var en deadEnd och den nuvarande inte är en T-korsning ska inte nya noder göras
	}
	else
	{
		return true;
	}
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
		if (validChange_g == 2)
		{
			return true;
		}
		validChange_g ++;
		return false;
	}
}

// MapMode
uint8_t checkIfNewNode()
{
	if ((isChangeDetected() == true) && (distanceToFrontWall_g > maxWallDistance))
	{
		return true;	// I detta fall är det en Tcrossing från sidan
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

// Får finnas i bägge, behövs i MapMode
uint8_t whatWayIn()
{
	return currentDirection_g;
}

// Får finnas i bägge, behövs i MapMode
uint8_t whatsNextDirection()
{
	// Anropar vad roboten tog för styrbeslut
}

// MapMode
void createNewNode()    // Skapar en ny nod och lägger den i arrayen
{
    nodeArray[currentNode_g].whatNode = whatNodeType();
    nodeArray[currentNode_g].nodeID = currentNode_g;
    nodeArray[currentNode_g].wayIn = whatWayIn();
    nodeArray[currentNode_g].nextDirection = whatsNextDirection();
    nodeArray[currentNode_g].northAvailible = tempNorthAvailible_g;
    nodeArray[currentNode_g].eastAvailible = tempEastAvailible_g;
    nodeArray[currentNode_g].southAvailible = tempSouthAvailible_g;
    nodeArray[currentNode_g].westAvailible = tempWestAvailible_g;
    nodeArray[currentNode_g].containsLeak = false;                // En ny nod kan inte initieras med en läcka
    nodeArray[currentNode_g].leakID = 0;
}

int main()
{
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
    nodeArray[0].leakID = 0;
    
    while(1)
    {
        updateTempDirections();
        updateLeakInfo();           // Kollar ifall läcka finns, och lägger till i noden om det fanns
        
        // Denna gör nya noder, ska bara finnas i MapMode
        if ((canMakeNew() == true) && (checkIfNewNode() == true))
        {
            currentNode_g ++;
            createNewNode();
        }
    }
}