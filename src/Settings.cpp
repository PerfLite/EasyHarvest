#include "pch.h"
#include "Settings.h"
#include <algorithm>

namespace EasyHarvest
{
    static std::string ExtractBool(const std::string& a_json, const std::string& a_key, bool a_default)
    {
        auto pos = a_json.find("\"" + a_key + "\"");
        if (pos == std::string::npos) return a_default ? "true" : "false";
        pos = a_json.find(':', pos);
        if (pos == std::string::npos) return a_default ? "true" : "false";

        auto end = a_json.find_first_of(",}\n", pos);
        std::string val = a_json.substr(pos + 1, end - pos - 1);
        return val.find("true") != std::string::npos ? "true" : "false";
    }

    static float ExtractFloat(const std::string& a_json, const std::string& a_key, float a_default)
    {
        auto pos = a_json.find("\"" + a_key + "\"");
        if (pos == std::string::npos) return a_default;
        pos = a_json.find(':', pos);
        if (pos == std::string::npos) return a_default;

        auto end = a_json.find_first_of(",}\n", pos);
        try {
            return std::stof(a_json.substr(pos + 1, end - pos - 1));
        } catch (...) {
            return a_default;
        }
    }

    static uint32_t ExtractUInt(const std::string& a_json, const std::string& a_key, uint32_t a_default)
    {
        auto pos = a_json.find("\"" + a_key + "\"");
        if (pos == std::string::npos) return a_default;
        pos = a_json.find(':', pos);
        if (pos == std::string::npos) return a_default;

        auto end = a_json.find_first_of(",}\n", pos);
        try {
            return static_cast<uint32_t>(std::stoul(a_json.substr(pos + 1, end - pos - 1)));
        } catch (...) {
            return a_default;
        }
    }

    static std::string ExtractString(const std::string& a_json, const std::string& a_key, const std::string& a_default)
    {
        auto pos = a_json.find("\"" + a_key + "\"");
        if (pos == std::string::npos) return a_default;
        pos = a_json.find(':', pos);
        if (pos == std::string::npos) return a_default;

        auto firstQuote = a_json.find('"', pos);
        if (firstQuote == std::string::npos) return a_default;
        auto secondQuote = a_json.find('"', firstQuote + 1);
        if (secondQuote == std::string::npos) return a_default;

        return a_json.substr(firstQuote + 1, secondQuote - firstQuote - 1);
    }

    void Settings::Load()
    {
        configPath_ = std::filesystem::current_path() / "Data" / "SKSE" / "Plugins" / "EasyHarvest.json";
        
        if (!std::filesystem::exists(configPath_)) {
            auto oldPath = std::filesystem::current_path() / "Data" / "SKSE" / "Plugins" / "AutoHarvest_PrismaUI.json";
            if (std::filesystem::exists(oldPath)) {
                configPath_ = oldPath;
            } else {
                Save();
                return;
            }
        }

        try {
            std::ifstream file(configPath_);
            if (!file.is_open()) return;

            std::stringstream buffer;
            buffer << file.rdbuf();
            FromJson(buffer.str());

            configPath_ = std::filesystem::current_path() / "Data" / "SKSE" / "Plugins" / "EasyHarvest.json";
            Save();
        } catch (...) {
        }
    }

    void Settings::Save()
    {
        try {
            if (configPath_.empty()) {
                configPath_ = std::filesystem::current_path() / "Data" / "SKSE" / "Plugins" / "EasyHarvest.json";
            }
            std::filesystem::create_directories(configPath_.parent_path());

            std::ofstream file(configPath_);
            if (file.is_open()) {
                file << ToJson();
            }
        } catch (...) {
        }
    }

    bool Settings::IsLocationExcluded(uint32_t a_formID) const
    {
        if (a_formID == 0) return false;
        for (const auto& item : config_.excludedLocations) {
            if (item.formID == a_formID) {
                return true;
            }
        }
        return false;
    }

    void Settings::AddExcludedLocation(uint32_t a_formID, const std::string& a_name)
    {
        if (a_formID == 0 || IsLocationExcluded(a_formID)) return;
        config_.excludedLocations.push_back({ a_formID, a_name.empty() ? "Локация" : a_name });
        Save();
    }

    void Settings::RemoveExcludedLocation(uint32_t a_formID)
    {
        auto it = std::remove_if(config_.excludedLocations.begin(), config_.excludedLocations.end(),
            [a_formID](const ExcludedLocation& loc) { return loc.formID == a_formID; });
        if (it != config_.excludedLocations.end()) {
            config_.excludedLocations.erase(it, config_.excludedLocations.end());
            Save();
        }
    }

    bool Settings::IsItemWhitelisted(uint32_t a_formID) const
    {
        return config_.IsWhitelisted(a_formID);
    }

    void Settings::AddWhitelistedItem(uint32_t a_formID, const std::string& a_name)
    {
        if (a_formID == 0 || config_.IsWhitelisted(a_formID)) return;
        config_.customWhitelist.push_back({ a_formID, a_name.empty() ? "Предмет" : a_name });
        Save();
    }

    void Settings::RemoveWhitelistedItem(uint32_t a_formID)
    {
        auto it = std::remove_if(config_.customWhitelist.begin(), config_.customWhitelist.end(),
            [a_formID](const WhitelistItem& item) {
                return item.formID == a_formID || ((item.formID & 0x00FFFFFF) == (a_formID & 0x00FFFFFF));
            });
        if (it != config_.customWhitelist.end()) {
            config_.customWhitelist.erase(it, config_.customWhitelist.end());
            Save();
        }
    }

    std::string Settings::ToJson() const
    {
        std::stringstream ss;
        ss << "{\n";
        ss << "  \"enabled\": " << (config_.enabled ? "true" : "false") << ",\n";
        ss << "  \"radius\": " << config_.radius << ",\n";
        ss << "  \"interval\": " << config_.interval << ",\n";
        ss << "  \"minValPerWeight\": " << config_.minValPerWeight << ",\n";

        // Categories
        ss << "  \"harvestFlora\": " << (config_.harvestFlora ? "true" : "false") << ",\n";
        ss << "  \"harvestCritters\": " << (config_.harvestCritters ? "true" : "false") << ",\n";
        ss << "  \"harvestCoins\": " << (config_.harvestCoins ? "true" : "false") << ",\n";
        ss << "  \"harvestLockpicks\": " << (config_.harvestLockpicks ? "true" : "false") << ",\n";
        ss << "  \"harvestKeys\": " << (config_.harvestKeys ? "true" : "false") << ",\n";
        ss << "  \"harvestGems\": " << (config_.harvestGems ? "true" : "false") << ",\n";
        ss << "  \"harvestIngredients\": " << (config_.harvestIngredients ? "true" : "false") << ",\n";
        ss << "  \"harvestPotions\": " << (config_.harvestPotions ? "true" : "false") << ",\n";
        ss << "  \"harvestFood\": " << (config_.harvestFood ? "true" : "false") << ",\n";
        ss << "  \"harvestDrinks\": " << (config_.harvestDrinks ? "true" : "false") << ",\n";
        ss << "  \"harvestSoulGems\": " << (config_.harvestSoulGems ? "true" : "false") << ",\n";
        ss << "  \"harvestSpellbooks\": " << (config_.harvestSpellbooks ? "true" : "false") << ",\n";
        ss << "  \"harvestSkillbooks\": " << (config_.harvestSkillbooks ? "true" : "false") << ",\n";
        ss << "  \"harvestBooks\": " << (config_.harvestBooks ? "true" : "false") << ",\n";
        ss << "  \"harvestScrolls\": " << (config_.harvestScrolls ? "true" : "false") << ",\n";
        ss << "  \"harvestOreIngots\": " << (config_.harvestOreIngots ? "true" : "false") << ",\n";
        ss << "  \"harvestOreVeins\": " << (config_.harvestOreVeins ? "true" : "false") << ",\n";
        ss << "  \"harvestAnimalParts\": " << (config_.harvestAnimalParts ? "true" : "false") << ",\n";
        ss << "  \"harvestAmmo\": " << (config_.harvestAmmo ? "true" : "false") << ",\n";
        ss << "  \"harvestMisc\": " << (config_.harvestMisc ? "true" : "false") << ",\n";

        // Equipment
        ss << "  \"harvestJewelry\": " << (config_.harvestJewelry ? "true" : "false") << ",\n";
        ss << "  \"harvestEnchantedWeapons\": " << (config_.harvestEnchantedWeapons ? "true" : "false") << ",\n";
        ss << "  \"harvestWeapons\": " << (config_.harvestWeapons ? "true" : "false") << ",\n";
        ss << "  \"harvestEnchantedArmor\": " << (config_.harvestEnchantedArmor ? "true" : "false") << ",\n";
        ss << "  \"harvestArmor\": " << (config_.harvestArmor ? "true" : "false") << ",\n";

        // Containers & Bodies
        ss << "  \"harvestContainers\": " << (config_.harvestContainers ? "true" : "false") << ",\n";
        ss << "  \"protectLockedContainers\": " << (config_.protectLockedContainers ? "true" : "false") << ",\n";
        ss << "  \"protectBossContainers\": " << (config_.protectBossContainers ? "true" : "false") << ",\n";
        ss << "  \"harvestDeadBodies\": " << (config_.harvestDeadBodies ? "true" : "false") << ",\n";
        ss << "  \"harvestAshPiles\": " << (config_.harvestAshPiles ? "true" : "false") << ",\n";

        // Behavior
        ss << "  \"pauseInCombat\": " << (config_.pauseInCombat ? "true" : "false") << ",\n";
        ss << "  \"pauseWeaponDrawn\": " << (config_.pauseWeaponDrawn ? "true" : "false") << ",\n";
        ss << "  \"stealingMode\": " << config_.stealingMode << ",\n";

        // Hotkey & Lang
        ss << "  \"toggleKey\": " << config_.toggleKey << ",\n";
        ss << "  \"toggleKeyName\": \"" << config_.toggleKeyName << "\",\n";
        ss << "  \"language\": \"" << config_.language << "\",\n";

        // Locations
        ss << "  \"ignorePlayerHouses\": " << (config_.ignorePlayerHouses ? "true" : "false") << ",\n";
        ss << "  \"excludedLocations\": [";
        for (size_t i = 0; i < config_.excludedLocations.size(); ++i) {
            const auto& loc = config_.excludedLocations[i];
            ss << "\n    {\"formID\": " << loc.formID << ", \"name\": \"" << loc.name << "\"}";
            if (i + 1 < config_.excludedLocations.size()) {
                ss << ",";
            }
        }
        if (!config_.excludedLocations.empty()) {
            ss << "\n  ";
        }
        ss << "],\n";

        // Custom Whitelist
        ss << "  \"customWhitelist\": [";
        for (size_t i = 0; i < config_.customWhitelist.size(); ++i) {
            const auto& item = config_.customWhitelist[i];
            char hexBuf[16];
            snprintf(hexBuf, sizeof(hexBuf), "%08X", item.formID);
            ss << "\n    {\"formID\": \"" << hexBuf << "\", \"name\": \"" << item.name << "\"}";
            if (i + 1 < config_.customWhitelist.size()) {
                ss << ",";
            }
        }
        if (!config_.customWhitelist.empty()) {
            ss << "\n  ";
        }
        ss << "]\n";
        ss << "}";
        return ss.str();
    }

    void Settings::FromJson(const std::string& a_json)
    {
        config_.enabled = ExtractBool(a_json, "enabled", config_.enabled) == "true";
        config_.radius = ExtractFloat(a_json, "radius", config_.radius);
        config_.interval = ExtractFloat(a_json, "interval", config_.interval);
        config_.minValPerWeight = ExtractFloat(a_json, "minValPerWeight", config_.minValPerWeight);

        // Categories
        config_.harvestFlora = ExtractBool(a_json, "harvestFlora", config_.harvestFlora) == "true";
        config_.harvestCritters = ExtractBool(a_json, "harvestCritters", config_.harvestCritters) == "true";
        config_.harvestCoins = ExtractBool(a_json, "harvestCoins", config_.harvestCoins) == "true";
        config_.harvestLockpicks = ExtractBool(a_json, "harvestLockpicks", config_.harvestLockpicks) == "true";
        config_.harvestKeys = ExtractBool(a_json, "harvestKeys", config_.harvestKeys) == "true";
        config_.harvestGems = ExtractBool(a_json, "harvestGems", config_.harvestGems) == "true";
        config_.harvestIngredients = ExtractBool(a_json, "harvestIngredients", config_.harvestIngredients) == "true";
        config_.harvestPotions = ExtractBool(a_json, "harvestPotions", config_.harvestPotions) == "true";
        config_.harvestFood = ExtractBool(a_json, "harvestFood", config_.harvestFood) == "true";
        config_.harvestDrinks = ExtractBool(a_json, "harvestDrinks", config_.harvestDrinks) == "true";
        config_.harvestSoulGems = ExtractBool(a_json, "harvestSoulGems", config_.harvestSoulGems) == "true";
        config_.harvestSpellbooks = ExtractBool(a_json, "harvestSpellbooks", config_.harvestSpellbooks) == "true";
        config_.harvestSkillbooks = ExtractBool(a_json, "harvestSkillbooks", config_.harvestSkillbooks) == "true";
        config_.harvestBooks = ExtractBool(a_json, "harvestBooks", config_.harvestBooks) == "true";
        config_.harvestScrolls = ExtractBool(a_json, "harvestScrolls", config_.harvestScrolls) == "true";
        config_.harvestOreIngots = ExtractBool(a_json, "harvestOreIngots", config_.harvestOreIngots) == "true";
        config_.harvestOreVeins = ExtractBool(a_json, "harvestOreVeins", config_.harvestOreVeins) == "true";
        config_.harvestAnimalParts = ExtractBool(a_json, "harvestAnimalParts", config_.harvestAnimalParts) == "true";
        config_.harvestAmmo = ExtractBool(a_json, "harvestAmmo", config_.harvestAmmo) == "true";
        config_.harvestMisc = ExtractBool(a_json, "harvestMisc", config_.harvestMisc) == "true";

        // Equipment
        config_.harvestJewelry = ExtractBool(a_json, "harvestJewelry", config_.harvestJewelry) == "true";
        config_.harvestEnchantedWeapons = ExtractBool(a_json, "harvestEnchantedWeapons", config_.harvestEnchantedWeapons) == "true";
        config_.harvestWeapons = ExtractBool(a_json, "harvestWeapons", config_.harvestWeapons) == "true";
        config_.harvestEnchantedArmor = ExtractBool(a_json, "harvestEnchantedArmor", config_.harvestEnchantedArmor) == "true";
        config_.harvestArmor = ExtractBool(a_json, "harvestArmor", config_.harvestArmor) == "true";

        // Containers & Bodies
        config_.harvestContainers = ExtractBool(a_json, "harvestContainers", config_.harvestContainers) == "true";
        config_.protectLockedContainers = ExtractBool(a_json, "protectLockedContainers", config_.protectLockedContainers) == "true";
        config_.protectBossContainers = ExtractBool(a_json, "protectBossContainers", config_.protectBossContainers) == "true";
        config_.harvestDeadBodies = ExtractBool(a_json, "harvestDeadBodies", config_.harvestDeadBodies) == "true";
        config_.harvestAshPiles = ExtractBool(a_json, "harvestAshPiles", config_.harvestAshPiles) == "true";

        // Behavior
        config_.pauseInCombat = ExtractBool(a_json, "pauseInCombat", config_.pauseInCombat) == "true";
        config_.pauseWeaponDrawn = ExtractBool(a_json, "pauseWeaponDrawn", config_.pauseWeaponDrawn) == "true";
        config_.stealingMode = ExtractUInt(a_json, "stealingMode", config_.stealingMode);

        // Hotkey & Language
        config_.toggleKey = ExtractUInt(a_json, "toggleKey", 34);
        if (config_.toggleKey == 35 || config_.toggleKey == 210 || config_.toggleKey == 0) {
            config_.toggleKey = 34; // Set to G
            config_.toggleKeyName = "G";
        } else {
            config_.toggleKeyName = ExtractString(a_json, "toggleKeyName", "G");
        }

        config_.language = ExtractString(a_json, "language", "auto");
        config_.ignorePlayerHouses = ExtractBool(a_json, "ignorePlayerHouses", config_.ignorePlayerHouses) == "true";

        // Parse excludedLocations array
        auto pos = a_json.find("\"excludedLocations\"");
        if (pos != std::string::npos) {
            auto startArr = a_json.find('[', pos);
            auto endArr = a_json.find(']', startArr);
            if (startArr != std::string::npos && endArr != std::string::npos) {
                config_.excludedLocations.clear();
                std::string arrStr = a_json.substr(startArr + 1, endArr - startArr - 1);
                size_t objPos = 0;
                while ((objPos = arrStr.find('{', objPos)) != std::string::npos) {
                    size_t objEnd = arrStr.find('}', objPos);
                    if (objEnd == std::string::npos) break;
                    std::string itemStr = arrStr.substr(objPos, objEnd - objPos + 1);

                    uint32_t formID = 0;
                    auto idPos = itemStr.find("\"formID\"");
                    if (idPos != std::string::npos) {
                        auto colon = itemStr.find(':', idPos);
                        if (colon != std::string::npos) {
                            formID = static_cast<uint32_t>(std::strtoul(itemStr.c_str() + colon + 1, nullptr, 10));
                        }
                    }

                    std::string name;
                    auto namePos = itemStr.find("\"name\"");
                    if (namePos != std::string::npos) {
                        auto firstQuote = itemStr.find('"', namePos + 6);
                        if (firstQuote != std::string::npos) {
                            auto secondQuote = itemStr.find('"', firstQuote + 1);
                            if (secondQuote != std::string::npos) {
                                name = itemStr.substr(firstQuote + 1, secondQuote - firstQuote - 1);
                            }
                        }
                    }

                    if (formID != 0) {
                        config_.excludedLocations.push_back({ formID, name.empty() ? "Локация" : name });
                    }
                    objPos = objEnd + 1;
                }
            }
        }

        // Parse customWhitelist array
        auto wPos = a_json.find("\"customWhitelist\"");
        if (wPos != std::string::npos) {
            auto startArr = a_json.find('[', wPos);
            auto endArr = a_json.find(']', startArr);
            if (startArr != std::string::npos && endArr != std::string::npos) {
                config_.customWhitelist.clear();
                std::string arrStr = a_json.substr(startArr + 1, endArr - startArr - 1);
                size_t p = 0;
                while (p < arrStr.size()) {
                    while (p < arrStr.size() && (std::isspace(static_cast<unsigned char>(arrStr[p])) || arrStr[p] == ',')) {
                        p++;
                    }
                    if (p >= arrStr.size()) break;

                    if (arrStr[p] == '{') {
                        size_t objEnd = arrStr.find('}', p);
                        if (objEnd == std::string::npos) break;
                        std::string itemStr = arrStr.substr(p, objEnd - p + 1);

                        uint32_t formID = 0;
                        auto idPos = itemStr.find("\"formID\"");
                        if (idPos != std::string::npos) {
                            auto colon = itemStr.find(':', idPos);
                            if (colon != std::string::npos) {
                                auto q1 = itemStr.find('"', colon);
                                if (q1 != std::string::npos) {
                                    auto q2 = itemStr.find('"', q1 + 1);
                                    if (q2 != std::string::npos) {
                                        std::string hexStr = itemStr.substr(q1 + 1, q2 - q1 - 1);
                                        formID = static_cast<uint32_t>(std::strtoul(hexStr.c_str(), nullptr, 16));
                                    }
                                } else {
                                    formID = static_cast<uint32_t>(std::strtoul(itemStr.c_str() + colon + 1, nullptr, 10));
                                }
                            }
                        }

                        std::string name;
                        auto namePos = itemStr.find("\"name\"");
                        if (namePos != std::string::npos) {
                            auto q1 = itemStr.find('"', namePos + 6);
                            if (q1 != std::string::npos) {
                                auto q2 = itemStr.find('"', q1 + 1);
                                if (q2 != std::string::npos) {
                                    name = itemStr.substr(q1 + 1, q2 - q1 - 1);
                                }
                            }
                        }

                        if (formID != 0) {
                            config_.customWhitelist.push_back({ formID, name.empty() ? "Предмет" : name });
                        }
                        p = objEnd + 1;
                    } else if (arrStr[p] == '"') {
                        size_t q2 = arrStr.find('"', p + 1);
                        if (q2 == std::string::npos) break;
                        std::string hexStr = arrStr.substr(p + 1, q2 - p - 1);
                        uint32_t formID = static_cast<uint32_t>(std::strtoul(hexStr.c_str(), nullptr, 16));
                        if (formID != 0) {
                            config_.customWhitelist.push_back({ formID, "Предмет" });
                        }
                        p = q2 + 1;
                    } else {
                        size_t numEnd = arrStr.find_first_of(", \r\n\t", p);
                        std::string numStr = arrStr.substr(p, numEnd == std::string::npos ? numEnd : (numEnd - p));
                        uint32_t formID = static_cast<uint32_t>(std::strtoul(numStr.c_str(), nullptr, 10));
                        if (formID != 0) {
                            config_.customWhitelist.push_back({ formID, "Предмет" });
                        }
                        p = (numEnd == std::string::npos) ? arrStr.size() : numEnd + 1;
                    }
                }
            }
        }
    }
}
