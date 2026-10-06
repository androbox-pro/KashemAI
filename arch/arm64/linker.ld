ENTRY(_start)

SECTIONS
{
  . = 0x40080000;

  .text.boot : ALIGN(8) { KEEP(*(.text.boot)) }
  .text      : ALIGN(8) { *(.text*) }
  .rodata    : ALIGN(8) { *(.rodata*) }
  .rodata.vip : ALIGN(8) { KEEP(*(.rodata.vip)) }
  .data      : ALIGN(8) { *(.data*) }
  .bss       : ALIGN(16) { *(.bss*) *(COMMON) }

  /DISCARD/ : { *(.comment) }
}
