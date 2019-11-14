/**
  * \file S01Definitions.cpp
  * \date November 2019
  *
  * Definitions for Class S01TranslateFile
  */

#include <stdio.h>
#include <time.h>

//basic definitions
#define NPMT      112     ///< PMT number                                       //total number of phms
#define NTE       512     ///< number of experimental time bins in event
#define LEN       200     ///< filename max lenght
//#define VerbFlag1   0     ///

#define CHANPMT              8  ///< Channels on one Fadc board
#define MAXADDRON           14  ///< Max number of Fadc boards
#define NB             1000000  ///< number of bins for trigger spectrum 
#define NCHAN         (2*NPMT)  ///< Thick+Thin channels
#define DebugPrint           0  ///< Debug flag
#define DebugPrint2          1  ///< High-Level debug flag
#define CharTestFlag         0  ///< Flag to test char reading from binary file
#define SEP               '\t'  ///< separation char to write in telemetry data file
