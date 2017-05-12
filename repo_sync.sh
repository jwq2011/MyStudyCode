#!/bin/bash 
echo "================start repo sync==============="  
../bin/repo sync -f -j10  
while [ $? == 1 ]; do  
echo "================sync failed, re-sync again============="  
sleep 3  
../bin/repo sync -f -j10  
done
