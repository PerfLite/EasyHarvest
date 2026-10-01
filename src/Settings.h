#pragma once

#include <string>
#include <cstdint>
#include <vector>
#include <filesystem>
#include <fstream>
#include <sstream>

namespace EasyHarvest
{
    struct ExcludedLocation
    {
        uint32_t formID{ 0 };
        std::string name;
    };

    struct WhitelistItem
    {
        uint32_t formID{ 0 };
        std::string name;
    };

    struct Config
    {
        bool enabled{ true };
        float radius{ 600.0f };
        float interval{ 0.5f };

        // Value / Weight threshold (0.0 = disabled)
        float minValPerWeight{ 0.0f };

        // Detailed Categories (AutoHarvestSE parity)
        bool harvestFlora{ true };
        bool harvestCritters{ true };
        bool harvestCoins{ true };
        bool harvestLockpicks{ true };
        bool harvestKeys{ true };
        bool harvestGems{ true };
        bool harvestIngredients{ true };
        bool harvestPotions{ true };
        bool harvestFood{ true };
        bool harvestDrinks{ true };
        bool harvestSoulGems{ true };
        bool harvestSpellbooks{ true };
        bool harvestSkillbooks{ true };
        bool harvestBooks{ false }; // Disabled by default per user request
        bool harvestScrolls{ true };
        bool harvestOreIngots{ true };
        bool harvestOreVeins{ true };
        bool harvestAnimalParts{ true };
        bool harvestAmmo{ true };
        bool harvestMisc{ false }; // Disabled by default per user request

        // Equipment & Valuables
        bool harvestJewelry{ true };
        bool harvestEnchantedWeapons{ true };
        bool harvestWeapons{ true };
        bool harvestEnchantedArmor{ true };
        bool harvestArmor{ true };

        // Containers & Dead bodies
        bool harvestContainers{ true };
        bool protectLockedContainers{ true };
        bool protectBossContainers{ false }; // Disabled by default per user request (looted once unlocked)
        bool harvestDeadBodies{ true };
        bool harvestAshPiles{ true };

        // Behavior & Disables
        bool pauseInCombat{ false };
        bool pauseWeaponDrawn{ false };
        uint32_t stealingMode{ 0 }; // 0 = Never steal, 1 = Sneak only (if undetected), 2 = Steal always

        // Quick toggle hotkey (Default: 34 = 'G')
        uint32_t toggleKey{ 34 };
        std::string toggleKeyName{ "G" };

        // Language: "auto", "ru", "en"
        std::string language{ "auto" };

        // Locations & Player Houses
        bool ignorePlayerHouses{ true };
        std::vector<ExcludedLocation> excludedLocations;

        // Custom Whitelist (always looted regardless of other filters)
        std::vector<WhitelistItem> customWhitelist;

        bool IsWhitelisted(uint32_t a_formID) const
        {
            if (a_formID == 0) return false;
            for (const auto& item : customWhitelist) {
                if (item.formID == a_formID) return true;
                // For modded items (high byte != 0x00), match if local 24-bit ID matches (handling load order shifts)
                if ((item.formID >> 24) != 0x00 && (a_formID >> 24) != 0x00) {
                    if ((item.formID & 0x00FFFFFF) == (a_formID & 0x00FFFFFF)) {
                        return true;
                    }
                }
            }
            return false;
        }
    };

    class Settings
    {
    public:
        static Settings& GetSingleton()
        {
            static Settings instance;
            return instance;
        }

        Config& GetConfig() { return config_; }
        const Config& GetConfig() const { return config_; }

        void Load();
        void Save();

        bool IsLocationExcluded(uint32_t a_formID) const;
        void AddExcludedLocation(uint32_t a_formID, const std::string& a_name);
        void RemoveExcludedLocation(uint32_t a_formID);

        bool IsItemWhitelisted(uint32_t a_formID) const;
        void AddWhitelistedItem(uint32_t a_formID, const std::string& a_name);
        void RemoveWhitelistedItem(uint32_t a_formID);

        std::string ToJson() const;
        void FromJson(const std::string& a_json);

    private:
        Settings() = default;
        Config config_;
        std::filesystem::path configPath_;
    };
}
