#!/bin/sh
echo "   REMOVE OLD EXECUTABLES   "
echo "----------------------------"
rm *.exe
echo "____________________________"
echo "  CLEAN AND COMPILE SENDER  "
echo "----------------------------"
cd  sender
make clean
make
echo "____________________________"
echo " CLEAN AND COMPILE LISTENER "
echo "----------------------------"
cd ../listener
make clean
make
echo "____________________________"
echo "       RE-CREATE FIFO       "
echo "----------------------------"
cd ..
rm fifo
mkfifo fifo

