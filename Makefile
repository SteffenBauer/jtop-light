jltop: main.c cpu.c mem.c gpu.c fan.c power.c temp.c jltop.h
	gcc main.c cpu.c mem.c gpu.c fan.c power.c temp.c -O3 -lncurses -o jltop

clean:
	rm jltop
