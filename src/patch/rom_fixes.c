/**
 * @file rom_fixes.c
 * @brief Game-specific patch data. See rom_fixes.h.
 *
 * Comments give the release number and title of the game.
 */
#include "patch/rom_fixes.h"

/**
 * Extra IRQ-vector related replacements, applied whenever any hook is enabled.
 * Several entries per game are applied in table order.
 */
const irq_fix_t ROM_IRQ_FIXES[] = {
    {0x4A4E4941, 0x11C, 0xE241100C},   /* 0414 - Initial D - Another Stage(JP) */
    {0x504D3941, 0x2608, 0xE5810FF4},  /* 0643 - Motoracer Advance(EU) OK */
    {0x455A5641, 0xDC, 0xE3A01A07},    /* 0826 - Super Bubble Pop(US) */
    {0x455A5641, 0x147D8, 0xE58030F4}, /* 0826 - Super Bubble Pop(US) */
    {0x45533841, 0x110, 0xE241100C},   /* 0854 - Digimon - Battle Spirit(US) OK */
    {0x45523241, 0x28610, 0x39284939}, /* 0911 Bratz(US) */
    {0x454E3941, 0x97A8, 0xE5810FF4},  /* 0913 - Piglet's Big Game(US) */
    {0x50523241, 0x28614, 0x39284939}, /* 1017 Bratz(EU) */
    {0x50464C41, 0x211D8, 0x63484916}, /* 1056 - Dragon Ball Z - The Legacy of Goku II(EU) */
    {0x45464C41, 0x20D28, 0x63484916}, /* 1085 - Dragon Ball Z - The Legacy of Goku II(US) */
    {0x50533841, 0x110, 0xE241100C},   /* 1133 - Digimon - Battle Spirit(EU) */
    {0x584E3941, 0x100, 0xE1A00000},   /* 1178 - Piglet's Big Game(EU) */
    {0x584E3941, 0x9834, 0xE5810FF4},  /* 1178 - Piglet's Big Game(EU) */
    {0x45524E41, 0x79B8, 0x63504A31},  /* 1260 - Cartoon Network - Speedway(US) */
    {0x505A5641, 0xDC, 0xE3A01A07},    /* 1361 - Super Bubble Pop(EU) */
    {0x505A5641, 0x14938, 0xE58030F4}, /* 1361 - Super Bubble Pop(EU) */
    {0x454D5842, 0x7A7C, 0x63504A31},  /* 1534 - XS Moto(US) */
    {0x45474C41, 0x9A14, 0x3007FBC},   /* 0434 - Dragon Ball Z - The Legacy of Goku(US) */
    {0x4A464C41, 0x22CA8,
     0x63484916}, /* 1591 - Dragon Ball Z - The Legacy of Goku II International(JP) */
    {0x45334742, 0x3F3DC, 0x63484916},  /* 1646 - Dragon Ball Z - Buu's Fury(US) OK */
    {0x45554642, 0x2E20C, 0x4A576350},  /* 1786 - Fear Factor - Unleashed(US) */
    {0x45574C42, 0x14C, 0x03007FF0},    /* 1953 - LEGO Star Wars - The Video Game(UE) */
    {0x4A574C42, 0x14C, 0x03007FF0},    /* 2052 - LEGO Star Wars - The Video Game(JP) */
    {0x45345442, 0x34FF8, 0x63484916},  /* 2079 - Dragon Ball GT - Transformation(US) */
    {0x45455742, 0xBDAC, 0x49296348},   /* 2151 - Whac-A-Mole(US) */
    {0x45444342, 0xA7A8, 0x49266348},   /* 2152 - Cinderella - Magical Dreams(US) */
    {0x45363842, 0x292BC, 0x63484916},  /* 2242 - Hello Kitty - Happy Party Pals(US) */
    {0x50495742, 0xB22C, 0x49296348},   /* 2255 - Winx Club(EU) */
    {0x45495742, 0xB22C, 0x49296348},   /* 2301 - Winx Club(US) */
    {0x58363842, 0x2245C, 0x63484916},  /* 2305 - Hello Kitty - Happy Party Pals(EU) */
    {0x50444342, 0xA7A8, 0x49296348},   /* 2430 - Cinderella - Magical Dreams(EU) */
    {0x45545A42, 0xB448, 0x63514A31},   /* 2466 - VeggieTales - LarryBoy and the Bad Apple(US) */
    {0x45465542, 0x307C, 0x63484916},   /* 2472 - 2 Games in 1 - Dragon Ball Z - Buu's Fury + Dragon
                                           Ball GT - Transformation(US) */
    {0x45465542, 0x6FAA8, 0x63484916},  /* 2472 - 2 Games in 1 - Dragon Ball Z - Buu's Fury + Dragon
                                           Ball GT - Transformation(US) */
    {0x45465542, 0x835244, 0x63484916}, /* 2472 - 2 Games in 1 - Dragon Ball Z - Buu's Fury + Dragon
                                           Ball GT - Transformation(US) */
    {0x45364C42, 0x228B0,
     0x63484916}, /* 2487 - My Little Pony - Crystal Princess - The Runaway Rainbow(US) */
    {0x45375442, 0x21548, 0x63484916},  /* 2597 - Tonka - On the Job(US) */
    {0x454D3941, 0x2654, 0xE5810FF4},   /* 2602 - Motoracer Advance(US) */
    {0x50363842, 0x292BC, 0x63484916},  /* 2627 - Hello Kitty - Happy Party Pals(EU) */
    {0x45544641, 0xE0, 0xE3A01A07},     /* 0096 - F-14 Tomcat(UE) */
    {0x454D5041, 0x1421C, 0x494D6345},  /* 0241 - Planet Monsters(US) */
    {0x504D5041, 0x1508C, 0x494D6345},  /* 0314 - Planet Monsters(EU) */
    {0x50534341, 0xAD64, 0x494D6345},   /* 0338 - Casper(EU) */
    {0x50514B41, 0x13A0, 0x494D6345},   /* 0500 - Kong - The Animated Series(EU) */
    {0x50505641, 0x1374, 0x494D6345},   /* 0549 - V.I.P.(EU) */
    {0x45413541, 0x14B7C8, 0xE58320F4}, /* 0685 - Bionicle - Matoran Adventures(UE) */
    {0x50524941, 0x158, 0xE3A00005},    /* 0843 - Inspector Gadget Racing(EU) */
    {0x45514B41, 0x13A0, 0x494D6345},   /* 1915 - Kong - The Animated Series(US) */
    {0x45534341, 0xAD64, 0x494D6345},   /* 2117 - Casper(US) */
};

/** Games whose payload location must be fixed by hand. */
const trim_override_t ROM_TRIM_OVERRIDES[] = {
    {0x4A4D4F41, 0xFE3000}, /* 1058 - Disney Sports - Motocross(JP) */
    {0x504D4F41, 0xFE3000}, /* 1068 - Disney Sports - Motocross(EU) */
    {0x4A494442, 0xFE4000}, /* 1176 - Koinu to Issho - Aijou Monogatari(JP) */
    {0x4A324942, 0xFE4000}, /* 1741 - Koinu to Issho 2(JP) */
    {0x454A4142, 0xFE4000}, /* 1864 - Banjo Pilot(US) */
    {0x504A4142, 0xFE4000}, /* 1899 - Banjo Pilot(EU) */
    {0x4A324D42, 0xFE4000}, /* 2045 - Momotarou Densetsu G - Gold Deck wo Tsukure!(JP) */
    {0x4A464842,
     0xFE4000}, /* 2071 - Twin Series 4 - Hamu Hamu Monster EX + F Puzzle Hamusuta(JP) */
    {0x50514442, 0xFE4000}, /* 2214 - Donkey Kong Country 3(EU) */
    {0x45514442, 0xFE4000}, /* 2220 - Donkey Kong Country 3(US) */
    {0x4A514442, 0xFE4000}, /* 2270 - Super Donkey Kong Country 3(JP) */
    {0x4A564642, 0xFE4000}, /* 2286 - Twin Series 1 - Mezase Debut! Fashion Designer Monogatari +
                               Kawaii Pet Game Gallery 2(JP) */
    {0x4A575A42, 0xFE4000}, /* 2335 - Akagi(JP) */
    {0x50385442, 0xFE4000}, /* 2342 - 2 Games in 1 - Teenage Mutant Ninja Turtles + Teenage Mutant
                               Ninja Turtles 2 - Battle Nexus(EU) */
    {0x50384E41, 0xFE4000}, /* 2351 - Tales of Phantasia(EU) */
    {0x4A534842, 0xFE4000}, /* 2457 - Hamster Monogatari 3EX + 4 Special(JP) */
    {0x45385442, 0xFE4000}, /* 2526 - 2 Games in 1 - Teenage Mutant Ninja Turtles + Teenage Mutant
                               Ninja Turtles 2 - Battle Nexus(US) */
    {0x45505342, 0xFE1000}, /* 1553 - Spider-Man 2(UE) */
    {0x58505342, 0xFE1000}, /* 1565 - Spider-Man 2(EU) */
    {0x49505342, 0xFE1000}, /* 1657 - Spider-Man 2(IT).zip */
    {0x45585142, 0xFE1000}, /* 2618 - Superman Returns - Fortress of Solitude(UE) */
    {0x44504C42,
     0x3A9FD0}, /* 1815 - 2 Games in 1 - Disneys Konig der Lowen + Disneys Prinzessinnen(DE) */
    {0x46504C42, 0x5ABF00}, /* 1827 - 2 Games in 1 - Roi Lion, Le + Disney Princesse(FR) */
    {0x58424C42, 0xFFB510}, /* 1867 - 2 Games in 1 - Brother Bear + Lion King, The(EU) */
    {0x50413542, 0x5E3CC0}, /* 2053 - 2 Games in 1 - Spyro - Season of Ice + Crash Bandicoot 2 -
                               N-Tranced(EU) */
    {0x45533842,
     0x7CFF00}, /* 2394 - 2 Games in 1 - Spyro - Season of Ice + Spyro 2 - Season of Flame(US) */
    {0x46425742,
     0x7FF000}, /* 2503 - 2 Games in 1 - Disney Princesse + Frere des Ours(FR) cant work */
    {0x49425742,
     0x7FEC30}, /* 2703 - 2 Games in 1 - Disney Principesse + Koda, Fratello Orso(IT) cant work */
    {0x44425742, 0x731000}, /* 2009 - 2 Games in 1 - Disneys Prinzessinnen + Baren Bruder(DE) */
    {0x53425742, 0x731000}, /* 2012 - 2 Games in 1 - Disney Princesas + Hermano Oso(ES) */
    {0x53434B42, 0xAFF000}, /* 2361 - Shin-chan - Aventuras en Cineland(ES) */
    {0x50504C42, 0x738000}, /* 2745 - 2 Games in 1 - Lion King, The + Disney Princess(EU) */
    {0x50425742, 0x738000}, /* 2780 - 2 Games in 1 - Disney Princess + Brother Bear(EU) */
};

const size_t ROM_IRQ_FIX_COUNT = sizeof(ROM_IRQ_FIXES) / sizeof(ROM_IRQ_FIXES[0]);
const size_t ROM_TRIM_OVERRIDE_COUNT = sizeof(ROM_TRIM_OVERRIDES) / sizeof(ROM_TRIM_OVERRIDES[0]);

/* Dragon Ball Z - The Legacy of Goku I & II and Top Gun: skip checks that
 * fail on flash carts. */
#define NOP 0x46C0 /* Thumb "mov r8, r8" */

static const rom_write16_t DBZ_2IN1_US[] = {
    {0x40356, 0}, {0x4035E, 0}, {0x4037E, 0}, {0x40382, 0}, {0xBB9016, 0x1001},
};
static const rom_write16_t DBZ_1_EU[] = {
    {0x033C, NOP}, {0x0340, NOP}, {0x0356, NOP}, {0x035A, NOP}, {0x035E, NOP},
    {0x0384, NOP}, {0x0388, NOP}, {0x494C, NOP}, {0x4950, NOP}, {0x4978, NOP},
    {0x497C, NOP}, {0x998E, NOP}, {0x9992, NOP},
};
static const rom_write16_t DBZ_1_US[] = {
    {0x356, 0},
    {0x35E, 0},
    {0x37E, 0},
    {0x382, 0},
};
static const rom_write16_t DBZ_2_EU[] = {{0x6F42B2, 0x1001}};
static const rom_write16_t DBZ_2_US[] = {{0x3B8E9E, 0x1001}};
static const rom_write16_t DBZ_2_JP[] = {{0x3FC8F6, 0x1001}};
static const rom_write16_t TOP_GUN_US[] = {
    {0x088816, 0x3401}, {0x088814, NOP}, {0x088932, NOP}, {0x088938, NOP}, {0x08893C, NOP},
    {0x08897C, NOP},    {0x088982, NOP}, {0x088986, NOP}, {0x088988, NOP},
};

#define FIX(code, table)                                                                           \
    {                                                                                              \
        code, table, sizeof(table) / sizeof(table[0])                                              \
    }

const rom_write_fix_t ROM_WRITE_FIXES[] = {
    FIX(0x45464C42, DBZ_2IN1_US), /* 2288 - 2 Games in 1 - Dragon Ball Z I & II (US) */
    FIX(0x50474C41, DBZ_1_EU),    /* 0639 - Dragon Ball Z - The Legacy of Goku (EU) */
    FIX(0x45474C41, DBZ_1_US),    /* 0434 - Dragon Ball Z - The Legacy of Goku (US) */
    FIX(0x50464C41, DBZ_2_EU),    /* 1056 - Dragon Ball Z - The Legacy of Goku II (EU) */
    FIX(0x45464C41, DBZ_2_US),    /* 1085 - Dragon Ball Z - The Legacy of Goku II (US) */
    FIX(0x4A464C41, DBZ_2_JP),    /* 1591 - Dragon Ball Z - The Legacy of Goku II Int. (JP) */
    FIX(0x45593241, TOP_GUN_US),  /* 1928 - Top Gun - Combat Zones (US) */
};
const size_t ROM_WRITE_FIX_COUNT = sizeof(ROM_WRITE_FIXES) / sizeof(ROM_WRITE_FIXES[0]);

/* Fire Emblem games save through routines that do not work with the FPGA's
 * automatic save; they are redirected to a payload and auto-save is turned off. */
const fire_emblem_fix_t ROM_FIRE_EMBLEM_FIXES[] = {
    {0x4A454641, /* 0378 - Fire Emblem - Fuuin no Tsurugi (JP) */
     {0x858B0, 0x85048, 0x84FF0, 0x85194, 0x850F8},
     0x7FF100,
     {0x01, 0x17, 0x27, 0x35, 0x47},
     FE_PAYLOAD_0378,
     0},
    {0x4A384542, /* 1692 - Fire Emblem - Seima no Kouseki (JP) */
     {0xA9844, 0xA989C, 0xA99F8, 0xA9B14, 0xAA5D0},
     0xF00000,
     {0x01, 0x0F, 0x1D, 0x2D, 0x3D},
     FE_PAYLOAD_1692,
     0},
    {0x4A374541, /* 0979 - Fire Emblem - Rekka no Ken (JP) */
     {0xA0FE0, 0xA1038, 0xA1178, 0xA1264, 0xA1BA8},
     0xFFF900,
     {0x01, 0x0F, 0x1D, 0x1D, 0x2D},
     FE_PAYLOAD_A,
     0x80B3DAF},
    {0x45374541, /* 1235 - Fire Emblem (US) */
     {0xA0654, 0xA06AC, 0xA07EC, 0xA08D8, 0xA1214},
     0xFFF900,
     {0x01, 0x0F, 0x1D, 0x1D, 0x2D},
     FE_PAYLOAD_A,
     0x80B2F8B},
    {0x58374541, /* 1574 - Fire Emblem (EU) */
     {0xA09C8, 0xA0A20, 0xA0B60, 0xA0C4C, 0xA1560},
     0xFFF900,
     {0x01, 0x0F, 0x1D, 0x1D, 0x2D},
     FE_PAYLOAD_A,
     0x80B3A57},
    {0x59374541, /* 1575 - Fire Emblem (EU) */
     {0xA09CC, 0xA0A24, 0xA0B64, 0xA0C50, 0xA1564},
     0xFFF900,
     {0x01, 0x0F, 0x1D, 0x1D, 0x2D},
     FE_PAYLOAD_A,
     0x80B3A3B},
    {0x45384542, /* 1997 - Fire Emblem - The Sacred Stones (US) */
     {0xA4E00, 0xA4E58, 0xA4FDC, 0xA50FC, 0xA5BB8},
     0xFFF900,
     {0x01, 0x0F, 0x1D, 0x1D, 0x2D},
     FE_PAYLOAD_B,
     0x80B5D6B},
    {0x50384542, /* 2215 - Fire Emblem - The Sacred Stones (EU) */
     {0xA5738, 0xA5790, 0xA5914, 0xA5A34, 0xA64F0},
     0x1FFDD00,
     {0x01, 0x0F, 0x1D, 0x1D, 0x2D},
     FE_PAYLOAD_B,
     0x80B670F},
    {0x43454641, /* Fire Emblem (Prototype, iQue) */
     {0x84FF0, 0x85048, 0x850F8, 0x85194, 0x858B0},
     0xFFCC00,
     {0x01, 0x0F, 0x1F, 0x2D, 0x3D},
     FE_PAYLOAD_IQUE,
     0},
};
const size_t ROM_FIRE_EMBLEM_FIX_COUNT =
    sizeof(ROM_FIRE_EMBLEM_FIXES) / sizeof(ROM_FIRE_EMBLEM_FIXES[0]);
