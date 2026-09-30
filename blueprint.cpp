#include "blueprint.h"
#include <stdio.h>
#include <algorithm>

typedef int (__thiscall *EngineLoadAIV_t)(void* thisPtr, void* schema, const char* filename);

// ---------------------------------------------------------------------------
// AI Lord character table - all Crusader / Crusader Extreme lords
// Format: { prefix, display name }
// ---------------------------------------------------------------------------
struct CharacterDef {
    const char* prefix;       // AIV filename prefix (e.g. "snake" -> aiv\snake1.aiv)
    const char* displayName;  // Full name shown in HUD
};

static const CharacterDef s_characters[] = {
    { "rat",       "The Rat"        },
    { "snake",     "The Snake"      },
    { "pig",       "The Pig"        },
    { "wolf",      "The Wolf"       },
    { "saladin",   "Saladin"        },
    { "caliph",    "The Caliph"     },
    { "Sultan",    "The Sultan"     },
    { "richard",   "Richard"        },
    { "frederick", "Frederick"      },
    { "phillip",   "Phillip"        },
    { "wazir",     "The Wazir"      },
    { "Emir",      "The Emir"       },
    { "Nizar",     "Nizar"          },
    { "sheriff",   "The Sheriff"    },
    { "marshal",   "The Marshal"    },
    { "Abbot",     "The Abbot"      },
};
static const int s_charCount = (int)(sizeof(s_characters) / sizeof(s_characters[0]));

BlueprintManager::BlueprintManager()
    : m_enabled(true)
    , m_showAll(true)
    , m_currentStep(1)
    , m_maxStep(1)
    , m_builtCount(0)
    , m_castleName("rat1.aiv")
    , m_charIndex(0)
    , m_castleVariant(1)
    , m_playerKeepX(-1)
    , m_playerKeepY(-1)
{
}

bool BlueprintManager::Initialize() {
    // Load the starting castle (snake #1) via ReloadCurrentAIV
    ReloadCurrentAIV();
    return true;
}

bool BlueprintManager::LoadAIV(const char* filename) {
    void* pAivLoaderObj = (void*)0xf2b3d0;
    void* pSchemaDesc   = (void*)0xb46128;
    EngineLoadAIV_t pfnLoadAIV = (EngineLoadAIV_t)0x475690;
    bool loadedOk = false;

    __try {
        int res = pfnLoadAIV(pAivLoaderObj, pSchemaDesc, filename);
        if (res <= 0) return false;

        unsigned short* pBuildingGrid = (unsigned short*)0x018a5b70;
        unsigned int*   pStepGrid     = (unsigned int*)0x018aa990;

        int keepX = -1, keepY = -1;
        for (int y = 0; y < 100; y++) {
            for (int x = 0; x < 100; x++) {
                int bType = pBuildingGrid[y * 100 + x];
                if (bType == BUILDING_KEEP) {
                    keepX = x;
                    keepY = y;
                    break;
                }
            }
            if (keepX >= 0) break;
        }

        if (keepX < 0) {
            int minX = 100, maxX = 0, minY = 100, maxY = 0;
            int count = 0;
            for (int y = 0; y < 100; y++) {
                for (int x = 0; x < 100; x++) {
                    if (pBuildingGrid[y * 100 + x] != 0) {
                        if (x < minX) minX = x;
                        if (x > maxX) maxX = x;
                        if (y < minY) minY = y;
                        if (y > maxY) maxY = y;
                        count++;
                    }
                }
            }
            if (count > 0) {
                keepX = (minX + maxX) / 2;
                keepY = (minY + maxY) / 2;
            } else {
                return false;
            }
        }

        m_entries.clear();
        m_maxStep = 1;

        for (int y = 0; y < 100; y++) {
            for (int x = 0; x < 100; x++) {
                int idx = y * 100 + x;
                int bType = pBuildingGrid[idx];
                int step  = (int)pStepGrid[idx];

                if (bType != 0 && bType != BUILDING_KEEP) {
                    if (step < 1) step = 1;
                    m_entries.push_back({ x - keepX, y - keepY, bType, step });
                    if (step > m_maxStep) {
                        m_maxStep = step;
                    }
                }
            }
        }
        loadedOk = !m_entries.empty();
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }

    if (!loadedOk) return false;

    PostProcessEntries();
    return true;
}

void BlueprintManager::PostProcessEntries() {
    // Post-process: convert stone wall tiles in/around military recruitment & staging buildings
    // (Barracks, Mercenary Post, Engineers Guild, Tunnelers Guild, Oil Smelter) into Training Ground.
    auto isMilitary = [](int id) {
        return id == BUILDING_BARRACKS           // 55
            || id == BUILDING_MERCENARY_POST     // 39
            || id == BUILDING_MERCENARY_POST_ALT // 74
            || id == BUILDING_ENGINEERS_GUILD    // 57
            || id == BUILDING_TUNNELERS_GUILD    // 58
            || id == BUILDING_OIL_SMELTER;       // 35
    };
    auto isWallTile = [](int id) {
        return (id >= 1 && id <= 4) || (id >= 10 && id <= 13);
    };

    std::vector<BlueprintEntry> militaryEntries;
    for (const auto& e : m_entries) {
        if (isMilitary(e.buildingId)) {
            militaryEntries.push_back(e);
        }
    }

    if (!militaryEntries.empty()) {
        for (auto& e : m_entries) {
            if (isWallTile(e.buildingId)) {
                for (const auto& m : militaryEntries) {
                    if (e.step == m.step) {
                        int dist = (std::max)(std::abs(e.dx - m.dx), std::abs(e.dy - m.dy));
                        if (dist <= 5) {
                            e.buildingId = BUILDING_TRAINING_GROUND;
                            break;
                        }
                    }
                }
            }
        }
    }

    std::sort(m_entries.begin(), m_entries.end(), [](const BlueprintEntry& a, const BlueprintEntry& b) {
        return a.step < b.step;
    });
}

void BlueprintManager::ToggleEnabled() {
    m_enabled = !m_enabled;
}

void BlueprintManager::NextStep() {
    if (m_currentStep < m_maxStep) {
        m_currentStep++;
    }
}

void BlueprintManager::PrevStep() {
    if (m_currentStep > 1) {
        m_currentStep--;
    }
}

void BlueprintManager::ToggleShowAll() {
    m_showAll = !m_showAll;
}

// ---------------------------------------------------------------------------
// Character / Castle variant selection (F3 / F4)
// ---------------------------------------------------------------------------

const char* BlueprintManager::GetCurrentCharacter() const {
    return s_characters[m_charIndex].displayName;
}

void BlueprintManager::NextCharacter() {
    m_charIndex = (m_charIndex + 1) % s_charCount;
    m_castleVariant = 1;   // Reset to variant #1 on character switch
    m_currentStep   = 1;
    ReloadCurrentAIV();
}

void BlueprintManager::PrevCharacter() {
    m_charIndex = (m_charIndex - 1 + s_charCount) % s_charCount;
    m_castleVariant = 1;   // Reset to variant #1 on character switch
    m_currentStep   = 1;
    ReloadCurrentAIV();
}

void BlueprintManager::NextCastleVariant() {
    m_castleVariant = (m_castleVariant % 8) + 1;  // Cycle 1..8
    m_currentStep   = 1;
    ReloadCurrentAIV();
}

void BlueprintManager::PrevCastleVariant() {
    m_castleVariant = m_castleVariant > 1 ? m_castleVariant - 1 : 8;
    m_currentStep   = 1;
    ReloadCurrentAIV();
}

void BlueprintManager::ReloadCurrentAIV() {
    const CharacterDef& c = s_characters[m_charIndex];

    // Build path: "aiv\<prefix><variant>.aiv"
    char path[MAX_PATH];
    sprintf_s(path, "aiv\\%s%d.aiv", c.prefix, m_castleVariant);

    // Build display name shown in HUD
    char name[64];
    sprintf_s(name, "%s #%d", c.displayName, m_castleVariant);
    m_castleName = name;

    LogMessage("[Blueprint] Loading AIV: %s\n", path);

    bool ok = LoadAIV(path);
    if (!ok || m_entries.empty()) {
        // Fallback: single placeholder Keep tile so the overlay still shows something
        m_entries.clear();
        m_entries.push_back({ 0, 0, BUILDING_KEEP, 1 });
        m_maxStep = 1;
        LogMessage("[Blueprint] LoadAIV failed or empty — using placeholder.\n");
    } else {
        LogMessage("[Blueprint] Loaded %d entries, maxStep=%d\n",
                   (int)m_entries.size(), m_maxStep);
    }
}

bool BlueprintManager::IsTileBuilt(int mapX, int mapY, int buildingId) const {
    if (mapX < 0 || mapY < 0 || mapX >= 800 || mapY >= 800) return false;

    __try {
        int* pRowTable = (int*)0x1a93208;
        if (!pRowTable) return false;

        int tileIdx = pRowTable[mapY] + mapX;
        if (tileIdx < 0 || tileIdx >= 800 * 800) return false;

        auto isMilitaryId = [](int t) {
            return t == 55 || t == 87 ||                          // Barracks
                   t == 39 || t == 74 || t == 86 || t == 78 ||    // Mercenary Post
                   t == 57 || t == 88 ||                          // Engineers Guild
                   t == 58 || t == 89 ||                          // Tunnelers Guild
                   t == 35 || t == 180;                           // Oil Smelter
        };

        auto isWall = [](int id) {
            return (id >= 1 && id <= 4) || (id >= 10 && id <= 13) ||
                   id == 25 || id == 35 || id == 46;
        };

        short bldIdx = *(short*)(0x1a93208 + tileIdx * 2 + 0x2029b0);
        if (bldIdx > 0 && bldIdx < 4000) {
            short bType = *(short*)((uintptr_t)0xf98606 + bldIdx * 0x32c);
            if (bType == buildingId) {
                return true;
            }

            int expectedGameId = AivToGameId(buildingId);
            if (bType == expectedGameId) {
                return true;
            }

            // Training ground built if military building or wall is placed here
            if (buildingId == BUILDING_TRAINING_GROUND) {
                if (isMilitaryId(bType) || isWall(bType)) {
                    return true;
                }
            }

            // Wall variations interchangeability
            if (isWall(buildingId) && isWall(bType)) {
                return true;
            }

            // Wall Stairs interchangeability (IDs 5, 14..19, and game IDs 26, 181..186)
            auto isStairs = [](int id) {
                return id == 5 || (id >= 14 && id <= 19) ||
                       id == 26 || (id >= 181 && id <= 186);
            };
            if (isStairs(buildingId) && isStairs(bType)) {
                return true;
            }

            // Small Stone Gatehouse
            auto isSmallGate = [](int id) {
                return id == 40 || id == 41 || id == 144 || id == 145;
            };
            if (isSmallGate(buildingId) && isSmallGate(bType)) {
                return true;
            }

            // Large Stone Gatehouse
            auto isLargeGate = [](int id) {
                return id == 42 || id == 43 || id == 146 || id == 147;
            };
            if (isLargeGate(buildingId) && isLargeGate(bType)) {
                return true;
            }

            // European Barracks
            if ((buildingId == 55 || buildingId == 87) && (bType == 55 || bType == 87)) {
                return true;
            }

            // Arabian Mercenary Post
            if ((buildingId == 39 || buildingId == 86 || buildingId == 74) &&
                (bType == 39 || bType == 86 || bType == 74)) {
                return true;
            }

            // Dancing Bear
            if ((buildingId == 91 || buildingId == 324) && (bType == 91 || bType == 324)) {
                return true;
            }

            // Town Garden
            if ((buildingId == 94 || buildingId == 169) && (bType == 94 || bType == 169)) {
                return true;
            }

            // Pitch Ditch (AIV 24, Game 99)
            if ((buildingId == 24 || buildingId == 99) && (bType == 24 || bType == 99)) {
                return true;
            }

            // Killing Pit (AIV 37, Game 98)
            if ((buildingId == 37 || buildingId == 98) && (bType == 37 || bType == 98)) {
                return true;
            }
        } else if (buildingId == BUILDING_TRAINING_GROUND) {
            // Training ground open space: if the associated military building was placed nearby,
            // the training yard is automatically completed!
            for (int dy = -5; dy <= 5; dy++) {
                int ny = mapY + dy;
                if (ny < 0 || ny >= 800) continue;
                for (int dx = -5; dx <= 5; dx++) {
                    int nx = mapX + dx;
                    if (nx < 0 || nx >= 800) continue;
                    int nIdx = pRowTable[ny] + nx;
                    if (nIdx < 0 || nIdx >= 800 * 800) continue;
                    short nBldIdx = *(short*)(0x1a93208 + nIdx * 2 + 0x2029b0);
                    if (nBldIdx > 0 && nBldIdx < 4000) {
                        short nType = *(short*)((uintptr_t)0xf98606 + nBldIdx * 0x32c);
                        if (isMilitaryId(nType)) {
                            return true;
                        }
                    }
                }
            }
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
    }
    return false;
}

void BlueprintManager::Update() {
    // 1. Find the Player Keep
    m_playerKeepX = -1;
    m_playerKeepY = -1;

    __try {
        // Method 1: Scan building table (base 0xf98600)
        // bType is at 0xf98606 + i * 0x32c
        // TileX is at 0xf98622 + i * 0x32c
        // TileY is at 0xf98624 + i * 0x32c
        for (int i = 1; i < 4000; i++) {
            short bType = *(short*)((uintptr_t)0xf98606 + i * 0x32c);
            if (bType == BUILDING_KEEP) {
                short bX = *(short*)((uintptr_t)0xf98622 + i * 0x32c);
                short bY = *(short*)((uintptr_t)0xf98624 + i * 0x32c);
                if (bX > 0 && bY > 0 && bX < 1000 && bY < 1000) {
                    m_playerKeepX = bX;
                    m_playerKeepY = bY;
                    break;
                }
            }
        }

        // Method 2: Check Player struct keep position (+0x115be90, +0x115be94)
        if (m_playerKeepX < 0) {
            int playerIdx = *(int*)0x1a275dc;
            if (playerIdx >= 0 && playerIdx < 8) {
                short px = *(short*)(0x115be90 + playerIdx * 0x39f4);
                short py = *(short*)(0x115be94 + playerIdx * 0x39f4);
                if (px > 0 && py > 0 && px < 1000 && py < 1000) {
                    m_playerKeepX = px;
                    m_playerKeepY = py;
                }
            }
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
    }

    static int s_lastKeepX = -1;
    static int s_lastKeepY = -1;
    if (m_playerKeepX != s_lastKeepX || m_playerKeepY != s_lastKeepY) {
        s_lastKeepX = m_playerKeepX;
        s_lastKeepY = m_playerKeepY;
        if (HasKeep()) {
            LogMessage("[Blueprint] Player Keep detected at map coordinates: (%d, %d)\n", m_playerKeepX, m_playerKeepY);
        } else {
            LogMessage("[Blueprint] Player Keep cleared / not found.\n");
        }
    }

    // 2. Update count of built structures & auto-advance step
    if (HasKeep()) {
        int built = 0;
        for (const auto& entry : m_entries) {
            if (IsTileBuilt(m_playerKeepX + entry.dx, m_playerKeepY + entry.dy, entry.buildingId)) {
                built++;
            }
        }

        static int s_prevBuilt = -1;
        bool builtChanged = (built != s_prevBuilt);
        s_prevBuilt = built;
        m_builtCount = built;

        // Auto-advance step when current step is finished or when new building placed
        if (builtChanged) {
            while (m_currentStep < m_maxStep) {
                int totalInStep = 0;
                int unbuiltInStep = 0;
                for (const auto& entry : m_entries) {
                    if (entry.step == m_currentStep) {
                        totalInStep++;
                        if (!IsTileBuilt(m_playerKeepX + entry.dx, m_playerKeepY + entry.dy, entry.buildingId)) {
                            unbuiltInStep++;
                        }
                    }
                }
                if (totalInStep > 0 && unbuiltInStep == 0) {
                    m_currentStep++;
                } else if (totalInStep == 0) {
                    m_currentStep++;
                } else {
                    break;
                }
            }
        }
    } else {
        m_builtCount = 0;
    }
}

const char* BlueprintManager::GetCurrentStepBuildingName() const {
    const char* candidate = nullptr;
    for (const auto& entry : m_entries) {
        if (entry.step == m_currentStep) {
            if (entry.buildingId != BUILDING_TRAINING_GROUND) {
                return GetBuildingName(entry.buildingId);
            }
            if (!candidate) {
                candidate = GetBuildingName(entry.buildingId);
            }
        }
    }
    if (candidate) return candidate;
    return "All Complete";
}

int BlueprintManager::GetCurrentStepUnbuiltCount() const {
    if (!HasKeep()) return 0;
    int count = 0;
    for (const auto& entry : m_entries) {
        if (entry.step == m_currentStep) {
            if (!IsTileBuilt(m_playerKeepX + entry.dx, m_playerKeepY + entry.dy, entry.buildingId)) {
                count++;
            }
        }
    }
    return count;
}

int BlueprintManager::GetCurrentStepTotalCount() const {
    int count = 0;
    for (const auto& entry : m_entries) {
        if (entry.step == m_currentStep) {
            count++;
        }
    }
    return count;
}
