CC=gcc
COPT=-Wall -g

all: main.o nids.o hash.o stream.o http_parser.o output.o base64.o
	$(CC) $(COPT) -o http_capture main.o nids.o hash.o stream.o http_parser.o output.o base64.o -lnids -lpcap -lpthread -ljson -lcrypto -lrdkafka -lz -lrt

nids.o: nids.c nids.h
	$(CC) $(COPT) -c nids.c

nidsdummy.o: nidsdummy.c nids.h
	$(CC) $(COPT) -c nidsdummy.c

base64.o: base64.c base64.h
	$(CC) $(COPT) -c base64.c

hash.o: hash.c hash.h config.h
	$(CC) $(COPT) -c hash.c

stream.o: stream.c stream.h config.h
	$(CC) $(COPT) -c stream.c

http_parser.o: http_parser.c http_parser.h config.h
	$(CC) $(COPT) -c http_parser.c

output.o: output.c output.h
	$(CC) $(COPT) -c output.c

main.o: main.c config.h
	$(CC) $(COPT) -c main.c

clean:
	rm -f *.o *~ httpsniff
