ENTRY (main)

MEMORY
{
  ram : ORIGIN = 0x00218000, LENGTH = 32M - 0x00218000
  rom : ORIGIN = 0x60000000, LENGTH = 8M
}

SECTIONS
{
  /* The actual instructions */
  text : {
    *(.text)
    *(.got)
    *(.got.plt)
    *(.rodata)
    *(.glue_7)
    *(.glue_7t)
  } >ram

  /* Initialized data */
  .data : {
    _DataStart = . ;
    *(.data)
    *(.data.rel.local)
    *(.data.rel.ro)
    _DataEnd = . ;
  } >ram

  /* Uninitialized data */
  .bss : {
    _BssStart = . ;
    *(.bss)
    _BssEnd = . ;
  } >ram

  rodata : {
    *(.rodata.str1.4)
  } >ram
}
