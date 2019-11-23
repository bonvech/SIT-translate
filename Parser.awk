#
# usage:
# awk -f thp.awk log.txt
# awk -f thp.awk log.txt | awk 'NF >= 2' > thp00
BEGIN {
    fifo_err = 0
    event = 0

    #  Print header
    #  !!! Comment this line to reduce head line !!!
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
    #  Read high voltage line
    if(/Ianode/)
    {
        gsub(/=/, " ")
        #printf "\n"$0 "\n"

        #  Find number of the word "Ianode" in the line
        for(i = 1; i <= NF; i++)
        {
            if($i == "Ianode")
                icur = i + 1
            if($i == "Up")
                iup = i + 1
            if($i == "T")
                it = i + 1
        }
        Tmos = $it
        Ianode = $icur
        Up = $iup
        #printf "\n!!!!Up=>"Up"<-----Ia=>"Ianode"<-----Tmos=>"Tmos"<----\n"
    }

    #  ------------------------------------------------
    #  Read fifo_err
    if(/Fifo_err/)
    {
        fifo_err += 1
    }

    #  ------------------------------------------------
    #   Print info for previous event. Read new Event number.
    #
    if(/<K/)
    {
        #  print info for previous event
        if(event != 0)
        {
            printf event"   "Ianode"   "Tmos"   "Up"   "fifo_err"\n"
        }

        #  init fifo_err counter
        fifo_err = 0

        #  read new event number
        for(i=1; i<=NF; i++)
        {
            if( index($i,"<K"))
            {
                sub(/<K/," ")
                sub(/>/," ")

                #  if "<K" is not in the firts word 
                if(i != 1)
                {
                    iev=i+1
                    event = $iev
                }
                #  if "<K" is in the first word
                else
                    event = $1
            }
        }
    }
}
#  ----------------------------------------------------
#  ----------------------------------------------------
END {

}
