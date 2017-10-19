OBJECTS = voc_detect.o DieWithError.o
CC = gcc
CFLAGS = -g -c
TARGET = voc_detect

$(TARGET) : $(OBJECTS)
	$(CC) -o $(TARGET) $(OBJECTS) -lwiringPi -lm

clean :
	rm -rf $(OBJECTS) $(TARGET) 
