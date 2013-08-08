SERVER=server

ARM_ARCH=arm-unknown-linux-gnu
ARM_CC=arm-linux-gnueabihf-g++
ARM_LDFLAGS=-lpthread -ldl -lstdc++ -L../wiringPi/wiringPi -L../nmealib/lib -L../jpeg-9/.libs -L../v4l-utils-0.9.5/lib/libv4l2/.libs -L../v4l-utils-0.9.5/lib/libv4lconvert/.libs
ARM_LIBS=-lm -lnsl -lrt -lpthread -lwiringPi -lnmea -ljpeg -lv4l2 -lv4lconvert
ARM_DEFINES=
ARM_CFLAGS=-g -Wall $(ARM_DEFINES) -I../jpeg-9/ -I../v4l-utils-0.9.5/lib/include -I../wiringPi/wiringPi -I../nmealib/include

all:
	echo ***ARM***
	$(ARM_CC) $(SERVER)/utils.c -c -o $(SERVER)/utils.o $(ARM_CFLAGS)
	$(ARM_CC) $(SERVER)/camera.c -c -o $(SERVER)/camera.o $(ARM_CFLAGS)
	$(ARM_CC) $(SERVER)/gpio.c -c -o $(SERVER)/gpio.o $(ARM_CFLAGS)
	$(ARM_CC) $(SERVER)/gps.c -c -o $(SERVER)/gps.o $(ARM_CFLAGS)
	$(ARM_CC) $(SERVER)/bmp085.c -c -o $(SERVER)/bmp085.o $(ARM_CFLAGS)
	$(ARM_CC) $(SERVER)/main.c -c -o $(SERVER)/main.o $(ARM_CFLAGS)
	$(ARM_CC) $(SERVER)/utils.o $(SERVER)/camera.o $(SERVER)/gpio.o $(SERVER)/gps.o $(SERVER)/bmp085.o $(SERVER)/main.o -o $(SERVER)/$(SERVER) $(ARM_LDFLAGS) $(ARM_LIBS)

clean:
	rm -f $(SERVER)/utils.o
	rm -f $(SERVER)/camera.o
	rm -f $(SERVER)/gpio.o
	rm -f $(SERVER)/gps.o
	rm -f $(SERVER)/bmp085.o
	rm -f $(SERVER)/main.o
	rm -f $(SERVER)/$(SERVER)
	