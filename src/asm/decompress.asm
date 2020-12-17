; Copyright 2017-2020 Matt "MateoConLechuga" Waltz
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

macro print msg, offset
	display msg
	repeat 1,x:offset
		display `x, 10
	end repeat
end macro

relocaddr := ti.plotSScreen
relocoffset := compressedprgm.relocate - ti.userMem
asmcompsize := 2
sizebytessize := 2

compressedprgm:
	ld	hl,0
decompressentrylabel := $% - 3
	ld	de,relocaddr
	ld	bc,compresseddata - decompressprgm
	ldir
	jp	relocaddr
.relocate:
	org	relocaddr
decompressprgm:
	ld	hl,0
deltasizelabel := $% - 3
	push	hl
	call	ti.ErrNotEnoughMem
	pop	hl
	ld	de,0
deltastartlabel := $% - 3
	call	ti.InsertMem
	ld	hl,0
prgrmsizelabel := $% - 3
	ld	(ti.asm_prgm_size),hl
	ld	hl,0
compressedendlabel := $% - 3
	ld	de,0
uncompressedendlabel := $% - 3
	call	dzx7_standard_back
	inc	hl
	ld	de,0 ; compressedstart
compressedstartlabel := $% - 3
	push	de
	ld	bc,0 ; uncompressedsize - 2176
uncompressedsizelabel := $% - 3
	ldir
	ld	de,0 ; 2176
realsizesizelabel := $% - 3
	ld	hl,0 ;compressedendlabel - 2176
resizelabel := $% - 3
	jp	ti.DelMem

; -----------------------------------------------------------------------------
; ZX7 decoder by Einar Saukas, Antonio Villena & Metalbrain
; "Standard" version (69 bytes only) - BACKWARDS VARIANT
; -----------------------------------------------------------------------------
; Parameters:
;   HL: last source address (compressed data)
;   DE: last destination address (decompressing)
; -----------------------------------------------------------------------------
dzx7_standard_back:
	ld	a,$80
dzx7s_copy_byte_loop_b:
	ldd
dzx7s_main_loop_b:
	call	dzx7s_next_bit_b
	jr	nc,dzx7s_copy_byte_loop_b
	push	de
	ld	bc,0
	ld	d,b
dzx7s_len_size_loop_b:
	inc	d
	call	dzx7s_next_bit_b
	jr	nc,dzx7s_len_size_loop_b
dzx7s_len_value_loop_b:
	call	nc,dzx7s_next_bit_b
	rl	c
	rl	b
	jr	c,dzx7s_exit_b
	dec	d
	jr	nz,dzx7s_len_value_loop_b
	inc	bc
	ld	e,(hl)
	dec	hl
	sla	e
	jr	nc,dzx7s_offset_end_b
	ld	d,$10
dzx7s_rld_next_bit_b:
	call	dzx7s_next_bit_b
	rl	d
	jr	nc,dzx7s_rld_next_bit_b
	inc	d
	srl	d
dzx7s_offset_end_b:
	rr	e
	inc.s	de
	ex	(sp),hl
	ex	de,hl
	add	hl,de
	lddr
dzx7s_exit_b:
	pop	hl
	jr	nc,dzx7s_main_loop_b
dzx7s_next_bit_b:
	add	a,a
	ret	nz
	ld	a,(hl)
	dec	hl
	rla
	ret

compresseddata:

display 10
print "#define DECOMPRESS_ENTRY_OFFSET ", decompressentrylabel
print "#define DECOMPRESS_DELTA_SIZE_OFFSET ", deltasizelabel
print "#define DECOMPRESS_DELTA_START_OFFSET ", deltastartlabel
print "#define DECOMPRESS_PRGM_SIZE_OFFSET ", prgrmsizelabel
print "#define DECOMPRESS_COMPRESSED_END_OFFSET ", compressedendlabel
print "#define DECOMPRESS_UNCOMPRESSED_END_OFFSET ", uncompressedendlabel
print "#define DECOMPRESS_COMPRESSED_START_OFFSET ", compressedstartlabel
print "#define DECOMPRESS_UNCOMPRESSED_SIZE_OFFSET ", uncompressedsizelabel
print "#define DECOMPRESS_RESIZE_OFFSET ", resizelabel
print "#define DECOMPRESS_RESIZE_SIZE_OFFSET ", realsizesizelabel
display 10
print "size: ", $%
