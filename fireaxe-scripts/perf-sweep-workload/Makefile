RISCV_PFX=riscv64-unknown-elf-
RISCV_GCC=$(RISCV_PFX)g++
RISCV_OBJDUMP=$(RISCV_PFX)objdump

OBJECT_OPT=-fno-common -fno-builtin-printf -specs=htif_nano.specs
BINARY_OPT=-static -specs=htif_nano.specs
OBJECT_DUMP_OPT=--disassemble-all

TARGET=run-cycles
TARGET_RISCV=$(TARGET).riscv
TARGET_OBJDUMP=$(TARGET).riscv.dump

OUTPUT_DIR=run-cycles

all: $(TARGET_RISCV) $(TARGET_OBJDUMP)

$(TARGET_RISCV): run-cycles.c
	$(RISCV_GCC) $(BINARY_OPT) -o $@ $^
	mkdir -p $(OUTPUT_DIR)
	mv $(TARGET_RISCV) $(OUTPUT_DIR)

$(TARGET_OBJDUMP): $(TARGET_RISCV)
	$(RISCV_OBJDUMP) $(OBJECT_DUMP_OPT) $(OUTPUT_DIR)/$< > $@
	mkdir -p $(OUTPUT_DIR)
	mv $(TARGET_OBJDUMP) $(OUTPUT_DIR)

.PHONY: clean
clean:
	rm -rf $(OUTPUT_DIR) *.o
