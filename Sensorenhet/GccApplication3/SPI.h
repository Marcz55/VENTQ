/*
 * SPI.h
 *
 * Created: 4/20/2015 11:26:43 AM
 *  Author: marwa828
 */ 


#ifndef SPI_H_
#define SPI_H_

#define DISTANCE_NORTH 202 //14
#define DISTANCE_EAST 208 //15
#define DISTANCE_SOUTH 216 //16
#define DISTANCE_WEST 224 //17
#define ANGLE_NORTH 232 //18
#define ANGLE_EAST 240 //19
#define ANGLE_SOUTH 248 //20
#define ANGLE_WEST 136 //6

#define TOTAL_ANGLE 144 //7
#define LEAK_HEADER 152 //8
#define NODE_INFO 160 //9
#define TRASH 184 //11

#define SENSOR_1 200 //13
#define SENSOR_2 192 //12
#define SENSOR_3 168 //10
#define SENSOR_4 120 //5
#define SENSOR_5 112 //4
#define SENSOR_6 104 //3
#define SENSOR_7 96 //2
#define SENSOR_8 88 //1


unsigned char inbuffer;

void spiMasterInit(void);
void spiSlaveInit();
void spiTransmit(unsigned char data);
void transmitDataToCommUnit(int header_, int data);
void transmitDataToSensorUnit(int header_, int data);



#endif /* SPI_H_ */