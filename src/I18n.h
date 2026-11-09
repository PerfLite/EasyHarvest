#pragma once
#include <string>
#include <string_view>
#include <filesystem>
#include <utility>

namespace EasyHarvest
{
    struct I18nString : std::string
    {
        using std::string::string;
        I18nString() = default;
        I18nString(std::string a_s) : std::string(std::move(a_s)) {}
        I18nString(const char* a_s) : std::string(a_s ? a_s : "") {}
        I18nString& operator=(std::string a_s) { std::string::operator=(std::move(a_s)); return *this; }
        I18nString& operator=(const char* a_s) { std::string::operator=(a_s ? a_s : ""); return *this; }
        operator const char*() const noexcept { return c_str(); }
    };

    struct I18nStrings
    {
        // Menu item titles (left pane)
        I18nString menuSettingsTitle{ "Settings" };
        I18nString menuCategoriesTitle{ "Categories" };
        I18nString menuEquipmentTitle{ "Equipment" };
        I18nString menuLocationsTitle{ "Locations" };
        I18nString menuWhitelistTitle{ "Whitelist" };

        // Settings page
        I18nString settingsHeader{ "EasyHarvest - General Settings" };
        I18nString masterEnabled{ "Master EasyHarvest Enabled" };
        I18nString radius{ "Detection Radius" };
        I18nString interval{ "Scan Frequency" };
        I18nString hotkeyInfo{ "Toggle Hotkey: %s (Scancode: %u)" };
        I18nString valPerWeight{ "Min Value / Weight Ratio" };
        I18nString valPerWeightHelp{ "0 = No filter, 1+ = Item Value >= N or Value/Weight >= N" };

        // Behavior / Disables
        I18nString behaviorHeader{ "Behavior & Safety:" };
        I18nString pauseCombat{ "Pause In Combat" };
        I18nString pauseWeaponDrawn{ "Pause With Weapon Drawn" };
        I18nString pauseBeastForm{ "Pause in Beast / Vampire Lord Form" };
        I18nString stealModeHeader{ "Stealing Protection Mode:" };
        I18nString stealModeNever{ "Never Steal (Owned Items Protected)" };
        I18nString stealModeSneak{ "Steal Only In Sneak When Undetected" };
        I18nString stealModeAlways{ "Steal Always (Loot Crime Items)" };

        // Language
        I18nString langHeader{ "Language / Язык:" };

        // Categories page
        I18nString categoriesHeader{ "EasyHarvest - Harvest Categories" };
        I18nString catToggleAll{ "Toggle All Categories (On / Off)" };
        I18nString btnSelectAll{ "Select All" };
        I18nString btnDeselectAll{ "Deselect All" };
        I18nString catFlora{ "Flora, Plants, Herbs & Mushrooms" };
        I18nString catCritters{ "Critters (Butterflies, Fireflies, Fish)" };
        I18nString catCoins{ "Gold / Septims" };
        I18nString catLockpicks{ "Lockpicks" };
        I18nString catKeys{ "Keys" };
        I18nString catGems{ "Gems (Rubies, Sapphires, Diamonds)" };
        I18nString catIngredients{ "Ingredients" };
        I18nString catPotions{ "Potions & Poisons" };
        I18nString catFood{ "Food & Provisions" };
        I18nString catDrinks{ "Drinks & Alcohol" };
        I18nString catSoulGems{ "Soul Gems" };
        I18nString catSpellbooks{ "Spell Tomes" };
        I18nString catSkillbooks{ "Skill Books" };
        I18nString catBooks{ "Regular Books & Notes" };
        I18nString catScrolls{ "Scrolls" };
        I18nString catOreIngots{ "Ore & Metal Ingots" };
        I18nString catOreVeins{ "Auto-Mine Ore Veins" };
        I18nString catAnimalParts{ "Animal Hides, Pelts & Parts" };
        I18nString catAmmo{ "Ammo (Arrows & Bolts)" };
        I18nString catMisc{ "Miscellaneous & Modded Items (Crafting, Clutter)" };

        // Equipment page
        I18nString equipmentHeader{ "EasyHarvest - Equipment & Valuables" };
        I18nString equipToggleAll{ "Toggle All Equipment (On / Off)" };
        I18nString catJewelry{ "Jewelry (Rings & Amulets)" };
        I18nString catEnchantedWeapons{ "Enchanted Weapons" };
        I18nString catWeapons{ "All Weapons (Including unenchanted)" };
        I18nString catEnchantedArmor{ "Enchanted Armor" };
        I18nString catArmor{ "All Armor & Clothes (Including unenchanted)" };

        // Containers & Bodies
        I18nString containersHeader{ "Containers & Corpses Looting:" };
        I18nString catContainers{ "Loot From Containers (Chests, Urns, Barrels)" };
        I18nString catDeadBodies{ "Loot From Dead Bodies (Corpses of Enemies)" };
        I18nString catAshPiles{ "Loot From Ash Piles (Vampires / Disintegration)" };
        I18nString protectLockedContainers{ "Never Open Locked Containers" };
        I18nString protectBossContainers{ "Protect Boss Chests (Manual Loot Only)" };

        // Locations page
        I18nString locationsHeader{ "EasyHarvest - Location Blacklist" };
        I18nString ignoreHouses{ "Ignore Player Houses Automatically" };
        I18nString currentLocation{ "Current: %s [0x%08X]" };
        I18nString statusBlocked{ "Status: [BLOCKED IN EXCLUSIONS]" };
        I18nString statusAllowed{ "Status: [ALLOWED]" };
        I18nString btnBlock{ "Block This Location" };
        I18nString btnUnblock{ "Unblock This Location" };
        I18nString excludedHeader{ "Excluded Locations (%zu):" };
        I18nString btnRemove{ "Remove##%u" };
        I18nString locOpenWorld{ "Current location: Open World" };
        I18nString locInteriorOnly{ "(Blocking is only available inside buildings and interiors)" };
        I18nString locBuilding{ "Building / Room" };
        I18nString locInterior{ "Interior" };
        I18nString locEmpty{ "Location blacklist is empty. You can block your current location with the button above." };

        // Whitelist page
        I18nString whitelistHeader{ "EasyHarvest - Custom Item Whitelist" };
        I18nString whitelistDesc{ "Items in this list will ALWAYS be looted, bypassing all category and value filters." };
        I18nString invItemsHeader{ "Add from player inventory:" };
        I18nString btnAddWhitelist{ "+ Whitelist##%08X" };
        I18nString activeWhitelistHeader{ "Active Whitelist (%zu items):" };
        I18nString btnRemoveWhitelist{ "Remove##WL_%08X" };
        I18nString searchLabel{ "Search:" };
        I18nString searchHint{ "Item name..." };
        I18nString btnClear{ "Clear##Clear" };
        I18nString searchNoMatch{ "- No items match your search -" };
        I18nString invNoItems{ "- No new items in inventory to add -" };
        I18nString btnPrevPage{ "< Prev##InvPrev" };
        I18nString btnNextPage{ "Next >##InvNext" };
        I18nString pageFormat{ "Page %zu of %zu" };
        I18nString txtPrevDisabled{ "< Prev" };
        I18nString txtNextDisabled{ "Next >" };
        I18nString whitelistEmpty{ "Whitelist is empty. Select items from inventory above to add them." };

        // Notifications
        I18nString notifyEnabled{ "EasyHarvest: Enabled" };
        I18nString notifyDisabled{ "EasyHarvest: Disabled" };
    };

    class I18n
    {
    public:
        static I18n& GetSingleton();

        void Load(std::string_view a_override = "");
        const I18nStrings& Strings() const { return m_strings; }
        const std::string& GetCurrentSuffix() const { return m_currentSuffix; }

    private:
        I18n() = default;
        ~I18n() = default;

        std::string DetectLanguageSuffix();
        void LoadFromFile(const std::filesystem::path& a_path);
        void SetRussianDefaults();
        void SetEnglishDefaults();

        I18nStrings m_strings;
        std::string m_currentSuffix{ "EN" };
    };

    inline const I18nStrings& Tr()
    {
        return I18n::GetSingleton().Strings();
    }
}
