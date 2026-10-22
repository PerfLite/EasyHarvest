#include "pch.h"
#include "I18n.h"
#include <RE/I/INISettingCollection.h>
#include <RE/I/INIPrefSettingCollection.h>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <algorithm>

extern "C" {
    __declspec(dllimport) long __stdcall SHGetFolderPathA(void* hwnd, int csidl, void* hToken, unsigned long dwFlags, char* pszPath);
    __declspec(dllimport) unsigned long __stdcall GetPrivateProfileStringA(const char* lpAppName, const char* lpKeyName, const char* lpDefault, char* lpReturnedString, unsigned long nSize, const char* lpFileName);
}

#ifndef CSIDL_MYDOCUMENTS
#define CSIDL_MYDOCUMENTS 0x0005
#endif

namespace EasyHarvest
{
    static std::string ExtractJsonString(const std::string& a_json, const std::string& a_key, const std::string& a_default)
    {
        auto pos = a_json.find("\"" + a_key + "\"");
        if (pos == std::string::npos) return a_default;
        pos = a_json.find(':', pos);
        if (pos == std::string::npos) return a_default;
        auto q1 = a_json.find('"', pos);
        if (q1 == std::string::npos) return a_default;
        auto q2 = a_json.find('"', q1 + 1);
        while (q2 != std::string::npos && a_json[q2 - 1] == '\\') {
            q2 = a_json.find('"', q2 + 1);
        }
        if (q2 == std::string::npos) return a_default;

        std::string val = a_json.substr(q1 + 1, q2 - q1 - 1);
        std::string res;
        for (size_t i = 0; i < val.size(); ++i) {
            if (val[i] == '\\' && i + 1 < val.size()) {
                char next = val[i + 1];
                if (next == '"') { res += '"'; ++i; continue; }
                if (next == '\\') { res += '\\'; ++i; continue; }
                if (next == 'n') { res += '\n'; ++i; continue; }
                if (next == 't') { res += '\t'; ++i; continue; }
            }
            res += val[i];
        }
        return res;
    }

    I18n& I18n::GetSingleton()
    {
        static I18n s_instance;
        return s_instance;
    }

    std::string I18n::DetectLanguageSuffix()
    {
        std::string rawLang;

        try {
            if (auto* ini = RE::INISettingCollection::GetSingleton()) {
                if (auto* s = ini->GetSetting("sLanguage:General")) {
                    const char* v = s->GetString();
                    if (v && v[0]) rawLang = v;
                }
            }

            if (rawLang.empty()) {
                if (auto* iniPref = RE::INIPrefSettingCollection::GetSingleton()) {
                    if (auto* s = iniPref->GetSetting("sLanguage:General")) {
                        const char* v = s->GetString();
                        if (v && v[0]) rawLang = v;
                    }
                }
            }
        } catch (...) {}

        if (rawLang.empty()) {
            char docPath[260] = { 0 };
            if (SHGetFolderPathA(nullptr, CSIDL_MYDOCUMENTS, nullptr, 0, docPath) >= 0) {
                auto p = std::filesystem::path(docPath) / "My Games" / "Skyrim Special Edition" / "Skyrim.ini";
                char buf[64] = { 0 };
                GetPrivateProfileStringA("General", "sLanguage", "", buf, sizeof(buf), p.string().c_str());
                if (buf[0]) rawLang = buf;
            }
        }

        std::string upperLang = rawLang;
        std::transform(upperLang.begin(), upperLang.end(), upperLang.begin(), [](unsigned char c) {
            return static_cast<char>(std::toupper(c));
        });

        if (upperLang.find("RUS") != std::string::npos || upperLang == "RU") return "RU";
        if (upperLang.find("ENG") != std::string::npos || upperLang == "EN") return "EN";
        if (upperLang.find("GER") != std::string::npos || upperLang == "DE") return "DE";
        if (upperLang.find("FRE") != std::string::npos || upperLang == "FR") return "FR";
        if (upperLang.find("SPA") != std::string::npos || upperLang == "ES") return "ES";
        if (upperLang.find("POR") != std::string::npos || upperLang == "PT") return "PT";
        if (upperLang.find("POL") != std::string::npos || upperLang == "PL") return "PL";
        if (upperLang.find("CHI") != std::string::npos || upperLang == "ZH") return "ZH";
        if (upperLang.find("JAP") != std::string::npos || upperLang == "JA") return "JA";

        return upperLang.empty() ? "EN" : upperLang;
    }

    void I18n::SetEnglishDefaults()
    {
        m_strings.menuSettingsTitle = "Settings";
        m_strings.menuCategoriesTitle = "Categories";
        m_strings.menuEquipmentTitle = "Equipment";
        m_strings.menuLocationsTitle = "Locations";
        m_strings.menuWhitelistTitle = "Whitelist";

        m_strings.settingsHeader = "EasyHarvest - General Settings";
        m_strings.masterEnabled = "Master EasyHarvest Enabled";
        m_strings.radius = "Detection Radius";
        m_strings.interval = "Scan Frequency";
        m_strings.hotkeyInfo = "Toggle Hotkey: %s (Scancode: %u)";
        m_strings.valPerWeight = "Min Value / Weight Ratio";
        m_strings.valPerWeightHelp = "0 = No filter, 1+ = Item Value >= N or Value/Weight >= N";

        m_strings.behaviorHeader = "Behavior & Safety:";
        m_strings.pauseCombat = "Pause In Combat";
        m_strings.pauseWeaponDrawn = "Pause With Weapon Drawn";
        m_strings.stealModeHeader = "Stealing Protection Mode:";
        m_strings.stealModeNever = "Never Steal (Owned Items Protected)";
        m_strings.stealModeSneak = "Steal Only In Sneak When Undetected";
        m_strings.stealModeAlways = "Steal Always (Loot Crime Items)";

        m_strings.langHeader = "Language / Язык:";

        m_strings.categoriesHeader = "EasyHarvest - Harvest Categories";
        m_strings.catToggleAll = "Toggle All Categories (On / Off)";
        m_strings.btnSelectAll = "Select All";
        m_strings.btnDeselectAll = "Deselect All";
        m_strings.catFlora = "Flora, Plants, Herbs & Mushrooms";
        m_strings.catCritters = "Critters (Butterflies, Fireflies, Fish)";
        m_strings.catCoins = "Gold / Septims";
        m_strings.catLockpicks = "Lockpicks";
        m_strings.catKeys = "Keys";
        m_strings.catGems = "Gems (Rubies, Sapphires, Diamonds)";
        m_strings.catIngredients = "Ingredients";
        m_strings.catPotions = "Potions & Poisons";
        m_strings.catFood = "Food & Provisions";
        m_strings.catDrinks = "Drinks & Alcohol";
        m_strings.catSoulGems = "Soul Gems";
        m_strings.catSpellbooks = "Spell Tomes";
        m_strings.catSkillbooks = "Skill Books";
        m_strings.catBooks = "Regular Books & Notes";
        m_strings.catScrolls = "Scrolls";
        m_strings.catOreIngots = "Ore & Metal Ingots";
        m_strings.catOreVeins = "Auto-Mine Ore Veins";
        m_strings.catAnimalParts = "Animal Hides, Pelts & Parts";
        m_strings.catAmmo = "Ammo (Arrows & Bolts)";
        m_strings.catMisc = "Miscellaneous & Modded Items (Crafting, Clutter)";

        m_strings.equipmentHeader = "EasyHarvest - Equipment & Valuables";
        m_strings.equipToggleAll = "Toggle All Equipment (On / Off)";
        m_strings.catJewelry = "Jewelry (Rings & Amulets)";
        m_strings.catEnchantedWeapons = "Enchanted Weapons";
        m_strings.catWeapons = "All Weapons (Including unenchanted)";
        m_strings.catEnchantedArmor = "Enchanted Armor";
        m_strings.catArmor = "All Armor & Clothes (Including unenchanted)";

        m_strings.containersHeader = "Containers & Corpses Looting:";
        m_strings.catContainers = "Loot From Containers (Chests, Urns, Barrels)";
        m_strings.catDeadBodies = "Loot From Dead Bodies (Corpses of Enemies)";
        m_strings.catAshPiles = "Loot From Ash Piles (Vampires / Disintegration)";
        m_strings.protectLockedContainers = "Never Open Locked Containers";
        m_strings.protectBossContainers = "Protect Boss Chests (Manual Loot Only)";

        m_strings.locationsHeader = "EasyHarvest - Location Blacklist";
        m_strings.ignoreHouses = "Ignore Player Houses Automatically";
        m_strings.currentLocation = "Current: %s [0x%08X]";
        m_strings.statusBlocked = "Status: [BLOCKED IN EXCLUSIONS]";
        m_strings.statusAllowed = "Status: [ALLOWED]";
        m_strings.btnBlock = "Block This Location";
        m_strings.btnUnblock = "Unblock This Location";
        m_strings.excludedHeader = "Excluded Locations (%zu):";
        m_strings.btnRemove = "Remove##%u";
        m_strings.locOpenWorld = "Current location: Open World";
        m_strings.locInteriorOnly = "(Blocking is only available inside buildings and interiors)";
        m_strings.locBuilding = "Building / Room";
        m_strings.locInterior = "Interior";
        m_strings.locEmpty = "Location blacklist is empty. You can block your current location with the button above.";

        m_strings.whitelistHeader = "EasyHarvest - Custom Item Whitelist";
        m_strings.whitelistDesc = "Items in this list will ALWAYS be looted, bypassing all category and value filters.";
        m_strings.invItemsHeader = "Add from player inventory:";
        m_strings.btnAddWhitelist = "+ Whitelist##%08X";
        m_strings.activeWhitelistHeader = "Active Whitelist (%zu items):";
        m_strings.btnRemoveWhitelist = "Remove##WL_%08X";
        m_strings.searchLabel = "Search:";
        m_strings.searchHint = "Item name...";
        m_strings.btnClear = "Clear##Clear";
        m_strings.searchNoMatch = "- No items match your search -";
        m_strings.invNoItems = "- No new items in inventory to add -";
        m_strings.btnPrevPage = "< Prev##InvPrev";
        m_strings.btnNextPage = "Next >##InvNext";
        m_strings.pageFormat = "Page %zu of %zu";
        m_strings.txtPrevDisabled = "< Prev";
        m_strings.txtNextDisabled = "Next >";
        m_strings.whitelistEmpty = "Whitelist is empty. Select items from inventory above to add them.";

        m_strings.notifyEnabled = "EasyHarvest: Enabled";
        m_strings.notifyDisabled = "EasyHarvest: Disabled";
    }

    void I18n::SetRussianDefaults()
    {
        m_strings.menuSettingsTitle = "Настройки";
        m_strings.menuCategoriesTitle = "Категории";
        m_strings.menuEquipmentTitle = "Экипировка";
        m_strings.menuLocationsTitle = "Локации";
        m_strings.menuWhitelistTitle = "Белый список";

        m_strings.settingsHeader = "EasyHarvest - Основные настройки";
        m_strings.masterEnabled = "Автосбор включен";
        m_strings.radius = "Радиус сбора";
        m_strings.interval = "Интервал сканирования";
        m_strings.hotkeyInfo = "Горячая клавиша: %s (Код: %u)";
        m_strings.valPerWeight = "Порог цена / вес (Ц/В)";
        m_strings.valPerWeightHelp = "0 = без фильтра, 1+ = цена >= N или соотношение >= N";

        m_strings.behaviorHeader = "Поведение и отключения:";
        m_strings.pauseCombat = "Отключить сбор в бою";
        m_strings.pauseWeaponDrawn = "Отключить при обнажении оружия";
        m_strings.stealModeHeader = "Режим кражи (чужие предметы):";
        m_strings.stealModeNever = "Не воровать (только бесхозное)";
        m_strings.stealModeSneak = "Воровать только в скрытности (если не видят)";
        m_strings.stealModeAlways = "Воровать всегда (подбирать всё подряд)";

        m_strings.langHeader = "Язык / Language:";

        m_strings.categoriesHeader = "EasyHarvest - Категории добычи";
        m_strings.catToggleAll = "Включить / Выключить все категории";
        m_strings.btnSelectAll = "Включить всё";
        m_strings.btnDeselectAll = "Выключить всё";
        m_strings.catFlora = "Флора, растения, травы и грибы";
        m_strings.catCritters = "Мелкие существа (бабочки, светлячки, рыба)";
        m_strings.catCoins = "Золото и септимы";
        m_strings.catLockpicks = "Отмычки";
        m_strings.catKeys = "Ключи";
        m_strings.catGems = "Драгоценные камни (рубины, алмазы, сапфиры)";
        m_strings.catIngredients = "Ингредиенты";
        m_strings.catPotions = "Зелья и яды";
        m_strings.catFood = "Еда и провизия";
        m_strings.catDrinks = "Напитки и выпивка";
        m_strings.catSoulGems = "Камни душ";
        m_strings.catSpellbooks = "Тома заклинаний";
        m_strings.catSkillbooks = "Книги навыков";
        m_strings.catBooks = "Обычные книги и записки";
        m_strings.catScrolls = "Свитки";
        m_strings.catOreIngots = "Руда и слитки металлов";
        m_strings.catOreVeins = "Авто-добыча руды из жил";
        m_strings.catAnimalParts = "Шкуры, кожа и части животных";
        m_strings.catAmmo = "Боеприпасы (стрелы и болты)";
        m_strings.catMisc = "Разное (предметы модов, ремесло, хлам)";

        m_strings.equipmentHeader = "EasyHarvest - Экипировка и ценности";
        m_strings.equipToggleAll = "Включить / Выключить всю экипировку";
        m_strings.catJewelry = "Ювелирные изделия (кольца, амулеты)";
        m_strings.catEnchantedWeapons = "Зачарованное оружие";
        m_strings.catWeapons = "Всё оружие (включая обычное)";
        m_strings.catEnchantedArmor = "Зачарованная броня";
        m_strings.catArmor = "Вся броня и одежда (включая обычную)";

        m_strings.containersHeader = "Сбор из сундуков и трупов:";
        m_strings.catContainers = "Сбор из контейнеров (сундуки, урны, бочки)";
        m_strings.catDeadBodies = "Сбор с тел (трупы врагов и животных)";
        m_strings.catAshPiles = "Сбор из куч пепла (испепелённые останки)";
        m_strings.protectLockedContainers = "Не открывать запертые сундуки";
        m_strings.protectBossContainers = "Не трогать сундуки боссов (только вручную)";

        m_strings.locationsHeader = "EasyHarvest - Черный список локаций";
        m_strings.ignoreHouses = "Игнорировать дома игрока";
        m_strings.currentLocation = "Текущая: %s [0x%08X]";
        m_strings.statusBlocked = "Статус: [В ЧЕРНОМ СПИСКЕ]";
        m_strings.statusAllowed = "Статус: [РАЗРЕШЕНО]";
        m_strings.btnBlock = "Заблокировать эту локацию";
        m_strings.btnUnblock = "Разблокировать эту локацию";
        m_strings.excludedHeader = "Исключённые локации (%zu):";
        m_strings.btnRemove = "Удалить##%u";
        m_strings.locOpenWorld = "Текущая локация: Открытый мир";
        m_strings.locInteriorOnly = "(Блокировка сбора доступна только внутри зданий и помещений)";
        m_strings.locBuilding = "Здание / Помещение";
        m_strings.locInterior = "Интерьер";
        m_strings.locEmpty = "Список исключений пуст. Вы можете заблокировать текущую локацию кнопкой выше.";

        m_strings.whitelistHeader = "EasyHarvest - Белый список предметов";
        m_strings.whitelistDesc = "Предметы из этого списка забираются ВСЕГДА, игнорируя любые фильтры.";
        m_strings.invItemsHeader = "Добавить из инвентаря игрока:";
        m_strings.btnAddWhitelist = "+ В белый список##%08X";
        m_strings.activeWhitelistHeader = "Активный белый список (%zu предм.):";
        m_strings.btnRemoveWhitelist = "Удалить##WL_%08X";
        m_strings.searchLabel = "Поиск:";
        m_strings.searchHint = "Название предмета...";
        m_strings.btnClear = "Сброс##Clear";
        m_strings.searchNoMatch = "- Ничего не найдено по запросу -";
        m_strings.invNoItems = "- Нет новых предметов в инвентаре для добавления -";
        m_strings.btnPrevPage = "< Назад##InvPrev";
        m_strings.btnNextPage = "Вперед >##InvNext";
        m_strings.pageFormat = "Стр. %zu из %zu";
        m_strings.txtPrevDisabled = "< Назад";
        m_strings.txtNextDisabled = "Вперед >";
        m_strings.whitelistEmpty = "Белый список пуст. Выберите предметы из инвентаря выше, чтобы добавить их.";

        m_strings.notifyEnabled = "EasyHarvest: Включен";
        m_strings.notifyDisabled = "EasyHarvest: Выключен";
    }

    void I18n::LoadFromFile(const std::filesystem::path& a_path)
    {
        std::ifstream file(a_path);
        if (!file.is_open()) return;

        std::stringstream buf;
        buf << file.rdbuf();
        std::string json = buf.str();
        file.close();

        m_strings.menuSettingsTitle = ExtractJsonString(json, "Menu.SettingsTitle", m_strings.menuSettingsTitle);
        m_strings.menuCategoriesTitle = ExtractJsonString(json, "Menu.CategoriesTitle", m_strings.menuCategoriesTitle);
        m_strings.menuEquipmentTitle = ExtractJsonString(json, "Menu.EquipmentTitle", m_strings.menuEquipmentTitle);
        m_strings.menuLocationsTitle = ExtractJsonString(json, "Menu.LocationsTitle", m_strings.menuLocationsTitle);
        m_strings.menuWhitelistTitle = ExtractJsonString(json, "Menu.WhitelistTitle", m_strings.menuWhitelistTitle);

        m_strings.settingsHeader = ExtractJsonString(json, "Settings.Header", m_strings.settingsHeader);
        m_strings.masterEnabled = ExtractJsonString(json, "Settings.MasterEnabled", m_strings.masterEnabled);
        m_strings.radius = ExtractJsonString(json, "Settings.Radius", m_strings.radius);
        m_strings.interval = ExtractJsonString(json, "Settings.Interval", m_strings.interval);
        m_strings.hotkeyInfo = ExtractJsonString(json, "Settings.HotkeyInfo", m_strings.hotkeyInfo);
        m_strings.valPerWeight = ExtractJsonString(json, "Settings.ValPerWeight", m_strings.valPerWeight);
        m_strings.valPerWeightHelp = ExtractJsonString(json, "Settings.ValPerWeightHelp", m_strings.valPerWeightHelp);

        m_strings.behaviorHeader = ExtractJsonString(json, "Behavior.Header", m_strings.behaviorHeader);
        m_strings.pauseCombat = ExtractJsonString(json, "Behavior.PauseCombat", m_strings.pauseCombat);
        m_strings.pauseWeaponDrawn = ExtractJsonString(json, "Behavior.PauseWeaponDrawn", m_strings.pauseWeaponDrawn);
        m_strings.stealModeHeader = ExtractJsonString(json, "Behavior.StealModeHeader", m_strings.stealModeHeader);
        m_strings.stealModeNever = ExtractJsonString(json, "Behavior.StealModeNever", m_strings.stealModeNever);
        m_strings.stealModeSneak = ExtractJsonString(json, "Behavior.StealModeSneak", m_strings.stealModeSneak);
        m_strings.stealModeAlways = ExtractJsonString(json, "Behavior.StealModeAlways", m_strings.stealModeAlways);

        m_strings.langHeader = ExtractJsonString(json, "Language.Header", m_strings.langHeader);

        m_strings.categoriesHeader = ExtractJsonString(json, "Categories.Header", m_strings.categoriesHeader);
        m_strings.catToggleAll = ExtractJsonString(json, "Categories.ToggleAll", m_strings.catToggleAll);
        m_strings.btnSelectAll = ExtractJsonString(json, "Categories.SelectAll", m_strings.btnSelectAll);
        m_strings.btnDeselectAll = ExtractJsonString(json, "Categories.DeselectAll", m_strings.btnDeselectAll);
        m_strings.catFlora = ExtractJsonString(json, "Categories.Flora", m_strings.catFlora);
        m_strings.catCritters = ExtractJsonString(json, "Categories.Critters", m_strings.catCritters);
        m_strings.catCoins = ExtractJsonString(json, "Categories.Coins", m_strings.catCoins);
        m_strings.catLockpicks = ExtractJsonString(json, "Categories.Lockpicks", m_strings.catLockpicks);
        m_strings.catKeys = ExtractJsonString(json, "Categories.Keys", m_strings.catKeys);
        m_strings.catGems = ExtractJsonString(json, "Categories.Gems", m_strings.catGems);
        m_strings.catIngredients = ExtractJsonString(json, "Categories.Ingredients", m_strings.catIngredients);
        m_strings.catPotions = ExtractJsonString(json, "Categories.Potions", m_strings.catPotions);
        m_strings.catFood = ExtractJsonString(json, "Categories.Food", m_strings.catFood);
        m_strings.catDrinks = ExtractJsonString(json, "Categories.Drinks", m_strings.catDrinks);
        m_strings.catSoulGems = ExtractJsonString(json, "Categories.SoulGems", m_strings.catSoulGems);
        m_strings.catSpellbooks = ExtractJsonString(json, "Categories.Spellbooks", m_strings.catSpellbooks);
        m_strings.catSkillbooks = ExtractJsonString(json, "Categories.Skillbooks", m_strings.catSkillbooks);
        m_strings.catBooks = ExtractJsonString(json, "Categories.Books", m_strings.catBooks);
        m_strings.catScrolls = ExtractJsonString(json, "Categories.Scrolls", m_strings.catScrolls);
        m_strings.catOreIngots = ExtractJsonString(json, "Categories.OreIngots", m_strings.catOreIngots);
        m_strings.catOreVeins = ExtractJsonString(json, "Categories.OreVeins", m_strings.catOreVeins);
        m_strings.catAnimalParts = ExtractJsonString(json, "Categories.AnimalParts", m_strings.catAnimalParts);
        m_strings.catAmmo = ExtractJsonString(json, "Categories.Ammo", m_strings.catAmmo);
        m_strings.catMisc = ExtractJsonString(json, "Categories.Misc", m_strings.catMisc);

        m_strings.equipmentHeader = ExtractJsonString(json, "Equipment.Header", m_strings.equipmentHeader);
        m_strings.equipToggleAll = ExtractJsonString(json, "Equipment.ToggleAll", m_strings.equipToggleAll);
        m_strings.catJewelry = ExtractJsonString(json, "Equipment.Jewelry", m_strings.catJewelry);
        m_strings.catEnchantedWeapons = ExtractJsonString(json, "Equipment.EnchantedWeapons", m_strings.catEnchantedWeapons);
        m_strings.catWeapons = ExtractJsonString(json, "Equipment.Weapons", m_strings.catWeapons);
        m_strings.catEnchantedArmor = ExtractJsonString(json, "Equipment.EnchantedArmor", m_strings.catEnchantedArmor);
        m_strings.catArmor = ExtractJsonString(json, "Equipment.Armor", m_strings.catArmor);

        m_strings.containersHeader = ExtractJsonString(json, "Containers.Header", m_strings.containersHeader);
        m_strings.catContainers = ExtractJsonString(json, "Containers.Containers", m_strings.catContainers);
        m_strings.catDeadBodies = ExtractJsonString(json, "Containers.DeadBodies", m_strings.catDeadBodies);
        m_strings.catAshPiles = ExtractJsonString(json, "Containers.AshPiles", m_strings.catAshPiles);
        m_strings.protectLockedContainers = ExtractJsonString(json, "Containers.ProtectLocked", m_strings.protectLockedContainers);
        m_strings.protectBossContainers = ExtractJsonString(json, "Containers.ProtectBoss", m_strings.protectBossContainers);

        m_strings.locationsHeader = ExtractJsonString(json, "Locations.Header", m_strings.locationsHeader);
        m_strings.ignoreHouses = ExtractJsonString(json, "Locations.IgnoreHouses", m_strings.ignoreHouses);
        m_strings.currentLocation = ExtractJsonString(json, "Locations.Current", m_strings.currentLocation);
        m_strings.statusBlocked = ExtractJsonString(json, "Locations.StatusBlocked", m_strings.statusBlocked);
        m_strings.statusAllowed = ExtractJsonString(json, "Locations.StatusAllowed", m_strings.statusAllowed);
        m_strings.btnBlock = ExtractJsonString(json, "Locations.BtnBlock", m_strings.btnBlock);
        m_strings.btnUnblock = ExtractJsonString(json, "Locations.BtnUnblock", m_strings.btnUnblock);
        m_strings.excludedHeader = ExtractJsonString(json, "Locations.ExcludedHeader", m_strings.excludedHeader);
        m_strings.btnRemove = ExtractJsonString(json, "Locations.BtnRemove", m_strings.btnRemove);
        m_strings.locOpenWorld = ExtractJsonString(json, "Locations.OpenWorld", m_strings.locOpenWorld);
        m_strings.locInteriorOnly = ExtractJsonString(json, "Locations.InteriorOnly", m_strings.locInteriorOnly);
        m_strings.locBuilding = ExtractJsonString(json, "Locations.Building", m_strings.locBuilding);
        m_strings.locInterior = ExtractJsonString(json, "Locations.Interior", m_strings.locInterior);
        m_strings.locEmpty = ExtractJsonString(json, "Locations.Empty", m_strings.locEmpty);

        m_strings.whitelistHeader = ExtractJsonString(json, "Whitelist.Header", m_strings.whitelistHeader);
        m_strings.whitelistDesc = ExtractJsonString(json, "Whitelist.Desc", m_strings.whitelistDesc);
        m_strings.invItemsHeader = ExtractJsonString(json, "Whitelist.InvItemsHeader", m_strings.invItemsHeader);
        m_strings.btnAddWhitelist = ExtractJsonString(json, "Whitelist.BtnAdd", m_strings.btnAddWhitelist);
        m_strings.activeWhitelistHeader = ExtractJsonString(json, "Whitelist.ActiveHeader", m_strings.activeWhitelistHeader);
        m_strings.btnRemoveWhitelist = ExtractJsonString(json, "Whitelist.BtnRemove", m_strings.btnRemoveWhitelist);
        m_strings.searchLabel = ExtractJsonString(json, "Whitelist.SearchLabel", m_strings.searchLabel);
        m_strings.searchHint = ExtractJsonString(json, "Whitelist.SearchHint", m_strings.searchHint);
        m_strings.btnClear = ExtractJsonString(json, "Whitelist.BtnClear", m_strings.btnClear);
        m_strings.searchNoMatch = ExtractJsonString(json, "Whitelist.SearchNoMatch", m_strings.searchNoMatch);
        m_strings.invNoItems = ExtractJsonString(json, "Whitelist.InvNoItems", m_strings.invNoItems);
        m_strings.btnPrevPage = ExtractJsonString(json, "Whitelist.BtnPrevPage", m_strings.btnPrevPage);
        m_strings.btnNextPage = ExtractJsonString(json, "Whitelist.BtnNextPage", m_strings.btnNextPage);
        m_strings.pageFormat = ExtractJsonString(json, "Whitelist.PageFormat", m_strings.pageFormat);
        m_strings.txtPrevDisabled = ExtractJsonString(json, "Whitelist.TxtPrevDisabled", m_strings.txtPrevDisabled);
        m_strings.txtNextDisabled = ExtractJsonString(json, "Whitelist.TxtNextDisabled", m_strings.txtNextDisabled);
        m_strings.whitelistEmpty = ExtractJsonString(json, "Whitelist.Empty", m_strings.whitelistEmpty);

        m_strings.notifyEnabled = ExtractJsonString(json, "Notify.Enabled", m_strings.notifyEnabled);
        m_strings.notifyDisabled = ExtractJsonString(json, "Notify.Disabled", m_strings.notifyDisabled);

        SKSE::log::info("EasyHarvest: Loaded translations from {}", a_path.filename().string());
    }

    void I18n::Load(std::string_view a_override)
    {
        std::string suffix;
        if (!a_override.empty() && a_override != "auto" && a_override != "Auto") {
            suffix = a_override;
            std::transform(suffix.begin(), suffix.end(), suffix.begin(), [](unsigned char c) {
                return static_cast<char>(std::toupper(c));
            });
        } else {
            suffix = DetectLanguageSuffix();
        }

        m_currentSuffix = suffix;
        SKSE::log::info("EasyHarvest: Using language: {}", suffix);

        if (suffix == "RU") {
            SetRussianDefaults();
        } else {
            SetEnglishDefaults();
        }

        std::vector<std::filesystem::path> searchDirs = {
            std::filesystem::current_path() / "Data" / "SKSE" / "Plugins",
            std::filesystem::path("Data") / "SKSE" / "Plugins",
            std::filesystem::path("SKSE") / "Plugins"
        };

        for (const auto& dir : searchDirs) {
            auto langFile = dir / ("EasyHarvestStrings_" + suffix + ".json");
            if (std::filesystem::exists(langFile)) {
                LoadFromFile(langFile);
                return;
            }
        }

        for (const auto& dir : searchDirs) {
            auto defaultFile = dir / "EasyHarvestStrings.json";
            if (std::filesystem::exists(defaultFile)) {
                LoadFromFile(defaultFile);
                return;
            }
        }

        for (const auto& dir : searchDirs) {
            auto enFile = dir / "EasyHarvestStrings_EN.json";
            if (std::filesystem::exists(enFile)) {
                LoadFromFile(enFile);
                return;
            }
        }

        SKSE::log::info("EasyHarvest: Using built-in defaults ({})", suffix == "RU" ? "Russian" : "English");
    }
}
