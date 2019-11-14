#!/bin/bash
fff=trans2019
er=$fff.out
out=trans.config

echo  $0 "is working!" 
# delete old trans.out
[ -e $er ]  && rm $er && echo "old file $er deleted"
# delete old "trans.config"
[ -e $out ] && rm $out && echo "old file $out deleted"

date
for f in *
do
#     [ -d "$f"  ] && continue
    [[ "$f" != s* ]] && continue
    [[ "$f" == *dbg ]] && continue
    [[ "$f" == *spec ]] && continue
    [[ "$f" == syn* ]] && continue
    [[ "$f" == $0 ]] && continue

    echo 
    echo $f        | tee -a $er
    ./$fff $f | tee -a $er
    echo >> $er    | tee -a $er

done
echo
date