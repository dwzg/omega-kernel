@ =============================================================================
@ reset.s - leave the kernel and start the game that is mapped into ROM.
@
@ Everything here lives in IWRAM because the cartridge ROM space already shows
@ the game when these routines run.
@
@   reset_soft          Wipe IWRAM and restart at 0x08000000 via BIOS SoftReset.
@   reset_hard          Restart through the BIOS boot sequence (logo, multiboot).
@   reset_register_ram  Thumb wrapper for BIOS RegisterRamReset (SWI 0x01).
@
@ The instruction sequences are unchanged from the original kernel; only the
@ labels were renamed and comments added.
@ =============================================================================
	.section	.iwram,"ax",%progbits

@ -----------------------------------------------------------------------------
@ void reset_soft(void)
@
@ 1. Copy the tail of this file (reset_soft_stub .. reset_end) to EWRAM
@    0x02001000, because IWRAM is about to be wiped.
@ 2. Zero IWRAM from 0x03000000 up to iwram_keep_start, and from iwram_keep_end
@    to the end of IWRAM. The only part left intact is the copy/clear loop
@    below, which is still executing.
@ 3. Jump (Thumb) into the EWRAM copy of reset_soft_stub.
@ -----------------------------------------------------------------------------
	.code 16
iwram_keep_start:			@ data label: start of the region kept intact
	.global	reset_soft
	.thumb_func			@ entry point: set the Thumb bit in its address
reset_soft:
	ldr	r1, =reset_soft_arm
	bx	r1

	.code 32
reset_soft_arm:
	adr	r1, reset_soft_stub
	adr	r3, reset_end
	mov	r2, #0x02000000
	add	r2, #0x1000
copy_loop:
	ldr	r0, [r1], #4
	str	r0, [r2], #4
	cmp	r1, r3
	blt	copy_loop

	mov	r2, #0x3000000		@ clear IWRAM below this routine
	ldr	r3, =iwram_keep_start
	mov	r0, #0
clear_low:
	str	r0, [r2], #+4
	cmp	r2, r3
	blt	clear_low

	ldr	r2, =iwram_keep_end	@ clear IWRAM above this routine
	ldr	r3, =0x3008000
	mov	r0, #0
clear_high:
	str	r0, [r2], #+4
	cmp	r2, r3
	blt	clear_high

	mov	r0, #0x02000000		@ continue in the EWRAM copy (Thumb)
	add	r0, r0, #0x1000
	add	r0, r0, #1
	bx	r0

@ -----------------------------------------------------------------------------
@ void reset_register_ram(u32 flags) - BIOS RegisterRamReset.
@ -----------------------------------------------------------------------------
	.code 16
iwram_keep_end:				@ data label: end of the region kept intact
	.global	reset_register_ram
	.thumb_func
reset_register_ram:
	swi	1
	bx	lr

@ -----------------------------------------------------------------------------
@ Runs from EWRAM (copied by reset_soft). Disables interrupts, clears the
@ BIOS "return address" flag at 0x03007FFA so SoftReset starts the cartridge
@ ROM, sets the stack and calls BIOS SoftReset (SWI 0x00).
@ -----------------------------------------------------------------------------
reset_soft_stub:
	ldr	r3, =0x04000208		@ REG_IME = 0
	mov	r2, #0
	str	r2, [r3, #0]
	ldr	r6, =0x03007F00
	mov	r5, #1
	str	r5, [r6]
	ldr	r6, =0x03007FFA		@ 0 = restart from ROM
	mov	r7, #0x0
	str	r7, [r6, #0]
	ldr	r1, =0x03007f00
	mov	sp, r1
	swi	0

@ -----------------------------------------------------------------------------
@ void reset_hard(void) - BIOS HardReset (SWI 0x26): full boot sequence.
@ -----------------------------------------------------------------------------
	.global	reset_hard
	.thumb_func
reset_hard:
	ldr	r3, =0x04000208		@ REG_IME = 0
	mov	r2, #0
	str	r2, [r3, #0]
	ldr	r1, =0x3007f00
	mov	SP, r1
	swi	0x26

	.align
	.ltorg
reset_end:
