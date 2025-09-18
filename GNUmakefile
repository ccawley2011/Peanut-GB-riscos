CC=$(ARCHIESDK)/tools/bin/arm-archie-gcc
AS=$(CC)
LD=$(CC)
OBJCOPY=$(ARCHIESDK)/tools/bin/arm-archie-objcopy
CFLAGS=-O3 -Wall -Wextra -mno-poke-function-name -fomit-frame-pointer -mno-thumb-interwork
ASFLAGS= -x assembler-with-cpp -c
LDFLAGS=

EXE=!PeanutGB/!RunImage,ff8
ELF=PeanutGB,e1f
OBJS=main.o emu.o gui.o msgs.o copyasm.o

$(EXE): $(ELF)
	$(OBJCOPY) -O binary $< $@

$(ELF): $(OBJS)
	$(LD) $(LDFLAGS) -o $@ $^ -lOSLib32

benchmark: bench,ff8

bench,ff8: bench,e1f
	$(OBJCOPY) -O binary $< $@

bench,e1f: bench.o
	$(LD) $(LDFLAGS) -o $@ $^

clean:
	$(RM) $(EXE) $(ELF) $(OBJS)
	$(RM) bench,ff8 bench,e1f bench.o
