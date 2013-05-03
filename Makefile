SERVER=server
CLIENT=client

HOST_PJ=../pjproject-i686
HOST_ARCH=i686-pc-linux-gnu
HOST_CC=gcc
HOST_LDFLAGS=-lpthread -ldl -L$(HOST_PJ)/pjlib/lib -L$(HOST_PJ)/pjlib-util/lib -L$(HOST_PJ)/pjnath/lib
HOST_LIBS=-lpjnath-$(HOST_ARCH) -lpjlib-util-$(HOST_ARCH) -lpj-$(HOST_ARCH) -lm -lnsl -lrt -lpthread -lopencv_core -lopencv_highgui
HOST_DEFINES=-DCLIENT_SIDE=1
HOST_CFLAGS=-g -Werror -Wall $(HOST_DEFINES) -I$(HOST_PJ)/pjlib/include -I$(HOST_PJ)/pjnath/include -I$(HOST_PJ)/pjlib-util/include

ARM_PJ=../pjproject-arm
ARM_ARCH=arm-unknown-linux-gnu
ARM_CC=arm-linux-gnueabihf-gcc
ARM_LDFLAGS=-lpthread -ldl -lstdc++ -L$(ARM_PJ)/pjlib/lib -L$(ARM_PJ)/pjlib-util/lib -L$(ARM_PJ)/pjnath/lib -L../OpenCV-2.3.1/raspberry/lib -L../wiringPi/wiringPi
ARM_LIBS=-lpjnath-$(ARM_ARCH) -lpjlib-util-$(ARM_ARCH) -lpj-$(ARM_ARCH) -lm -lnsl -lrt -lpthread -lopencv_core -lopencv_highgui -lwiringPi
ARM_DEFINES=-DPJ_IS_BIG_ENDIAN=0 -DPJ_IS_LITTLE_ENDIAN=1
ARM_CFLAGS=-g -Wall $(ARM_DEFINES) -I$(ARM_PJ)/pjlib/include -I$(ARM_PJ)/pjnath/include -I$(ARM_PJ)/pjlib-util/include -I../OpenCV-2.3.1/modules/core/include -I../OpenCV-2.3.1/modules/highgui/include -I../wiringPi/wiringPi

all:
	echo ***ARM***
	$(ARM_CC) http.c -c -o $(SERVER)/http.o $(ARM_CFLAGS)
	$(ARM_CC) connection.c -c -o $(SERVER)/connection.o $(ARM_CFLAGS)
	$(ARM_CC) $(SERVER)/camera.c -c -o $(SERVER)/camera.o $(ARM_CFLAGS)
	$(ARM_CC) $(SERVER)/gpio.c -c -o $(SERVER)/gpio.o $(ARM_CFLAGS)
	$(ARM_CC) $(SERVER)/main.c -c -o $(SERVER)/main.o $(ARM_CFLAGS)
	$(ARM_CC) $(SERVER)/http.o $(SERVER)/connection.o $(SERVER)/camera.o $(SERVER)/gpio.o $(SERVER)/main.o -o $(SERVER)/$(SERVER) $(ARM_LDFLAGS) $(ARM_LIBS)
	echo ***HOST***
	$(HOST_CC) http.c -c -o $(CLIENT)/http.o $(HOST_CFLAGS)
	$(HOST_CC) connection.c -c -o $(CLIENT)/connection.o $(HOST_CFLAGS)
	$(HOST_CC) $(CLIENT)/control.c -c -o $(CLIENT)/control.o $(HOST_CFLAGS)
	$(HOST_CC) $(CLIENT)/display.c -c -o $(CLIENT)/display.o $(HOST_CFLAGS)
	$(HOST_CC) $(CLIENT)/main.c -c -o $(CLIENT)/main.o $(HOST_CFLAGS)
	$(HOST_CC) $(CLIENT)/http.o $(CLIENT)/connection.o $(CLIENT)/control.o $(CLIENT)/display.o $(CLIENT)/main.o -o $(CLIENT)/$(CLIENT) $(HOST_LDFLAGS) $(HOST_LIBS)

clean:
	rm -f $(SERVER)/http.o
	rm -f $(CLIENT)/http.o
	rm -f $(SERVER)/connection.o
	rm -f $(CLIENT)/connection.o
	rm -f $(SERVER)/camera.o
	rm -f $(SERVER)/gpio.o
	rm -f $(SERVER)/main.o
	rm -f $(CLIENT)/control.o
	rm -f $(CLIENT)/display.o
	rm -f $(CLIENT)/main.o
	rm -f $(SERVER)/$(SERVER)
	rm -f $(CLIENT)/$(CLIENT)
	