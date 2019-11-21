#!/bin/bash
fff=fifo_err
fff=High_voltage
fff=Parser
out=$fff.dat

[ -e $out ] && rm $out && echo "previous file $out deleted"
echo $0

for f in ./data.dbg/*
do
    echo $f 
#    [[ "$f" != flight* ]] && continue
    #echo $f >> $out
    #echo >> $out

    awk -f $fff.awk $f >> $out
    #echo >> $out
done
