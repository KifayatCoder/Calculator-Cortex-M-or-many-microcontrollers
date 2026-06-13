# ================================================================
# Makefile  —  Calculator 1.0  (universal MPS2 QEMU build)
#
# Supported boards (pass BOARD=<name>):
#   an385   ARM Cortex-M3          (no FPU,  soft-float)
#   an386   ARM Cortex-M4 + FPv4   (hard-float single)  ← default
#   an500   ARM Cortex-M7 + FPv5   (hard-float double)
#   an505   ARM Cortex-M33 + FPv5  (hard-float single, TrustZone)
#   an511   ARM Cortex-M3 DesignStart (no FPU, soft-float)
#
# Usage examples:
#   make                         Build for an386 (default)
#   make BOARD=an385             Build for Cortex-M3
#   make BOARD=an500 qemu        Build + run in QEMU mps2-an500
#   make BOARD=an505 qemu-gdb    Build + start QEMU with GDB server
#   make all-boards              Build ELFs for every board at once
#   make clean                   Remove build directory
# ================================================================

# ---- Default board (override on command line) ---------------
BOARD ?= an386

# ================================================================
# Board-specific CPU flags, preprocessor define, QEMU machine
# ================================================================

ifeq ($(BOARD), an385)
  CPU_FLAGS  := -mcpu=cortex-m3 -mthumb -mfloat-abi=soft
  BOARD_DEF  := -DBOARD_AN385
  QEMU_MACH  := mps2-an385
  BOARD_LABEL:= AN385 (Cortex-M3)

else ifeq ($(BOARD), an386)
  CPU_FLAGS  := -mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard
  BOARD_DEF  := -DBOARD_AN386
  QEMU_MACH  := mps2-an386
  BOARD_LABEL:= AN386 (Cortex-M4 + FPv4-SP)

else ifeq ($(BOARD), an500)
  CPU_FLAGS  := -mcpu=cortex-m7 -mthumb -mfpu=fpv5-d16 -mfloat-abi=hard
  BOARD_DEF  := -DBOARD_AN500
  QEMU_MACH  := mps2-an500
  BOARD_LABEL:= AN500 (Cortex-M7 + FPv5-D16)

else ifeq ($(BOARD), an505)
  CPU_FLAGS  := -mcpu=cortex-m33 -mthumb -mfpu=fpv5-sp-d16 -mfloat-abi=hard
  BOARD_DEF  := -DBOARD_AN505
  QEMU_MACH  := mps2-an505
  BOARD_LABEL:= AN505 (Cortex-M33 + FPv5-SP)

else ifeq ($(BOARD), an511)
  CPU_FLAGS  := -mcpu=cortex-m3 -mthumb -mfloat-abi=soft
  BOARD_DEF  := -DBOARD_AN511
  QEMU_MACH  := mps2-an511
  BOARD_LABEL:= AN511 (Cortex-M3 DesignStart)

else
  $(error Unknown BOARD "$(BOARD)". Choose: an385 an386 an500 an505 an511)
endif

# ================================================================
# Toolchain
# ================================================================
CC      := arm-none-eabi-gcc
LD      := arm-none-eabi-gcc
OBJCOPY := arm-none-eabi-objcopy
OBJDUMP := arm-none-eabi-objdump
SIZE    := arm-none-eabi-size
GDB     := arm-none-eabi-gdb

# ================================================================
# Paths
# ================================================================
TARGET  := calculator_$(BOARD)
SRC_DIR := src
BUILD   := build/$(BOARD)
LDSCRIPT:= mps2.ld

# ================================================================
# Compilation flags
# ================================================================
CFLAGS  := $(CPU_FLAGS)               \
            $(BOARD_DEF)              \
            -std=c11                  \
            -O2                       \
            -Wall -Wextra             \
            -Wno-unused-parameter     \
            -ffunction-sections       \
            -fdata-sections           \
            -ffreestanding            \
            -fno-exceptions           \
            -fno-asynchronous-unwind-tables \
            -I$(SRC_DIR)

# Assembler: use GCC as the driver so the C preprocessor runs
# (-x assembler-with-cpp lets startup.s use #include / #if).
ASFLAGS := $(CPU_FLAGS) $(BOARD_DEF) -x assembler-with-cpp -I$(SRC_DIR)

LDFLAGS := $(CPU_FLAGS)               \
            -T $(LDSCRIPT)            \
            -Wl,--gc-sections         \
            -Wl,-Map=$(BUILD)/$(TARGET).map,--cref \
            -Wl,--print-memory-usage  \
            -nostartfiles             \
            -specs=nano.specs         \
            -u _printf_float          \
            -u _scanf_float           \
            -lm

# ================================================================
# Sources and objects
# ================================================================
C_SRCS   := $(wildcard $(SRC_DIR)/*.c)
ASM_SRCS := $(wildcard $(SRC_DIR)/*.s)

C_OBJS   := $(patsubst $(SRC_DIR)/%.c,  $(BUILD)/%.o, $(C_SRCS))
ASM_OBJS := $(patsubst $(SRC_DIR)/%.s,  $(BUILD)/%.o, $(ASM_SRCS))
OBJS     := $(ASM_OBJS) $(C_OBJS)

ELF := $(BUILD)/$(TARGET).elf
BIN := $(BUILD)/$(TARGET).bin
HEX := $(BUILD)/$(TARGET).hex

# ================================================================
# Build targets
# ================================================================
.PHONY: all qemu qemu-gdb debug disasm all-boards clean help-make

all: $(ELF) $(BIN) $(HEX)
	@echo ""
	@echo "  ── Board: $(BOARD_LABEL) ──────────────────────────"
	@$(SIZE) --format=berkeley $(ELF)
	@echo ""
	@echo "  ELF  → $(ELF)"
	@echo "  BIN  → $(BIN)"
	@echo "  Run  → make BOARD=$(BOARD) qemu"
	@echo ""

$(BUILD):
	mkdir -p $(BUILD)

$(BUILD)/%.o: $(SRC_DIR)/%.c | $(BUILD)
	@echo "  CC [$(BOARD)]  $<"
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/%.o: $(SRC_DIR)/%.s | $(BUILD)
	@echo "  AS [$(BOARD)]  $<"
	$(CC) $(ASFLAGS) -c $< -o $@

$(ELF): $(OBJS) $(LDSCRIPT)
	@echo "  LD [$(BOARD)]  $@"
	$(LD) $(OBJS) $(LDFLAGS) -o $@

$(BIN): $(ELF)
	$(OBJCOPY) -O binary $< $@

$(HEX): $(ELF)
	$(OBJCOPY) -O ihex   $< $@

# ================================================================
# QEMU targets
# ================================================================
QEMU     := qemu-system-arm
QEMU_FLAGS := -M $(QEMU_MACH) -kernel $(ELF) -nographic

qemu: $(ELF)
	@echo "  Starting QEMU  [$(QEMU_MACH)]  — Ctrl-A X to quit"
	$(QEMU) $(QEMU_FLAGS)

qemu-gdb: $(ELF)
	@echo "  QEMU + GDB server on :1234 (halted at reset)"
	$(QEMU) $(QEMU_FLAGS) -s -S

debug: $(ELF)
	$(GDB) -ex "target remote :1234"  \
	       -ex "monitor reset halt"   \
	       -ex "load"                 \
	       -ex "b main"               \
	       -ex "continue"             \
	       $(ELF)

# ================================================================
# Build ALL boards in one shot
# ================================================================
ALL_BOARDS := an385 an386 an500 an505 an511

all-boards:
	@echo "  ── Building all MPS2 boards ──────────────────────"
	@for b in $(ALL_BOARDS); do                           \
	    echo "";                                          \
	    echo "  ── BOARD=$$b ──────────────────────────────────"; \
	    $(MAKE) --no-print-directory BOARD=$$b all || exit 1; \
	done
	@echo ""
	@echo "  ── All boards built successfully ─────────────────"
	@echo ""
	@ls -lh build/*/calculator_*.elf 2>/dev/null | \
	    awk '{printf "  %-45s  %s\n", $$NF, $$5}'

# ================================================================
# Disassembly
# ================================================================
disasm: $(ELF)
	$(OBJDUMP) -d -S $(ELF) | less

# ================================================================
# Clean
# ================================================================
clean:
	rm -rf build/

# ================================================================
# Help
# ================================================================
help-make:
	@echo ""
	@echo "  make [BOARD=<b>]          Build for board <b>  (default: an386)"
	@echo "  make BOARD=<b> qemu       Build + run in QEMU"
	@echo "  make BOARD=<b> qemu-gdb   Build + QEMU paused, GDB on :1234"
	@echo "  make BOARD=<b> debug      Connect GDB (needs qemu-gdb running)"
	@echo "  make BOARD=<b> disasm     Disassemble ELF with source interleave"
	@echo "  make all-boards           Build every target in one pass"
	@echo "  make clean                Delete build/ directory"
	@echo ""
	@echo "  Boards: an385  an386  an500  an505  an511"
	@echo ""
