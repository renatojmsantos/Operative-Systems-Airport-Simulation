#!/bin/bash

echo "ARRIVAL TP022 init: 8 eta: 10 fuel: 150" >> input_pipe
sleep .1
echo "ARRIVAL TP333 init: 5 eta: 10 fuel: 30" >> input_pipe
sleep .1
echo "ARRIVAL TP444 init: 7 eta: 10 fuel: 150" >> input_pipe
sleep .1
echo "DEPARTURE TP30 init: 5 takeoff: 13" >> input_pipe
sleep .1
echo "DEPARTURE TP100 init: 6 takeoff: 15" >> input_pipe
sleep .1
echo "DEPARTURE TP555 init: 7 takeoff: 17" >> input_pipe
sleep .1
#echo "ARRIVAL TP555 init: 8 eta: 20 fuel: 50" >> input_pipe
#sleep .1
#echo "ARRIVAL TP666 init: 9 eta: 20 fuel: 20" >> input_pipe
#sleep .1
#echo "ARRIVAL TP666 init: 11 eta: 20 fuel: 30" >> input_pipe
#sleep .1