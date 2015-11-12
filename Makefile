CC=gcc
CFLAGS=-c -Wall
LDFLAGS=-lncurses
OBJDIR=build

debug: CFLAGS += -g
debug: LDFLAGS += -g
debug: fiveman

fiveman: build/main.o build/fiveman_instruction.o build/fiveman_process_state.o build/fiveman_process_state_table.o build/ncurses_screen.o build/options.o build/procfile.o build/signal_handlers.o
	$(CC) $(LDFLAGS) -o fiveman $(OBJDIR)/*.o

clean:
	rm $(OBJDIR)/*.o
	rm fiveman

build/main.o: src/main.c
	$(CC) $(CFLAGS) -I src/ -o build/main.o src/main.c

build/fiveman_instruction.o: src/fiveman_instruction.c
	$(CC) $(CFLAGS) -I src/ -o build/fiveman_instruction.o src/fiveman_instruction.c

build/fiveman_process_state.o: src/fiveman_process_state.c
	$(CC) $(CFLAGS) -I src/ -o build/fiveman_process_state.o src/fiveman_process_state.c

build/fiveman_process_state_table.o: src/fiveman_process_state_table.c
	$(CC) $(CFLAGS) -I src/ -o build/fiveman_process_state_table.o src/fiveman_process_state_table.c

build/ncurses_screen.o: src/ncurses_screen.c
	$(CC) $(CFLAGS) -I src/ -o build/ncurses_screen.o src/ncurses_screen.c

build/options.o: src/options.c
	$(CC) $(CFLAGS) -I src/ -o build/options.o src/options.c

build/procfile.o: src/procfile.c
	$(CC) $(CFLAGS) -I src/ -o build/procfile.o src/procfile.c

build/signal_handlers.o: src/signal_handlers.c
	$(CC) $(CFLAGS) -I src/ -o build/signal_handlers.o src/signal_handlers.c
