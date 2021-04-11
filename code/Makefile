CC = gcc
CFLAGS = -g
HEADER = mftp.h

all: mftp mftpserve

mftp: mftp.c commonfunc.c $(HEADER)
	gcc -o mftp mftp.c commonfunc.c

mftpserve: mftpserve.c commonfunc.c $(HEADER)
	gcc -o mftpserve mftpserve.c commonfunc.c

-PHONY: clean
clean:
	rm -rf mftpserve.o mftp.o
