CC=gcc
CFLAGS=-c -Wall
LDFLAGS=-lncurses -ldtrace -lpthread
OBJDIR=build
PREFIX=/usr/local
BINDIR=$(PREFIX)/bin
INSTALL=install
CHMOD=chmod
CHOWN=chown
ROOT=root
SUDO=sudo
TEST=test

fiveman: clean_binary build/main.o build/fiveman_instruction.o build/fiveman_process_state.o build/fiveman_process_state_table.o build/ncurses_screen.o build/options.o build/procfile.o build/signal_handlers.o build/fiveman_process_statistics_mac.o build/fiveman_status_thread.o build/fiveman_pager_fork.o
	$(CC) $(LDFLAGS) -o fiveman $(OBJDIR)/*.o
	$(CHMOD) 4750 ./fiveman
	$(SUDO) $(CHOWN) $(ROOT) fiveman


debug: CFLAGS += -g
debug: LDFLAGS += -g
debug: fiveman

install: fiveman
	$(INSTALL) fiveman $(BINDIR)/fiveman

clean_binary:
	$(TEST) -f fiveman && $(SUDO) rm fiveman || sh -c "exit 0"

clean_objects:
	rm $(OBJDIR)/*.o

clean: clean_binary clean_objects

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

build/fiveman_process_statistics_mac.o: src/fiveman_process_statistics_mac.c
	$(CC) $(CFLAGS) -I src/ -o build/fiveman_process_statistics_mac.o src/fiveman_process_statistics_mac.c

build/fiveman_status_thread.o: src/fiveman_status_thread.c
	$(CC) $(CFLAGS) -I src/ -o build/fiveman_status_thread.o src/fiveman_status_thread.c

build/fiveman_pager_fork.o: src/fiveman_pager_fork.c
	$(CC) $(CFLAGS) -I src/ -o build/fiveman_pager_fork.o src/fiveman_pager_fork.c
