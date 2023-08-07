#!/bin/sh

#Recived 

if [ $# -ne 2 ]; 
then
	echo "1.0 error"
	exit 1
fi 


if [ -d $1 ]
then
	var=$(ls $1 | wc -l )
	#grep -rno "$1" -e "$2"
	var2=$(grep -rn "$1" -e "$2" | wc -l)
	if [ $var2 -eq 0 ];
	then
		echo "Error 1"
	else 
		echo  "The number of files are $var and the number of matching lines are $var2"
	fi
	
else
	exit 1
fi





