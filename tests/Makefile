CC = gcc
CFLAGS = -O
INCLUDES = -I../../embunit/ -I./
LIBS_DIR = -L../../embunit/lib
LIBS = -lembUnit -lm
RM = rm
TARGET = tests
OBJS = main_tests.o wiringPiI2C.o bmp085.o bmp085_test.o

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) -o $@ $(OBJS) $(LIBS_DIR) $(LIBS)

bmp085.o:
	$(CC) $(CFLAGS) $(INCLUDES) -c ../server/bmp085.c

.c.o:
	$(CC) $(CFLAGS) $(INCLUDES) -c $<

clean:
	-$(RM) $(TARGET) $(OBJS)

.PHONY: clean all
