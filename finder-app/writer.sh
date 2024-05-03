#!/bin/sh

if [ $# -ne 2 ]; 
then
	echo "1.2 error"
	exit 1
fi 

if [ -f $1 ];
then
    echo $2 > $1
elif  [ ! -f $1 ]; 
then
    mkdir -p "$(dirname "$1")";
    touch "$1";
    echo $2 > $1;

else
    exit 1
fi
