/**
  * \file S02TranslateFile.cpp
  * \date November 2019
  *
  * Class S01TranslateFile. 
  * Reads and translates one binary file of the SIT detector. 
  * Makes text files with events and telemetry data.
  */

//#define VERBOSE

int isDigit(unsigned char);

const char SyncroFile[]    = "syncro.csv";      ///< syncro pulse file name
const char TelemetryFile[] = "telemetry.csv";   ///< telemetry file name

extern int EventNumber;

/** 
 * 23 канал дублирован на 39 канал из-за проблем с одним из каналов 23.
 * 23 не отключен в таблице, чтобы было можно проанализировать его корреляцию с 39.
 */

/// Table of channels in use
int CHANUSE[NCHAN] = {  1,1,1,1,1,1,1,1,  1,1,1,1,1,1,1,1,
                        1,1,1,1,1,1,2,1,  1,1,1,1,1,1,1,1,
                        1,1,1,1,1,0,1,1,  1,0,0,1,1,0,0,1,
                        1,0,0,1,1,0,0,1,  1,0,0,1,1,0,0,0,
                        0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,
                        0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,
                        0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0 };

/// Union to convert 4-byte integer number to array of char[4]
union CharInt_converter
{
    unsigned char tChar[4]; ///< four chars
    unsigned int  tInt;     ///< one 4-byte integer number
} Conv; ///< union to convert integer to bytes


/**
 * Class with all data and functions to translation of
 * binary files of SIT detector to human readable files.
 * \brief Class Translation.
 */
class S01TranslateFile
{
  public:
    S01TranslateFile();
    ~S01TranslateFile();
    int TranslateFile(FILE *fp, FILE *fpn);
    int Process(FILE *fp, FILE *fpn);
    int EventData(FILE *fp, FILE *fpn);
    int ReadCounters(FILE *fp);
    int ReadData(FILE *fp);
    unsigned int get_bit(unsigned int number, short bit);
    int PrintData(FILE *fpn);
    int PrintDataFull(FILE *fpn);
    int PrintDataDouble(FILE *fp);

    // new
    int EventDataFull(FILE *fp, FILE *fpn);
    int PrintEid(FILE *fp);
    int ReadCurrents(FILE *fp);
    int PrintCurrents(FILE *ff);
    //float kod_to_I(float kod);
    int ReadLocalTime(FILE *fp);
    int PrintLocalTime(FILE *ff);
    int ReadTriggerTime(FILE *fp);
    int PrintTriggerTime(FILE *fp);
    int ReadInclinometer(FILE *fp);
    int PrintInclinometer(FILE *fp);
    int ReadMagnitometer(FILE *fp);
    int PrintMagnitometer(FILE *fp);
    int ReadGPSStamp(FILE *fp);
    int PrintGPSStamp(FILE *fp);

    // 2019
    int PrintTunkaNumber();
    int PrintTunkaNumberCsv();
    int PrintBinTunkaNumberCsv();
    int PrintTelemetryCsv(char sep);
    int PrintTelemetryHead(char sep);
    int BaselineProcess();
    int PrintBaselineSpec();
    int PrintBaselines();
    int CalculateTunkaNumber();
    double CalculateSignalSum();
    int PrintCountersToFile();



private:
//STATUS FLAGS
    int AFlag;   ///< addron  status flag
    int BFlag;   ///< buf2    status flag
    int CFlag;   ///< chanmax status flag
    int IFlag, SimFlag; //a,b,c init flag

    int var,buf2,chanmax,addron;
    int ppstime,trigtime,gnum;
    char gbuf[255], mygbuf[255];
    char time_string[40];

    const int baseline_width = 3;  ///< Полуширина распределения амплитуд  для подсчета пьедесталов

    int NTotal,   ///< Full number of events in current run
        NSim;     ///< Simulated events counter

    short MyData[NCHAN][NTE];
    short MyTrig[NCHAN][NTE];
    short MyDisc[NCHAN][NTE];    // bit of discriminator in channel

    double Signal[NCHAN][NTE];
    double Baseline1[NCHAN], Baseline2[NCHAN];
    unsigned int GlobalSpec1[NCHAN][1024];
    unsigned int GlobalSpec2[NCHAN][1024];
    int Syncro[60];
    int Counters[MAXADDRON][CHANPMT];


    unsigned char trig[NB];

    union CharInt_converter Trig;
    union CharInt_converter PPS;
    struct tm* ptm;
    struct timeval tv0,tv1;
    FILE *fptest;
    FILE *CharTest;

    //
    int Eid;
    int EInclination, EMagnitation;
    int ETriggertime, ELocaltime;
    int ECurrent[NPMT];  // currents
    int Egph, Egpm, Egps;
    int EgphPrev, EgpmPrev, EgpsPrev;
    int TunkaNumber;
    float EHeight;
    float EHeightPrev;
};


// ==================================================================
// ==================================================================
// ==================================================================
/// Constructor
S01TranslateFile::S01TranslateFile()
{
    buf2 = 510;    // defaults values of buf2
    chanmax = 16;
    addron = 14;

    Egph = 99;
    Egpm = 99;
    Egps = 99;
    EHeight = 1000;
    TunkaNumber = -1;

    for(int i = 0; i < NCHAN; i++)
    {
        for (int j = 0; j < 1024; j++)	
        {
            GlobalSpec1[i][j] = 0;
            GlobalSpec2[i][j] = 0;
        }
    }
}


// ==================================================================
/// Destructor
S01TranslateFile::~S01TranslateFile()
{}


// ==================================================================
/**
 * Main function to translate one binary file from binary to text format
 * \param fp  binary file name pointer
 * \param fpn NULL
 */
int S01TranslateFile::TranslateFile(FILE *fp, FILE *fpn) //input Fregat file and output namefile
{
    SimFlag= 0;
    AFlag= 0; BFlag= 0; CFlag= 0; //addron, buf2, chanmax not initialized
    IFlag= 0;

    if (CharTestFlag>0)
    {
        CharTest= fopen("TestC","a");
        if (CharTest==NULL)
        {
            printf("S01Translate::TranslateFile Error--file TestC could not be opened\n");
            return(1);
        }
    }
    PrintTelemetryHead(SEP);

    while ((var= getc(fp)) != EOF)
    {
        Process(fp,fpn);
        if ((CharTestFlag>0) && (CharTest!=NULL)) 
            fprintf(CharTest," %c\n",var);
    }
    if (CharTestFlag>0) fclose(CharTest);

    PrintBaselineSpec();
    PrintCountersToFile();

    return 0;
}


// ==================================================================
/** Read characteristic flags from binary file
 * \param fp  binary file name pointer
 * \param fpn NULL
 */
int S01TranslateFile::Process(FILE *fp, FILE *fpn)
{
    int i = 0; //, NChan = 0;
    unsigned char flag;
    char tmp = '0';

    Conv.tInt = 0;

    flag = (unsigned char) (var);
#ifdef VERBOSE
    printf(">%c<=%d\n", flag, flag);
#endif

    // COMPUTE NChan - number of active channels
    if (IFlag==0)
    {
        if ((AFlag==1) && (BFlag==1) && (CFlag==1))
        {   //NChan= 96;
            //NChan = addron*chanmax; //12*16=96
            IFlag = 1;
        }
    }

    if(fp == NULL)
    {
        printf("File not found");
        return 1;
    }

    /// Flag a - number ON Fadc adresses
    if(flag == 'a') //there are sudden 'a', 'b', 'c' errors
    {
        // FADC boards on
        if(AFlag == 0)
        {
            Conv.tInt = 0;
            fscanf(fp, "%c%c", &Conv.tChar[1], &Conv.tChar[0]);
            addron = Conv.tInt;
            if (DebugPrint>0)
            {
                printf("AddrOn= %i\n", addron);
                if(fptest!=NULL) 
                    fprintf(fptest,"AddrOn= %i\n", addron);
            }
            if((CharTestFlag>0) && (CharTest!=NULL)) 
                fprintf(CharTest," a ");
        }
        AFlag = 1; //addron is initialized
    }

    /// Flag b - number of buffer buf2 depth --- time bins in event
    else if(flag=='b')
    {
        if (BFlag==0)
        {
            Conv.tInt = 0;
            fscanf(fp, "%c%c", &Conv.tChar[1], &Conv.tChar[0]);
            buf2 = Conv.tInt;
            if (DebugPrint)
            { 
                printf("Buf2= %i\n", buf2);
                if (fptest) fprintf(fptest,"BUF2= %i\n", buf2);
            }
            if ((CharTestFlag>0)&&(CharTest!=NULL)) fprintf(CharTest," b ");
        }
        BFlag= 1;
    }

    /// Flag c - chanmax - number of channels (thick+thin) on one Fadc board
    else if (flag=='c')
    {
        if (CFlag==0)
        {
            fscanf(fp, "%c%c", &Conv.tChar[1], &Conv.tChar[0]);
            chanmax = Conv.tInt;
            if (DebugPrint>0)
            { //chanmax= 16;
                printf("Chanmax= %i\n", chanmax);
                if (fptest) fprintf(fptest,"Chanmax= %i\n", chanmax);
            }
            if ((CharTestFlag>0)&&(CharTest!=NULL)) fprintf(CharTest," c ");
        }
        CFlag= 1;
    }

    /// Flag B - barometer and T
    else if(flag=='B')
    {
        Conv.tInt = 0;
        fscanf(fp, "%c%c%c%c",&Conv.tChar[1],&Conv.tChar[0],&Conv.tChar[3],&Conv.tChar[2]);
        if (DebugPrint>0)
        {
            printf("Barometer: %i-%i-%i-%i\n",
            Conv.tChar[3],Conv.tChar[2],Conv.tChar[1],Conv.tChar[0]);
        }
        if ((CharTestFlag>0)&&(CharTest!=NULL))
            fprintf(CharTest," B ");
    }

    /// Flag t - localtime in seconds
    else if (flag=='t')
    {
        ReadLocalTime(fp);
        if (DebugPrint>0)
        {
            PrintLocalTime(stdout);
        }
    }

    /// Flag p - PPS time
    else if (flag=='p')
    {
        fscanf(fp, "%c%c%c%c",&Conv.tChar[1],&Conv.tChar[0],&Conv.tChar[3],&Conv.tChar[2]);
        for (i= 0; i<4; i++) PPS.tChar[i]= Conv.tChar[i];
            ppstime= Conv.tInt;
        if (DebugPrint>0)
        {
            printf("PPS: %i-%i-%i-%i\n",Conv.tChar[3],
                Conv.tChar[2],Conv.tChar[1],Conv.tChar[0]);
        }
        if ((CharTestFlag>0)&&(CharTest!=NULL))
        {
            fprintf(CharTest,"PPS: %i-%i-%i-%i ",
                Conv.tChar[3],Conv.tChar[2],Conv.tChar[1],Conv.tChar[0]);
            fprintf(CharTest," p ");
        }
    }

    /// Flag I - inclinometer (4 bytes)
    else if (flag=='I')
    {
        fscanf(fp, "%c%c%c%c",&Conv.tChar[1],&Conv.tChar[0],&Conv.tChar[3],&Conv.tChar[2]);
        if (DebugPrint>0)
        {
            printf("Inclinometer: %i-%i-%i-%i\n",
                Conv.tChar[3],Conv.tChar[2],Conv.tChar[1],Conv.tChar[0]);
        }
        if ((CharTestFlag>0)&&(CharTest!=NULL))
            fprintf(CharTest," I ");
    }

    /// Flag g - GPStime: corrected for read "GPGGA" standart sentence
    else if (flag=='g')
    {
        Conv.tInt = 0;
        fscanf(fp, "%c", &Conv.tChar[0]);
        gnum = Conv.tInt;

        for (i=0; i<gnum; i++) 
            fscanf(fp, "%c", &gbuf[i]);
        gbuf[gnum] = '\0';

        if(DebugPrint > 0)
        {
            printf("GPS: %s\n", gbuf);
            for (i=0; i<5; i++) printf("%c",gbuf[i+1]);
            printf("\n");
        }
        if((gbuf[1]=='G') && (gbuf[2]=='P') && (gbuf[3]=='G') && (gbuf[4]=='G') && (gbuf[5]=='A'))
        {
            for (i = 0; i < gnum; i++)
                mygbuf[i]= gbuf[i];
        }
        if ((CharTestFlag>0)&&(CharTest!=NULL))
        {
            fprintf(CharTest,"GPS: %s ", gbuf);
            fprintf(CharTest," g ");
        }
    }

    /// Flag r - counters. Call function ReadCounters()
    else if(flag=='r')
    {
        ReadCounters(fp);
        if ((CharTestFlag>0)&&(CharTest!=NULL))
            fprintf(CharTest," r ");
    }
    /// Flag r - counters. The same as flag r
    else if(flag=='R')
    {
        ReadCounters(fp);
        if ((CharTestFlag>0)&&(CharTest!=NULL))
            fprintf(CharTest," R ");
    }

    /// Flag s - simulated event. Call function EventData()
    else if(flag=='s')
    {
        SimFlag= 1;
        //inclinometer
        fscanf(fp, "%c",  &Conv.tChar[0]); // read 'I'
        //check FLAGS
        fscanf(fp, "%c%c%c%c",&Conv.tChar[1],&Conv.tChar[0],&Conv.tChar[3],&Conv.tChar[2]);
        //check 'k'
        fscanf(fp, "%c",  &Conv.tChar[0]); // read 'k'
        //if (PrintSimFlag>0)
        EventData(fp,fpn); //print simulated events
        SimFlag = 0;
        if ((CharTestFlag>0) && (CharTest!=NULL))
            fprintf(CharTest," s ");
        NSim++;
    }

    /// Flag < - real event. Call function EventDataFull()
    else if (flag=='<')
    {
        fscanf(fp, "%c",  &tmp); 
        //printf("<%c", tmp);
        if(tmp == 'K')
        {
            EventDataFull(fp, fpn); // event data
            if ((CharTestFlag>0) && (CharTest!=NULL))
                fprintf(CharTest," k ");
            NTotal++;
        }
        if (DebugPrint) printf("event OK!\n");
    }

    return(0);
}


// ==================================================================
/** Read full information about binary event
 * \param fp  binary file name pointer
 * \param fpn NULL
 */
int S01TranslateFile::EventDataFull(FILE *fp, FILE *fpn)
{
    unsigned char tmp = 'i';
    char nn[8] = {0};
    int i = 0;

    if(SimFlag)
    {
        if (DebugPrint2>0)
        printf("\nk%i SIMULATED ", NSim);
    }
    else
    {
        if (DebugPrint2>0)
        printf("k%i ", EventNumber);
    }

    /// Read Event number
    fscanf(fp, "%c",  &tmp);
    while(isDigit(tmp))
    {
        nn[i] = tmp;
        i ++;
        fscanf(fp, "%c",  &tmp);
    }

    sscanf(nn, "%d", &Eid);
    PrintEid(stdout);

    if(tmp != '>') 
    {
        printf("Error in reading \">\"  Read:>%c<\n", tmp);
        return 1;
    }

    /// Read GPS stamp. Call ReadGPSStamp()
    fscanf(fp, "%c", &tmp);
    if(tmp != 'g') 
    {
        printf("Error in reading \"g\"  Read:>%c<\n", tmp);
        return 1;
    }
    ReadGPSStamp(fp);
    if (DebugPrint) PrintGPSStamp(stdout);

    /// Read t - local time. Call ReadLocalTime()
    fscanf(fp, "%c",  &tmp);
    if(tmp != 't') 
    {
        printf("Error in reading \"t\" \n  Read:>%c<\n", tmp);
        fflush(stdout);
        return 1;
    }
    ReadLocalTime(fp);
    if (DebugPrint) PrintLocalTime(stdout);

    /// Read e - trigger time. Call ReadTriggerTime()
    fscanf(fp, "%c",  &tmp);
    if(tmp != 'e') 
    {
        printf("Error in reading \"e\" \n");
        fflush(stdout);
        return 2;
    }
    ReadTriggerTime(fp);
    if (DebugPrint) PrintTriggerTime(stdout);

    /// Read flag I - inclination. Call ReadInclinometer()
    fscanf(fp, "%c",  &tmp);
    if(tmp != 'I') 
    {
        printf("\nError in reading \"I\" \n Read:>%c<\n", tmp);
        return 2;
    }
    ReadInclinometer(fp);
    if (DebugPrint) PrintInclinometer(stdout);

    /// Read flag m - magnetometer. Call ReadMagnitometer()
    fscanf(fp, "%c",  &tmp);
    if(tmp != 'm') 
    {
        printf("Error in reading \"m\" \n");
        return 2;
    }
    ReadMagnitometer(fp);
    if (DebugPrint) PrintMagnitometer(stdout);

    /// Read flag i - currents. Call ReadCurrents()
    fscanf(fp, "%c",  &tmp);
    if(tmp != 'i') 
    {
        printf("Error in reading \"i\" \n");
        return 2;
    }
    ReadCurrents(fp);
    if (DebugPrint) PrintCurrents(stdout);

    /// Read k - FADC data. Call ReadData()
    fscanf(fp, "%c",  &tmp);
    if(tmp != 'k') 
    {
        printf("Error in reading \"k\" \n");
        return 2;
    }
    ReadData(fp);
    if (DebugPrint) printf("Read data OK!\n");

    /// Process baselines. Call BaselineProcess()
    BaselineProcess();
    if (DebugPrint) printf("Baseline data OK!\n");

    /// Print all data to all output files. Call PrintDataFull()
    PrintDataFull(fpn);
    //!!!PrintData(fpn);

    return 0;
}


// ===============================================
/**
 * Read NPMT currents from binary file
 */
int S01TranslateFile::ReadCurrents(FILE *fp)
{
    int z = 0;
    for(z = 0; z < NPMT; z++)
    {
        Conv.tChar[3] = 0;
        fscanf(fp, "%c%c%c",&Conv.tChar[2],&Conv.tChar[1],&Conv.tChar[0]);
        ECurrent[z] =  Conv.tInt;
    }
    return 0;
}


// ==================================================================
/**
 * Print currents to open file (e.g. stdout) as a table
 */
int S01TranslateFile::PrintCurrents(FILE *ff)
{
    //float tok = 0.0;

    fprintf(ff,"<Currents>\n");
    fprintf(ff,"     0[uA]  1[uA]  2[uA]  3[uA]  4[uA]  5[uA]  6[uA]  7[uA]  8[uA]  9[uA]");
    for(int ii = 0; ii < NPMT; ii++)
    {
        if(ii%10 == 0)    fprintf(ff,"\n%3d", ii/10);
        fprintf(ff," %5d ", ECurrent[ii]);
    }
    fprintf(ff,"\nInclinometer</Currents>\n");

    return 0;
}


// ==================================================================
/**
 * Read localtime from binary file
 */
int S01TranslateFile::ReadLocalTime(FILE *fp)
{
    fscanf(fp, "%c%c%c%c",&Conv.tChar[3],&Conv.tChar[2],&Conv.tChar[1],&Conv.tChar[0]);
    tv1.tv_sec = Conv.tInt;
    ELocaltime = Conv.tInt;
    return 0;
}


// ==================================================================
/**
 * Print localtime to open file (e.g. stdout)
 */
int S01TranslateFile::PrintLocalTime(FILE *ff)
{
    fprintf(ff,"<LOCALTIME>\n");
    tv1.tv_sec = ELocaltime;
    ptm = localtime (&tv1.tv_sec);
    strftime(time_string, sizeof(time_string), "%Y-%m-%d  %H:%M:%S", ptm);
    fprintf(ff, "%s\n", time_string);
    fprintf(ff, "%i", ELocaltime);
    fprintf(ff,"\n</LOCALTIME>\n");
    return 0;
}


// ==================================================================
/**
 * Read triggertime from binary file
 */
int S01TranslateFile::ReadTriggerTime(FILE *fp)
{
    fscanf(fp, "%c%c%c%c",&Conv.tChar[3],&Conv.tChar[2],&Conv.tChar[1],&Conv.tChar[0]);
    ETriggertime =  Conv.tInt;
    //ptm = localtime (&localtime.tv_sec);
    //strftime(localtime_string, sizeof(localtime_string), "%Y-%m-%d  %H:%M:%S", ptm);
    return 0;
}


// ==================================================================
/**
 * Print trigger time to open file (e.g. stdout)
 */
int S01TranslateFile::PrintTriggerTime(FILE *fp)
{
    fprintf(fp,"<TRIGGERTIME>\n");
    //fprintf(fp, "%d", ETriggertime);
    Conv.tInt = ETriggertime;
    fprintf(fp, " %3d %3d %3d %3d",Conv.tChar[1],Conv.tChar[0],Conv.tChar[3],Conv.tChar[2]);
    //fprintf(fp,"<TRIG>%3d %3d %3d %3d</TRIG>\n",Trig.tChar[1],Trig.tChar[0],Trig.tChar[3],Trig.tChar[2]);
    fprintf(fp,"\n</TRIGGERTIME>\n");    
    return 0;
}


// ==================================================================
/**
 * Read Inclinometer from binary file
 */
int S01TranslateFile::ReadInclinometer(FILE *fp)
{
    fscanf(fp, "%c%c%c%c",&Conv.tChar[1],&Conv.tChar[0],&Conv.tChar[3],&Conv.tChar[2]);
    EInclination = Conv.tInt;
    return 0;
}


// ==================================================================
/**
 * Print inclinometer to open file (e.g. stdout)
 */
int S01TranslateFile::PrintInclinometer(FILE *fp)
{
    short ang1 = 0;
    short ang2 = 0;
    float fang1 = 0., fang2 = 0;

    fprintf(fp,"<INCLINOMETER>\n");
    Conv.tInt = EInclination;
    ang1 = Conv.tChar[3] * 256 + Conv.tChar[2];
    ang2 = Conv.tChar[1] * 256 + Conv.tChar[0];

    fang1 = (float)ang1 * 0.001;
    fang2 = (float)ang2 * 0.001;
    fprintf(fp,"%2.1f  %2.1f", fang1, fang2);

    fprintf(fp,"\n</INCLINOMETER>\n");
    return 0;
}


// ==================================================================
/**
 * Read magnitometer
 */
int S01TranslateFile::ReadMagnitometer(FILE *fp)
{
    fscanf(fp, "%c%c%c%c",&Conv.tChar[3],&Conv.tChar[2],&Conv.tChar[1],&Conv.tChar[0]);
    Conv.tChar[3] = 0;
    Conv.tChar[2] = 0;
    EMagnitation = Conv.tInt;
    return 0;
}


// ==================================================================
/**
 * Print magnitometer to open file (e.g. stdout)
 */
int S01TranslateFile::PrintMagnitometer(FILE *fp)
{
    fprintf(fp,"<MAGNITOMETER>\n");
    fprintf(fp,"%d", EMagnitation);
    fprintf(fp,"\n</MAGNITOMETER>\n");
    return 0;
}


// ==================================================================
/**
 * Print Event number to open file (e.g. stdout)
 */
int S01TranslateFile::PrintEid(FILE *fp)
{
    fprintf(fp,"<EID>");
    fprintf(fp,"%05d", Eid);
    fprintf(fp,"</EID>\n");
    return 0;
}


// ==================================================================
/**
 * Read GPS stamp
 */
int S01TranslateFile::ReadGPSStamp(FILE *fp)
{
    int hh = 0;
    unsigned char gph = 0, gpm = 0, gps = 0;
    Conv.tInt = 0;

    // read back gps_stamp
    fscanf(fp,"%c%c%c%c%c", &gph,&gpm,&gps,&Conv.tChar[1],&Conv.tChar[0]);
    Egph = gph;
    Egpm = gpm;
    Egps = gps;
    hh = Conv.tChar[1] << 8;
    hh += Conv.tChar[0];
    EHeight = hh;
    EHeight /= 10.;
    return 0;
}


// ==================================================================
/**
 * Print GPS stamp to open file (e.g. stdout)
 */
int S01TranslateFile::PrintGPSStamp(FILE *fp)
{
    fprintf(fp,"<GPS Time>\n");
    fprintf(fp,"%02d:%02d:%02d",Egph,Egpm,Egps);
    fprintf(fp,"\n</GPS Time>\n");
    fprintf(fp,"<GPS Height>\n");
    fprintf(fp,"%.2f",EHeight);
    fprintf(fp,"\n</GPS Height>\n");
    return 0;
}



// ==================================================================
/**
 * Read data and print simulated event data to file
 */
int S01TranslateFile::EventData(FILE *fp, FILE *fpn)
{
    if(SimFlag)
    {
        if (DebugPrint2>0)
        printf("\nk%i SIMULATED ", NSim);
    }
    else
    {
        if (DebugPrint2>0)
        printf("\nk%i ", EventNumber);
    }
    ReadData(fp);
    PrintData(fpn);
    return(0);
}


// ==================================================================
/**
 * Read after flag 'r' - read counters from binary file to array r[][]
 */
int S01TranslateFile::ReadCounters(FILE *fp)
{
    int a, i;

    for (a=0; a<addron; a++)
        for (i= 0; i<CHANPMT; i++)
            Counters[a][i]= 0;

    if ((CharTestFlag>0)&&(CharTest!=NULL))
        fprintf(CharTest," r ");

    for (a = 0; a < addron; a ++)
    {
        for (i = 0; i < CHANPMT; i++)
        {
            fscanf(fp,"%c%c",&Conv.tChar[1],&Conv.tChar[0]);
            Counters[a][i]= Conv.tInt;
            if ((CharTestFlag>0)&&(CharTest!=NULL))
                fprintf(CharTest,"%d ",Counters[a][i]);
        }
        if ((CharTestFlag>0)&&(CharTest!=NULL))
        fprintf(CharTest,"\n");
    }
    if ((CharTestFlag>0)&&(CharTest!=NULL))
        fprintf(CharTest,"\n");

    return(0);
}


// ==================================================================
/**
 * Read one experimental shower data from binary file
 */
int S01TranslateFile::ReadData(FILE *fp)
{
    int a = 0, i = 0, j = 0, ind = 0;
    int fdata, ftrig;

    if(fp == NULL)
        return 1;
    EventNumber ++;

    /// Init MyData
    for (i= 0; i<NCHAN; i++)
        for (j= 0; j<NTE; j++)
        {
            MyData[i][j]= 0;
            MyTrig[i][j]= 0;
        }

    /// Read MyData
    for (a= 0; a<addron; a++)
    {
        for (i= 0; i<buf2; i++)
        {
            for (j= 0; j<chanmax; j++)
            {
                if(fp) fscanf(fp,"%c%c", &Conv.tChar[1],&Conv.tChar[0]);

                ind = a*chanmax+j;
                if(ind >= NCHAN) continue; // to prevent Data array margins overflow!

                // get trigger bit
                ftrig= get_bit(Conv.tInt, 11); //trig= 0 event
                trig[(i * chanmax * addron) + (a * chanmax) + j]= ftrig;
                MyTrig[ind][i] = ftrig;

                // discriminator of channel
                ftrig = get_bit(Conv.tInt, 12); 
                MyDisc[ind][i]= ftrig;

                //fdata= (Conv.tInt&1023);  // read word
                fdata = Conv.tInt;  // read word
                if ((ind>=0)&&(ind<NCHAN)&&(i>=0)&&(i<NTE)) 
                    MyData[ind][i]= fdata;
                /*else
                {
                    continue;
                    printf("ind = %d addron=%d a = %d i= %d j =%d NTE = %d NCHAN = %d ", ind, addron, a, i, j, NTE, NCHAN);
                    printf("S01Translate::ReadData->Data array margins overflow!\n");
                }*/
                if ((CharTestFlag>0)&&(CharTest!=NULL))
                    fprintf(CharTest,"%4d ",MyData[ind][i]);
            }
            if ((CharTestFlag>0)&&(CharTest!=NULL))
                fprintf(CharTest,"\n");
        }
        if ((CharTestFlag>0)&&(CharTest!=NULL))
            fprintf(CharTest,"\n");
    }
    if ((CharTestFlag>0)&&(CharTest!=NULL))
        fprintf(CharTest,"\n");

    /// Read tg time
    if (fp)
        fscanf(fp,"%c%c%c%c",&Conv.tChar[3],&Conv.tChar[2],
                             &Conv.tChar[1],&Conv.tChar[0]);
    for (i= 0; i<4; i++) 
        Trig.tChar[i]= Conv.tChar[i];
    trigtime = Conv.tInt;
    if (DebugPrint>0) 
        printf("time TG %u = %xh\n", trigtime, trigtime);

    /// Read local time
    if (fp)
        fscanf(fp,"%c%c%c%c",&Conv.tChar[0],&Conv.tChar[1], &Conv.tChar[2],&Conv.tChar[3]);
    tv1.tv_sec = Conv.tInt;
    ptm = localtime (&tv1.tv_sec);
    strftime(time_string, sizeof(time_string), "%Y-%m-%d  %H:%M:%S", ptm);

    return(0);
}


// ==================================================================
/**
 * Get a bit from a number
 */
unsigned int S01TranslateFile::get_bit(unsigned int number, short bit)
{
    unsigned short bitt[16] = { 1,2,4,8,16,32,64,128,256,512,1024,
                                2048,4096,8192,16384,32768}; //,65536};
    if(bit >= 16) return 0;
    if(number & bitt[bit]) return 1;
    return 0;
}


// ==================================================================
/**
 * Print telemetry data to telemetry file, and data frame to new text file
 */
int S01TranslateFile::PrintDataFull(FILE *fpn)
{
    char d[LEN] = {0};
    FILE *fp;

    // not consider simulated events
    if (SimFlag > 0) return(1);

    // --------------------------------------------------
    /// Create new file for translated data frame 
    if(Eid)
        sprintf(d, "%05d.txt", Eid);
    else
        sprintf(d, "%05d.txt", EventNumber);
    if ((fp = fopen(d, "w")) == NULL)
    {
        printf("S01Translate::PrintData Error-- file %s could not be opened\n",d);
        return 1;
    }

    /// Print data frame
    PrintDataDouble(fp);
    fclose(fp);

    // --------------------------------------------------
    /// Print Tunka syncro-number to separate file
    //PrintTunkaNumber();
    //PrintTunkaNumberCsv();
    PrintBinTunkaNumberCsv();

    // --------------------------------------------------
    /// Print telemetry to separate file
    PrintTelemetryCsv(SEP);
    PrintBaselines();

    return 0;
}


// ==================================================================
/**
 * Print data frame to open file (e.g. stdout)
 */
int S01TranslateFile::PrintData(FILE *fp)
{
    int i, j;

    for (i=0; i < buf2; i++)
    {
        //fprintf(fp,"%3d ", 2 * i);
        for(j = 1; j < NCHAN; j+=2) 
        {
            if(MyData[j][i] & 1024) fprintf(fp," %4d ", 0);
            else                    fprintf(fp," %4d ", MyData[j][i]&1023);
        }
        fprintf(fp,"\n");
        //fprintf(fp,"%3d ", 2 * i + 1);
        for(j = 0; j < NCHAN; j+=2) 
        {
            //if(j%2 == 0) fprintf(fp," %4d ",MyData[j][i]);
            if(MyData[j][i] & 1024) fprintf(fp," %4d ", 0);
            else                    fprintf(fp," %4d ", MyData[j][i]&1023);
        }
        fprintf(fp,"\n");
    }
    return 0;
}

// ==================================================================
/**
 * Print data frame withot baselines to open file (e.g. stdout)
 */
int S01TranslateFile::PrintDataDouble(FILE *fp)
{
    int chanmax = 64; //PMT;
    /// \todo NPMT и кол-во плат привести в соответствие


    for (int i = 0; i < buf2; i++)
    {
        for(int j = 0; j < chanmax; j += 1) 
        {
            fprintf(fp,"%7.2f ", Signal[j][i]);
        }
        fprintf(fp,"\n");
    }
    return 0;
}

// ==================================================================
/**
 * Calculate Tunka syncro_pulse number
 */
int S01TranslateFile::CalculateTunkaNumber()
{
    int bit[16] = { 4, 7, 10, 13, 16, 20, 23, 26, 29, 32, 36, 39, 42, 45, 48, 52};
    int result1 = 0;
    int result2 = 0;
    int result = 0;

    /// Calculate two syncro numbers
    for(int i = 15; i >= 0; i--)
    {
        result1 *= 2;
        result1 += Syncro[bit[i]];
        result2 *= 2;
        result2 += Syncro[bit[i] + 1];
    }
    //printf("Syncro: %d %d %d ", TunkaNumber, result1, result2);

    result = result2;
    /// If two numders are different - compare it with syncro number of  previous event
    if(result1 != result2)
    {
        if(result < TunkaNumber)
            result = result1;
        if(result < TunkaNumber)
            result = -2; // Wrong number
    }
    /// If two numbers are smaller or equal to previous syncro number
    if(TunkaNumber >=0)
        if(result <= TunkaNumber)
            result = -2; // Wrong number

    return result;
}

// ==================================================================
/**
 * Print Tunka number syncro_pulse to file and calculate Tunka syncro number.
 */
int S01TranslateFile::PrintBinTunkaNumberCsv()
{
    FILE *fp;
    int threshold = 304;
    int i = 0, k = 0, bit = 0, n = 0;
    int pulse[2*buf2] = {0};

    /// Init Syncro pulse
    for(i = 0; i < 60; i++)
    {
        Syncro[i] = 0;
    }

    n = 0;
    for (i = 0; i < buf2; i++)
    {
        pulse[n] = MyData[127][i] & 1023;
        n++;
        pulse[n] = MyData[126][i] & 1023;
        n++;
    }

    /// Open file
    if ((fp = fopen(SyncroFile,"a")) == NULL)
    {
        printf("S01Translate::PrintTunkaNum() Error-- file %s could not be opened\n", SyncroFile);
        return 1;
    }

    /// Print pulse to file
    fprintf(fp, "%d", Eid);

    k = -1;
    for(i = 0; (pulse[i] < threshold) && (i < 2* buf2); i++);
    k = i;

    /// If syncro pulse exists - calculate it.
    if( (k > 0) && (k < 2 * buf2 - 60))
    {
        for(i = k; i < k + 60; i++)
        {
            if(pulse[i] > threshold)
                bit = 1;
            else
                bit = 0;
            Syncro[i-k] = bit;
            fprintf(fp,",%1d", bit);
        }
        /// Calculate Tunka syncro pulse number
        TunkaNumber = CalculateTunkaNumber();
    }
    /// If syncro pulse does not exists - write pulse of 0.
    else 
    {
        for(i = 0; i < 60; i++)
        {
            fprintf(fp,",%1d", 0);
        }
        TunkaNumber = -1;
    }

    fprintf(fp,",%d\n", TunkaNumber); // !!!!!!!!!
    // close file
    fclose(fp);
    return 0;
}


// ==================================================================
/**
 * Print Tunka syncro number pulse to file as two columns.
 */
int S01TranslateFile::PrintTunkaNumber()
{
    FILE *fp;
    char name[50];
    int i, j = 63 * 2 + 1;

    sprintf(name, "%05d.tun", Eid);
    if ((fp = fopen(name,"w")) == NULL)
    {
        printf("S01Translate::PrintTunkaNum() Error-- file %s could not be opened\n", name);
        return 1;
    }

    for (i=0; i < buf2; i++)
    {
        fprintf(fp,"%3d ", 2 * i);
        j = 63 * 2 + 1;
        fprintf(fp," %4d ", MyData[j][i] & 1023);
        fprintf(fp,"\n");
        fprintf(fp,"%3d ", 2 * i + 1);

        j = 63 * 2;
        fprintf(fp," %4d ", MyData[j][i] & 1023);
        fprintf(fp,"\n");
    }
    fclose(fp);
    return 0;
}


// ==================================================================
/**
 * Print Tunka syncro_pulse to csv file.
 */
int S01TranslateFile::PrintTunkaNumberCsv()
{
    FILE *fp;
    int i, j = 63 * 2 + 1;

    if ((fp = fopen(SyncroFile,"a")) == NULL)
    {
        printf("S01Translate::PrintTunkaNum() Error-- file %s could not be opened\n", SyncroFile);
        return 1;
    }

    fprintf(fp, "%d", Eid);
    for (i=0; i < buf2; i++)
    {
        j = 63 * 2 + 1;
        fprintf(fp,",%d", MyData[j][i] & 1023);
        j = 63 * 2;
        fprintf(fp,",%d", MyData[j][i] & 1023);
    }
    fprintf(fp,"\n");
    fclose(fp);
    return 0;
}


// ==================================================================
/**
 *  Check if argument is Digit
 * \param tmp - one char to check to be digit
 */
int isDigit(unsigned char tmp)
{
    if( (tmp < '0') || (tmp > '9') )
        return 0;
    return 1;
}


// ==================================================================
/**
 * Print telemetry information to special telemetry table file
 */
int S01TranslateFile::PrintTelemetryCsv(char sep)
{
    FILE *ff;
    double SignalSum = 0.;

    /// open telemetry file
    if ((ff = fopen(TelemetryFile, "a")) == NULL)
    {
        printf("S01Translate::PrintData Error-- file %s could not be opened\n", TelemetryFile);
        return 1;
    }

    /// print number Eid
    fprintf(ff,"%5d%c", Eid, sep);

    /// print Tunka syncro number
    fprintf(ff,"%d%c", TunkaNumber, sep);

    /// print computer time from GPS and Height
    fprintf(ff,"%02d:%02d:%02d.%.0f%c", Egph, Egpm, Egps, EHeight, sep);

    /// print localtime in seconds
    fprintf(ff, "%i%c", ELocaltime, sep);

    /// print local time as string
    tv1.tv_sec = ELocaltime;
    ptm = localtime (&tv1.tv_sec);
    strftime(time_string, sizeof(time_string), "%Y-%m-%d", ptm);
    fprintf(ff, "%s%c", time_string, sep);
    strftime(time_string, sizeof(time_string), "%H:%M:%S", ptm);
    fprintf(ff, "%s%c", time_string, sep);

    /// print current
    fprintf(ff," %5d%c", ECurrent[52], sep);

    /// print trigger time
    //Conv.tInt = ETriggertime;
    //fprintf(ff, " %3d %3d %3d %3d",Conv.tChar[1],Conv.tChar[0],Conv.tChar[3],Conv.tChar[2]);

    /// Calculate and print Signal Sum over trigger region
    SignalSum = CalculateSignalSum();
    fprintf(ff,"%.2f", SignalSum);

    /// close telemetry file
    fprintf(ff,"\n");
    fclose(ff);
    return 0;
}


// ==================================================================
/**
 * Print head to separate telemetry head file
 */
int S01TranslateFile::PrintTelemetryHead(char sep)
{
    FILE *ff;
    char name[100];

    /// open telemetry file
    sprintf(name, "%s%s", TelemetryFile, ".head");
    if ((ff = fopen(name, "w")) == NULL)
    {
        printf("S01Translate::PrintTelemetryHead Error-- file %s could not be opened\n", name);
        return 1;
    }

    fprintf(ff,"Eid%c", sep);

    /// print Tunka syncro number
    fprintf(ff,"Tunka%c", sep);

    /// print computer time from GPS and Height
    fprintf(ff,"CompTime%c", sep);

    /// print localtime in seconds
    fprintf(ff, "LocaltimeSec%c", sep);

    /// print local time as string
    fprintf(ff, "LocalDate%c", sep);
    fprintf(ff, "LocalTime%c", sep);

    /// print current
    fprintf(ff,"Current%c", sep);

    /// Print Signal Sum over trigger region
    fprintf(ff,"SignalSum");

    /// close telemetry head file
    fprintf(ff,"\n");
    fclose(ff);
    return 0;
}


// ==================================================================
/**
 * Calculate Signal summ in 20-120 time bins over all triggered channels
 */
double S01TranslateFile::CalculateSignalSum()
{
    int start = 20;   ///< start bin for sum
    int stop  = 120;  ///< stop  bin for sum
    double Sum = 0.;  ///< sum
    int maxchan = 64; // NPMT
    /// \todo NPMT и кол-во каналов привести в соответствие

    for (int i = start; i < stop; i++)
    {
        for(int j = 0; j < maxchan; j += 1)
        {
            /// 23 channel is not summed
            if (CHANUSE[j] == 1)
                Sum += Signal[j][i];
        }
    }
    return Sum;
}

// ==================================================================
/**
 * Calculate baselines
 */
int S01TranslateFile::BaselineProcess()
{
    int j = 0;
    int maxkod = 1024;
    int SpecMax1,SpecMaxInd1,SpecMax2,SpecMaxInd2;
    int BaseSpec1[1024], BaseSpec2[1024];
    double SigSum;

    for (int i = 0; i < NPMT; i++)
    {
        if (CHANUSE[i])
        {
            /// Baseline spectrum array clearing
            for (j = 0; j < maxkod;j++)
            {
                BaseSpec1[j] = 0;
                BaseSpec2[j] = 0;
            }

            /// Baseline spectrum filling
            for (j = 0; j < buf2; j++)
            {
                if (j%2)
                {
                    BaseSpec1[MyData[i][j]&1023] += 1;
                }
                else
                {
                    BaseSpec2[MyData[i][j]&1023] += 1;
                }
            }

            /// Baseline spectrum max finder
            SpecMax1 = 0;
            SpecMax2 = 0;
            for (j = 0; j < maxkod; j++)	
            {
                if (SpecMax1 <= BaseSpec1[j]) 
                {
                    SpecMax1 = BaseSpec1[j];
                    SpecMaxInd1 = j;
                }

                if (SpecMax2 <= BaseSpec2[j]) 
                {
                    SpecMax2 = BaseSpec2[j];
                    SpecMaxInd2 = j;
                }

                // Preparing to Save Basespec to file
                GlobalSpec1[i][j] += BaseSpec1[j];
                GlobalSpec2[i][j] += BaseSpec2[j];
            }

            /// Baseline estimation
            Baseline1[i]=0.;
            Baseline2[i]=0.;

            if( (SpecMaxInd1>baseline_width) && (SpecMaxInd1 < maxkod - baseline_width ))
            {
                SigSum=0;
                for (j = SpecMaxInd1-baseline_width; j < (SpecMaxInd1+baseline_width); j++)
                {
                    Baseline1[i] += BaseSpec1[j] * j;
                    SigSum       += BaseSpec1[j];
                }
                Baseline1[i] /= SigSum;
            }

            if( (SpecMaxInd2 > baseline_width) && (SpecMaxInd2 < maxkod - baseline_width))
            {
                SigSum=0;
                for(j = SpecMaxInd2-baseline_width; j < (SpecMaxInd2 + baseline_width); j++)
                {
                    Baseline2[i] += BaseSpec2[j] * j;
                    SigSum       += BaseSpec2[j];
                }
                Baseline2[i] /= SigSum;
            }

            /// Baseline subtraction
            // Signal = data - baseline --- write to file
            for (j = 0; j < buf2; j++)
            {
                if (j%2)
                {
                    Signal[i][j] = double(MyData[i][j]&1023) - Baseline1[i];
                }
                else
                {
                    Signal[i][j] = double(MyData[i][j]&1023) - Baseline2[i];
                }
            }
        }
    }
    return 0;
}


// ==================================================================
/**
 * Print Baselines Spectrum to separate file
 */
int S01TranslateFile::PrintBaselineSpec()
{
    FILE *fp;
    char name[50];
    int chanmax = 64; //PMT;
    /// \todo NPMT и кол-во плат привести в соответствие

    /// open file
    sprintf(name, "%s.spec", BinFileName);
    if ((fp = fopen(name, "w")) == NULL)
    {
        printf("S01Translate::PrintBaselineSpec Error-- file %s could not be opened\n", name);
        return 1;
    }

    /// print first channel spectrum to file
    for(int i = 0; i < chanmax; i++)
    {
        for (int j = 0; j < 1024; j++)	
        {
            fprintf(fp,"%6d ", GlobalSpec1[i][j]);
        }
        fprintf(fp,"\n");
    }
    fprintf(fp,"\n\n");

    /// print second channel spectrum to file
    for(int i = 0; i < chanmax; i++)
    {
        for (int j = 0; j < 1024; j++)	
        {
            fprintf(fp,"%6d ", GlobalSpec2[i][j]);
        }
        fprintf(fp,"\n");
    }

    // close file
    fclose(fp);
    return 0;
}


// ==================================================================
/**
 * Print baselines to special telemetry table file
 */
int S01TranslateFile::PrintBaselines()
{
    FILE *ff;
    int chanmax = 64; //PMT;
    /// \todo NPMT и кол-во плат привести в соответствие

    /// open file
    /// \todo name of file
    if ((ff = fopen("Baselines.txt", "a")) == NULL)
    {
        printf("S01Translate::PrintBaselineSpec Error-- file %s could not be opened\n", "Baselines");
        return 1;
    }

    // print to file
    /// print number Eid
    fprintf(ff,"%05d ", Eid);
    /// print baselines
    for(int i = 0; i < chanmax; i++)
    {
        fprintf(ff,"%6.2f ", Baseline1[i]);
        fprintf(ff,"%6.2f ", Baseline2[i]);
    }
    fprintf(ff,"\n");

    // close file
    fclose(ff);
    return 0;
}


// ==================================================================
/**
 * Print Counters to separate file
 */
int S01TranslateFile::PrintCountersToFile()
{
    FILE *fp;
    char name[50];

    /// open file
    sprintf(name, "%s", "Counters.txt");
    if ((fp = fopen(name, "a")) == NULL)
    {
        printf("S01Translate::PrintCountersToFile Error-- file %s could not be opened\n", name);
        return 1;
    }

    fprintf(fp,"%s\t", BinFileName);
    /// print counters to file
    for(int a = 0; a < addron; a++)
        for(int i = 0; i < CHANPMT; i++)
            fprintf(fp,"%6d", Counters[a][i]);
    fprintf(fp,"\n");

    // close file
    fclose(fp);
    return 0;
}