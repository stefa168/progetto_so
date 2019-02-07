CC = gcc
CCFLAGS = -O3 -Wall -std=c90
COM_DEPENDENCIES = ipc_utils.o settings_reader.o utils.o

%.o: %.c %.h
	$(CC) $(CCFLAGS) -c $<

%.r: %.c $(COM_DEPENDENCIES)
	$(CC) $(CCFLAGS) $^ -o $@

all: Gestore.r Studente.r

run: all

clean:
	rm -f *.o

ipc:
	ipcrm -M 34197
	ipcrm -Q 34197
	ipcrm -S 34197