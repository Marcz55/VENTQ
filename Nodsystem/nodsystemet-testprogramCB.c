//
//  main.c
//  AVR-test
//
//  Created by Isak Wiberg on 2015-04-09.
//  Copyright (c) 2015 Isak Wiberg. All rights reserved.
//


// Rasmus
// Kartnavigeringsnodv�gvalssystem

#include <stdio.h>


#define false 0
#define true  1

#define maxNodes 121        // En nod i varje 57cm i en 6x6m bana skulle motsvara 121 stycken noder.

#define corridor  0         // Dessa �r m�jliga tal i whatNode
#define turn	  1
#define deadEnd   2
#define Tcrossing 3
#define Zcrossing 4


//------------------------------------------------ Jocke har definierat dessa i sin kod
#define north 0
#define east  1
#define south 2
#define west  3
//------------------------------------------------ Ta bort dessa n�r denna kod integreras




int isLeakVisible_g = 0;
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






// DET SKA FINNAS TV� MODES, ETT D� ROBOTEN MAPPAR UPP OCH G�R DENNA ARRAY
// OCH ETT MODE D� ROBOTEN LETAR I LISTAN SOM REDAN FINNS, OCH G�R TILL VALD L�CKA
// l�gger en kommentar ovanf�r funktionerna f�r att visa vilka som ska finna i l�get d� roboten mappar upp kartan (MapMode)




#define closeEnoughToWall 350  // Roboten g�r rakt fram tills den h�r l�ngden
#define maxWallDistance 570   // Utanf�r denna l�ngd �r det ingen v�gg

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

struct node
{
    int     whatNode;        	// Alla typer av noder �r definade som siffror
    int     nodeID;          	// Nodens unika id
    int		waysExplored;		// �r b�gge h�llen utforskade �r denna 2
    int     wayIn;
    int     nextDirection;   	// V�derstrecken �r siffror som �r definade
    int     northAvailible;  	// Finns riktningen norr i noden?   Om sant => 1, annars 0
    int     eastAvailible;
    int     southAvailible;
    int     westAvailible;
    int     containsLeak;    	// Finns l�cka i "noden", kan bara finnas om det �r en korridor
    int		leakID;			 	// Fanns en l�cka f�r den ett unikt id, annars �r denna 0
};

struct node nodeArray[maxNodes];


// Ska finnas i b�gge modes
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

	if (northSensor_g > maxWallDistance) // Kan beh�va �ndra maxWallDistance
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

// Ska finnas i b�gge modes
int validLeak()
{
	if (isLeakVisible_g == true)
	{
		if (actualLeak_g == 2)      // N�r variabeln actualLeak_g 2
		{                           // s� �r det en faktisk l�cka
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

// Ska finnas i b�gge modes
void updateLeakInfo()
{
	if ((validLeak() == true) &&                              // Faktisk l�cka?
	(nodeArray[currentNode_g].containsLeak == false) &&   // Inneh�ller noden redan en l�cka?
	(nodeArray[currentNode_g].whatNode == corridor))      // L�ckor f�r bara finnas i nodtypen "corridor"
	{													  // Kollen ovan g�r �ven att n�r roboten inte g�r nya noder s� kan inte l�ckor l�ggas till
		leaksFound_g ++;								  // eftersom whatNode i det fallet �r Tcrossing eller deadEnd

		nodeArray[currentNode_g].containsLeak = true;         // Uppdaterar att korridornoden har en l�cka sig
		nodeArray[currentNode_g].leakID = leaksFound_g;		  // L�ckans id f�r det nummer som den aktuella l�ckan har
	}
}

// Ska bara finnas i MapMode
int canMakeNew()
{
    if (currentTcrossing_g == 0)
        return true;
	else if ((nodeArray[currentNode_g].whatNode == deadEnd) &&                                               	// Var senaste noden en �terv�ndsgr�nd (deadEnd)?
		!(tempNorthAvailible_g + tempEastAvailible_g + tempSouthAvailible_g + tempWestAvailible_g == 3))    // Detta kollar om roboten st�r i en T-korsning
	{
		return false; // Allts�: om senaste noden var en deadEnd och den nuvarande inte �r en T-korsning ska inte nya noder g�ras
	}
	else if ((nodeArray[currentNode_g].whatNode == Tcrossing) &&											  // Var senaste noden en Tcrossing
			 (nodeArray[currentNode_g].waysExplored == 2) &&											  	  // Om senaste noden va en Tcrossing, har den untforskats helt?
			 !(tempNorthAvailible_g + tempEastAvailible_g + tempSouthAvailible_g + tempWestAvailible_g == 3)) // is�fall ska inte nya noder g�ras
	{
		return false;
	}
	else
	{
		return true;
	}

	// L�gg till saker h�r: p� v�gen tillbaka efter en Tcrossing ska inte nya noder g�ras
}

int isChangeDetected()
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
// Denna funktion hanterar konstiga fenomen i Zcrossing, hanteras dock som tv� st 2v�gskorsningar
int checkIfNewNode()
{
	if ((isChangeDetected() == true) && (distanceToFrontWall_g > maxWallDistance))
	{
		return true;	// I detta fall �r det en Tcrossing fr�n sidan, eller ut fr�n en korsning, eller en corridor
	}
	else if ((isChangeDetected() == true) && (distanceToFrontWall_g < closeEnoughToWall))
	{
		return true;	// F�r�ndringen va en v�gg d�r fram, nu har roboten g�tt tillr�ckligt l�ngt
	}
	else
		return false;
}

// MapMode
int whatNodeType()
{
	if (tempNorthAvailible_g + tempEastAvailible_g + tempSouthAvailible_g + tempWestAvailible_g == 3)
	{
		return Tcrossing;            // Om det finns 3 v�gar s� �r det en v�gvalsnod
	}
	else if (tempNorthAvailible_g + tempEastAvailible_g + tempSouthAvailible_g + tempWestAvailible_g == 2)
	{
		if (tempNorthAvailible_g == tempSouthAvailible_g)
            return corridor;        // Detta m�ste vara en korridor
		else
            return turn; // Detta m�ste vara en 2v�gskorsning
	}
	else if (tempNorthAvailible_g + tempEastAvailible_g + tempSouthAvailible_g + tempWestAvailible_g == 1)
	{
		return deadEnd;             // Detta m�ste vara en �terv�ndsgr�nd
	}
	else // H�r �r summan lika med 0, dvs v�ggar p� alla sidor, b�r betyda Z-sv�ng
	{
		return Zcrossing;		// Funkar egentligen inte, en Z-sv�ng hanteras som tv� turns
	}
}

int TcrossingID()
{
	if (nodeArray[currentNode_g].whatNode == Tcrossing)
	{
		if (currentTcrossing_g == 0)	// F�rsta T-korsningen
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
			for (j; j > 0 ; j--)				    // Den h�r magiska forloopen letar r�tt p� en Tcrossing fr�n h�ger
			{
			    int i;								// i arrayen och kollar om den var "fylld" med waysExplored
				for (i = 120; i>1 ; i--)	        // och va den fylld kollar den p� den tidigare T-korsningen
				{
					if (nodeArray[i].nodeID == j - 1)
					{
						if(nodeArray[i].waysExplored == 2)
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

int calcWaysExplored()
{
	int ways = 0;

	if (nodeArray[currentNode_g].whatNode == Tcrossing)
	{
		int ID = nodeArray[currentNode_g].nodeID;

        int i;
		for(i = 0; i < currentNode_g; i++)
		{
			if (nodeArray[i].nodeID == ID)
				ways ++;
		}
	}
	return ways;
}

// F�r finnas i b�gge, beh�vs i MapMode
int whatWayIn()
{
	return currentDirection_g;
}

// F�r finnas i b�gge, beh�vs i MapMode
int whatsNextDirection()
{
	return HARDCODEDDIRECTION;
}

// MapMode
void createNewNode()    // Skapar en ny nod och l�gger den i arrayen
{
    nodeArray[currentNode_g].whatNode = whatNodeType();
    nodeArray[currentNode_g].nodeID = TcrossingID();		      // �r alltid 0 om det inte �r en Tcrossing
    nodeArray[currentNode_g].waysExplored = calcWaysExplored();	  // �r alltid 0 om det inte �r en Tcrossing
    nodeArray[currentNode_g].wayIn = whatWayIn();
    nodeArray[currentNode_g].nextDirection = whatsNextDirection();
    nodeArray[currentNode_g].northAvailible = tempNorthAvailible_g;
    nodeArray[currentNode_g].eastAvailible = tempEastAvailible_g;
    nodeArray[currentNode_g].southAvailible = tempSouthAvailible_g;
    nodeArray[currentNode_g].westAvailible = tempWestAvailible_g;
    nodeArray[currentNode_g].containsLeak = false;                // En ny nod kan inte initieras med en l�cka
    nodeArray[currentNode_g].leakID = 0;
}


//---------------------------------------------------------------------------------------------------------------------------------------------

void simulatedeadEnd(int openDir_)
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
	if ((testHelper >= 0) && (testHelper < 10))
	{
		simulateCorridorNorth();
		currentDirection_g = north;
		HARDCODEDDIRECTION = north;
	}
	else if ((testHelper > 9) && (testHelper < 20))
	{
		simulateTcrossing1();
		currentDirection_g = north;
		HARDCODEDDIRECTION = east;
	}
	else if ((testHelper > 19) && (testHelper < 30))
	{
		simulateCorridorEast();
		currentDirection_g = east;
		HARDCODEDDIRECTION = east;
	}
	else if ((testHelper > 29) && (testHelper < 40))
	{
		simulateEastToNorth();
		currentDirection_g = east;
		HARDCODEDDIRECTION = north;
	}
	else if ((testHelper > 39) && (testHelper < 50))
	{
		simulateCorridorNorth();
		currentDirection_g = north;
		HARDCODEDDIRECTION = north;
	}
	else if ((testHelper > 49) && (testHelper < 60))
	{
		simulateTcrossing2();
		currentDirection_g = north;
		HARDCODEDDIRECTION = east;
	}
	else if ((testHelper > 59) && (testHelper < 70))
	{
		simulateCorridorEast();
		currentDirection_g = east;
		HARDCODEDDIRECTION = east;

	}
	else if ((testHelper > 69) && (testHelper < 80))
	{
		simulatedeadEnd(west);
		currentDirection_g = east;
		HARDCODEDDIRECTION = west;
	}
	else if ((testHelper > 79) && (testHelper < 90))
	{
		simulateCorridorEast();
		currentDirection_g = west;
		HARDCODEDDIRECTION = west;
	}
	else if ((testHelper > 89) && (testHelper < 100))
	{
		simulateTcrossing2();
		currentDirection_g = west;
		HARDCODEDDIRECTION = west;
	}
	else if ((testHelper > 99) && (testHelper < 110))
	{
		simulateCorridorEast();
		currentDirection_g = west;
		HARDCODEDDIRECTION = west;
	}
	else if ((testHelper > 109) && (testHelper < 120))
	{
		simulateTcrossing1();
		currentDirection_g = west;
		HARDCODEDDIRECTION = north;
	}
	else if ((testHelper > 119) && (testHelper < 130))
	{
		simulateCorridorNorth();
		currentDirection_g = north;
		HARDCODEDDIRECTION = north;
	}
	else if ((testHelper > 129) && (testHelper < 140))
	{
		simulatedeadEnd(south);
		currentDirection_g = north;
		HARDCODEDDIRECTION = south;
	}
	else if ((testHelper > 139) && (testHelper < 150))
	{
		simulateCorridorNorth();
		currentDirection_g = south;
		HARDCODEDDIRECTION = south;
	}
	else if ((testHelper > 149) && (testHelper < 160))
	{
		simulateTcrossing1();
		currentDirection_g = south;
		HARDCODEDDIRECTION = south;
	}
	else if ((testHelper > 159) && (testHelper < 170))
	{
		simulateCorridorNorth();
		currentDirection_g = south;
		HARDCODEDDIRECTION = south;
	}
	else if ((testHelper > 169) && (testHelper < 180))
	{
		simulatedeadEnd(north);
		currentDirection_g = south;
		HARDCODEDDIRECTION = north;
	}
	else if ((testHelper > 179) && (testHelper < 190))
	{
		simulateCorridorNorth();
		currentDirection_g = north;
		HARDCODEDDIRECTION = north;
	}
	else if ((testHelper > 189) && (testHelper < 200))
	{
		simulateTcrossing1();
		currentDirection_g = north;
		HARDCODEDDIRECTION = east;
	}
	else if ((testHelper > 199) && (testHelper < 210))
	{
		simulateCorridorEast();
		currentDirection_g = east;
		HARDCODEDDIRECTION = east;
	}
	else if ((testHelper > 209) && (testHelper < 220))
	{
		simulateTcrossing2();
		currentDirection_g = east;
		HARDCODEDDIRECTION = south;
	}
	else if ((testHelper > 219) && (testHelper < 230))
	{
		simulateCorridorNorth();
		currentDirection_g = south;
		HARDCODEDDIRECTION = south;
	}
	else if ((testHelper > 229) && (testHelper < 240))
	{
		simulateEastToNorth();
		currentDirection_g = south;
		HARDCODEDDIRECTION = west;
	}
	else if ((testHelper > 239) && (testHelper < 250))
	{
		simulateCorridorEast();
		currentDirection_g = west;
		HARDCODEDDIRECTION = west;
	}
	else if ((testHelper > 249) && (testHelper < 260))
	{
		simulateTcrossing1();
		currentDirection_g = west;
		HARDCODEDDIRECTION = north;
	}
	else if ((testHelper > 259) && (testHelper < 270))
	{
		simulateCorridorNorth();
		currentDirection_g = north;
		HARDCODEDDIRECTION = north;
	}
	else if ((testHelper > 269) && (testHelper < 280))
	{
		simulateTcrossing2();
		currentDirection_g = north;
		HARDCODEDDIRECTION = east;
	}
	else if ((testHelper > 279) && (testHelper < 290))
	{
		simulateCorridorEast();
		currentDirection_g = east;
		HARDCODEDDIRECTION = east;
	}
	else if ((testHelper > 289) && (testHelper < 300))
	{
		simulatedeadEnd(west);
		currentDirection_g = east;
		HARDCODEDDIRECTION = west;
	}
	else if ((testHelper > 299) && (testHelper < 310))
	{
		simulateCorridorEast();
		currentDirection_g = west;
		HARDCODEDDIRECTION = west;
	}
	else if ((testHelper > 309) && (testHelper < 320))
	{
		simulateTcrossing2();
		currentDirection_g = west;
		HARDCODEDDIRECTION = west;
	}
	else if ((testHelper > 319) && (testHelper < 330))
	{
		simulateCorridorEast();
		currentDirection_g = west;
		HARDCODEDDIRECTION = west;
	}
	else if ((testHelper > 329) && (testHelper < 340))
	{
		simulatedeadEnd(east);
		currentDirection_g = west;
		HARDCODEDDIRECTION = east;
	}
	else if ((testHelper > 339) && (testHelper < 350))
	{
		simulateCorridorEast();
		currentDirection_g = east;
		HARDCODEDDIRECTION = east;
	}
	else if ((testHelper > 349) && (testHelper < 360))
	{
		simulateTcrossing2();
		currentDirection_g = east;
		HARDCODEDDIRECTION = south;
	}
	else if ((testHelper > 359) && (testHelper < 370))
	{
		simulateCorridorNorth();
		currentDirection_g = south;
		HARDCODEDDIRECTION = south;
	}
	else if ((testHelper > 369) && (testHelper < 380))
	{
		simulateTcrossing1();
		currentDirection_g = south;
		HARDCODEDDIRECTION = south;
	}
	else if ((testHelper > 379) && (testHelper < 390))
	{
		simulateCorridorNorth();
		currentDirection_g = south;
		HARDCODEDDIRECTION = south;
	}
	else if ((testHelper > 389) && (testHelper < 400))
	{
		simulatedeadEnd(north);
		currentDirection_g = south;
		HARDCODEDDIRECTION = south;
	}

	testHelper ++;
}

//---------------------------------------------------------------------------------------------------------------------------------------------

void print() {
    int i=0;

    for (i=0; i<31; i++) {
        printf("Nod nummer: %d\n" , i + 1);

        if (nodeArray[i].whatNode == 0)
            printf("whatNode: corridor \n", 0);
        else if(nodeArray[i].whatNode == 1)
            printf("whatNode: turn \n", 0);
        else if(nodeArray[i].whatNode == 2)
            printf("whatNode: deadEnd \n", 0);
        else
            printf("whatNode: Tcrossing \n", 0);

        printf("nodeID: %d\n", nodeArray[i].nodeID);
        printf("waysExplored: %d\n", nodeArray[i].waysExplored);

        if (nodeArray[i].wayIn == 0)
            printf("wayIn: north \n", 0);
        else if(nodeArray[i].wayIn == 1)
            printf("wayIn: east \n", 0);
        else if(nodeArray[i].wayIn == 2)
            printf("wayIn: south \n", 0);
        else
            printf("wayIn: west \n", 0);

        printf("------------------------- \n", 0);
    }
}

int main()
{
    // B�rjar i en �terv�ndsgr�nd med norr som framm�t
    nodeArray[0].whatNode = deadEnd;
    nodeArray[0].nodeID = 0;                // Nodens ID initieras som 0, �ndras om det �r en Tcrossing
    nodeArray[0].waysExplored = 0;
    nodeArray[0].wayIn = north;
    nodeArray[0].nextDirection = north;
    nodeArray[0].northAvailible = true;     // B�rjar i �terv�ndsgr�nd med tillg�nglig rikt. norr
    nodeArray[0].eastAvailible = false;
    nodeArray[0].southAvailible = false;
    nodeArray[0].westAvailible = false;
    nodeArray[0].containsLeak = false;
    nodeArray[0].leakID = 0;

    while(testHelper < 400)
    {

    	simulateTest();

        updateTempDirections();
        updateLeakInfo();           // Kollar ifall l�cka finns, och l�gger till i noden om det fanns

        // Denna g�r nya noder, ska bara finnas i MapMode
        if ((canMakeNew() == true) && (checkIfNewNode() == true))
        {
            currentNode_g ++;
            createNewNode();
        }
    }

    print();
}
