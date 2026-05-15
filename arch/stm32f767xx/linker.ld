/* Linker script for STM32F767xx — 2MB Flash, 512KB DTCM+SRAM at 0x20000000 */

ENTRY(Reset_Handler)

_estack = ORIGIN(RAM) + LENGTH(RAM);

_Min_Heap_Size  = 0x0;
_Min_Stack_Size = 0x800;

MEMORY
{
  FLASH (rx)  : ORIGIN = 0x08000000, LENGTH = 2048K
  RAM   (xrw) : ORIGIN = 0x20000000, LENGTH = 512K
}

SECTIONS
{
  .isr_vector :
  {
    . = ALIGN(4);
    KEEP(*(.isr_vector))
    . = ALIGN(4);
  } >FLASH

  .text :
  {
    . = ALIGN(4);

    *(.text*)
    *(.rodata*)

    KEEP(*(.init))
    KEEP(*(.fini))

    . = ALIGN(4);
    _etext = .;
  } >FLASH

  .preinit_array :
  {
    . = ALIGN(4);
    PROVIDE_HIDDEN (__preinit_array_start = .);
    KEEP (*(.preinit_array*))
    PROVIDE_HIDDEN (__preinit_array_end = .);
    . = ALIGN(4);
  } >FLASH

  .init_array :
  {
    . = ALIGN(4);
    PROVIDE_HIDDEN (__init_array_start = .);
    KEEP (*(SORT(.init_array.*)))
    KEEP (*(.init_array*))
    PROVIDE_HIDDEN (__init_array_end = .);
    . = ALIGN(4);
  } >FLASH

  .fini_array :
  {
    . = ALIGN(4);
    PROVIDE_HIDDEN (__fini_array_start = .);
    KEEP (*(SORT(.fini_array.*)))
    KEEP (*(.fini_array*))
    PROVIDE_HIDDEN (__fini_array_end = .);
    . = ALIGN(4);
  } >FLASH

  _sidata = LOADADDR(.data);

  .data :
  {
    . = ALIGN(4);
    _sdata = .;
    *(.data)
    *(.data*)
    . = ALIGN(4);
    _edata = .;
  } >RAM AT> FLASH

  .bss (NOLOAD) :
  {
    . = ALIGN(4);
    _sbss = .;
    __bss_start__ = _sbss;

    *(.bss*)
    *(COMMON)

    . = ALIGN(4);
    _ebss = .;
    __bss_end__ = _ebss;
  } >RAM

  .noinit (NOLOAD) :
  {
    . = ALIGN(4);
    _snoinit = .;

    *(.noinit*)
    *(.noinit.*)

    . = ALIGN(4);
    _enoinit = .;
  } >RAM

  . = ALIGN(8);
  _end = .;
  end = .;

  .ARM.attributes 0 :
  {
    *(.ARM.attributes)
  }

  /DISCARD/ :
  {
    *(.eh_frame*)
    *(.gcc_except_table*)
    *(.ARM.extab*)
    *(.ARM.exidx*)
    *(.comment*)
    *(.note*)
  }
}

ASSERT(_etext <= ORIGIN(FLASH) + LENGTH(FLASH), "FLASH overflow");
ASSERT(_end + _Min_Stack_Size <= ORIGIN(RAM) + LENGTH(RAM), "RAM overflow: .data/.bss/.noinit + reserved stack exceed RAM");
