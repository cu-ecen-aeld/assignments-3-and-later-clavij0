#write.c make
#rm -f $(TARGET) $(SRC)


#CROSS_COMPILE = aarch64-none-linux-gnu-
CROSS_COMPILE = arch64-buildroot-linux-gnu- 
CC = gcc
TARGET := writer

all: writer.c
ifeq ($(CROSS_COMPILE), arch64-buildroot-linux-gnu-)
	echo "foo equals ok"
	$(CROSS_COMPILE)$(CC) writer.c -o $(TARGET) 
else
	$(info YES -> CC)
	$(CC) writer.c -o $(TARGET) 

endif
#write: writer.c
#	$(CC) writer.c -o $(TARGET) 

clean:
	rm -f $(TARGET) *.o
