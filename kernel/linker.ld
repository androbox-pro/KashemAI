ENTRY(_start)

PHDRS
{
  text PT_LOAD FLAGS(5); /* R-X */
  data PT_LOAD FLAGS(6); /* RW- */
}

SECTIONS
{
  . = 0x8000;
  .text.entry : { *(.text.entry) } :text
  .text : { *(.text*) } :text
  .rodata : { *(.rodata*) } :text
  .data : { *(.data*) } :data
  .bss : { *(.bss*) *(COMMON) } :data
}
