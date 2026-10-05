/**
 * @file boot_messages.c
 * @brief Text for boot results. See boot.h.
 *
 * Kept apart from boot.c, which drives the hardware, so the user interface
 * can be built and tested on a PC.
 */
#include "loader/boot.h"

const char *boot_result_message(boot_result_t result)
{
    switch (result) {
    case BOOT_OK:
        return "Done.";
    case BOOT_ERR_GAME_FILE:
        return "The game file can't be read.";
    case BOOT_ERR_TOO_LARGE:
        return "The game is larger than 32 MB.";
    case BOOT_ERR_FRAGMENTED:
        return "The file is too fragmented. Copy it to the card again.";
    case BOOT_ERR_SAVE_FOLDER:
        return "The /SAVER folder can't be created.";
    case BOOT_ERR_SAVE_CREATE:
        return "The save file can't be created.";
    case BOOT_ERR_SAVE_READ:
        return "The save file can't be read.";
    case BOOT_ERR_SAVE_EMPTY:
        return "The save file is empty. Delete it to start a new one.";
    case BOOT_ERR_SAVE_STATE:
        return "The save state file can't be prepared.";
    case BOOT_ERR_NOR_MISSING:
        return "NOR flash was not detected.";
    case BOOT_ERR_NOR_FULL:
        return "There isn't enough free space in NOR flash.";
    }
    return "Unknown error.";
}
