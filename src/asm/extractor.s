; Copyright 2017-2026 Matt "MateoConLechuga" Waltz
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

.include "helper.inc"

	ld	hl,0
bootstrap_offset EXTRACTOR_ENTRY_OFFSET
	ld	de,ti.plotSScreen
	ld	bc,0
bootstrap_offset EXTRACTOR_PRGM_SIZE_OFFSET
	ldir
	jp	ti.plotSScreen

relocated_code
extractprgm:
	ld	de,0
relocated_offset EXTRACTOR_EXTRACT_SIZE_OFFSET
	call	ti.MemChk
	sbc	hl,de
	jp	c,ti.ErrMemory
	ld	de,.prgm_name
	call	ti.MovFROP1
	ld	hl,ti.userMem
	ld	de,(ti.asm_prgm_size)
	call	ti.DelMem			; remove this program
	xor	a,a
	sbc	hl,hl
	ld	(ti.asm_prgm_size),hl
	ld	hl,extractdata
.loop:
	ld	a,(hl)
	or	a,a
	jr	z,.done
	push	hl
	call	ti.Mov9ToOP1
.find:
	call	ti.ChkFindSym
	jr	c,.notfound
	call 	ti.ChkInRam
	jr	nz,.inarc
	call	ti.PushRealO1
	call	ti.Arc_Unarc
	call	ti.PopRealO1
	jr	.find

.done:
	ld	hl,.prgm_name
	call	ti.Mov9ToOP1
	jp	ti.userMem

.inarc:
	ex	de,hl
	ld	de,9
	add	hl,de
	ld	e,(hl)
	add	hl,de
	inc	hl
	call	ti.LoadDEInd_s
	push	hl
	push	de
	ld	de,ti.userMem
	ld	hl,(ti.asm_prgm_size)
	add	hl,de
	ex	de,hl
	pop	hl
	push	de
	push	hl
	call	ti.InsertMem
	pop	bc
	ld	hl,(ti.asm_prgm_size)
	add	hl,bc
	ld	(ti.asm_prgm_size),hl
	pop	de
	pop	hl
	ldir
	pop	hl
	ld	c,9			; ld bc, 9
	add	hl,bc
	jr	.loop

.notfound:
	pop	hl
	ld	de,ti.appErr1
	ld	hl,.missing_str
	ld	bc,12
	ldir
	jp	ti.ErrCustom1

.prgm_name:
	.space	9
.missing_str:
	.asciz	"Need AppVar"
extractdata:
relocated_offset EXTRACTOR_APPVARS_OFFSET, 0
