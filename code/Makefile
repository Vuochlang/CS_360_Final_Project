CC = gcc
CFLAGS = -g
HEADER = mftp.h

all: mftp mftpserve

mftp: mftp.c $(HEADER)
	gcc -o mftp mftp.c

mftpserve: mftpserve.c $(HEADER)
	gcc -o mftpserve mftpserve.c

-PHONY: clean
clean:
	rm -rf mftpserve.o mftp.o
