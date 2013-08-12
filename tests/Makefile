CC = gcc
CFLAGS = -O
INCLUDES = -I../../embunit/ -I./
LIBS_DIR = -L../../embunit/lib
LIBS = -lembUnit -lm
RM = rm
TARGET = tests
OBJS = main_tests.o wiringPi.o wiringPiI2C.o bmp085.o bmp085_test.o hmc5883l.o hmc5883l_test.o

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) -o $@ $(OBJS) $(LIBS_DIR) $(LIBS)

bmp085.o:
	$(CC) $(CFLAGS) $(INCLUDES) -c ../server/bmp085.c

hmc5883l.o:
	$(CC) $(CFLAGS) $(INCLUDES) -c ../server/hmc5883l.c

.c.o:
	$(CC) $(CFLAGS) $(INCLUDES) -c $<

clean:
	-$(RM) $(TARGET) $(OBJS)

.PHONY: clean all
