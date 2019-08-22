.phony all:
all: trains

trains: trains.c
	gcc trains.c -o trains -lpthread

.PHONY clean:
clean:
	rm -rf *.o *.exe trains
