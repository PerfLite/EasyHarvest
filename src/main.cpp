#include "pch.h"
#include "Settings.h"
#include "HarvestManager.h"
#include "Localization.h"
#include "SKSEMenuFramework_API.h"
#include <atomic>
#include <fstream>

namespace
{
    static void Dbg(std::string_view a_msg)
    {
        try {
            std::ofstream f(R"(C:\Users\Alik\Documents\My Games\Skyrim Special Edition\SKSE\EasyHarvest.log)", std::ios::app);
            f << a_msg << "\n";
        } catch (...) {
        }
    }

    static std::atomic<bool> g_quit{ false };
    using PlayerCharacter_Update_t = void (*)(RE::PlayerCharacter*, float);
    static PlayerCharacter_Update_t _originalPlayerCharacterUpdate = nullptr;
    static float s_timer = 0.0f;

    void PlayerCharacter_Update(RE::PlayerCharacter* a_this, float a_delta)
    {
        if (_originalPlayerCharacterUpdate) {
            _originalPlayerCharacterUpdate(a_this, a_delta);
        }

        s_timer += a_delta;
        const auto& cfg = EasyHarvest::Settings::GetSingleton().GetConfig();
        if (s_timer >= cfg.interval) {
            s_timer = 0.0f;
            EasyHarvest::HarvestManager::GetSingleton().Tick();
        }
    }

    // SKSE Menu Framework: Page 1 - General Settings & Behavior
    void RenderEasyHarvestSettings()
    {
        auto& settings = EasyHarvest::Settings::GetSingleton();
        auto& cfg = settings.GetConfig();
        const auto& str = EasyHarvest::Localization::Get(cfg.language);
        bool changed = false;

        SKSEMenuFramework::igText(str.settingsHeader);
        SKSEMenuFramework::igSeparator();

        // Language Selector
        SKSEMenuFramework::igText(str.langHeader);
        SKSEMenuFramework::igSameLine(0.0f, 10.0f);
        if (SKSEMenuFramework::igButton(cfg.language == "auto" ? "[Auto]" : "Auto", { 0.0f, 0.0f })) {
            cfg.language = "auto";
            changed = true;
        }
        SKSEMenuFramework::igSameLine(0.0f, 5.0f);
        if (SKSEMenuFramework::igButton(cfg.language == "en" ? "[English]" : "English", { 0.0f, 0.0f })) {
            cfg.language = "en";
            changed = true;
        }
        SKSEMenuFramework::igSameLine(0.0f, 5.0f);
        if (SKSEMenuFramework::igButton(cfg.language == "ru" ? "[\xd0\xa0\xd1\x83\xd1\x81\xd1\x81\xd0\xba\xd0\xb8\xd0\xb9]" : "\xd0\xa0\xd1\x83\xd1\x81\xd1\x81\xd0\xba\xd0\xb8\xd0\xb9", { 0.0f, 0.0f })) {
            cfg.language = "ru";
            changed = true;
        }

        SKSEMenuFramework::igSeparator();
        if (SKSEMenuFramework::igCheckbox(str.masterEnabled, &cfg.enabled)) changed = true;
        if (SKSEMenuFramework::igSliderFloat(str.radius, &cfg.radius, 100.0f, 1500.0f, "%.0f", 0)) changed = true;
        if (SKSEMenuFramework::igSliderFloat(str.interval, &cfg.interval, 0.2f, 2.0f, "%.1f sec", 0)) changed = true;

        char keyBuf[128];
        snprintf(keyBuf, sizeof(keyBuf), str.hotkeyInfo, cfg.toggleKeyName.c_str(), cfg.toggleKey);
        SKSEMenuFramework::igText(keyBuf);

        SKSEMenuFramework::igSeparator();
        // Price / Weight ratio filter
        if (SKSEMenuFramework::igSliderFloat(str.valPerWeight, &cfg.minValPerWeight, 0.0f, 50.0f, "%.0f", 0)) changed = true;
        SKSEMenuFramework::igText(str.valPerWeightHelp);

        SKSEMenuFramework::igSeparator();
        SKSEMenuFramework::igText(str.behaviorHeader);
        if (SKSEMenuFramework::igCheckbox(str.pauseCombat, &cfg.pauseInCombat)) changed = true;
        if (SKSEMenuFramework::igCheckbox(str.pauseWeaponDrawn, &cfg.pauseWeaponDrawn)) changed = true;

        SKSEMenuFramework::igSeparator();
        SKSEMenuFramework::igText(str.stealModeHeader);
        bool mode0 = (cfg.stealingMode == 0);
        if (SKSEMenuFramework::igCheckbox(str.stealModeNever, &mode0)) {
            if (mode0) {
                cfg.stealingMode = 0;
                changed = true;
            }
        }
        bool mode1 = (cfg.stealingMode == 1);
        if (SKSEMenuFramework::igCheckbox(str.stealModeSneak, &mode1)) {
            if (mode1) {
                cfg.stealingMode = 1;
                changed = true;
            }
        }
        bool mode2 = (cfg.stealingMode == 2);
        if (SKSEMenuFramework::igCheckbox(str.stealModeAlways, &mode2)) {
            if (mode2) {
                cfg.stealingMode = 2;
                changed = true;
            }
        }

        if (changed) {
            settings.Save();
        }
    }

    // SKSE Menu Framework: Page 2 - Categories
    void RenderEasyHarvestCategories()
    {
        auto& settings = EasyHarvest::Settings::GetSingleton();
        auto& cfg = settings.GetConfig();
        const auto& str = EasyHarvest::Localization::Get(cfg.language);
        bool changed = false;

        SKSEMenuFramework::igText(str.categoriesHeader);
        SKSEMenuFramework::igSeparator();

        bool allCategoriesEnabled = 
            cfg.harvestFlora && cfg.harvestCritters && cfg.harvestCoins && 
            cfg.harvestLockpicks && cfg.harvestKeys && cfg.harvestGems && cfg.harvestIngredients && 
            cfg.harvestPotions && cfg.harvestFood && cfg.harvestDrinks && 
            cfg.harvestSoulGems && cfg.harvestSpellbooks && cfg.harvestSkillbooks && 
            cfg.harvestScrolls && cfg.harvestOreIngots && cfg.harvestOreVeins && cfg.harvestAnimalParts && 
            cfg.harvestAmmo && cfg.harvestContainers && cfg.harvestDeadBodies && 
            cfg.harvestAshPiles && cfg.harvestJewelry && cfg.harvestEnchantedWeapons && 
            cfg.harvestWeapons && cfg.harvestEnchantedArmor && cfg.harvestArmor;

        bool toggleAll = allCategoriesEnabled;
        if (SKSEMenuFramework::igCheckbox(str.catToggleAll, &toggleAll)) {
            bool target = toggleAll;
            cfg.harvestFlora = target;
            cfg.harvestCritters = target;
            cfg.harvestCoins = target;
            cfg.harvestLockpicks = target;
            cfg.harvestKeys = target;
            cfg.harvestGems = target;
            cfg.harvestIngredients = target;
            cfg.harvestPotions = target;
            cfg.harvestFood = target;
            cfg.harvestDrinks = target;
            cfg.harvestSoulGems = target;
            cfg.harvestSpellbooks = target;
            cfg.harvestSkillbooks = target;
            cfg.harvestBooks = false; // Always keep simple books disabled by default
            cfg.harvestScrolls = target;
            cfg.harvestOreIngots = target;
            cfg.harvestOreVeins = target;
            cfg.harvestAnimalParts = target;
            cfg.harvestAmmo = target;
            cfg.harvestMisc = false; // Always keep misc disabled by default per user request
            cfg.harvestContainers = target;
            cfg.harvestDeadBodies = target;
            cfg.harvestAshPiles = target;
            cfg.harvestJewelry = target;
            cfg.harvestEnchantedWeapons = target;
            cfg.harvestWeapons = target;
            cfg.harvestEnchantedArmor = target;
            cfg.harvestArmor = target;
            changed = true;
        }

        SKSEMenuFramework::igSameLine(0.0f, 15.0f);
        if (SKSEMenuFramework::igButton(str.btnSelectAll, { 0.0f, 0.0f })) {
            cfg.harvestFlora = true;
            cfg.harvestCritters = true;
            cfg.harvestCoins = true;
            cfg.harvestLockpicks = true;
            cfg.harvestKeys = true;
            cfg.harvestGems = true;
            cfg.harvestIngredients = true;
            cfg.harvestPotions = true;
            cfg.harvestFood = true;
            cfg.harvestDrinks = true;
            cfg.harvestSoulGems = true;
            cfg.harvestSpellbooks = true;
            cfg.harvestSkillbooks = true;
            cfg.harvestBooks = false;
            cfg.harvestScrolls = true;
            cfg.harvestOreIngots = true;
            cfg.harvestOreVeins = true;
            cfg.harvestAnimalParts = true;
            cfg.harvestAmmo = true;
            cfg.harvestMisc = false; // Always keep misc disabled by default per user request
            cfg.harvestContainers = true;
            cfg.harvestDeadBodies = true;
            cfg.harvestAshPiles = true;
            cfg.harvestJewelry = true;
            cfg.harvestEnchantedWeapons = true;
            cfg.harvestWeapons = true;
            cfg.harvestEnchantedArmor = true;
            cfg.harvestArmor = true;
            changed = true;
        }
        SKSEMenuFramework::igSameLine(0.0f, 5.0f);
        if (SKSEMenuFramework::igButton(str.btnDeselectAll, { 0.0f, 0.0f })) {
            cfg.harvestFlora = false;
            cfg.harvestCritters = false;
            cfg.harvestCoins = false;
            cfg.harvestLockpicks = false;
            cfg.harvestKeys = false;
            cfg.harvestGems = false;
            cfg.harvestIngredients = false;
            cfg.harvestPotions = false;
            cfg.harvestFood = false;
            cfg.harvestDrinks = false;
            cfg.harvestSoulGems = false;
            cfg.harvestSpellbooks = false;
            cfg.harvestSkillbooks = false;
            cfg.harvestBooks = false;
            cfg.harvestScrolls = false;
            cfg.harvestOreIngots = false;
            cfg.harvestOreVeins = false;
            cfg.harvestAnimalParts = false;
            cfg.harvestAmmo = false;
            cfg.harvestMisc = false;
            cfg.harvestContainers = false;
            cfg.harvestDeadBodies = false;
            cfg.harvestAshPiles = false;
            cfg.harvestJewelry = false;
            cfg.harvestEnchantedWeapons = false;
            cfg.harvestWeapons = false;
            cfg.harvestEnchantedArmor = false;
            cfg.harvestArmor = false;
            changed = true;
        }
        SKSEMenuFramework::igSeparator();

        if (SKSEMenuFramework::igCheckbox(str.catFlora, &cfg.harvestFlora)) changed = true;
        if (SKSEMenuFramework::igCheckbox(str.catCritters, &cfg.harvestCritters)) changed = true;
        if (SKSEMenuFramework::igCheckbox(str.catCoins, &cfg.harvestCoins)) changed = true;
        if (SKSEMenuFramework::igCheckbox(str.catLockpicks, &cfg.harvestLockpicks)) changed = true;
        if (SKSEMenuFramework::igCheckbox(str.catKeys, &cfg.harvestKeys)) changed = true;
        if (SKSEMenuFramework::igCheckbox(str.catGems, &cfg.harvestGems)) changed = true;
        if (SKSEMenuFramework::igCheckbox(str.catIngredients, &cfg.harvestIngredients)) changed = true;
        if (SKSEMenuFramework::igCheckbox(str.catPotions, &cfg.harvestPotions)) changed = true;
        if (SKSEMenuFramework::igCheckbox(str.catFood, &cfg.harvestFood)) changed = true;
        if (SKSEMenuFramework::igCheckbox(str.catDrinks, &cfg.harvestDrinks)) changed = true;
        if (SKSEMenuFramework::igCheckbox(str.catSoulGems, &cfg.harvestSoulGems)) changed = true;
        if (SKSEMenuFramework::igCheckbox(str.catSpellbooks, &cfg.harvestSpellbooks)) changed = true;
        if (SKSEMenuFramework::igCheckbox(str.catSkillbooks, &cfg.harvestSkillbooks)) changed = true;
        if (SKSEMenuFramework::igCheckbox(str.catBooks, &cfg.harvestBooks)) changed = true;
        if (SKSEMenuFramework::igCheckbox(str.catScrolls, &cfg.harvestScrolls)) changed = true;
        if (SKSEMenuFramework::igCheckbox(str.catOreIngots, &cfg.harvestOreIngots)) changed = true;
        if (SKSEMenuFramework::igCheckbox(str.catOreVeins, &cfg.harvestOreVeins)) changed = true;
        if (SKSEMenuFramework::igCheckbox(str.catAnimalParts, &cfg.harvestAnimalParts)) changed = true;
        if (SKSEMenuFramework::igCheckbox(str.catAmmo, &cfg.harvestAmmo)) changed = true;
        if (SKSEMenuFramework::igCheckbox(str.catMisc, &cfg.harvestMisc)) changed = true;

        SKSEMenuFramework::igSeparator();
        SKSEMenuFramework::igText(str.containersHeader);
        if (SKSEMenuFramework::igCheckbox(str.catContainers, &cfg.harvestContainers)) changed = true;
        if (SKSEMenuFramework::igCheckbox(str.protectLockedContainers, &cfg.protectLockedContainers)) changed = true;
        if (SKSEMenuFramework::igCheckbox(str.protectBossContainers, &cfg.protectBossContainers)) changed = true;
        if (SKSEMenuFramework::igCheckbox(str.catDeadBodies, &cfg.harvestDeadBodies)) changed = true;
        if (SKSEMenuFramework::igCheckbox(str.catAshPiles, &cfg.harvestAshPiles)) changed = true;

        if (changed) {
            settings.Save();
        }
    }

    // SKSE Menu Framework: Page 3 - Equipment & Valuables
    void RenderEasyHarvestEquipment()
    {
        auto& settings = EasyHarvest::Settings::GetSingleton();
        auto& cfg = settings.GetConfig();
        const auto& str = EasyHarvest::Localization::Get(cfg.language);
        bool changed = false;

        SKSEMenuFramework::igText(str.equipmentHeader);
        SKSEMenuFramework::igSeparator();

        bool allEquipEnabled = cfg.harvestJewelry && cfg.harvestEnchantedWeapons && 
                               cfg.harvestWeapons && cfg.harvestEnchantedArmor && cfg.harvestArmor;
        bool toggleEquip = allEquipEnabled;
        if (SKSEMenuFramework::igCheckbox(str.equipToggleAll, &toggleEquip)) {
            bool target = toggleEquip;
            cfg.harvestJewelry = target;
            cfg.harvestEnchantedWeapons = target;
            cfg.harvestWeapons = target;
            cfg.harvestEnchantedArmor = target;
            cfg.harvestArmor = target;
            changed = true;
        }
        SKSEMenuFramework::igSeparator();

        if (SKSEMenuFramework::igCheckbox(str.catJewelry, &cfg.harvestJewelry)) changed = true;
        if (SKSEMenuFramework::igCheckbox(str.catEnchantedWeapons, &cfg.harvestEnchantedWeapons)) changed = true;
        if (SKSEMenuFramework::igCheckbox(str.catWeapons, &cfg.harvestWeapons)) changed = true;
        if (SKSEMenuFramework::igCheckbox(str.catEnchantedArmor, &cfg.harvestEnchantedArmor)) changed = true;
        if (SKSEMenuFramework::igCheckbox(str.catArmor, &cfg.harvestArmor)) changed = true;

        if (changed) {
            settings.Save();
        }
    }

    // SKSE Menu Framework: Page 3 - Locations
    void RenderEasyHarvestLocations()
    {
        auto& settings = EasyHarvest::Settings::GetSingleton();
        auto& cfg = settings.GetConfig();
        const auto& str = EasyHarvest::Localization::Get(cfg.language);
        bool changed = false;

        SKSEMenuFramework::igText(str.locationsHeader);
        SKSEMenuFramework::igSeparator();

        if (SKSEMenuFramework::igCheckbox(str.ignoreHouses, &cfg.ignorePlayerHouses)) {
            changed = true;
        }

        SKSEMenuFramework::igSeparator();
        auto* player = RE::PlayerCharacter::GetSingleton();
        if (player) {
            auto* cell = player->GetParentCell();
            bool isInterior = cell && cell->IsInteriorCell();

            if (!isInterior) {
                SKSEMenuFramework::igText(cfg.language == "ru" ? 
                    "Текущая локация: Открытый мир" : 
                    "Current location: Open World");
                SKSEMenuFramework::igText(cfg.language == "ru" ? 
                    "(Блокировка сбора доступна только внутри зданий и помещений)" : 
                    "(Blocking is only available inside buildings and interiors)");
            } else {
                auto* loc = player->GetCurrentLocation();
                if (!loc && cell) loc = cell->GetLocation();

                uint32_t formID = 0;
                std::string locName;
                if (loc) {
                    formID = loc->GetFormID();
                    const char* n = loc->GetName();
                    if (n && n[0]) locName = n;
                }
                if (locName.empty() && cell) {
                    if (formID == 0) formID = cell->GetFormID();
                    const char* n = cell->GetName();
                    if (n && n[0]) locName = n;
                }
                if (locName.empty()) {
                    locName = formID != 0 ? "Здание / Помещение" : "Интерьер";
                }

                char locBuf[128];
                snprintf(locBuf, sizeof(locBuf), str.currentLocation, locName.c_str(), formID);
                SKSEMenuFramework::igText(locBuf);

                bool isBlocked = settings.IsLocationExcluded(formID);
                if (isBlocked) {
                    SKSEMenuFramework::igText(str.statusBlocked);
                    if (SKSEMenuFramework::igButton(str.btnUnblock, { 0.0f, 0.0f })) {
                        settings.RemoveExcludedLocation(formID);
                        changed = true;
                    }
                } else {
                    SKSEMenuFramework::igText(str.statusAllowed);
                    if (SKSEMenuFramework::igButton(str.btnBlock, { 0.0f, 0.0f })) {
                        settings.AddExcludedLocation(formID, locName);
                        changed = true;
                    }
                }
            }
        }

        SKSEMenuFramework::igSeparator();
        char countBuf[64];
        snprintf(countBuf, sizeof(countBuf), str.excludedHeader, cfg.excludedLocations.size());
        SKSEMenuFramework::igText(countBuf);

        uint32_t toRemove = 0;
        for (const auto& item : cfg.excludedLocations) {
            char itemBuf[128];
            snprintf(itemBuf, sizeof(itemBuf), "- %s [0x%08X]", item.name.c_str(), item.formID);
            SKSEMenuFramework::igText(itemBuf);
            SKSEMenuFramework::igSameLine(0.0f, 15.0f);
            char btnBuf[32];
            snprintf(btnBuf, sizeof(btnBuf), str.btnRemove, item.formID);
            if (SKSEMenuFramework::igButton(btnBuf, { 0.0f, 0.0f })) {
                toRemove = item.formID;
            }
        }

        if (toRemove != 0) {
            settings.RemoveExcludedLocation(toRemove);
            changed = true;
        }

        if (changed) {
            settings.Save();
        }
    }

    static std::string ToLowerUtf8(std::string_view a_str)
    {
        std::string result;
        result.reserve(a_str.size());
        for (size_t i = 0; i < a_str.size(); ++i) {
            unsigned char c = static_cast<unsigned char>(a_str[i]);
            if (c >= 'A' && c <= 'Z') {
                result.push_back(static_cast<char>(c + 32));
            } else if (c == 0xD0 && i + 1 < a_str.size()) {
                unsigned char c2 = static_cast<unsigned char>(a_str[i + 1]);
                if (c2 >= 0x90 && c2 <= 0x9F) { // А - П -> а - п
                    result.push_back(static_cast<char>(0xD0));
                    result.push_back(static_cast<char>(c2 + 0x20));
                    i++;
                } else if (c2 >= 0xA0 && c2 <= 0xAF) { // Р - Я -> р - я
                    result.push_back(static_cast<char>(0xD1));
                    result.push_back(static_cast<char>(c2 - 0x20));
                    i++;
                } else if (c2 == 0x81) { // Ё -> ё
                    result.push_back(static_cast<char>(0xD1));
                    result.push_back(static_cast<char>(0x91));
                    i++;
                } else {
                    result.push_back(static_cast<char>(c));
                }
            } else {
                result.push_back(static_cast<char>(c));
            }
        }
        return result;
    }

    // SKSE Menu Framework: Page 5 - Custom Whitelist
    void RenderEasyHarvestWhitelist()
    {
        auto& settings = EasyHarvest::Settings::GetSingleton();
        auto& cfg = settings.GetConfig();
        const auto& str = EasyHarvest::Localization::Get(cfg.language);
        bool changed = false;

        // Title removed per user request: only keep description
        SKSEMenuFramework::igText(str.whitelistDesc);
        SKSEMenuFramework::igSeparator();

        // Search bar
        static char s_searchBuf[64] = "";
        static std::string s_lastSearch = "";
        static size_t s_invPage = 0;

        SKSEMenuFramework::igText(cfg.language == "ru" ? "Поиск:" : "Search:");
        SKSEMenuFramework::igSameLine(0.0f, 8.0f);
        if (SKSEMenuFramework::igSetNextItemWidth) {
            SKSEMenuFramework::igSetNextItemWidth(300.0f);
        }
        if (SKSEMenuFramework::igInputTextWithHint) {
            SKSEMenuFramework::igInputTextWithHint("##InvSearch", 
                cfg.language == "ru" ? "Название предмета..." : "Item name...", 
                s_searchBuf, sizeof(s_searchBuf), 0, nullptr, nullptr);
        } else if (SKSEMenuFramework::igInputText) {
            SKSEMenuFramework::igInputText("##InvSearch", s_searchBuf, sizeof(s_searchBuf), 0, nullptr, nullptr);
        }

        if (s_searchBuf[0] != '\0') {
            SKSEMenuFramework::igSameLine(0.0f, 8.0f);
            if (SKSEMenuFramework::igButton(cfg.language == "ru" ? "Сброс##Clear" : "Clear##Clear", { 0.0f, 0.0f })) {
                s_searchBuf[0] = '\0';
            }
        }

        if (s_lastSearch != s_searchBuf) {
            s_lastSearch = s_searchBuf;
            s_invPage = 0;
        }

        SKSEMenuFramework::igSeparator();

        // 1. Add from current player inventory
        auto* player = RE::PlayerCharacter::GetSingleton();
        if (player) {
            SKSEMenuFramework::igText(str.invItemsHeader);

            struct CandidateItem {
                uint32_t formID;
                std::string name;
                bool isModded;
            };
            std::vector<CandidateItem> candidates;

            std::string searchLower = ToLowerUtf8(s_searchBuf);

            auto inv = player->GetInventory();
            for (const auto& [item, itemData] : inv) {
                if (!item || itemData.first <= 0) continue;
                uint32_t formID = item->GetFormID();
                // Skip gold and lockpicks (already covered by dedicated categories)
                if (formID == 0x0000000F || formID == 0x0000000A) continue;
                if (settings.IsItemWhitelisted(formID)) continue;

                const char* rawName = item->GetName();
                if (!rawName || !rawName[0]) continue;

                if (!searchLower.empty()) {
                    std::string itemLower = ToLowerUtf8(rawName);
                    if (itemLower.find(searchLower) == std::string::npos) {
                        continue;
                    }
                }

                bool isModded = ((formID >> 24) != 0x00);
                candidates.push_back({ formID, rawName, isModded });
            }

            // Sort: Modded items first (so bounty heads, tokens, mod items are always on Page 1!), then alphabetical
            std::sort(candidates.begin(), candidates.end(), [](const CandidateItem& a, const CandidateItem& b) {
                if (a.isModded != b.isModded) {
                    return a.isModded > b.isModded;
                }
                return a.name < b.name;
            });

            if (candidates.empty()) {
                if (s_searchBuf[0] != '\0') {
                    SKSEMenuFramework::igText(cfg.language == "ru" ? 
                        "- Ничего не найдено по запросу -" : 
                        "- No items match your search -");
                } else {
                    SKSEMenuFramework::igText(cfg.language == "ru" ? 
                        "- Нет новых предметов в инвентаре для добавления -" : 
                        "- No new items in inventory to add -");
                }
            } else {
                constexpr size_t PAGE_SIZE = 6;
                size_t totalPages = (candidates.size() + PAGE_SIZE - 1) / PAGE_SIZE;
                if (totalPages == 0) totalPages = 1;
                if (s_invPage >= totalPages) s_invPage = totalPages - 1;

                if (totalPages > 1) {
                    if (s_invPage > 0) {
                        if (SKSEMenuFramework::igButton(cfg.language == "ru" ? "< Назад##InvPrev" : "< Prev##InvPrev", { 0.0f, 0.0f })) {
                            s_invPage--;
                        }
                    } else {
                        SKSEMenuFramework::igText("< Назад");
                    }

                    SKSEMenuFramework::igSameLine(0.0f, 15.0f);
                    char pageBuf[64];
                    snprintf(pageBuf, sizeof(pageBuf), cfg.language == "ru" ? "Стр. %zu из %zu" : "Page %zu of %zu", s_invPage + 1, totalPages);
                    SKSEMenuFramework::igText(pageBuf);

                    SKSEMenuFramework::igSameLine(0.0f, 15.0f);
                    if (s_invPage + 1 < totalPages) {
                        if (SKSEMenuFramework::igButton(cfg.language == "ru" ? "Вперед >##InvNext" : "Next >##InvNext", { 0.0f, 0.0f })) {
                            s_invPage++;
                        }
                    } else {
                        SKSEMenuFramework::igText("Вперед >");
                    }
                    SKSEMenuFramework::igSeparator();
                }

                size_t startIdx = s_invPage * PAGE_SIZE;
                size_t endIdx = std::min(startIdx + PAGE_SIZE, candidates.size());

                for (size_t i = startIdx; i < endIdx; ++i) {
                    const auto& cand = candidates[i];
                    std::string displayName = cand.name;
                    // Do not aggressively cut off: allow full names up to 50 characters
                    if (displayName.length() > 50) {
                        displayName = displayName.substr(0, 47) + "...";
                    }
                    SKSEMenuFramework::igText(displayName.c_str());

                    // Column alignment pushed right to 430px closer to the right edge
                    SKSEMenuFramework::igSameLine(430.0f, 0.0f);

                    char btnBuf[48];
                    snprintf(btnBuf, sizeof(btnBuf), str.btnAddWhitelist, cand.formID);
                    if (SKSEMenuFramework::igButton(btnBuf, { 0.0f, 0.0f })) {
                        settings.AddWhitelistedItem(cand.formID, cand.name);
                        changed = true;
                    }
                }
            }
        }

        SKSEMenuFramework::igSeparator();

        // 2. Active Whitelist
        char countBuf[64];
        snprintf(countBuf, sizeof(countBuf), str.activeWhitelistHeader, cfg.customWhitelist.size());
        SKSEMenuFramework::igText(countBuf);

        uint32_t toRemove = 0;
        for (const auto& item : cfg.customWhitelist) {
            std::string displayName = "- " + item.name;
            if (displayName.length() > 50) {
                displayName = displayName.substr(0, 47) + "...";
            }
            SKSEMenuFramework::igText(displayName.c_str());

            // Column alignment at 430px matching the buttons above
            SKSEMenuFramework::igSameLine(430.0f, 0.0f);

            char btnBuf[48];
            snprintf(btnBuf, sizeof(btnBuf), str.btnRemoveWhitelist, item.formID);
            if (SKSEMenuFramework::igButton(btnBuf, { 0.0f, 0.0f })) {
                toRemove = item.formID;
            }
        }

        if (toRemove != 0) {
            settings.RemoveWhitelistedItem(toRemove);
            changed = true;
        }

        if (changed) {
            settings.Save();
        }
    }

    class InputEventHandler : public RE::BSTEventSink<RE::InputEvent*>
    {
    public:
        RE::BSEventNotifyControl ProcessEvent(
            RE::InputEvent* const* a_event,
            RE::BSTEventSource<RE::InputEvent*>*) override
        {
            if (!a_event || g_quit.load()) {
                return RE::BSEventNotifyControl::kContinue;
            }

            for (auto* event = *a_event; event; event = event->next) {
                auto* button = event->AsButtonEvent();
                if (!button || !button->IsDown()) continue;

                uint32_t code = button->GetIDCode();
                auto& settings = EasyHarvest::Settings::GetSingleton();
                auto& cfg = settings.GetConfig();

                // Quick Toggle Key (Default: 34 = 'G')
                if (code == cfg.toggleKey) {
                    cfg.enabled = !cfg.enabled;
                    settings.Save();

                    const auto& str = EasyHarvest::Localization::Get(cfg.language);
                    const char* msg = cfg.enabled ? str.notifyEnabled : str.notifyDisabled;
                    RE::DebugNotification(msg);

                    char buf[96];
                    snprintf(buf, sizeof(buf), "[INPUT] ToggleKey ('%s' / %u) pressed! EasyHarvest = %s",
                        cfg.toggleKeyName.c_str(), cfg.toggleKey, cfg.enabled ? "true" : "false");
                    Dbg(buf);
                }
            }

            return RE::BSEventNotifyControl::kContinue;
        }
    };

    static InputEventHandler g_inputHandler;

    void Teardown()
    {
        g_quit.store(true);
        EasyHarvest::HarvestManager::GetSingleton().Stop();
    }

    void Setup()
    {
        Dbg("=== EasyHarvest Setup() Started ===");
        // 1. Загрузка конфигурации
        auto& settings = EasyHarvest::Settings::GetSingleton();
        settings.Load();
        const auto& cfg = settings.GetConfig();
        Dbg("Settings loaded successfully");

        // 2. Регистрация перехватчика ввода
        auto* inputDevice = RE::BSInputDeviceManager::GetSingleton();
        if (inputDevice) {
            inputDevice->AddEventSink(&g_inputHandler);
            Dbg("Input event sink registered");
        }

        // 3. Интеграция с SKSE Menu Framework (Dear ImGui)
        bool smfOk = SKSEMenuFramework::Init();
        Dbg(std::string("SKSEMenuFramework::Init() = ") + (smfOk ? "TRUE" : "FALSE"));

        if (smfOk) {
            if (SKSEMenuFramework::AddSectionItem) {
                const auto& str = EasyHarvest::Localization::Get(cfg.language);
                std::string sTitle = std::string("EasyHarvest/") + str.menuSettingsTitle;
                std::string cTitle = std::string("EasyHarvest/") + str.menuCategoriesTitle;
                std::string eTitle = std::string("EasyHarvest/") + str.menuEquipmentTitle;
                std::string lTitle = std::string("EasyHarvest/") + str.menuLocationsTitle;
                std::string wTitle = std::string("EasyHarvest/") + str.menuWhitelistTitle;

                SKSEMenuFramework::AddSectionItem(sTitle.c_str(), RenderEasyHarvestSettings);
                SKSEMenuFramework::AddSectionItem(cTitle.c_str(), RenderEasyHarvestCategories);
                SKSEMenuFramework::AddSectionItem(eTitle.c_str(), RenderEasyHarvestEquipment);
                SKSEMenuFramework::AddSectionItem(lTitle.c_str(), RenderEasyHarvestLocations);
                SKSEMenuFramework::AddSectionItem(wTitle.c_str(), RenderEasyHarvestWhitelist);

                Dbg("AddSectionItem registered successfully: " + sTitle + ", " + cTitle + ", " + eTitle + ", " + lTitle + ", " + wTitle);
            } else {
                Dbg("ERROR: AddSectionItem function pointer is NULL!");
            }
        } else {
            auto hMod = GetModuleHandle("SKSEMenuFramework.dll");
            Dbg(std::string("SKSEMenuFramework.dll handle: ") + (hMod ? "FOUND" : "NULL"));
        }

        // 4. Хук обновления PlayerCharacter (vfunc 0xAD)
        REL::Relocation<std::uintptr_t> playerCharacterVtbl{ RE::VTABLE_PlayerCharacter[0] };
        _originalPlayerCharacterUpdate = reinterpret_cast<PlayerCharacter_Update_t>(
            playerCharacterVtbl.write_vfunc(0xAD, PlayerCharacter_Update));
        Dbg("PlayerCharacter::Update vfunc 0xAD hooked");

        // 5. Запуск фонового менеджера сбора
        EasyHarvest::HarvestManager::GetSingleton().Start();
        Dbg("HarvestManager started");
    }
}

static void SKSEMessageHandler(SKSE::MessagingInterface::Message* message)
{
    switch (message->type) {
    case SKSE::MessagingInterface::kDataLoaded:
        Dbg("Received SKSE Message: kDataLoaded");
        Setup();
        break;
    case SKSE::MessagingInterface::kPostLoadGame:
    case SKSE::MessagingInterface::kNewGame:
        EasyHarvest::HarvestManager::GetSingleton().ClearDepletedCache();
        break;
    default:
        if (message->type > 8) {
            Teardown();
        }
        break;
    }
}

SKSEPluginInfo(
    .Version = REL::Version{ 1, 0, 0, 0 },
    .Name = "EasyHarvest",
    .Author = "Alik"
)

SKSEPluginLoad(const SKSE::LoadInterface* a_skse)
{
    Dbg("=== EasyHarvest PluginLoad ===");
    auto* messaging = static_cast<SKSE::MessagingInterface*>(
        a_skse->QueryInterface(SKSE::LoadInterface::kMessaging));

    if (!messaging) {
        Dbg("ERROR: Messaging interface is null!");
        return false;
    }

    SKSE::Init(a_skse);
    messaging->RegisterListener("SKSE", SKSEMessageHandler);
    Dbg("Registered SKSE message listener");

    return true;
}
