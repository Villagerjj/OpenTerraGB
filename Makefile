.SUFFIXES:

ROM = bin/terra.gb
GBDK_HOME = C:/gbdk/bin/
LCC = $(GBDK_HOME)lcc

# 0x1B is MBC5 with RAM + Battery
MBC := 27
ROMSIZE := 4
SRAMSIZE := 16
VERSION := 0

INCDIRS  = src\ src\include\
WARNINGS = 

CFLAGS  = $(addprefix -I, $(INCDIRS)) $(addprefix -W, $(WARNINGS))

SRCS := $(wildcard src/*.c)
SRAM_SRCS := $(wildcard src/sram/*.c)
OBJS := $(patsubst src/%.c, obj/%.o, $(SRCS)) \
        $(patsubst src/%.c, obj/%.o, $(SRAM_SRCS))

# `all` (Default target): build the ROM
all: $(ROM)
.PHONY: all

# `clean`: Clean temp and bin files
clean:
	rm -rf bin obj dep res
	rm -f src/include/charmap.inc

.PHONY: clean

# `rebuild`: Build everything from scratch
# It's important to do these two in order if we're using more than one job
rebuild:
	$(MAKE) clean
	$(MAKE) all
.PHONY: rebuild

bin/%.gb: $(OBJS)
	@mkdir -p $(@D)
	$(LCC) -Wl-yt$(MBC) -Wl-m -Wl-yo$(ROMSIZE) -Wl-ya$(SRAMSIZE) -o $@ $^

# SRAM needs a special build step with different flags
$(patsubst src/%.c, obj/%.o, $(SRAM_SRCS)): $(SRAM_SRCS)
	@mkdir -p $(@D)
	$(LCC) $(CFLAGS) -Wa-l -Wf-ba0 -c -o obj/sram/save0.o src/sram/save0.c
	$(LCC) $(CFLAGS) -Wa-l -Wf-ba1 -c -o obj/sram/save1.o src/sram/save1.c
	$(LCC) $(CFLAGS) -Wa-l -Wf-ba2 -c -o obj/sram/save2.o src/sram/save2.c
	$(LCC) $(CFLAGS) -Wa-l -Wf-ba3 -c -o obj/sram/save3.o src/sram/save3.c
	$(LCC) $(CFLAGS) -Wa-l -Wf-ba4 -c -o obj/sram/save4.o src/sram/save4.c
	$(LCC) $(CFLAGS) -Wa-l -Wf-ba5 -c -o obj/sram/save5.o src/sram/save5.c
	$(LCC) $(CFLAGS) -Wa-l -Wf-ba6 -c -o obj/sram/save6.o src/sram/save6.c
	$(LCC) $(CFLAGS) -Wa-l -Wf-ba7 -c -o obj/sram/save7.o src/sram/save7.c
	$(LCC) $(CFLAGS) -Wa-l -Wf-ba8 -c -o obj/sram/save8.o src/sram/save8.c
	$(LCC) $(CFLAGS) -Wa-l -Wf-ba9 -c -o obj/sram/save9.o src/sram/save9.c
	$(LCC) $(CFLAGS) -Wa-l -Wf-ba10 -c -o obj/sram/saveA.o src/sram/saveA.c
	$(LCC) $(CFLAGS) -Wa-l -Wf-ba11 -c -o obj/sram/saveB.o src/sram/saveB.c
	$(LCC) $(CFLAGS) -Wa-l -Wf-ba12 -c -o obj/sram/saveC.o src/sram/saveC.c

obj/%.o: src/%.c
	@mkdir -p $(@D)
	$(LCC) $(CFLAGS) -Wa-l -c -o $@ $^
