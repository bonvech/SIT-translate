#
# usage:
# awk -f thp.awk log.txt
# awk -f thp.awk log.txt | awk 'NF >= 2' > thp00

BEGIN {
    debug = 0
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
        if(debug)
            printf "\n>>>>"$0 "\n"

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
        if(debug)
            printf "\n!!!!Up=>"Up"<-----Ia=>"Ianode"<-----Tmos=>"Tmos"<----\n"
    }


    #  ------------------------------------------------
    # Read and calculate Up= Unegative + Upositive
    if(    (/Measure current:/)       \
        || (/read_vip_ADC:/)          \
        || (/Measure_high: current:/) \
        || (/Mosaic: measure_high:/)  \
    )
    {
        if(debug)
            printf "\n==>"$0 "<==\n"
        if(/current:/)
        {
            if(/current =/)
                gsub(/current =/, " ")

            #  Find number of the word "current:" in the line
            for(i = 1; i <= NF; i++)
                if($i == "current:")
                    iup = i + 1
            ch0 = $iup
            ch1 = $(iup + 1)
            ch2 = $(iup + 2)
            ch3 = $(iup + 3)
        }
        else if((/read_vip_ADC:/) || (/easure_high:/))
        {
            gsub(/\[/, " ")
            gsub(/\]/, " ")
            for(i = 1; i <= NF; i++)
                if($i == "CH0")
                    iup = i + 1
            ch0 = $iup
            ch1 = $(iup + 2)
            ch2 = $(iup + 4)
            ch3 = $(iup + 6)
        }
        if(debug)
            printf "CH:"ch0"  "ch1"  "ch2"  "ch3"\n"

        Upositive = ch1 / 2000.
        Unegative = 0.01514 * ch0 - 36.6
        Up  = -1 * (Unegative - Upositive)
        I   =  0.005012 * (ch2-ch1)
        Tmos = (ch3/2.0 - 500.0) * 0.1
        if(debug)
            printf "VA: I"Ianode"  Tmos"Tmos"  Up"Up" \n"
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
            Up  = -1 * (Unegative - Upositive)
            str_out = sprintf("%d\t%5.3f\t%6.2f\t%5.2f\t%2d", \
                            event, Ianode, Tmos, Up, fifo_err)
            print(str_out)
            #printf event"   "Ianode"   "Tmos"   "Up"   "fifo_err"\n"
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
