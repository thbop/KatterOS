BUILD_DIR?=build
ASM?=nasm
ASMFLAGS?=-f obj
CC16?=wcc
CFLAGS16?=-4 -d3 -s -wx -ms -zl -zq
LD16?=wlink

SOURCES_C=$(wildcard *.c)
SOURCES_ASM=$(wildcard *.asm)
OBJECTS_C=$(patsubst %.c, $(BUILD_DIR)/stage1/c/%.obj, $(SOURCES_C))
OBJECTS_ASM=$(patsubst %.asm, $(BUILD_DIR)/stage1/asm/%.obj, $(SOURCES_ASM))

.PHONY: all stage1 clean always

all: stage1

stage1: $(BUILD_DIR)/stage1.bin
$(BUILD_DIR)/stage1.bin: $(OBJECTS_ASM) $(OBJECTS_C)
	$(LD16) NAME $(BUILD_DIR)/stage1.bin FILE \{ $(OBJECTS_ASM) $(OBJECTS_C) \} OPTION MAP=$(BUILD_DIR)/stage1.map @linker.lnk

$(BUILD_DIR)/stage1/c/%.obj: %.c always
	export PATH="$PATH:/d/WATCOM/binnt64"
	$(CC16) $(CFLAGS16) -fo=$@ $<

$(BUILD_DIR)/stage1/asm/%.obj: %.asm always
	$(ASM) $(ASMFLAGS) -o $@ $<

always:
	mkdir -p $(BUILD_DIR)/stage1/c
	mkdir -p $(BUILD_DIR)/stage1/asm

clean:
	rm -f $(BUILD_DIR)/stage1.bin