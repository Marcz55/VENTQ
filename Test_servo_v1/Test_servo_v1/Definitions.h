﻿/*
 * Definitions.h
 *
 * Created: 4/25/2015 2:57:20 PM
 *  Author: isawi527
 */ 


#ifndef DEFINITIONS_H_
#define DEFINITIONS_H_

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


//----Konstanter-Inverskinematik----
#define a1 50
#define a2 67
#define a3 130
#define a1Square 2500
#define a2Square 4489
#define a3Square 16900
#define PI 3.141592

#define FRONT_LEFT_LEG 1
#define FRONT_RIGHT_LEG 2
#define REAR_LEFT_LEG 3
#define REAR_RIGHT_LEG 4

#define FRONT_LEFT_LEG_X 1
#define FRONT_LEFT_LEG_Y 2
#define FRONT_LEFT_LEG_Z 3
#define FRONT_RIGHT_LEG_X 4
#define FRONT_RIGHT_LEG_Y 5
#define FRONT_RIGHT_LEG_Z 6
#define REAR_LEFT_LEG_X 7
#define REAR_LEFT_LEG_Y 8
#define REAR_LEFT_LEG_Z 9
#define REAR_RIGHT_LEG_X 10
#define REAR_RIGHT_LEG_Y 11
#define REAR_RIGHT_LEG_Z 12

#define INCREMENT_PERIOD_10 10
#define INCREMENT_PERIOD_20 20
#define INCREMENT_PERIOD_30 30
#define INCREMENT_PERIOD_40 40
#define INCREMENT_PERIOD_50 50
#define INCREMENT_PERIOD_60 60
#define INCREMENT_PERIOD_70 70
#define INCREMENT_PERIOD_80 80
#define INCREMENT_PERIOD_90 90
#define INCREMENT_PERIOD_100 100
#define INCREMENT_PERIOD_200 200
#define INCREMENT_PERIOD_300 300
#define INCREMENT_PERIOD_400 400
#define INCREMENT_PERIOD_500 500


#define NORTH 1
#define NORTH_EAST 5
#define EAST 4
#define SOUTH_EAST 6
#define SOUTH 2
#define SOUTH_WEST 10
#define WEST 8
#define NORTH_WEST 9
#define NO_MOVEMENT_DIRECTION 0

#define CW_ROTATION 1
#define CCW_ROTATION 2
#define NO_ROTATION 0

#define TRUE 1
#define FALSE 0

// -- Gångstilar


#define STAND_STILL_GAIT 0
#define TROT_GAIT 1
#define CREEP_GAIT 2


#endif /* DEFINITIONS_H_ */