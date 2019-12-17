/**
  * \file trans2019.cpp
  * \date December 2019
  *
  * Translator for binary files produced by detector SIT based at 
  * the TAIGA array in Tunka valley.
  *
  */

char BinFileName[100] = "";  ///< Binary file name 
int EventNumber = 0;         ///< counter of events in current binary file

#include "S01Definitions.cpp"
#include "S02TranslateFile.cpp"

FILE *fp;               ///< binary file
S01TranslateFile trl;   ///< main object

int main(int argc, char *argv[])
{
    if(argc != 2) {
        printf("Error!\nInpurt data filename in comand line or use script tr.sh\n");
        return 1;
    }

    /// ------- translate one data file ---------------
    sprintf(BinFileName, "%s", argv[1]);
    if ( (fp = fopen(BinFileName,"r")) == NULL)
    {
        printf("File Data file %s not found!\n", BinFileName);
        return 2;
    }
    trl.TranslateFile(fp, NULL);

    /// Close all files
    fclose(fp);
    return EventNumber;
}

