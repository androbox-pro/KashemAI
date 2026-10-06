.code16
.intel_syntax noprefix
.section .text
.global _start

.equ KERNEL_LBA_START, 1
.equ KERNEL_SECTORS, 32
.equ VIP_LBA_START, 33
.equ VIP_SECTORS, 8

_start:
    cli

    # Normalize the firmware entry point to 07C0:0000.. so that
    # labels in this sector can be addressed through DS=CS regardless
    # of whether the BIOS entered as 0000:7C00 or 07C0:0000.
    jmp 0x07C0:boot_entry

boot_entry:
    mov ax, cs
    mov ds, ax
    mov es, ax
    xor ax, ax
    mov ss, ax
    mov sp, 0x7C00
    mov [boot_drive], dl

    # Early marker: proves BIOS transferred control to our boot sector.
    lea si, boot_start_msg
    call debug_string

    # Load 32 sectors of kernel to physical 0x8000 (0x0800:0)
    mov word ptr [dap_buffer_segment], 0x0800
    mov dword ptr [dap_lba_low], KERNEL_LBA_START
    mov dword ptr [dap_lba_high], 0
    mov word ptr [sectors_left], KERNEL_SECTORS
    mov word ptr [buffer_segment], 0x0800
    call load_sectors

    # Load 8 sectors of the demo .vip package to physical 0x20000 (0x2000:0)
    mov word ptr [dap_buffer_segment], 0x2000
    mov dword ptr [dap_lba_low], VIP_LBA_START
    mov dword ptr [dap_lba_high], 0
    mov word ptr [sectors_left], VIP_SECTORS
    mov word ptr [buffer_segment], 0x2000
    call load_sectors

    # CI-visible boot marker via QEMU debug console (I/O port 0xE9)
    lea si, boot_ok_msg
    call debug_string

    jmp 0x0800:0x0000

# BIOS INT 13h Extensions (AH=42h), one sector per call.
load_sectors:
1:
    cmp word ptr [sectors_left], 0
    je 3f
    mov word ptr [dap_count], 1
    mov word ptr [dap_buffer_offset], 0
    mov ax, [buffer_segment]
    mov word ptr [dap_buffer_segment], ax
    lea si, dap
    mov dl, [boot_drive]
    mov ah, 0x42
    int 0x13
    jc disk_error

    # Next LBA
    add dword ptr [dap_lba_low], 1
    adc dword ptr [dap_lba_high], 0

    # Next 512-byte segment destination
    add word ptr [buffer_segment], 0x20
    dec word ptr [sectors_left]
    jmp 1b
3:
    ret

disk_error:
    lea si, disk_fail_msg
    call debug_string
    lea si, disk_msg
    call print_string
    cli
4:
    hlt
    jmp 4b

debug_string:
1:
    lodsb
    test al, al
    jz 2f
    mov dx, 0x00E9
    out dx, al
    jmp 1b
2:
    ret

boot_start_msg: .asciz "VIPOS_BOOT_START\n"
boot_ok_msg: .asciz "VIPOS_BOOTLOADER_OK\n"
disk_fail_msg: .asciz "VIPOS_DISK_READ_FAIL\n"

print_string:
5:
    lodsb
    test al, al
    jz 6f
    mov ah, 0x0E
    mov bh, 0
    int 0x10
    jmp 5b
6:
    ret

boot_drive: .byte 0
sectors_left: .word 0
buffer_segment: .word 0

dap:
    .byte 0x10          # size
    .byte 0x00          # reserved
    dap_count: .word 1
    dap_buffer_offset: .word 0
    dap_buffer_segment: .word 0x0800
    dap_lba_low: .long 1
    dap_lba_high: .long 0

 disk_msg: .asciz "VIPOS: disk read error\r\n"

.org 510
.word 0xAA55
