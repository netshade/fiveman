all: main.o fiveman_instruction.o fiveman_process_state.o fiveman_process_state_table.o ncurses_screen.o options.o procfile.o signal_handlers.o
	gcc -lncurses -o fiveman build/*

clean:
	rm build/*.o
	rm fiveman

main.o: src/main.c
	gcc -c -I src/ -o build/main.o src/main.c

fiveman_instruction.o: src/fiveman_instruction.c
	gcc -c -I src/ -o build/fiveman_instruction.o src/fiveman_instruction.c

fiveman_process_state.o: src/fiveman_process_state.c
	gcc -c -I src/ -o build/fiveman_process_state.o src/fiveman_process_state.c

fiveman_process_state_table.o: src/fiveman_process_state_table.c
	gcc -c -I src/ -o build/fiveman_process_state_table.o src/fiveman_process_state_table.c

ncurses_screen.o: src/ncurses_screen.c
	gcc -c -I src/ -o build/ncurses_screen.o src/ncurses_screen.c

options.o: src/options.c
	gcc -c -I src/ -o build/options.o src/options.c

procfile.o: src/procfile.c
	gcc -c -I src/ -o build/procfile.o src/procfile.c

signal_handlers.o: src/signal_handlers.c
	gcc -c -I src/ -o build/signal_handlers.o src/signal_handlers.c
