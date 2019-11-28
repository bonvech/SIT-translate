/**
  * \file S01Definitions.cpp
  * \date November 2019
  *
  * Definitions for Class S01TranslateFile
  */

#include <stdio.h>
#include <time.h>

//basic definitions
#define NPMT         112  ///< PMT number - total number of PMTs
#define NCHAN   (2*NPMT)  ///< Thick+Thin channels
#define NTE          512  ///< max number of experimental time bins in event
#define CHANPMT        8  ///< Channels on one Fadc board
#define MAXADDRON     14  ///< Max number of Fadc boards
//#define VerbFlag1    0  ///

#define LEN          200  ///< filename max lenght
#define NB       1000000  ///< number of bins for trigger spectrum
#define DebugPrint     0  ///< Debug flag
#define DebugPrint2    1  ///< High-Level debug flag
#define CharTestFlag   0  ///< Flag to test char reading from binary file
#define SEP         '\t'  ///< separation char to write in telemetry data file
