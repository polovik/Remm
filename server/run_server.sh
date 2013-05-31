#!/bin/sh

echo "28" > /sys/class/gpio/export
echo "out" > /sys/class/gpio/gpio28/direction

echo "30" > /sys/class/gpio/export
echo "out" > /sys/class/gpio/gpio30/direction

echo "31" > /sys/class/gpio/export
echo "out" > /sys/class/gpio/gpio31/direction

echo "1" > /sys/class/gpio/gpio28/value
echo "1" > /sys/class/gpio/gpio30/value
echo "1" > /sys/class/gpio/gpio31/value

sleep 1
echo RED
echo "0" > /sys/class/gpio/gpio28/value

sleep 1
echo BLUE
echo "1" > /sys/class/gpio/gpio28/value
echo "0" > /sys/class/gpio/gpio30/value

sleep 1
echo GREEN
echo "1" > /sys/class/gpio/gpio30/value
echo "0" > /sys/class/gpio/gpio31/value

sleep 1
echo "1" > /sys/class/gpio/gpio28/value
echo "1" > /sys/class/gpio/gpio30/value
echo "1" > /sys/class/gpio/gpio31/value

echo "28" > /sys/class/gpio/unexport
echo "30" > /sys/class/gpio/unexport
echo "31" > /sys/class/gpio/unexport


echo "29" > /sys/class/gpio/export
echo "in" > /sys/class/gpio/gpio29/direction

while [ 1 ]
do
	button_pressed=`cat /sys/class/gpio/gpio29/value`
	if [ $button_pressed = "1" ]
	then
		echo Button has pressed
		stty -F /dev/ttyAMA0 9600 cs8 -icanon
		./server
	fi
	sleep 1
done

echo "29" > /sys/class/gpio/unexport
