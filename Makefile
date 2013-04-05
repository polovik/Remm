SERVER=server
CLIENT=client

HOST_PJ=../pjproject-i686
HOST_ARCH=i686-pc-linux-gnu
HOST_CC=gcc
HOST_LDFLAGS=-lpthread -ldl -L$(HOST_PJ)/pjlib/lib -L$(HOST_PJ)/pjlib-util/lib -L$(HOST_PJ)/pjnath/lib
HOST_LIBS=-lpjnath-$(HOST_ARCH) -lpjlib-util-$(HOST_ARCH) -lpj-$(HOST_ARCH) -lm -lnsl -lrt -lpthread
HOST_DEFINES=-DCLIENT=1
HOST_CFLAGS=-g $(HOST_DEFINES) -I$(HOST_PJ)/pjlib/include -I$(HOST_PJ)/pjnath/include -I$(HOST_PJ)/pjlib-util/include

ARM_PJ=../pjproject-arm
ARM_ARCH=arm-unknown-linux-gnu
ARM_CC=arm-unknown-linux-gnueabi-gcc
ARM_LDFLAGS=-lpthread -ldl -L$(ARM_PJ)/pjlib/lib -L$(ARM_PJ)/pjlib-util/lib -L$(ARM_PJ)/pjnath/lib
ARM_LIBS=-lpjnath-$(ARM_ARCH) -lpjlib-util-$(ARM_ARCH) -lpj-$(ARM_ARCH) -lm -lnsl -lrt -lpthread
ARM_DEFINES=-DSERVER=1 -DPJ_IS_BIG_ENDIAN=0 -DPJ_IS_LITTLE_ENDIAN=1
ARM_CFLAGS=-g $(ARM_DEFINES) -I$(ARM_PJ)/pjlib/include -I$(ARM_PJ)/pjnath/include -I$(ARM_PJ)/pjlib-util/include

all:
	echo ***ARM***
	$(ARM_CC) connection.c -c -o $(SERVER)/connection.o $(ARM_CFLAGS)
	$(ARM_CC) $(SERVER)/main.c -c -o $(SERVER)/main.o $(ARM_CFLAGS)
	$(ARM_CC) $(SERVER)/connection.o $(SERVER)/main.o -o $(SERVER)/$(SERVER) $(ARM_LDFLAGS) $(ARM_LIBS)
	echo ***HOST***
	$(HOST_CC) connection.c -c -o $(CLIENT)/connection.o $(HOST_CFLAGS)
	$(HOST_CC) $(CLIENT)/main.c -c -o $(CLIENT)/main.o $(HOST_CFLAGS)
	$(HOST_CC) $(CLIENT)/connection.o $(CLIENT)/main.o -o $(CLIENT)/$(CLIENT) $(HOST_LDFLAGS) $(HOST_LIBS)

clean:
	rm -f $(SERVER)/connection.o
	rm -f $(CLIENT)/connection.o
	rm -f $(SERVER)/main.o
	rm -f $(CLIENT)/main.o
	rm -f $(SERVER)/$(SERVER)
	rm -f $(CLIENT)/$(CLIENT)
	