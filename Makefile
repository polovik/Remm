SERVER=server

ARM_ARCH=arm-unknown-linux-gnu
ARM_CC=arm-linux-gnueabihf-gcc
ARM_LDFLAGS=-lpthread -ldl -lstdc++ -L../OpenCV-2.3.1/raspberry/lib -L../wiringPi/wiringPi -L../nmealib/lib
ARM_LIBS=-lm -lnsl -lrt -lpthread -lopencv_core -lopencv_highgui -lwiringPi -lnmea
ARM_DEFINES=
ARM_CFLAGS=-g -Wall $(ARM_DEFINES) -I../OpenCV-2.3.1/modules/core/include -I../OpenCV-2.3.1/modules/highgui/include -I../wiringPi/wiringPi -I../nmealib/include

all:
	echo ***ARM***
	$(ARM_CC) $(SERVER)/utils.c -c -o $(SERVER)/utils.o $(ARM_CFLAGS)
	$(ARM_CC) $(SERVER)/camera.c -c -o $(SERVER)/camera.o $(ARM_CFLAGS)
	$(ARM_CC) $(SERVER)/gpio.c -c -o $(SERVER)/gpio.o $(ARM_CFLAGS)
	$(ARM_CC) $(SERVER)/gps.c -c -o $(SERVER)/gps.o $(ARM_CFLAGS)
	$(ARM_CC) $(SERVER)/main.c -c -o $(SERVER)/main.o $(ARM_CFLAGS)
	$(ARM_CC) $(SERVER)/utils.o $(SERVER)/camera.o $(SERVER)/gpio.o $(SERVER)/gps.o $(SERVER)/main.o -o $(SERVER)/$(SERVER) $(ARM_LDFLAGS) $(ARM_LIBS)

clean:
	rm -f $(SERVER)/utils.o
	rm -f $(SERVER)/camera.o
	rm -f $(SERVER)/gpio.o
	rm -f $(SERVER)/gps.o
	rm -f $(SERVER)/main.o
	rm -f $(SERVER)/$(SERVER)
	