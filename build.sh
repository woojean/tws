#!/bin/bash

echo "start building"
os=`uname -s`

if [ $os = "Linux" ] ; then
  echo "Linux" 
  `gcc -o bin/tws -lrt src/tws.c src/base.c`
else 
  `gcc -o bin/tws src/tws.c src/base.c`
fi

