; =============================================================================
;  KarnixOS boot.asm  –  Multiboot1 + protected-mode entry + ALL ISR stubs
;  nasm -f elf32 boot.asm -o boot.o
; =============================================================================

MBOOT_MAGIC     equ 0x1BADB002
MBOOT_ALIGN     equ 1 << 0
MBOOT_MEMINFO   equ 1 << 1
MBOOT_FLAGS     equ MBOOT_ALIGN | MBOOT_MEMINFO
MBOOT_CHECKSUM  equ -(MBOOT_MAGIC + MBOOT_FLAGS)

; ── Multiboot header ─────────────────────────────────────────────────────────
section .multiboot
align 4
    dd MBOOT_MAGIC
    dd MBOOT_FLAGS
    dd MBOOT_CHECKSUM

; ── Kernel stack (32 KB) ─────────────────────────────────────────────────────
section .bss
align 16
stack_bottom:
    resb 32768
stack_top:

; ── Entry + ISR stubs ────────────────────────────────────────────────────────
section .text

global  _start
extern  kernel_main

_start:
    mov  esp, stack_top
    push dword 0
    popfd
    push ebx            ; mb_info  (arg 2)
    push eax            ; mb_magic (arg 1)
    call kernel_main
.hang:
    cli
    hlt
    jmp .hang

; =============================================================================
;  ISR stubs – pure asm so the compiler cannot mis-compile them.
;  Pattern:  pusha / call C_handler / popa / iret
; =============================================================================
global _isr_irq0
global _isr_irq1
global _isr_irq12
global _isr_dummy

extern irq0_handler
extern irq1_handler
extern irq12_handler

; IRQ0 – PIT timer
_isr_irq0:
    pusha
    call irq0_handler
    popa
    iret

; IRQ1 – PS/2 keyboard
_isr_irq1:
    pusha
    call irq1_handler
    popa
    iret

; IRQ12 – PS/2 mouse
_isr_irq12:
    pusha
    call irq12_handler
    popa
    iret

; Catch-all dummy (no handler, just iret)
_isr_dummy:
    iret
