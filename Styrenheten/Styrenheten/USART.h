/*
 * USART.h
 *
 * Created: 4/21/2015 8:31:14 AM
 *  Author: Isak Wiberg
 */ 


#ifndef USART_H_
#define USART_H_


#include <avr/io.h>
#include <avr/interrupt.h>
#define F_CPU 16000000UL
#include <util/delay.h>

// -- USART definitioner
#define TXD0_READY bit_is_set(UCSR0A,5)
#define TXD0_FINISHED bit_is_set(UCSR0A,6)
#define RXD0_READY bit_is_set(UCSR0A,7)
#define TXD0_DATA (UDR0)
#define RXD0_DATA (UDR0)

//--- Control Table Address ---
//EEPROM AREA
#define P_MODEL_NUMBER_L 0
#define P_MODOEL_NUMBER_H 1
#define P_VERSION 2
#define P_ID 3
#define P_BAUD_RATE 4
#define P_RETURN_DELAY_TIME 5
#define P_CW_ANGLE_LIMIT_L 6
#define P_CW_ANGLE_LIMIT_H 7
#define P_CCW_ANGLE_LIMIT_L 8
#define P_CCW_ANGLE_LIMIT_H 9
#define P_SYSTEM_DATA2 10
#define P_LIMIT_TEMPERATURE 11
#define P_DOWN_LIMIT_VOLTAGE 12
#define P_UP_LIMIT_VOLTAGE 13
#define P_MAX_TORQUE_L 14
#define P_MAX_TORQUE_H 15
#define P_RETURN_LEVEL 16
#define P_ALARM_LED 17
#define P_ALARM_SHUTDOWN 18
#define P_OPERATING_MODE 19
#define P_DOWN_CALIBRATION_L 20
#define P_DOWN_CALIBRATION_H 21
#define P_UP_CALIBRATION_L 22
#define P_UP_CALIBRATION_H 23
#define P_TORQUE_ENABLE (24)
#define P_LED (25)
#define P_CW_COMPLIANCE_MARGIN (26)
#define P_CCW_COMPLIANCE_MARGIN (27)
#define P_CW_COMPLIANCE_SLOPE (28)
#define P_CCW_COMPLIANCE_SLOPE (29)
#define P_GOAL_POSITION_L (30)
#define P_GOAL_POSITION_H (31)
#define P_GOAL_SPEED_L (32)
#define P_GOAL_SPEED_H (33)
#define P_TORQUE_LIMIT_L (34)
#define P_TORQUE_LIMIT_H (35)
#define P_PRESENT_POSITION_L (36)
#define P_PRESENT_POSITION_H (37)
#define P_PRESENT_SPEED_L (38)
#define P_PRESENT_SPEED_H (39)
#define P_PRESENT_LOAD_L (40)
#define P_PRESENT_LOAD_H (41)
#define P_PRESENT_VOLTAGE (42)
#define P_PRESENT_TEMPERATURE (43)
#define P_REGISTERED_INSTRUCTION (44)
#define P_PAUSE_TIME (45)
#define P_MOVING (46)
#define P_LOCK (47)
#define P_PUNCH_L (48)
#define P_PUNCH_H (49)

//--- Instruction ---
#define INST_PING 0x01
#define INST_READ 0x02
#define INST_WRITE 0x03
#define INST_REG_WRITE 0x04
#define INST_ACTION 0x05
#define INST_RESET 0x06
#define INST_DIGITAL_RESET 0x07
#define INST_SYSTEM_READ 0x0C
#define INST_SYSTEM_WRITE 0x0D
#define INST_SYNC_WRITE 0x83
#define INST_SYNC_REG_WRITE 0x84


void USART0RecieveMode();
void USART0SendMode();
void initUSART();
void USARTWriteChar(char data);
void USARTSendInstruction0(int ID, int instruction);
void USARTSendInstruction1(int ID, int instruction, int parameter0);
void USARTSendInstruction2(int ID, int instruction, int parameter0, int parameter1);
void USARTSendInstruction3(int ID, int instruction, int parameter0, int parameter1, int parameter2);
void USARTSendInstruction4(int ID, int instruction, int parameter0, int parameter1, int parameter2, int parameter3);
void USARTSendInstruction5(int ID, int instruction, int parameter0, int parameter1, int parameter2, int parameter3, int parameter4);
char USARTReadChar();
int USARTReadStatusPacket();
void disableStatusPacketsFromActuator(int ID);
void enableStatusPacketsFromActuator(int ID);
int ReadTemperatureLimitFromActuator(int ID);
int readCurrentTemperatureFromActuator(int ID);
int readAlarmShutdownStatus(int ID);
void setNewIDOfActuator (int ID, int newID);

#endif /* USART_H_ */