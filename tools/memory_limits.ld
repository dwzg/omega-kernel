/*
 * Extra checks added to devkitARM's gba_cart.ld at link time (see Makefile).
 * The stock script lets static data run past the end of EWRAM and IWRAM
 * without complaint; these turn that into a link error.
 */

/* EWRAM: 256 KiB at 0x02000000. */
ASSERT(__sbss_end__ <= 0x02040000, "EWRAM overflow: static data exceeds 256 KiB")

/* IWRAM: 32 KiB at 0x03000000. Keep 4 KiB free for the stacks at the top. */
ASSERT(__iwram_overlay_end <= 0x03007000, "IWRAM overflow: less than 4 KiB left for the stacks")
