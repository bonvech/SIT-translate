#
#usage:
#awk -f thp.awk log.txt
#awk -f thp.awk log.txt | awk 'NF >= 2' > thp00
BEGIN {
    flaf = 0; # flag to 
    CHAN = 64
    fifo_err = 0
    ianode = 0
    event = 0
    printf "Event\tIanode\tTmos\tUp\tfifo_err\n"
}
#  ----------------------------------------------------
#  ----------------------------------------------------
{
    if(/File/) 
    {
        sub(/is open!/,"    ")
        sub(/File/,"")
        #printf  $0 "\n"
    }

    #  ------------------------------------------------
    #  Read high voltage
    if(/Ianode/)
    {
        gsub(/=/," ")
        #printf "\n"$0 "\n"

        #  Find number of word Ianode in the line
        for(i=1;i<=NF;i++)
        {
            if($i=="Ianode")
                icur = i+1
            if($i=="Up")
                iup = i+1
            if($i=="T")
                it = i+1
        }
        Tmos = $it
        Ianode = $icur
        Up = $iup

        #printf "\n!!!!Up=>"Up"<-----Ia=>"Ianode"<-----Tmos=>"Tmos"<----\n"
    }


    if(flag == 1) 
    {
        #printf $0 "\n"
    }


    #  ------------------------------------------------
    #  Read fifo_err
    if(/Fifo_err/)
    {
        fifo_err += 1
    }

    #  ------------------------------------------------
    #   Read Event number, print info of last event
    #
    if(/<K/)
    {
        #  print info for previous event
        if(event != 0)
        {
            printf event"   "Ianode"   "Tmos"   "Up"   "fifo_err"\n"
            #printf  $1 kod"  "tem"   "hv1"   "hv2"\n"
        }

        #  init fifo_err counter
        fifo_err = 0

        #  read new event number
        sub(/<K/,"")
        sub(/>/,"")
        event = $1
    }
}
#  ----------------------------------------------------
#  ----------------------------------------------------
END {

}
