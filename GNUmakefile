CC=arm-unknown-riscos-gcc
LD=$(CC)
AS=asasm
CFLAGS=-mlibscl -O3 -Wall -Wextra -mno-poke-function-name -fomit-frame-pointer
LDFLAGS=-mlibscl

EXE=!PeanutGB/!RunImage,ff8
OBJS=main.o emu.o copyasm.o

$(EXE): $(OBJS)
	$(LD) $(LDFLAGS) -o $@ $^

clean:
	$(RM) $(EXE) $(OBJS)
