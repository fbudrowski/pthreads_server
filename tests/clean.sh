#!/bin/bash


killall klient
killall serwer
rm -f mainPipe
rm -f resourceAcquiredPipe-*
rm -f workDonePipe-*
make
reset
