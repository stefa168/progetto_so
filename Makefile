CC = cc
CCFLAGS = -O3 -Wall -pedantic-errors
COM_DEPENDENCIES = ipc_utils.o settings_reader.o utils.o

# $< = file delle dipendenze che stiamo generando
%.o: %.c %.h
	$(CC) $(CCFLAGS) -c $<

# $^ = Tutti i file che vengono compilati
# $@ = Nome del file che stiamo generando
%.r: %.c $(COM_DEPENDENCIES)
	$(CC) $(CCFLAGS) $^ -o $@

all: Gestore.r Studente.r

run: all

clean:
	rm -f *.o

reset:
	rm -f *.r

resetall:
	rm -f *.r
	rm -f opt.conf

ipc:
	-ipcrm -M 34197
	-ipcrm -Q 34197
	-ipcrm -S 34197

run:
	./Gestore.r