# =============================================================================
#  KarnixOS – Bare-Metal Kernel  Makefile
#  Targets: make build   – compile kernel ELF
#           make iso     – create bootable ISO (needs grub + xorriso)
#           make run     – boot in QEMU  (needs qemu-system-i386)
#           make run-iso – boot ISO in QEMU
#           make clean   – remove artefacts
#
#  macOS setup (Apple Silicon or Intel):
#    brew install i686-elf-gcc i686-elf-binutils
#    brew install nasm
#    brew install qemu
#    brew install xorriso        (for ISO creation)
#    brew install grub           (may need: grub2-install)
#
#  Then:  cd os/karnix_kernel && make run
# =============================================================================

# ── Cross-compiler detection ─────────────────────────────────────────────────
CC_CROSS := $(shell which i686-elf-gcc 2>/dev/null)
LD_CROSS := $(shell which i686-elf-ld  2>/dev/null)
OC_CROSS := $(shell which i686-elf-objcopy 2>/dev/null)

ifneq ($(CC_CROSS),)
    CC      = i686-elf-gcc
    LD      = i686-elf-ld
    OBJCOPY = i686-elf-objcopy
else
    # Try llvm clang cross target (available on macOS without brew formula)
    CC      = clang -target i386-pc-none-elf
    LD      = clang -target i386-pc-none-elf
    OBJCOPY = llvm-objcopy
    $(warning "i686-elf-gcc not found – trying clang i386 cross target")
endif

NASM    = nasm
QEMU    = qemu-system-i386

# ── Flags ───────────────────────────────────────────────────────────────────
CFLAGS  = -std=gnu99 -ffreestanding -O2 -Wall -Wextra           \
          -Wno-unused-parameter                                 \
          -fno-stack-protector -fno-builtin -fno-pie            \
          -m32

LDFLAGS = -T linker.ld -m elf_i386 --oformat elf32-i386

NASMFLAGS = -f elf32

# Auto-detect macOS (use cocoa display) vs Linux (use gtk/sdl)
UNAME := $(shell uname)
ifeq ($(UNAME), Darwin)
    QDISPLAY = -display cocoa
else
    QDISPLAY = -display gtk
endif

# QEMU flags
# NOTE: We do NOT add -device usb-tablet because that bypasses PS/2 mouse
#       (IRQ12). The default QEMU PS/2 mouse is emulated and works with our
#       kernel's IRQ12 handler. Mouse is grabbed into the QEMU window on click;
#       press Ctrl+Alt+G to release.
QFLAGS  = -m 32M        \
          -vga std       \
          -no-reboot     \
          -no-shutdown   \
          $(QDISPLAY)

# ── Sources ──────────────────────────────────────────────────────────────────
ASM_SRCS = boot.asm
C_SRCS   = kernel.c

ASM_OBJ  = boot.o
C_OBJ    = kernel.o

ELF      = karnix.elf
ISO      = karnix.iso
ISO_DIR  = isodir

# =============================================================================
.PHONY: all build iso run run-iso clean info data

all: build

# ── Step 0: generate db_data.h from PostgreSQL (or use fallback) ─────────────
# Run  make data  once (or any time the DB changes) to refresh embedded data.
# If db_data.h does not yet exist it is auto-created by a fallback generator
# embedded in db_exporter.js so the kernel always compiles.
DB_EXPORTER  = db_exporter.js
NODE_MODULES = node_modules/.package-lock.json

$(NODE_MODULES): package.json
	@echo "  NPM  installing pg driver..."
	npm install --silent

# 'make data' – pull from live PostgreSQL and write db_data.h
data: $(NODE_MODULES)
	@echo "  DB   Fetching from PostgreSQL -> db_data.h ..."
	node $(DB_EXPORTER)
	@echo "  DB   db_data.h regenerated. Run 'make iso' to rebuild kernel."

# Auto-generate db_data.h if missing (uses fallback embedded data)
db_data.h:
	@echo "  DB   db_data.h missing – generating with fallback data..."
	@command -v node >/dev/null 2>&1 && \
	  ( [ -f $(NODE_MODULES) ] || npm install --silent ) && \
	  node $(DB_EXPORTER) || \
	  ( echo "  WARN node not found – writing minimal fallback db_data.h"; \
	    printf '#ifndef DB_DATA_H\n#define DB_DATA_H\n' > db_data.h; \
	    printf '#define DB_TOTAL_PATIENTS 20\n#define DB_HIGH_RISK 6\n' >> db_data.h; \
	    printf '#define DB_MOD_RISK 4\n#define DB_LOW_RISK 10\n' >> db_data.h; \
	    printf '#define DB_AVG_RISK_X10 542\n#define DB_CNT_ABNORMAL 4\n' >> db_data.h; \
	    printf '#define DB_CNT_NORMAL 10\n#define DB_CNT_SPONDYL 3\n#define DB_CNT_DISK 3\n' >> db_data.h; \
	    printf '#define DB_SOURCE "FALLBACK"\n' >> db_data.h; \
	    printf 'typedef struct{const char*patient_id;int age;const char*sex;const char*date;int risk_score_int;int abnorm_pct;const char*classification;const char*risk_category;int cobb_x10;int bone_x10;int frac_grade;int disc_grade;}SpineRec;\n' >> db_data.h; \
	    printf '#define NRECORDS 20\nstatic const SpineRec g_records[NRECORDS]={{"PT-001",45,"M","2026-02-26",577,99,"Abnormal","HIGH",180,-120,1,2},{"PT-002",32,"F","2026-02-26",1000,0,"Normal","LOW",40,50,0,0},{"PT-003",58,"M","2026-02-15",421,87,"Abnormal","HIGH",240,-210,2,3},{"PT-004",29,"F","2026-02-20",934,6,"Normal","LOW",30,110,0,0},{"PT-005",67,"M","2026-02-22",123,76,"DiskHernia","HIGH",80,-280,1,4},{"PT-006",41,"F","2026-02-23",890,11,"Normal","LOW",50,20,0,1},{"PT-007",53,"M","2026-02-24",310,91,"Spondylolisthesis","HIGH",120,-180,2,3},{"PT-008",36,"F","2026-02-25",985,1,"Normal","LOW",20,80,0,0},{"PT-009",72,"M","2026-02-26",89,96,"Abnormal","HIGH",310,-310,3,4},{"PT-010",44,"F","2026-02-26",756,24,"DiskHernia","MODERATE",90,-50,1,2},{"PT-011",38,"M","2026-02-26",921,7,"Normal","LOW",40,60,0,0},{"PT-012",61,"F","2026-02-26",204,84,"Spondylolisthesis","HIGH",190,-240,2,3},{"PT-013",27,"M","2026-02-27",876,12,"Normal","LOW",30,120,0,0},{"PT-014",55,"F","2026-02-27",448,69,"DiskHernia","MODERATE",110,-100,1,3},{"PT-015",48,"M","2026-02-27",611,38,"DiskHernia","MODERATE",70,-80,1,2},{"PT-016",33,"F","2026-02-27",999,0,"Normal","LOW",20,100,0,0},{"PT-017",76,"M","2026-02-28",55,98,"Abnormal","HIGH",360,-350,3,4},{"PT-018",42,"F","2026-02-28",780,21,"Normal","LOW",50,30,0,1},{"PT-019",59,"M","2026-02-28",315,88,"Spondylolisthesis","HIGH",140,-150,2,3},{"PT-020",31,"F","2026-02-28",945,5,"Normal","LOW",30,90,0,0}};\n' >> db_data.h; \
	    printf 'typedef struct{int pi10,pt10,ll10,ss10,pr10,sp10;const char*label;}UCIRecord;\n#define NUCI 10\nstatic const UCIRecord g_uci[NUCI]={{630,225,396,404,986,-2,"Normal"},{390,100,250,290,1144,45,"Normal"},{688,222,500,466,1059,-35,"Normal"},{377,119,233,257,1130,216,"DiskHernia"},{771,287,498,484,1030,65,"DiskHernia"},{880,390,817,489,1166,567,"Spondylolisthesis"},{656,236,582,419,1240,522,"Spondylolisthesis"},{508,185,272,322,1127,321,"DiskHernia"},{701,284,492,416,1195,654,"Spondylolisthesis"},{407,135,311,302,1312,768,"Spondylolisthesis"}};\n#endif\n' >> db_data.h )

# ── Step 1: assemble boot stub ───────────────────────────────────────────────
$(ASM_OBJ): $(ASM_SRCS)
	$(NASM) $(NASMFLAGS) $< -o $@
	@echo "  ASM  $< -> $@"

# ── Step 2: compile kernel ───────────────────────────────────────────────────
$(C_OBJ): $(C_SRCS) db_data.h
	$(CC) $(CFLAGS) -c $< -o $@
	@echo "  CC   $< -> $@"

# ── Step 3: link ELF ─────────────────────────────────────────────────────────
$(ELF): $(ASM_OBJ) $(C_OBJ)
	$(LD) $(LDFLAGS) -o $@ $^
	@echo "  LD   $@ (ELF32 multiboot kernel)"
	@echo ""
	@echo "  Build SUCCESS  ->  $(ELF)"
	@echo ""

build: $(ELF)

# ── Step 4: create ISO with GRUB ─────────────────────────────────────────────
iso: build
	mkdir -p $(ISO_DIR)/boot/grub
	cp $(ELF) $(ISO_DIR)/boot/karnix.elf
	@echo "  Running i686-elf-grub-mkrescue..."
	i686-elf-grub-mkrescue -o $(ISO) $(ISO_DIR) 2>&1 || \
	grub-mkrescue -o $(ISO) $(ISO_DIR) 2>&1 || \
	grub2-mkrescue -o $(ISO) $(ISO_DIR) 2>&1 || \
	( echo "  WARNING: grub-mkrescue not found – use 'make run'" )

# ── Step 5a: run direct ELF via QEMU multiboot  ──────────────────────────────
run: build
	@echo ""
	@echo "  Launching KarnixOS (ELF direct) in QEMU..."
	@echo "  Click window to grab mouse.  Ctrl+Alt+G = release."
	@echo "  Type at the KarnixOS> prompt.  Arrow keys navigate records."
	@echo ""
	$(QEMU) $(QFLAGS) -kernel $(abspath $(ELF))

# ── Step 5b: run from ISO ─────────────────────────────────────────────────────
run-iso: iso
	@echo ""
	@echo "  Launching KarnixOS (ISO/GRUB) in QEMU..."
	@echo "  Click window to grab mouse.  Ctrl+Alt+G = release."
	@echo ""
	$(QEMU) $(QFLAGS) -cdrom $(abspath $(ISO))

# ── Clean ─────────────────────────────────────────────────────────────────────
clean:
	rm -f $(ASM_OBJ) $(C_OBJ) $(ELF) $(ISO)
	@echo "  Cleaned."

# 'make cleanall' also removes generated DB header
cleanall: clean
	rm -f db_data.h
	@echo "  Also removed db_data.h"

# ── Info ──────────────────────────────────────────────────────────────────────
info:
	@echo "Cross compiler : $(CC)"
	@echo "Linker         : $(LD)"
	@echo "NASM           : $(NASM)"
	@echo "QEMU           : $(QEMU)"
