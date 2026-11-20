.include "ti84pce.inc"
.assume adl=1

.section .bootstrap, "ax"
.Lbootstrap:

; The loader copies the following code from immediately after the bootstrap
; to plotSScreen. Patch offsets are relative to the start of the whole blob.
.macro relocated_code
	.equ .Lbootstrap_size, . - .Lbootstrap
	.section .relocated, "ax"
.Lrelocated:
.endm

; By default, define the offset of the preceding 24-bit operand.
.macro bootstrap_offset symbol, width=3
	.global \symbol
	.equ \symbol, . - .Lbootstrap - \width
.endm

.macro relocated_offset symbol, width=3
	.global \symbol
	.equ \symbol, .Lbootstrap_size + (. - .Lrelocated) - \width
.endm
