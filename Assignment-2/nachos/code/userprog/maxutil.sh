#!/bin/bash
START=11
END=124
for (( i=$START; i <= $END; ++i ))
do
   echo "i: $i" >> 1.txt
   var="3 $i"
   sed "1s/.*/$var/" ab.txt  > batch1
   
   eval "$(./nachos -F batch1 > temp.txt)"
   grep "CPU Utilization" temp.txt >> 1.txt



done




