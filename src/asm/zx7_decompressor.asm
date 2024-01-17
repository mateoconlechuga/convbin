; Copyright 2017-2024 Matt "MateoConLechuga" Waltz
;
; Redistribution and use in source and binary forms, with or without
; modification, are permitted provided that the following conditions are met:
;
; 1. Redistributions of source code must retain the above copyright notice,
;    this list of conditions and the following disclaimer.
;
; 2. Redistributions in binary form must reproduce the above copyright notice,
;    this list of conditions and the following disclaimer in the documentation
;    and/or other materials provided with the distribution.
;
; 3. Neither the name of the copyright holder nor the names of its contributors
;    may be used to endorse or promote products derived from this software
;    without specific prior written permission.
;
; THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
; AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
; IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
; ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
; LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
; CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
; SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
; INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
; CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
; ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
; POSSIBILITY OF SUCH DAMAGE.

include '../deps/fasmg-ez80/ez80.inc'
include './ti84pceg.inc'

relocaddr := ti.plotSScreen

	ld	hl, 0
__decompressor := $% - 3
	ld	de, relocaddr
	ld	bc, compresseddata - prgm
	ldir
	jp	relocaddr
.relocate:
	org	relocaddr
prgm:
	ld	hl, 0
__insertmem_size := $% - 3
	push	hl
	call	ti.ErrNotEnoughMem
	call	ti.PushRealO1
	pop	hl
	ld	de, 0
__insertmem_addr := $% - 3
	push	de
	call	ti.InsertMem
	ld	hl, 0
__compressed_data_addr := $% - 3
	call	dzx7_standard
	ld	hl,0
__delmem_addr := $% - 3
	ld	de, 0
__delmem_size := $% - 3
__delmem_call := $%
	call	ti.DelMem
	ld	de, 0
__asm_prgm_size_delta := $% - 3
	ld	hl, (ti.asm_prgm_size)
	add	hl, de
	ld	(ti.asm_prgm_size), hl
	jp	ti.PopRealO1

; -----------------------------------------------------------------------------
; ZX7 decoder by Einar Saukas, Antonio Villena & Metalbrain
; "Standard" version (modified for ez80)
; -----------------------------------------------------------------------------
; Parameters:
;   HL: source address (compressed data)
;   DE: destination address (decompressing)
; -----------------------------------------------------------------------------

dzx7_standard:
	ld	a, $80
dzx7s_copy_byte_loop:
	ldi					; copy literal byte
dzx7s_main_loop:
	call	dzx7s_next_bit
	jr	nc, dzx7s_copy_byte_loop	; next bit indicates either literal or sequence

; determine number of bits used for length (Elias gamma coding)
	push	de
	ld	bc, 0
	ld	de, 0
	ld	d, b
dzx7s_len_size_loop:
	inc	d
	call	dzx7s_next_bit
	jr	nc, dzx7s_len_size_loop

; determine length
dzx7s_len_value_loop:
	call	nc, dzx7s_next_bit
	rl	c
	rl	b
	jr	c, dzx7s_exit			; check end marker
	dec	d
	jr	nz, dzx7s_len_value_loop
	inc	bc				; adjust length

; determine offset
	ld	e, (hl)				; load offset flag (1 bit) + offset value (7 bits)
	inc	hl
	sla	e
	inc	e
	jr	nc, dzx7s_offset_end		; if offset flag is set, load 4 extra bits
	ld	d, $10				; bit marker to load 4 bits
dzx7s_rld_next_bit:
	call	dzx7s_next_bit
	rl	d				; insert next bit into D
	jr	nc, dzx7s_rld_next_bit		; repeat 4 times, until bit marker is out
	inc	d				; add 128 to DE
	srl	d				; retrieve fourth bit from D
dzx7s_offset_end:
	rr	e				; insert fourth bit into E

; copy previous sequence
	ex	(sp), hl			; store source, restore destination
	push	hl				; store destination
	sbc	hl, de				; HL = destination - offset - 1
	pop	de				; DE = destination
	ldir
dzx7s_exit:
	pop	hl				; restore source address (compressed data)
	jr	nc, dzx7s_main_loop
dzx7s_next_bit:
	add	a, a				; check next bit
	ret	nz				; no more bits left?
	ld	a, (hl)				; load another group of 8 bits
	inc	hl
	rla
	ret
; -----------------------------------------------------------------------------
compresseddata:

	; actual program data is stored here

macro print msg, offset
	display msg
	repeat 1,x:offset
		display `x, 10
	end repeat
end macro

display "#ifndef DZX7_H", 10
display "#define DZX7_H", 10, 10
print "#define DZX7_DECOMPRESSOR_OFFSET ", __decompressor
print "#define DZX7_COMPRESSED_DATA_ADDR_OFFSET ", __compressed_data_addr
print "#define DZX7_INSERTMEM_SIZE_OFFSET ", __insertmem_size
print "#define DZX7_INSERTMEM_ADDR_OFFSET ", __insertmem_addr
print "#define DZX7_DELMEM_CALL_OFFSET ", __delmem_call
print "#define DZX7_DELMEM_ADDR_OFFSET ", __delmem_addr
print "#define DZX7_DELMEM_SIZE_OFFSET ", __delmem_size
print "#define DZX7_ASM_PRGM_SIZE_DELTA_OFFSET ", __asm_prgm_size_delta
display 10
display "extern unsigned char zx7_decompressor[];", 10
display "extern unsigned int zx7_decompressor_len;", 10
display 10
display "#endif"
