/*
 * SPI.h
 *
 * Created: 4/20/2015 11:26:43 AM
 *  Author: marwa828
 */ 


#ifndef SPI_H_
#define SPI_H_


#define DISTANCE_NORTH 202
#define DISTANCE_EAST 208
#define DISTANCE_SOUTH 216
#define DISTANCE_WEST 224
#define ANGLE_NORTH 232
#define ANGLE_EAST 240
#define ANGLE_SOUTH 248
#define ANGLE_WEST 136

#define CONTROL_DECISION 252
#define TOTAL_ANGLE 144
#define LEAK_HEADER 152
#define NODE_INFO 160
#define TRASH 184

unsigned char inbuffer;

void spiMasterInit(void);
void spiSlaveInit();
void spiTransmitToSensorUnit(unsigned char data);
void spiTransmitToCommUnit(unsigned char data);
void transmitDataToCommUnit(int header_, int data);
void transmitDataToSensorUnit(int header_, int data);
int fetchDataFromSensorUnit(int header_);


#endif /* SPI_H_ */