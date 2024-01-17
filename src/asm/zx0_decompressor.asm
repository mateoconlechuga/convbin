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
	call	dzx0_standard
	ld	hl, 0
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
; ZX0 decoder by Einar Saukas & Urusergi
; "Standard" version (68 bytes only)
; -----------------------------------------------------------------------------
; Parameters:
;   HL: source address (compressed data)
;   DE: destination address (decompressing)
; -----------------------------------------------------------------------------

dzx0_standard:
	ld	bc, -1			; preserve default offset 1
	push	bc
	inc	bc
	ld	a, $80
dzx0s_literals:
	call	dzx0s_elias		; obtain length
	ldir				; copy literals
	add	a, a			; copy from last offset or new offset?
	jr	c, dzx0s_new_offset
	call	dzx0s_elias		; obtain length
dzx0s_copy:
	ex	(sp), hl		; preserve source, restore offset
	push	hl			; preserve offset
	add	hl, de			; calculate destination - offset
	ldir				; copy from offset
	pop	hl			; restore offset
	ex	(sp), hl		; preserve offset, restore source
	add	a, a			; copy from literals or new offset?
	jr	nc, dzx0s_literals
dzx0s_new_offset:
	pop	bc			; discard last offset
	ld	c, $fe			; prepare negative offset
	call	dzx0s_elias_loop	; obtain offset MSB
	inc	c
	ret	z			; check end marker
	ld	b, c
	ld	c, (hl)			; obtain offset LSB
	inc	hl
	rr	b			; last offset bit becomes first length bit
	rr	c
	push	bc			; preserve new offset
	ld	bc, 1			; obtain length
	call	nc, dzx0s_elias_backtrack
	inc	bc
	jr	dzx0s_copy
dzx0s_elias:
	inc	c			; interlaced Elias gamma coding
dzx0s_elias_loop:
	add	a, a
	jr	nz, dzx0s_elias_skip
	ld	a, (hl)			; load another group of 8 bits
	inc	hl
	rla
dzx0s_elias_skip:
	ret	c
dzx0s_elias_backtrack:
	add	a, a
	rl	c
	rl	b
	jr	dzx0s_elias_loop
; -----------------------------------------------------------------------------
compresseddata:

	; actual program data is stored here

macro print msg, offset
	display msg
	repeat 1,x:offset
		display `x, 10
	end repeat
end macro

display "#ifndef DZX0_H", 10
display "#define DZX0_H", 10, 10
print "#define DZX0_DECOMPRESSOR_OFFSET ", __decompressor
print "#define DZX0_COMPRESSED_DATA_ADDR_OFFSET ", __compressed_data_addr
print "#define DZX0_INSERTMEM_SIZE_OFFSET ", __insertmem_size
print "#define DZX0_INSERTMEM_ADDR_OFFSET ", __insertmem_addr
print "#define DZX0_DELMEM_CALL_OFFSET ", __delmem_call
print "#define DZX0_DELMEM_ADDR_OFFSET ", __delmem_addr
print "#define DZX0_DELMEM_SIZE_OFFSET ", __delmem_size
print "#define DZX0_ASM_PRGM_SIZE_DELTA_OFFSET ", __asm_prgm_size_delta
display 10
display "extern unsigned char zx0_decompressor[];", 10
display "extern unsigned int zx0_decompressor_len;", 10
display 10
display "#endif"
