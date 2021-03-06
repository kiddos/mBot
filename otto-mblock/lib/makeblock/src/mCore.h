///@file Makeblock.h head file of Makeblock Library V2.1.0625
/// Define the interface of Makeblock Library

#ifndef mCore_h
#define mCore_h

#include "Me7SegmentDisplay.h"
#include "MeBuzzer.h"
#include "MeDCMotor.h"
#include "MeIR.h"
#include "MeInfraredReceiver.h"
#include "MeLEDMatrix.h"
#include "MeLineFollower.h"
#include "MePort.h"
#include "MeRGBLed.h"
#include "MeTemperature.h"
#include "MeUltrasonic.h"

Board_type MakeblockBoard = mCore;

MePort_Sig mePort[11] = {{255, 255}, {11, 12}, {9, 10}, {A2, A3},
                         {A0, A1}, {6, 7},   {5, 4},  {A7, 13},
                         {8, A6},  {6, 7},   {5, 4}};

///@brief Class for MeBoard
// class MeBoard
// {
// public:
//     MeBoard(uint8_t boards);
// };

#endif
