/*
 * SPI.h
 *
 * Created: 4/20/2015 11:26:43 AM
 *  Author: marwa828
 */ 


#ifndef SPI_H_
#define SPI_H_

// ---- Headers för komm mellan styr- och sensorenhet ----

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

#define SENSOR_1 200 
#define SENSOR_2 192 
#define SENSOR_3 168 
#define SENSOR_4 120 
#define SENSOR_5 112 
#define SENSOR_6 104 
#define SENSOR_7 96 
#define SENSOR_8 88 

// --- Headers för meddelanden om gånginställningar -----

#define STEP_INCREMENT_TIME_HEADER 128
#define STEP_LENGTH_HEADER 80
#define STEP_HEIGHT_HEADER 72
#define LEG_DISTANCE_HEADER 64
#define ROBOT_HEIGHT_HEADER 56
#define K_P_ANGLE_HEADER 48
#define K_P_TRANS_HEADER 40


unsigned char inbuffer;

void spiMasterInit(void);
void spiSlaveInit();
void spiTransmitToSensorUnit(unsigned char data);
void spiTransmitToCommUnit(unsigned char data);
void transmitDataToCommUnit(int header_, int data);
void transmitDataToSensorUnit(int header_, int data);
int fetchDataFromSensorUnit(int header_);


#endif /* SPI_H_ */