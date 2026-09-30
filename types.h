#pragma once
#include <windows.h>

// Stronghold Crusader Engine & AIV Building Type IDs
// Re-engineered directly from Stronghold Crusader.exe (functions 0x475690, 0x4ecfe0, 0x409370)
enum BuildingType {
    BUILDING_NONE                  = 0,

    // Walls & Stairs (AIV IDs 1..5)
    // NOTE: Crusader has NO wooden walls - only stone walls!
    BUILDING_LOW_WALL              = 1,
    BUILDING_STONE_WALL            = 2,
    BUILDING_CRENELATED_WALL       = 3,
    BUILDING_STONE_WALL_EXT        = 4,
    BUILDING_STAIRS                = 5,

    // Compact Wall & Stair IDs in AIV files (10..19)
    BUILDING_STONE_WALL_ALT        = 10,
    BUILDING_LOW_WALL_ALT          = 11,
    BUILDING_CRENELATED_WALL_ALT   = 12,
    BUILDING_LOW_WALL_ALT2         = 13,
    BUILDING_STAIRS_ALT1           = 14,
    BUILDING_STAIRS_ALT2           = 15,
    BUILDING_STAIRS_ALT3           = 16,
    BUILDING_STAIRS_ALT4           = 17,
    BUILDING_STAIRS_ALT5           = 18,
    BUILDING_STAIRS_ALT6           = 19,

    // Moat & Pit Traps (AIV IDs 20..24)
    BUILDING_MOAT                  = 20,
    BUILDING_MOAT_21               = 21,
    BUILDING_MOAT_22               = 22,
    BUILDING_MOAT_23               = 23,
    BUILDING_PITCH_DITCH           = 24,

    // Towers & Heavy Defenses (AIV IDs 30..37)
    BUILDING_LOOKOUT_TOWER         = 30, // ST 74
    BUILDING_PERIMETER_TURRET      = 31, // ST 75
    BUILDING_DEFENSE_TURRET        = 32, // ST 76
    BUILDING_SQUARE_TOWER          = 33, // ST 77
    BUILDING_ROUND_TOWER           = 34, // ST 78
    BUILDING_OIL_SMELTER           = 35, // ST 28
    BUILDING_DOG_CAGE              = 36, // ST 99
    BUILDING_KILLING_PIT           = 37, // ST 67

    // Keep & Military (AIV IDs 38..44)
    BUILDING_KEEP                  = 38, // ST 41 (Stone Keep)
    BUILDING_MERCENARY_POST        = 39, // Mercenary Post (Arabian Barracks)
    BUILDING_STONE_GATEHOUSE_S     = 40, // Small Stone Gatehouse
    BUILDING_STONE_GATEHOUSE_S_ALT = 41, // Small Stone Gatehouse
    BUILDING_STONE_GATEHOUSE_L     = 42, // Large Stone Gatehouse
    BUILDING_STONE_GATEHOUSE_L_ALT = 43, // Large Stone Gatehouse
    BUILDING_DRAWBRIDGE            = 44, // ST 49

    // Weapon Workshops & Guilds (AIV IDs 50..59)
    BUILDING_POLETURNER            = 50, // ST 14
    BUILDING_FLETCHER              = 51, // ST 12
    BUILDING_BLACKSMITH            = 52, // ST 13
    BUILDING_TANNER                = 53, // ST 16
    BUILDING_ARMORER               = 54, // ST 15
    BUILDING_BARRACKS              = 55, // Barracks (European Barracks)
    BUILDING_ARMORY                = 56, // ST 11
    BUILDING_ENGINEERS_GUILD       = 57, // ST 24
    BUILDING_TUNNELERS_GUILD       = 58, // ST 25
    BUILDING_STABLES               = 59, // ST 35

    // Industry & Resource Gathering (AIV IDs 60..66)
    BUILDING_STOCKPILE             = 60, // ST 10
    BUILDING_WOODCUTTER            = 61, // ST 3
    BUILDING_QUARRY                = 62, // ST 20
    BUILDING_OX_TETHER             = 63, // ST 4
    BUILDING_IRON_MINE             = 64, // ST 5
    BUILDING_PITCH_RIG             = 65, // ST 6
    BUILDING_MARKET                = 66, // ST 26

    // Food, Processing & Community (AIV IDs 70..86)
    BUILDING_GRANARY               = 70, // ST 19
    BUILDING_APPLE_ORCHARD         = 71, // ST 32
    BUILDING_DAIRY_FARM            = 72, // ST 33
    BUILDING_WHEAT_FARM            = 73, // ST 30
    BUILDING_MERCENARY_POST_ALT    = 74, // ST 7
    BUILDING_HOP_FARM              = 75, // ST 31
    BUILDING_MILL                  = 76, // ST 34
    BUILDING_BAKERY                = 77, // ST 17
    BUILDING_BREWERY               = 78, // ST 18
    BUILDING_INN                   = 79, // ST 22
    BUILDING_HOVEL                 = 80, // ST 1
    BUILDING_CHAPEL                = 81, // ST 36
    BUILDING_CHURCH                = 82, // ST 37
    BUILDING_CATHEDRAL             = 83, // ST 38
    BUILDING_APOTHECARY            = 84, // ST 23
    BUILDING_WELL                  = 85, // ST 27
    BUILDING_WATER_POT             = 86, // ST 70

    // Good Things (AIV IDs 90..96)
    BUILDING_MAYPOLE               = 90, // ST 65
    BUILDING_DANCING_BEAR          = 91, // Dancing Bear
    BUILDING_SHRINE                = 92,
    BUILDING_STATUE                = 93,
    BUILDING_TOWN_GARDEN           = 94, // Town Garden
    BUILDING_POND                  = 95,
    BUILDING_BEAR_CAVE             = 96,

    // Bad Things (AIV IDs 100..109)
    BUILDING_GALLOWS               = 100, // ST 62
    BUILDING_STOCKS                = 101,
    BUILDING_WITCH_HOIST           = 102,
    BUILDING_CESSPIT               = 103,
    BUILDING_BURNING_STAKE         = 104,
    BUILDING_GIBBET                = 105,
    BUILDING_DUNGEON               = 106,
    BUILDING_STRETCHING_RACK       = 107,
    BUILDING_FLOGGING_RACK         = 108,
    BUILDING_CHOPPING_BLOCK        = 109,

    // Backward-compatible aliases
    BUILDING_LOW_STONE_WALL        = 1,
    BUILDING_LARGE_GATEHOUSE       = 42,
    BUILDING_SMALL_GATEHOUSE       = 40,
    BUILDING_STONE_GATEHOUSE       = 42,
    BUILDING_WOOD_GATEHOUSE_S      = 40,
    BUILDING_WOOD_GATEHOUSE_L      = 41,
    BUILDING_HUNTER_HUT            = 55,
    BUILDING_GARDEN                = 91,
    BUILDING_POND_LARGE            = 94,

    // Military staging & training yard (open space adjacent to barracks/guilds)
    BUILDING_TRAINING_GROUND       = 250
};

struct BlueprintEntry {
    int dx;          // Offset X from Keep
    int dy;          // Offset Y from Keep
    int buildingId;  // Building type ID
    int step;        // Construction phase
};

// Full building names in English (matching Stronghold Crusader strings)
inline const char* GetBuildingName(int bType) {
    switch (bType) {
        case 1:   return "Low Stone Wall";
        case 2:   return "Stone Wall";
        case 3:   return "Crenelated Wall";
        case 4:   return "Stone Wall";
        case 5:   return "Wall Stairs";
        case 10:  return "Stone Wall";
        case 11:  return "Low Stone Wall";
        case 12:  return "Crenelated Wall";
        case 13:  return "Low Stone Wall";
        case 14:  return "Wall Stairs";
        case 15:  return "Wall Stairs";
        case 16:  return "Wall Stairs";
        case 17:  return "Wall Stairs";
        case 18:  return "Wall Stairs";
        case 19:  return "Wall Stairs";
        case 20:
        case 21:
        case 22:
        case 23:  return "Moat";
        case 24:  return "Pitch Ditch";
        case 30:  return "Lookout Tower";
        case 31:  return "Perimeter Turret";
        case 32:  return "Defense Turret";
        case 33:  return "Square Tower";
        case 34:  return "Round Tower";
        case 35:  return "Oil Smelter";
        case 36:  return "Dog Cage";
        case 37:  return "Killing Pit";
        case 38:  return "Keep";
        case 39:  return "Mercenary Post";
        case 40:  return "Small Stone Gatehouse";
        case 41:  return "Small Stone Gatehouse";
        case 42:  return "Large Stone Gatehouse";
        case 43:  return "Large Stone Gatehouse";
        case 44:  return "Drawbridge";
        case 50:  return "Poleturner";
        case 51:  return "Fletcher";
        case 52:  return "Blacksmith";
        case 53:  return "Tanner";
        case 54:  return "Armorer";
        case 55:  return "Barracks";
        case 56:  return "Armory";
        case 57:  return "Engineers Guild";
        case 58:  return "Tunnelers Guild";
        case 59:  return "Stables";
        case 60:  return "Stockpile";
        case 61:  return "Woodcutter";
        case 62:  return "Quarry";
        case 63:  return "Ox Tether";
        case 64:  return "Iron Mine";
        case 65:  return "Pitch Rig";
        case 66:  return "Marketplace";
        case 70:  return "Granary";
        case 71:  return "Apple Orchard";
        case 72:  return "Dairy Farm";
        case 73:  return "Wheat Farm";
        case 74:  return "Mercenary Post";
        case 75:  return "Hop Farm";
        case 76:  return "Mill";
        case 77:  return "Bakery";
        case 78:  return "Brewery";
        case 79:  return "Inn";
        case 80:  return "Hovel";
        case 81:  return "Chapel";
        case 82:  return "Church";
        case 83:  return "Cathedral";
        case 84:  return "Apothecary";
        case 85:  return "Well";
        case 86:  return "Water Pot";
        case 90:  return "Maypole";
        case 91:  return "Dancing Bear";
        case 92:  return "Shrine";
        case 93:  return "Statue";
        case 94:  return "Town Garden";
        case 95:  return "Pond";
        case 96:  return "Bear Cave";
        case 100: return "Gallows";
        case 101: return "Stocks";
        case 102: return "Witch Hoist";
        case 103: return "Cesspit";
        case 104: return "Burning Stake";
        case 105: return "Gibbet";
        case 106: return "Dungeon";
        case 107: return "Stretching Rack";
        case 108: return "Flogging Rack";
        case 109: return "Chopping Block";
        case 250: return "Training Ground";
        default:  return "Structure";
    }
}

// Short uppercase labels for on-tile display (null = clean outline, no text clutter)
inline const char* GetBuildingShortLabel(int bType) {
    switch (bType) {
        case 5:
        case 14:
        case 15:
        case 16:
        case 17:
        case 18:
        case 19: return "STAIRS";
        case 20:
        case 21:
        case 22:
        case 23: return "MOAT";
        case 24: return "PITCH";
        case 30: return "LOOKOUT";
        case 31: return "TOWER";
        case 32: return "TOWER";
        case 33: return "TOWER";
        case 34: return "TOWER";
        case 35: return "OIL";
        case 36: return "DOGS";
        case 37: return "PIT";
        case 38: return "KEEP";
        case 39: return "MERC";      // Mercenary Post
        case 40:
        case 41: return "S.GATE";    // Small Stone Gatehouse
        case 42:
        case 43: return "L.GATE";    // Large Stone Gatehouse
        case 44: return "BRIDGE";
        case 50: return "SPEARS";
        case 51: return "BOWS";
        case 52: return "SWORDS";
        case 53: return "LEATHER";
        case 54: return "ARMOR";
        case 55: return "BARRACKS";  // Barracks
        case 56: return "ARMORY";
        case 57: return "ENGIN";
        case 58: return "TUNNEL";
        case 59: return "STABLES";
        case 60: return "STOCK";
        case 61: return "WOOD";
        case 62: return "QUARRY";
        case 63: return "OXEN";
        case 64: return "IRON";
        case 65: return "PITCH";
        case 66: return "MARKET";
        case 70: return "GRANARY";
        case 71: return "APPLES";
        case 72: return "DAIRY";
        case 73: return "WHEAT";
        case 74: return "MERC";
        case 75: return "HOPS";
        case 76: return "MILL";
        case 77: return "BAKERY";
        case 78: return "BREWERY";
        case 79: return "INN";
        case 80: return "HOVEL";
        case 81: return "CHAPEL";
        case 82: return "CHURCH";
        case 83: return "CATHEDRAL";
        case 84: return "HEALER";
        case 85: return "WELL";
        case 86: return "WATER";
        case 90: return "MAYPOLE";
        case 91: return "BEAR";      // Dancing Bear
        case 92: return "SHRINE";
        case 93: return "STATUE";
        case 94: return "GARDEN";    // Town Garden
        case 95: return "POND";
        case 100: return "GALLOWS";
        case 250: return "YARD";
        default: return NULL; // Walls & simple outlines (1..4, 10..13) remain clean with no text clutter!
    }
}

// Category colors for blueprint visualization
inline COLORREF GetBuildingColor(int bType) {
    switch (bType) {
        // Walls & Gatehouses & Stairs & Drawbridge: Sky Cyan
        case 1: case 2: case 3: case 4: case 5:
        case 10: case 11: case 12: case 13:
        case 14: case 15: case 16: case 17: case 18: case 19:
        case 40: case 41: case 42: case 43: case 44:
            return RGB(0, 200, 255);

        // Towers: Amber / Gold
        case 30: case 31: case 32: case 33: case 34:
            return RGB(255, 215, 0);

        // Moat & Water: Deep Blue / Aqua
        case 20: case 21: case 22: case 23:
            return RGB(30, 144, 255);

        // Farms & Agriculture: Lush Green
        case 71: case 72: case 73: case 75:
            return RGB(40, 240, 80);

        // Food Processing & Community: Lavender / Violet
        case 70: // Granary
        case 76: // Mill
        case 77: // Bakery
        case 78: // Brewery
        case 79: // Inn
        case 80: // Hovel
        case 81: case 82: case 83: // Religion
            return RGB(200, 130, 255);

        // Heavy Industry & Raw Materials: Deep Orange
        case 60: // Stockpile
        case 61: // Woodcutter
        case 62: // Quarry
        case 63: // Ox
        case 64: // Iron Mine
        case 65: // Pitch Rig
        case 66: // Market
            return RGB(255, 140, 20);

        // Military & Weapon Production: Coral Red
        case 39: // Mercenary Post
        case 55: // Barracks
        case 56: // Armory
        case 74: // Mercenary Post alias
        case 50: case 51: case 52: case 53: case 54: // Workshops
        case 57: case 58: case 59: // Guilds & Stables
            return RGB(255, 75, 75);

        // Training Ground & Military Yard: Warm Orange-Red
        case 250:
            return RGB(255, 120, 60);

        // Traps & Defenses: Neon Orange / Yellow
        case 24: // Killing Pit
        case 35: // Oil Smelter
        case 36: // Dog Cage
        case 37: // Pitch Ditch
        case 85: // Well
        case 86: // Water Pot
            return RGB(255, 185, 30);

        // Bad Things: Dark Crimson
        case 100: case 101: case 102: case 103: case 104:
        case 105: case 106: case 107: case 108: case 109:
            return RGB(220, 50, 50);

        // Good Things: Bright Pink
        case 90: case 91: case 92: case 93: case 94: case 95: case 96:
            return RGB(255, 105, 180);

        default:
            return RGB(200, 200, 200);
    }
}

// Map AIV building ID to Stronghold Crusader internal engine building ID
inline int AivToGameId(int aivId) {
    if (aivId >= 10 && aivId <= 24) {
        switch (aivId) {
            case 10: return 25;  // Stone Wall
            case 11: return 46;  // Low Stone Wall
            case 12: return 26;  // Crenelated Wall
            case 13: return 35;  // Low Wall
            case 14: return 181; // Stairs N
            case 15: return 182; // Stairs E
            case 16: return 183; // Stairs S
            case 17: return 184; // Stairs W
            case 18: return 185; // Stairs 5
            case 19: return 186; // Stairs 6
            case 20: case 21: case 22: case 23: return 106; // Moat
            case 24: return 99;  // Pitch Ditch
        }
    }
    if (aivId >= 30 && aivId <= 109) {
        __try {
            int* table = (int*)0xb461a0;
            return table[aivId];
        } __except(1) {}
    }
    return aivId;
}
