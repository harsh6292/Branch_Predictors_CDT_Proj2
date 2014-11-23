CC = g++
OPT = -O3 -m32
WARN = -w
CFLAGS = $(OPT) $(WARN)

# List all your .cc files here (source files, excluding header files)
SIM_SRC = sim.c

# rule for making sim
sim: $(SIM_OBJ)
	$(CC) -o sim $(CFLAGS) $(SIM_SRC)


# generic rule for converting any .c file to any .o file
 
.c.o:
	$(CC) $(CFLAGS)  -c $*.c


# type "make clean" to remove all .o files plus the sim_cache binary

clean:
	rm -f *.o sim


# type "make clobber" to remove all .o files (leaves sim_cache binary)

clobber:
	rm -f *.o
