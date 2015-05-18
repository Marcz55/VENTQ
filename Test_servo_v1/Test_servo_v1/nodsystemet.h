/*
 * nodsystemet.h
 *
 * Created: 5/7/2015 2:37:09 PM
 *  Author: isawi527
 */ 


#ifndef NODSYSTEMET_H_
#define NODSYSTEMET_H_

#include <stdio.h>

#define MAX_NODES 121        // En nod i varje 57cm i en 6x6m bana skulle motsvara 121 stycken noder.

typedef struct node node;
struct node
{
    int     whatNode;           // Alla typer av noder är definade som siffror
    int     nodeID;             // Nodens unika id, maxvärde 31 
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

// RingBuffer
typedef struct {
    uint8_t northAvailable,
            eastAvailable,
            southAvailable,
            westAvailable;
} simpleNode;

typedef struct {
    simpleNode array[5];
    uint8_t writeIndex;
} NodeRingBuffer;

NodeRingBuffer nodeRingBuffer;


struct node nodeArray[MAX_NODES];
struct node currentNode_g;

int lastAddedNodeIndex_g;

int makeNodeData(node* nodeToSend);
simpleNode getNode(NodeRingBuffer* buffer, uint8_t elementID);
void addNode(NodeRingBuffer* buffer, simpleNode newNode);
void initNodeRingBuffer();
void updateTempDirections();
int validLeak();
void updateLeakInfo();
int canMakeNew();
int isChangeDetected();
void decideChangeFromMajority();
int checkIfNewNode();
int whatNodeType();
int TcrossingID(int whatNode_);
int calcPathsExplored(int whatNode_, int nodeID_);
int whatWayIn();
void updateCurrentNode();          // Uppdaterar nuvarande nod, eg. skapar en ny nod
void placeNodeInArray();
void makeLeakPath(int findThisLeak_);
int northToNext();
int eastToNext();
int southToNext();
int westToNext();
int decideDirection();      // Autonoma läget
void initNodeAndSteering();
void chooseAndSetFrontSensors();
int nodesAndControl();

#endif /* NODSYSTEMET_H_ */
