#pragma once
#include <windows.h>
#include <vector>
#include <string>
#include "types.h"

void LogMessage(const char* fmt, ...);

class BlueprintManager {
public:
    static BlueprintManager& Instance() {
        static BlueprintManager instance;
        return instance;
    }

    bool Initialize();
    void Update();

    // Hotkey actions
    void ToggleEnabled();
    void NextStep();
    void PrevStep();
    void ToggleShowAll();

    // F3: cycle through AI lords (snake, rat, pig, wolf, ...)
    void NextCharacter();
    void PrevCharacter();
    // F4: cycle through castle variants (1..8)
    void NextCastleVariant();
    void PrevCastleVariant();

    // State
    bool IsEnabled() const { return m_enabled; }
    int  GetCurrentStep() const { return m_currentStep; }
    int  GetMaxStep() const { return m_maxStep; }
    bool IsShowAll() const { return m_showAll; }
    const std::string& GetCastleName() const { return m_castleName; }

    // Current character / variant
    const char* GetCurrentCharacter() const;
    int  GetCurrentVariant() const { return m_castleVariant; }

    int GetPlayerKeepX() const { return m_playerKeepX; }
    int GetPlayerKeepY() const { return m_playerKeepY; }
    bool HasKeep() const { return m_playerKeepX >= 0 && m_playerKeepY >= 0; }

    const std::vector<BlueprintEntry>& GetEntries() const { return m_entries; }
    int GetBuiltCount() const { return m_builtCount; }
    int GetTotalCount() const { return (int)m_entries.size(); }

    bool IsTileBuilt(int mapX, int mapY, int buildingId) const;

    const char* GetCurrentStepBuildingName() const;
    int  GetCurrentStepUnbuiltCount() const;
    int  GetCurrentStepTotalCount() const;

private:
    BlueprintManager();
    bool LoadAIV(const char* filename);
    void ReloadCurrentAIV();   // Reload based on m_charIndex + m_castleVariant
    void PostProcessEntries();

    bool m_enabled;
    bool m_showAll;
    int  m_currentStep;
    int  m_maxStep;
    int  m_builtCount;
    std::string m_castleName;

    // Character / variant selection
    int m_charIndex;      // Index into s_characters[]
    int m_castleVariant;  // 1..8

    int m_playerKeepX;
    int m_playerKeepY;

    std::vector<BlueprintEntry> m_entries;
};
