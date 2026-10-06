CC ?= gcc
LD ?= ld
OBJCOPY ?= objcopy
AS ?= as
PYTHON ?= python3
CFLAGS := -m32 -ffreestanding -fno-stack-protector -fno-pic -fno-pie -fno-builtin -Wall -Wextra -Werror -O2 -Iinclude
LDFLAGS := -m elf_i386 -T kernel/linker.ld

BUILD := build

.PHONY: all clean vip image check ci

all: image

$(BUILD)/boot.o: boot/boot.S
	mkdir -p $(BUILD)
	$(AS) --32 $< -o $@

$(BUILD)/entry.o: kernel/entry.S
	mkdir -p $(BUILD)
	$(AS) --32 $< -o $@

$(BUILD)/kmain.o: kernel/kmain.c include/vipos.h
	mkdir -p $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/kernel.elf: $(BUILD)/entry.o $(BUILD)/kmain.o kernel/linker.ld
	$(LD) $(LDFLAGS) -o $@ $(BUILD)/entry.o $(BUILD)/kmain.o

$(BUILD)/kernel.bin: $(BUILD)/kernel.elf
	$(OBJCOPY) -O binary $< $@
	@test $$(stat -c%s $@) -le 16384 || (echo "kernel.bin exceeds 32 sectors"; exit 1)
	truncate -s 16384 $@

vip: $(BUILD)/demo.vip

$(BUILD)/demo.vip: samples/hello.manifest samples/hello.payload tools/mkvip.py
	mkdir -p $(BUILD)
	python3 tools/mkvip.py samples/hello.manifest samples/hello.payload $@
	@test $$(stat -c%s $@) -le 4096 || (echo "demo.vip exceeds 8 sectors"; exit 1)
	truncate -s 4096 $@

$(BUILD)/boot.bin: $(BUILD)/boot.o
	$(OBJCOPY) -O binary $< $@
	@test $$(stat -c%s $@) -eq 512 || (echo "boot.bin must be exactly 512 bytes"; exit 1)

image: $(BUILD)/boot.bin $(BUILD)/kernel.bin $(BUILD)/demo.vip
	cat $(BUILD)/boot.bin $(BUILD)/kernel.bin $(BUILD)/demo.vip > $(BUILD)/vipos.img
	@test $$(stat -c%s $(BUILD)/vipos.img) -eq 20992 || (echo "unexpected image size"; exit 1)
	cp $(BUILD)/vipos.img vipos.img

check: all
	python3 tools/test_vipos.py
	@echo "== ELF =="
	readelf -h $(BUILD)/kernel.elf | sed -n '1,16p'
	@echo "== Sizes =="
	ls -lh $(BUILD)/boot.bin $(BUILD)/kernel.bin $(BUILD)/demo.vip vipos.img
	@echo "== VIP header =="
	python3 tools/inspectvip.py $(BUILD)/demo.vip

clean:
	rm -rf $(BUILD) vipos.img

ci: check
	@echo "VIPOS CI checks completed."

# AArch64 / QEMU virt
CLANG ?= clang
LLD ?= ld.lld
ARM64_CFLAGS := --target=aarch64-none-elf -ffreestanding -fno-stack-protector -fno-pic -fno-pie -fno-builtin -Wall -Wextra -Werror -O2 -Iinclude
ARM64_LDFLAGS := -m aarch64elf -T arch/arm64/linker.ld

.PHONY: arm64 arm64-check

arm64: $(BUILD)/vipos-arm64.elf $(BUILD)/demo.vip

$(BUILD)/arm64-start.o: arch/arm64/start.S
	mkdir -p $(BUILD)
	$(CLANG) --target=aarch64-none-elf -c $< -o $@

$(BUILD)/arm64-kmain.o: arch/arm64/kmain.c include/vipos.h
	mkdir -p $(BUILD)
	$(CLANG) $(ARM64_CFLAGS) -c $< -o $@

$(BUILD)/arm64-vip.o: arch/arm64/vip.S $(BUILD)/demo.vip
	mkdir -p $(BUILD)
	$(CLANG) --target=aarch64-none-elf -c $< -o $@

$(BUILD)/vipos-arm64.elf: $(BUILD)/arm64-start.o $(BUILD)/arm64-kmain.o $(BUILD)/arm64-vip.o $(BUILD)/demo.vip arch/arm64/linker.ld
	$(LLD) $(ARM64_LDFLAGS) -o $@ $(BUILD)/arm64-start.o $(BUILD)/arm64-kmain.o $(BUILD)/arm64-vip.o

arm64-check: arm64
	$(PYTHON) -c "from pathlib import Path; p=Path('build/vipos-arm64.elf'); data=p.read_bytes(); assert data[:4] == b'\\x7fELF', 'AArch64 ELF missing'; assert b'VIPOS_ARM64_OK' in data, 'ARM64 boot marker missing from ELF'; print('AArch64 ELF static check passed.')"
