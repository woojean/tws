#!/bin/bash

echo "building..."
os=`uname -s`

if [ $os = "Linux" ] ; then
	echo "."
  `gcc -o bin/tws -lrt src/tws.c src/base.c`
else 
	echo "."
  `gcc -o bin/tws src/tws.c src/base.c`
fi
echo "finished !"
