#include "pch.h"
#include "HarvestManager.h"
#include <cctype>
#include <chrono>
#include <unordered_map>
#include <unordered_set>
#include <RE/I/IObjectHandlePolicy.h>
#include <RE/V/VirtualMachine.h>
#include <RE/P/PackUnpack.h>
#include <RE/S/ScrollItem.h>

namespace EasyHarvest
{
    void HarvestManager::Start()
    {
        running_.store(true);
    }

    void HarvestManager::Stop()
    {
        running_.store(false);
    }

    void HarvestManager::NotifyLoot(const std::string& a_name, const std::string& a_category, int a_count)
    {
        if (onLoot_) {
            onLoot_(a_name, a_category, a_count);
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
                    result.push_back(static_cast<char>(c2 - 0x20));
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

    static bool IsFirewood(RE::TESBoundObject* a_item)
    {
        if (!a_item) return false;

        // 1. Vanilla Firewood FormID: 0x0006F993
        if (a_item->GetFormID() == 0x0006F993) {
            return true;
        }

        // 2. Keywords
        if (auto* kw = a_item->As<RE::BGSKeywordForm>()) {
            if (kw->HasKeywordString("VendorItemWood") || 
                kw->HasKeywordString("VendorItemFirewood") || 
                kw->HasKeywordString("Firewood") ||
                kw->HasKeywordString("isFirewood")) {
                return true;
            }
        }

        // 3. EditorID check
        const char* edid = a_item->GetFormEditorID();
        if (edid && edid[0]) {
            std::string edidLower = edid;
            for (auto& c : edidLower) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
            if (edidLower.find("firewood") != std::string::npos ||
                edidLower.find("woodlog") != std::string::npos ||
                edidLower.find("choppedwood") != std::string::npos) {
                return true;
            }
        }

        // 4. Name check (Russian & English) - only for Misc items (firewood is always Misc)
        if (a_item->Is(RE::FormType::Misc)) {
            const char* rawName = a_item->GetName();
            if (rawName && rawName[0]) {
                std::string nameLower = ToLowerUtf8(rawName);
                if (nameLower.find("\xd0\xbf\xd0\xbe\xd0\xbb\xd0\xb5\xd0\xbd") != std::string::npos ||     // полен (полено, поленья)
                    nameLower.find("\xd0\xbf\xd0\xbe\xd0\xbb\xd0\xb5\xd1\x88\xd0\xba") != std::string::npos || // полешк (полешко, полешки)
                    nameLower.find("firewood") != std::string::npos ||
                    nameLower.starts_with("\xd0\xb4\xd1\x80\xd0\xbe\xd0\xb2") ||                               // дров... (дрова)
                    nameLower.find(" \xd0\xb4\xd1\x80\xd0\xbe\xd0\xb2") != std::string::npos) {                // ... дров... (вязанка дров)
                    return true;
                }
            }
        }

        return false;
    }

    static bool IsJunkClutter(RE::TESBoundObject* a_item)
    {
        if (!a_item) return false;

        // 1. Well-known FormIDs (Hearthfire construction junk & Vanilla paper rolls / blank scrolls)
        RE::FormID localId = a_item->GetFormID() & 0x00FFFFFF;
        // BYOHMaterialGlass (0x00005A69), BYOHMaterialStraw (0x00005A68), BYOHMaterialGoatHorns (0x0000303F)
        // PaperRoll (0x00033760), RuinsPaperRoll01 (0x0006277D), BlankScroll (0x000EA5BB), Scroll (0x000F62EB)
        if (localId == 0x00005A69 || localId == 0x00005A68 || localId == 0x0000303F ||
            localId == 0x00033760 || localId == 0x0006277D || localId == 0x000EA5BB || localId == 0x000F62EB) {
            return true;
        }

        // 2. ScrollItem with no spell effects (empty/blank scroll)
        if (auto* scroll = a_item->As<RE::ScrollItem>()) {
            if (scroll->effects.empty()) {
                return true;
            }
        }

        // 3. EditorID check
        const char* edid = a_item->GetFormEditorID();
        if (edid && edid[0]) {
            std::string edidLower = edid;
            for (auto& c : edidLower) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));

            // Straw
            if (edidLower.find("straw") != std::string::npos && edidLower.find("strawberry") == std::string::npos) {
                return true;
            }
            if (edidLower.find("solom") != std::string::npos) {
                return true;
            }

            // Glass clutter
            if (edidLower == "byohmaterialglass" ||
                edidLower == "glassshard" ||
                edidLower.find("brokenglass") != std::string::npos) {
                return true;
            }

            // Goat horns
            if (edidLower == "byohmaterialgoathorns") {
                return true;
            }

            // Blank scrolls & paper rolls
            if (edidLower.find("blankscroll") != std::string::npos ||
                edidLower.find("emptyscroll") != std::string::npos ||
                edidLower.find("scrollblank") != std::string::npos ||
                edidLower.find("scrollempty") != std::string::npos ||
                edidLower.find("scroll_blank") != std::string::npos ||
                edidLower.find("scroll_empty") != std::string::npos ||
                edidLower.find("paperroll") != std::string::npos ||
                edidLower.find("rollpaper") != std::string::npos) {
                return true;
            }
        }

        // 4. Name check (Russian & English)
        const char* rawName = a_item->GetName();
        if (rawName && rawName[0]) {
            std::string nameLower = ToLowerUtf8(rawName);

            // Солома (Straw) - all forms: "солома", "пучок соломы", "сноп соломы", "тюк соломы", etc.
            if (nameLower.find("\xd1\x81\xd0\xbe\xd0\xbb\xd0\xbe\xd0\xbc") != std::string::npos || // солом
                (nameLower.find("straw") != std::string::npos && nameLower.find("strawberry") == std::string::npos)) {
                return true;
            }

            // Свиток пустой / Чистый свиток / Blank Scroll / Empty Scroll
            if ((nameLower.find("\xd0\xbf\xd1\x83\xd1\x81\xd1\x82") != std::string::npos && // пуст...
                 (nameLower.find("\xd1\x81\xd0\xb2\xd0\xb8\xd1\x82\xd0\xbe\xd0\xba") != std::string::npos || nameLower.find("scroll") != std::string::npos)) || // свиток / scroll
                nameLower == "\xd1\x87\xd0\xb8\xd1\x81\xd1\x82\xd1\x8b\xd0\xb9 \xd1\x81\xd0\xb2\xd0\xb8\xd1\x82\xd0\xbe\xd0\xba" || // чистый свиток
                nameLower == "\xd1\x81\xd0\xb2\xd0\xb8\xd1\x82\xd0\xbe\xd0\xba \xd1\x87\xd0\xb8\xd1\x81\xd1\x82\xd1\x8b\xd0\xb9" || // свиток чистый
                nameLower.find("blank scroll") != std::string::npos ||
                nameLower.find("empty scroll") != std::string::npos ||
                nameLower.find("scroll (blank)") != std::string::npos ||
                nameLower.find("scroll (empty)") != std::string::npos) {
                return true;
            }

            // Рулон бумаги / Roll of paper
            if (nameLower.find("\xd1\x80\xd1\x83\xd0\xbb\xd0\xbe\xd0\xbd \xd0\xb1\xd1\x83\xd0\xbc\xd0\xb0\xd0\xb3") != std::string::npos || // рулон бумаг...
                nameLower.find("\xd1\x80\xd1\x83\xd0\xbb\xd0\xbe\xd0\xbd\xd1\x87\xd0\xb8\xd0\xba \xd0\xb1\xd1\x83\xd0\xbc\xd0\xb0\xd0\xb3") != std::string::npos || // рулончик бумаг...
                nameLower.find("roll of paper") != std::string::npos ||
                nameLower.find("paper roll") != std::string::npos) {
                return true;
            }

            // Рога козы (Goat horns)
            if (nameLower == "\xd1\x80\xd0\xbe\xd0\xb3\xd0\xb0 \xd0\xba\xd0\xbe\xd0\xb7\xd1\x8b" ||
                nameLower == "goat horns") {
                return true;
            }

            // Glass clutter - only for Misc items (not Malachite ore/ingots or glass weapons/armor)
            if (a_item->Is(RE::FormType::Misc)) {
                if (nameLower == "\xd1\x81\xd1\x82\xd0\xb5\xd0\xba\xd0\xbb\xd0\xbe" || // стекло
                    nameLower == "glass" ||
                    (nameLower.find("\xd1\x81\xd1\x82\xd0\xb5\xd0\xba\xd0\xbb") != std::string::npos && 
                     (nameLower.find("\xd0\xbe\xd1\x81\xd0\xba\xd0\xbe\xd0\xbb") != std::string::npos || // осколок стекла
                      nameLower.find("\xd0\xba\xd1\x83\xd1\x81\xd0\xbe\xd0\xba") != std::string::npos ||  // кусок стекла
                      nameLower.find("\xd0\xb1\xd0\xb8\xd1\x82") != std::string::npos)) ||                 // битое стекло
                    nameLower == "glass shard" ||
                    nameLower == "broken glass") {
                    return true;
                }
            }
        }

        return false;
    }


    static RE::TESBoundObject* GetOreItemForVein(RE::TESObjectREFR* a_refr, RE::TESBoundObject* a_base, RE::BSTSmartPointer<RE::BSScript::Object> a_scriptObj)
    {
        // 1. Пытаемся получить свойство "Ore" из Papyrus scriptObj (если привязано)
        if (a_scriptObj) {
            auto* vm = RE::BSScript::Internal::VirtualMachine::GetSingleton();
            if (vm) {
                auto* policy = vm->GetObjectHandlePolicy();
                RE::BSScript::Variable varOre;
                if (vm->GetPropertyValue(a_scriptObj, "Ore", varOre) && varOre.IsObject()) {
                    auto obj = varOre.GetObject();
                    if (obj && policy) {
                        auto h = obj->GetHandle();
                        auto* form = static_cast<RE::TESForm*>(policy->GetObjectForHandle(RE::TESForm::FORMTYPE, h));
                        if (form) {
                            if (auto* bound = form->As<RE::TESBoundObject>()) {
                                return bound;
                            }
                        }
                    }
                }
            }
        }

        // 2. Проверяем точные vanilla FormID жил в Skyrim.esm
        RE::FormID baseId = a_base->GetFormID() & 0x00FFFFFF;
        switch (baseId) {
            // Orichalcum (0x0005ACDD)
            case 0x000A2BEF: case 0x000A2C63: case 0x000A2C64:
            case 0x000A2C78: case 0x000A2C79: case 0x000A2C7A: case 0x000A2C7B: case 0x000A2C7C:
            case 0x0010ECE4:
                return RE::TESForm::LookupByID<RE::TESBoundObject>(0x0005ACDD);
            // Iron (0x00071CF3)
            case 0x000A2BEB: case 0x000A2C5F: case 0x000A2C60:
            case 0x000A2C6D: case 0x000A2C6E: case 0x000A2C6F: case 0x000A2C70: case 0x000A2C71:
            case 0x0010ECE3:
                return RE::TESForm::LookupByID<RE::TESBoundObject>(0x00071CF3);
            // Corundum (0x0005ACDB)
            case 0x000A2BE5: case 0x000A2C59: case 0x000A2C5A:
            case 0x000A2C6B: case 0x000A2C6C:
                return RE::TESForm::LookupByID<RE::TESBoundObject>(0x0005ACDB);
            // Ebony (0x0005ACDC)
            case 0x000A2BE7: case 0x000A2C5B: case 0x000A2C5C:
            case 0x000A2C72: case 0x000A2C73:
                return RE::TESForm::LookupByID<RE::TESBoundObject>(0x0005ACDC);
            // Gold (0x0005ACDE)
            case 0x000A2BE9: case 0x000A2C5D: case 0x000A2C5E:
            case 0x000A2C74: case 0x000A2C75: case 0x000E2BC2:
                return RE::TESForm::LookupByID<RE::TESBoundObject>(0x0005ACDE);
            // Moonstone (0x0005ACE0)
            case 0x000A2BED: case 0x000A2C61: case 0x000A2C62:
            case 0x000A2C76: case 0x000A2C77:
                return RE::TESForm::LookupByID<RE::TESBoundObject>(0x0005ACE0);
            // Quicksilver (0x0005ACE2)
            case 0x000A2BF1: case 0x000A2C65: case 0x000A2C66:
            case 0x000A2C7D: case 0x000A2C7E:
                return RE::TESForm::LookupByID<RE::TESBoundObject>(0x0005ACE2);
            // Silver (0x0005ACDF)
            case 0x000A2BF3: case 0x000A2C67: case 0x000A2C68:
            case 0x000A2C7F: case 0x000A2C80:
                return RE::TESForm::LookupByID<RE::TESBoundObject>(0x0005ACDF);
            // Malachite (0x0005ACE1)
            case 0x000A2BF5: case 0x000A2C69: case 0x000A2C6A:
            case 0x000A2C81: case 0x000A2C82:
                return RE::TESForm::LookupByID<RE::TESBoundObject>(0x0005ACE1);
        }

        // 3. Определяем по EditorID и названию (рус/англ)
        std::string s;
        const char* edid = a_base->GetFormEditorID();
        if (edid && edid[0]) {
            s += ToLowerUtf8(edid);
        }
        std::string name = a_refr->GetName();
        if (name.empty()) name = a_base->GetName();
        if (!name.empty()) {
            s += " " + ToLowerUtf8(name);
        }

        // Орихалк (0x0005ACDD)
        if (s.find("orichalcum") != std::string::npos || s.find("\xd0\xbe\xd1\x80\xd0\xb8\xd1\x85\xd0\xb0\xd0\xbb\xd0\xba") != std::string::npos) {
            return RE::TESForm::LookupByID<RE::TESBoundObject>(0x0005ACDD);
        }
        // Железо (0x00071CF3)
        if (s.find("iron") != std::string::npos || s.find("\xd0\xb6\xd0\xb5\xd0\xbb\xd0\xb5\xd0\xb7") != std::string::npos) {
            return RE::TESForm::LookupByID<RE::TESBoundObject>(0x00071CF3);
        }
        // Корунд (0x0005ACDB)
        if (s.find("corundum") != std::string::npos || s.find("\xd0\xba\xd0\xbe\xd1\x80\xd1\x83\xd0\xbd\xd0\xb4") != std::string::npos) {
            return RE::TESForm::LookupByID<RE::TESBoundObject>(0x0005ACDB);
        }
        // Эбонит (0x0005ACDC)
        if (s.find("ebony") != std::string::npos || s.find("\xd1\x8d\xd0\xb1\xd0\xbe\xd0\xbd\xd0\xb8\xd1\x82") != std::string::npos) {
            return RE::TESForm::LookupByID<RE::TESBoundObject>(0x0005ACDC);
        }
        // Золото (0x0005ACDE)
        if (s.find("gold") != std::string::npos || s.find("\xd0\xb7\xd0\xbe\xd0\xbb\xd0\xbe\xd1\x82") != std::string::npos) {
            return RE::TESForm::LookupByID<RE::TESBoundObject>(0x0005ACDE);
        }
        // Серебро (0x0005ACDF)
        if (s.find("silver") != std::string::npos || s.find("\xd1\x81\xd0\xb5\xd1\x80\xd0\xb5\xd0\xb1\xd1\x80") != std::string::npos) {
            return RE::TESForm::LookupByID<RE::TESBoundObject>(0x0005ACDF);
        }
        // Лунный камень (0x0005ACE0)
        if (s.find("moonstone") != std::string::npos || s.find("\xd0\xbb\xd1\x83\xd0\xbd\xd0\xbd") != std::string::npos) {
            return RE::TESForm::LookupByID<RE::TESBoundObject>(0x0005ACE0);
        }
        // Малахит (0x0005ACE1)
        if (s.find("malachite") != std::string::npos || s.find("\xd0\xbc\xd0\xb0\xd0\xbb\xd0\xb0\xd1\x85\xd0\xb8\xd1\x82") != std::string::npos) {
            return RE::TESForm::LookupByID<RE::TESBoundObject>(0x0005ACE1);
        }
        // Ртуть (0x0005ACE2)
        if (s.find("quicksilver") != std::string::npos || s.find("\xd1\x80\xd1\x82\xd1\x83\xd1\x82") != std::string::npos) {
            return RE::TESForm::LookupByID<RE::TESBoundObject>(0x0005ACE2);
        }
        // Сталгрим (Dragonborn)
        if (s.find("stalhrim") != std::string::npos || s.find("\xd1\x81\xd1\x82\xd0\xb0\xd0\xbb\xd0\xb3\xd1\x80\xd0\xb8\xd0\xbc") != std::string::npos) {
            auto* item = RE::TESForm::LookupByEditorID<RE::TESBoundObject>("DLC2OreStalhrim");
            if (!item) item = RE::TESForm::LookupByEditorID<RE::TESBoundObject>("DLC2Stalhrim");
            if (item) return item;
        }
        // Глина (Hearthfire)
        if (s.find("clay") != std::string::npos || s.find("\xd0\xb3\xd0\xbb\xd0\xb8\xd0\xbd") != std::string::npos) {
            auto* item = RE::TESForm::LookupByEditorID<RE::TESBoundObject>("BYOHMaterialClay");
            if (item) return item;
        }
        // Карьерный камень (Hearthfire)
        if (s.find("stone") != std::string::npos || s.find("\xd0\xba\xd0\xb0\xd0\xbc\xd0\xb5\xd0\xbd") != std::string::npos) {
            auto* item = RE::TESForm::LookupByEditorID<RE::TESBoundObject>("BYOHMaterialQuarriedStone");
            if (item) return item;
        }

        return RE::TESForm::LookupByID<RE::TESBoundObject>(0x00071CF3); // По умолчанию железная руда
    }

    static std::unordered_set<RE::FormID> s_depletedVeins;
    static std::unordered_set<RE::FormID> s_merchantChests;
    static std::unordered_set<RE::FormID> s_harvestedRefs;

    void HarvestManager::ClearDepletedCache()
    {
        s_depletedVeins.clear();
        s_harvestedRefs.clear();
    }

    void HarvestManager::InitMerchantChests()
    {
        s_merchantChests.clear();
        auto* dh = RE::TESDataHandler::GetSingleton();
        if (!dh) return;

        auto& factions = dh->GetFormArray<RE::TESFaction>();
        for (auto* faction : factions) {
            if (faction && faction->vendorData.merchantContainer) {
                s_merchantChests.insert(faction->vendorData.merchantContainer->GetFormID());
                auto* base = faction->vendorData.merchantContainer->GetBaseObject();
                if (base) {
                    s_merchantChests.insert(base->GetFormID());
                }
            }
        }
    }

    size_t HarvestManager::GetMerchantChestsCount() const
    {
        return s_merchantChests.size();
    }

    static bool IsMerchantChest(RE::TESObjectREFR* a_refr, RE::TESBoundObject* a_base)
    {
        if (!a_refr || !a_base) return false;

        // Если кэш ещё не инициализирован, заполняем его
        if (s_merchantChests.empty()) {
            HarvestManager::GetSingleton().InitMerchantChests();
        }

        // 1. Прямая проверка по FormID торговых сундуков всех фракций Скайрима (включая CarlottaChest 0x000ABB41)
        if (s_merchantChests.contains(a_refr->GetFormID()) || s_merchantChests.contains(a_base->GetFormID())) {
            return true;
        }

        // 2. Проверка владельца (Торговая фракция или NPC-торговец)
        auto* owner = a_refr->GetOwner();
        if (owner) {
            if (auto* faction = owner->As<RE::TESFaction>()) {
                if (faction->IsVendor() || faction->vendorData.merchantContainer == a_refr) {
                    s_merchantChests.insert(a_refr->GetFormID());
                    return true;
                }
            } else if (auto* npc = owner->As<RE::TESNPC>()) {
                for (const auto& f : npc->factions) {
                    if (f.faction && (f.faction->IsVendor() || f.faction->vendorData.merchantContainer == a_refr)) {
                        s_merchantChests.insert(a_refr->GetFormID());
                        return true;
                    }
                }
            }
        }

        // 3. Проверка EditorID (если доступен)
        const char* baseEdid = a_base->GetFormEditorID();
        if (baseEdid && baseEdid[0]) {
            std::string s = baseEdid;
            for (auto& c : s) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
            if (s.find("merchant") != std::string::npos ||
                s.find("vendor") != std::string::npos ||
                s.find("donotdelete") != std::string::npos ||
                s.find("holdingchest") != std::string::npos) {
                s_merchantChests.insert(a_refr->GetFormID());
                return true;
            }
        }

        const char* refEdid = a_refr->GetFormEditorID();
        if (refEdid && refEdid[0]) {
            std::string s = refEdid;
            for (auto& c : s) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
            if (s.find("merchant") != std::string::npos ||
                s.find("vendor") != std::string::npos ||
                s.find("donotdelete") != std::string::npos ||
                s.find("holdingchest") != std::string::npos) {
                s_merchantChests.insert(a_refr->GetFormID());
                return true;
            }
        }

        // 4. Проверка ключевых слов (Keywords)
        if (auto* kw = a_base->As<RE::BGSKeywordForm>()) {
            if (kw->HasKeywordString("VendorChest") || 
                kw->HasKeywordString("MerchantChest") ||
                kw->HasKeywordString("isMerchantChest") ||
                kw->HasKeywordString("VendorContainer")) {
                s_merchantChests.insert(a_refr->GetFormID());
                return true;
            }
        }
        if (auto* kw = a_refr->As<RE::BGSKeywordForm>()) {
            if (kw->HasKeywordString("VendorChest") || 
                kw->HasKeywordString("MerchantChest") ||
                kw->HasKeywordString("isMerchantChest") ||
                kw->HasKeywordString("VendorContainer")) {
                s_merchantChests.insert(a_refr->GetFormID());
                return true;
            }
        }

        return false;
    }

    static bool PassesValueFilter(RE::TESBoundObject* a_item, RE::InventoryEntryData* a_entry, float a_minVal)
    {
        if (a_minVal <= 0.0f) {
            return true;
        }

        float weight = a_item->GetWeight();
        int gold = a_item->GetGoldValue();
        if (a_entry) {
            int entryVal = a_entry->GetValue();
            if (entryVal > 0) {
                gold = entryVal;
            }
        }

        // 1. Если абсолютная стоимость предмета удовлетворяет порогу (цена >= N, например 1) -> собираем!
        if (static_cast<float>(gold) >= a_minVal) {
            return true;
        }

        // 2. Если соотношение цена/вес удовлетворяет порогу -> собираем!
        if (weight > 0.0f) {
            if ((static_cast<float>(gold) / weight) >= a_minVal) {
                return true;
            }
        }

        // 3. Невесомые ценные предметы (вес <= 0, цена > 0)
        if (weight <= 0.0f && gold > 0) {
            return true;
        }

        return false;
    }

    static bool ShouldLootBoundItem(RE::TESBoundObject* a_item, RE::InventoryEntryData* a_entry, const Config& a_cfg)
    {
        if (!a_item) return false;

        // 0. Custom Whitelist check (ALWAYS loots whitelisted items!)
        if (a_cfg.IsWhitelisted(a_item->GetFormID())) {
            return true;
        }

        // Firewood is heavy clutter (5.0 weight each) - ignore unless explicitly whitelisted!
        if (IsFirewood(a_item)) {
            return false;
        }

        // Junk & construction clutter (Glass, Straw, Blank scrolls, Paper rolls, Goat horns) - ignore unless explicitly whitelisted!
        if (IsJunkClutter(a_item)) {
            return false;
        }

        // 1. Gold / Septims (никогда не фильтруется по цене/весу)
        if (a_cfg.harvestCoins && a_item->Is(RE::FormType::Misc) && a_item->GetFormID() == 0x0000000F) {
            return true;
        }

        // 2. Lockpicks & Keys (отмычки и ключи жизненно важны, у ключей цена 0)
        if (a_cfg.harvestLockpicks && a_item->GetFormID() == 0x0000000A) {
            return true;
        }

        auto* kw = a_item->As<RE::BGSKeywordForm>();

        // Keys (KeyMaster form type, or VendorItemKey)
        if (a_cfg.harvestKeys && (a_item->Is(RE::FormType::KeyMaster) || (kw && kw->HasKeywordString("VendorItemKey")))) {
            return true;
        }

        // 3. Gems & Soul Gems (драгоценные камни и камни душ всегда полезны)
        if (a_cfg.harvestGems && a_item->Is(RE::FormType::Misc) && kw && kw->HasKeywordString("VendorItemGem")) {
            return true;
        }

        if (a_cfg.harvestSoulGems && a_item->Is(RE::FormType::SoulGem)) {
            return true;
        }

        // 4. Ingredients (алхимические ингредиенты)
        if (a_cfg.harvestIngredients && a_item->Is(RE::FormType::Ingredient)) {
            return true;
        }

        // 5. Ammo (стрелы и болты)
        if (a_cfg.harvestAmmo && a_item->Is(RE::FormType::Ammo)) {
            return true;
        }

        // 6. Ore & Metal Ingots (руда и слитки металлов)
        if (a_cfg.harvestOreIngots && a_item->Is(RE::FormType::Misc) && kw && kw->HasKeywordString("VendorItemOreIngot")) {
            return true;
        }

        // 7. Books (Spell Tomes, Skill Books, Notes & Books)
        if (auto* book = a_item->As<RE::TESObjectBOOK>()) {
            if (!book->CanBeTaken()) return false;
            if (book->TeachesSpell()) {
                return a_cfg.harvestSpellbooks;
            }
            if (book->TeachesSkill()) {
                return a_cfg.harvestSkillbooks;
            }
            if (a_cfg.harvestBooks) {
                return PassesValueFilter(a_item, a_entry, a_cfg.minValPerWeight);
            }
            return false;
        }

        // Scrolls
        if (a_cfg.harvestScrolls && (a_item->Is(RE::FormType::Scroll) || (kw && kw->HasKeywordString("VendorItemScroll")))) {
            return PassesValueFilter(a_item, a_entry, a_cfg.minValPerWeight);
        }

        // 8. Potions, Poisons, Food & Drinks
        if (auto* alch = a_item->As<RE::AlchemyItem>()) {
            if (alch->IsFood()) {
                bool isDrink = (alch->data.consumptionSound && alch->data.consumptionSound->GetFormID() == 0x000B6435) ||
                               (kw && kw->HasKeywordString("VendorItemDrink"));
                if (isDrink) {
                    if (!a_cfg.harvestDrinks) return false;
                } else {
                    if (!a_cfg.harvestFood) return false;
                }
            } else {
                if (!a_cfg.harvestPotions) return false;
            }
            return PassesValueFilter(a_item, a_entry, a_cfg.minValPerWeight);
        }

        // 9. Animal Pelts, Hides & Parts
        if (a_cfg.harvestAnimalParts && a_item->Is(RE::FormType::Misc) && kw) {
            if (kw->HasKeywordString("VendorItemAnimalHide") || kw->HasKeywordString("VendorItemAnimalPart")) {
                return PassesValueFilter(a_item, a_entry, a_cfg.minValPerWeight);
            }
        }

        // 10. Weapons (Enchanted & Standard)
        if (auto* weapon = a_item->As<RE::TESObjectWEAP>()) {
            if (!weapon->GetPlayable()) return false;
            if (!PassesValueFilter(a_item, a_entry, a_cfg.minValPerWeight)) return false;
            bool isEnchanted = (weapon->formEnchanting != nullptr) || (a_entry && a_entry->IsEnchanted());
            if (isEnchanted) {
                return a_cfg.harvestEnchantedWeapons || a_cfg.harvestWeapons;
            } else {
                return a_cfg.harvestWeapons;
            }
        }

        // 11. Armor & Jewelry (Rings, Amulets, Enchanted & Standard Armor)
        if (auto* armor = a_item->As<RE::TESObjectARMO>()) {
            if (!armor->GetPlayable()) return false;
            if (!PassesValueFilter(a_item, a_entry, a_cfg.minValPerWeight)) return false;
            bool isEnchanted = (armor->formEnchanting != nullptr) || (a_entry && a_entry->IsEnchanted());
            bool isJewelry = armor->HasPartOf(RE::BGSBipedObjectForm::BipedObjectSlot::kRing) ||
                             armor->HasPartOf(RE::BGSBipedObjectForm::BipedObjectSlot::kAmulet) ||
                             (kw && (kw->HasKeywordString("VendorItemRing") || kw->HasKeywordString("VendorItemNecklace") || kw->HasKeywordString("VendorItemJewelry")));
            if (isJewelry) {
                if (a_cfg.harvestJewelry) return true;
                if (isEnchanted && a_cfg.harvestEnchantedArmor) return true;
                return false;
            }
            if (isEnchanted) {
                return a_cfg.harvestEnchantedArmor || a_cfg.harvestArmor;
            } else {
                return a_cfg.harvestArmor;
            }
        }

        // 12. Miscellaneous & Modded items (Crafting components, claws, tokens, trophies, torches)
        if (a_cfg.harvestMisc) {
            if (a_item->Is(RE::FormType::Misc)) {
                if (a_item->formFlags & 0x04) return false; // kNonPlayable
                return PassesValueFilter(a_item, a_entry, a_cfg.minValPerWeight);
            }
            if (a_item->Is(RE::FormType::Light)) {
                auto* light = a_item->As<RE::TESObjectLIGH>();
                if (light && light->CanBeCarried()) {
                    return PassesValueFilter(a_item, a_entry, a_cfg.minValPerWeight);
                }
            }
        }

        return false;
    }

    static bool CanLootCrime(RE::TESObjectREFR* a_refr, RE::PlayerCharacter* a_player, const Config& a_cfg)
    {
        if (!a_refr->IsCrimeToActivate()) {
            return true;
        }

        // 0 = Never steal
        if (a_cfg.stealingMode == 0) {
            return false;
        }

        // 1 = Steal only in sneak
        if (a_cfg.stealingMode == 1) {
            if (!a_player->IsSneaking()) return false;
            return true;
        }

        // 2 = Steal always
        return true;
    }

    bool HarvestManager::ProcessReference(RE::TESObjectREFR* a_refr, RE::PlayerCharacter* a_player, const Config& a_cfg)
    {
        if (!a_refr || !a_player || a_refr == a_player) return false;
        if (a_refr->IsDisabled() || a_refr->IsDeleted()) return false;

        // 1. ПРОВЕРКА СУЩЕСТВ И NPC (Лисы, кролики, люди, животные)
        if (auto* actor = a_refr->As<RE::Actor>()) {
            // ЖИВЫХ существ НИКОГДА не активируем и не открываем меню!
            if (!actor->IsDead()) {
                return false;
            }

            // Мёртвые тела: лутаем ТОЛЬКО если включена опция harvestDeadBodies, и БЕЗ вызова меню!
            if (a_cfg.harvestDeadBodies) {
                if (!CanLootCrime(a_refr, a_player, a_cfg)) return false;
                auto inv = actor->GetInventory();
                bool lootedAny = false;
                auto* equipMgr = RE::ActorEquipManager::GetSingleton();

                for (const auto& [item, itemData] : inv) {
                    if (!item || itemData.first <= 0) continue;
                    if (ShouldLootBoundItem(item, itemData.second.get(), a_cfg)) {
                        if (itemData.second && itemData.second->IsWorn() && equipMgr) {
                            equipMgr->UnequipObject(actor, item, nullptr, itemData.first, nullptr, false, true, false, true);
                        }
                        actor->RemoveItem(item, itemData.first, RE::ITEM_REMOVE_REASON::kRemove, nullptr, a_player);
                        lootedAny = true;
                    }
                }
                return lootedAny;
            }
            return false;
        }

        if (!CanLootCrime(a_refr, a_player, a_cfg)) {
            return false;
        }

        // 2. Стрелы и болты, воткнутые в поверхности / объекты
        if (a_cfg.harvestAmmo) {
            auto formType = a_refr->GetFormType();
            if (formType == RE::FormType::ProjectileArrow || formType == RE::FormType::ProjectileMissile) {
                if (auto* proj = a_refr->AsProjectile()) {
                    auto& data = proj->GetProjectileRuntimeData();
                    if (auto* ammo = data.ammoSource) {
                        bool destroyed = (data.flags & (1 << 25)) != 0;
                        bool impacted = (data.flags & (1 << 22)) != 0;
                        float speedSq = std::max(data.linearVelocity.SqrLength(), data.velocity.SqrLength());
                        if (!destroyed && (impacted || speedSq < 0.05f)) {
                            if (ShouldLootBoundItem(ammo, nullptr, a_cfg)) {
                                RE::FormID refId = a_refr->GetFormID();
                                if (refId != 0 && s_harvestedRefs.contains(refId)) {
                                    return false;
                                }

                                if (refId != 0) {
                                    s_harvestedRefs.insert(refId);
                                }

                                std::string name = ammo->GetName();
                                if (name.empty()) name = a_refr->GetName();

                                // Напрямую добавляем стрелу в инвентарь игрока
                                a_player->AddObjectToContainer(ammo, nullptr, 1, nullptr);

                                // Удаляем снаряд из мира игры, исключая повторный сбор
                                a_refr->Disable();
                                a_refr->SetDelete(true);

                                NotifyLoot(name.empty() ? "Ammo" : name, "ammo", 1);
                                return true;
                            }
                        }
                    }
                }
                return false;
            }
        }

        auto* base = a_refr->GetBaseObject();
        if (!base) return false;

        // 2. Флора и деревья с урожаем (цветы, грибы, мох, корни нирна)
        if (a_cfg.harvestFlora) {
            bool isFloraOrTree = base->Is(RE::FormType::Flora) || base->Is(RE::FormType::Tree);
            bool isNirnroot = false;

            if (!isFloraOrTree && base->Is(RE::FormType::Activator)) {
                if (!s_harvestedRefs.contains(a_refr->GetFormID())) {
                    RE::FormID rawBaseId = base->GetFormID() & 0x00FFFFFF;
                    // Vanilla Nirnroot (0x000E1FB2) and Crimson Nirnroot (0x000B6FB9)
                    if (rawBaseId == 0x000E1FB2 || rawBaseId == 0x000B6FB9) {
                        isNirnroot = true;
                    } else {
                        const char* edid = base->GetFormEditorID();
                        if (edid && edid[0]) {
                            std::string edidLower = edid;
                            for (auto& c : edidLower) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
                            if ((edidLower.find("nirnroot") != std::string::npos ||
                                 edidLower.find("treefloranirn") != std::string::npos) &&
                                edidLower.find("empty") == std::string::npos &&
                                edidLower.find("glow") == std::string::npos) {
                                isNirnroot = true;
                            }
                        }
                        if (!isNirnroot) {
                            std::string name = a_refr->GetName();
                            if (name.empty()) name = base->GetName();
                            if (!name.empty()) {
                                std::string nameLower = ToLowerUtf8(name);
                                if (nameLower.find("\xd0\xbd\xd0\xb8\xd1\x80\xd0\xbd") != std::string::npos || // нирн
                                    nameLower.find("nirnroot") != std::string::npos) {
                                    isNirnroot = true;
                                }
                            }
                        }
                    }
                }
            }

            if (isFloraOrTree || isNirnroot) {
                if (!(a_refr->formFlags & RE::TESObjectREFR::RecordFlags::kHarvested)) {
                    // Проверяем урожай (produce item): если это дрова или хлам и не в белом списке — пропускаем
                    if (auto* produce = base->As<RE::TESProduceForm>()) {
                        if (produce->produceItem && (IsFirewood(produce->produceItem) || IsJunkClutter(produce->produceItem))) {
                            if (!a_cfg.IsWhitelisted(produce->produceItem->GetFormID()) &&
                                !a_cfg.IsWhitelisted(base->GetFormID())) {
                                return false;
                            }
                        }
                    }

                    // Проверяем имя поленницы / кучи дров / соломы
                    std::string name = a_refr->GetName();
                    if (name.empty()) name = base->GetName();
                    std::string nameLower = ToLowerUtf8(name);
                    if (nameLower.find("\xd0\xbf\xd0\xbe\xd0\xbb\xd0\xb5\xd0\xbd") != std::string::npos ||     // полен (поленница)
                        nameLower.find("\xd0\xbf\xd0\xbe\xd0\xbb\xd0\xb5\xd1\x88\xd0\xba") != std::string::npos || // полешк
                        nameLower.find("firewood") != std::string::npos ||
                        nameLower.starts_with("\xd0\xb4\xd1\x80\xd0\xbe\xd0\xb2") ||                               // дров... (дрова)
                        nameLower.find(" \xd0\xb4\xd1\x80\xd0\xbe\xd0\xb2") != std::string::npos ||                // ... дров
                        nameLower.find("\xd1\x81\xd0\xbe\xd0\xbb\xd0\xbe\xd0\xbc") != std::string::npos ||         // солом (солома, пучок соломы...)
                        (nameLower.find("straw") != std::string::npos && nameLower.find("strawberry") == std::string::npos)) {
                        if (!a_cfg.IsWhitelisted(base->GetFormID())) {
                            return false;
                        }
                    }

                    if (isNirnroot) {
                        s_harvestedRefs.insert(a_refr->GetFormID());
                    }

                    a_refr->ActivateRef(a_player, 0, nullptr, 1, false);
                    NotifyLoot(name.empty() ? "Flora" : name, "flora", 1);
                    return true;
                }
                return false;
            }
        }

        // 3. Мелкие летающие насекомые и рыба (СТРОГО Activator, а не живые животные!)
        if (a_cfg.harvestCritters && base->Is(RE::FormType::Activator)) {
            const char* edid = base->GetFormEditorID();
            std::string s = edid ? edid : "";
            const char* n = base->GetName();
            if (n) s += n;
            for (auto& c : s) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));

            if (s.find("critter") != std::string::npos ||
                s.find("moth") != std::string::npos ||
                s.find("firefly") != std::string::npos ||
                s.find("butterfly") != std::string::npos ||
                s.find("dragonfly") != std::string::npos ||
                s.find("bee") != std::string::npos ||
                s.find("salmon") != std::string::npos ||
                s.find("\xd1\x81\xd0\xb2\xd0\xb5\xd1\x82\xd0\xbb\xd1\x8f\xd1\x87") != std::string::npos || // светляч
                s.find("\xd0\xb1\xd0\xb0\xd0\xb1\xd0\xbe\xd1\x87\xd0\xba") != std::string::npos ||         // бабочк
                s.find("\xd0\xbc\xd0\xbe\xd1\x82\xd1\x8b\xd0\xbb") != std::string::npos ||             // мотыл
                s.find("\xd1\x81\xd1\x82\xd1\x80\xd0\xb5\xd0\xba\xd0\xbe\xd0\xb7") != std::string::npos ||   // стрекоз
                s.find("\xd0\xbf\xd1\x87\xd0\xb5\xd0\xbb") != std::string::npos ||                     // пчел
                s.find("\xd0\xbb\xd0\xbe\xd1\x81\xd0\xbe\xd1\x81") != std::string::npos) {             // лосос
                std::string critterName = a_refr->GetName();
                if (critterName.empty()) critterName = base->GetName();
                a_refr->ActivateRef(a_player, 0, nullptr, 1, false);
                NotifyLoot(critterName.empty() ? "Critter" : critterName, "critter", 1);
                return true;
            }
        }

        // 4. Кучи пепла (Ash Piles)
        if (a_cfg.harvestAshPiles && base->Is(RE::FormType::Activator)) {
            auto* extraAsh = a_refr->extraList.GetByType<RE::ExtraAshPileRef>();
            if (extraAsh) {
                RE::TESObjectREFR* targetSource = a_refr;
                auto actorRef = extraAsh->ashPileRef.get();
                if (actorRef) {
                    targetSource = actorRef.get();
                }

                auto inv = targetSource->GetInventory();
                if (inv.empty() && targetSource != a_refr) {
                    inv = a_refr->GetInventory();
                    targetSource = a_refr;
                }

                bool lootedAny = false;
                auto* actorTarget = targetSource->As<RE::Actor>();
                auto* equipMgr = RE::ActorEquipManager::GetSingleton();

                for (const auto& [item, itemData] : inv) {
                    if (!item || itemData.first <= 0) continue;
                    if (ShouldLootBoundItem(item, itemData.second.get(), a_cfg)) {
                        if (actorTarget && itemData.second && itemData.second->IsWorn() && equipMgr) {
                            equipMgr->UnequipObject(actorTarget, item, nullptr, itemData.first, nullptr, false, true, false, true);
                        }
                        targetSource->RemoveItem(item, itemData.first, RE::ITEM_REMOVE_REASON::kRemove, nullptr, a_player);
                        lootedAny = true;
                    }
                }
                return lootedAny;
            }
        }

        // 5. Авто-добыча руды из жил (Ore Veins)
        if (a_cfg.harvestOreVeins && (base->Is(RE::FormType::Activator) || base->Is(RE::FormType::Furniture))) {
            RE::TESObjectREFR* targetRefr = a_refr;

            // Если это маркер мебели для добычи (PickaxeMining...Marker), перенаправляем на связанный активатор жилы
            if (base->Is(RE::FormType::Furniture)) {
                if (auto* linked = a_refr->GetLinkedRef(nullptr)) {
                    if (linked->GetBaseObject() && linked->GetBaseObject()->Is(RE::FormType::Activator)) {
                        targetRefr = linked;
                        base = targetRefr->GetBaseObject();
                    }
                } else {
                    return false;
                }
            }

            if (targetRefr->formFlags & RE::TESForm::RecordFlags::kDestroyed) {
                return false;
            }

            if (s_depletedVeins.contains(targetRefr->GetFormID()) || s_depletedVeins.contains(a_refr->GetFormID())) {
                return false;
            }

            bool isOreVein = false;
            RE::FormID rawBaseId = base->GetFormID() & 0x00FFFFFF;

            // Проверка по диапазонам FormID ванильных жил Skyrim.esm
            if ((rawBaseId >= 0x000A2BE5 && rawBaseId <= 0x000A2CB1) ||
                (rawBaseId >= 0x0010DC2A && rawBaseId <= 0x0010F0E7) ||
                rawBaseId == 0x000E2BC2) {
                isOreVein = true;
            }

            // Проверка по FormEditorID
            if (!isOreVein) {
                const char* edid = base->GetFormEditorID();
                if (edid && edid[0]) {
                    std::string edidLower = ToLowerUtf8(edid);
                    if (edidLower.find("mineore") != std::string::npos ||
                        edidLower.find("orevein") != std::string::npos ||
                        edidLower.find("mine_ore") != std::string::npos ||
                        edidLower.find("outcrop") != std::string::npos ||
                        edidLower.find("stalhrim") != std::string::npos ||
                        edidLower.find("claydeposit") != std::string::npos ||
                        edidLower.find("stonequarry") != std::string::npos) {
                        isOreVein = true;
                    }
                }
            }

            // Проверка по имени (рус/англ)
            if (!isOreVein) {
                std::string nameLower = ToLowerUtf8(targetRefr->GetName());
                if (nameLower.empty()) nameLower = ToLowerUtf8(base->GetName());
                if (!nameLower.empty()) {
                    if (nameLower.find("\xd0\xb6\xd0\xb8\xd0\xbb") != std::string::npos ||         // жил (жила, жилы...)
                        nameLower.find("\xd0\xb7\xd0\xb0\xd0\xbb\xd0\xb5\xd0\xb6") != std::string::npos ||     // залеж (залежь...)
                        nameLower.find("\xd0\xb2\xd1\x8b\xd1\x85\xd0\xbe\xd0\xb4") != std::string::npos ||     // выход (выход орихалка, выход руды...)
                        nameLower.find("\xd1\x80\xd1\x83\xd0\xb4") != std::string::npos ||         // руд (руда, рудный...)
                        nameLower.find("\xd0\xbf\xd0\xbb\xd0\xb0\xd1\x81\xd1\x82") != std::string::npos ||     // пласт
                        nameLower.find("\xd0\xbc\xd0\xb5\xd1\x81\xd1\x82\xd0\xbe\xd1\x80\xd0\xbe\xd0\xb6\xd0\xb4") != std::string::npos || // месторожд
                        nameLower.find("\xd0\xba\xd0\xb0\xd1\x80\xd1\x8c\xd0\xb5\xd1\x80") != std::string::npos ||     // карьер
                        nameLower.find("\xd1\x81\xd1\x82\xd0\xb0\xd0\xbb\xd0\xb3\xd1\x80\xd0\xb8\xd0\xbc") != std::string::npos || // сталгрим
                        nameLower.find("\xd0\xbe\xd1\x80\xd0\xb8\xd1\x85\xd0\xb0\xd0\xbb\xd0\xba") != std::string::npos || // орихалк
                        nameLower.find("\xd0\xba\xd0\xbe\xd1\x80\xd1\x83\xd0\xbd\xd0\xb4") != std::string::npos ||     // корунд
                        nameLower.find("\xd1\x8d\xd0\xb1\xd0\xbe\xd0\xbd\xd0\xb8\xd1\x82") != std::string::npos ||     // эбонит
                        nameLower.find("\xd0\xbc\xd0\xb0\xd0\xbb\xd0\xb0\xd1\x85\xd0\xb8\xd1\x82") != std::string::npos || // малахит
                        nameLower.find("vein") != std::string::npos ||
                        nameLower.find("ore") != std::string::npos ||
                        nameLower.find("deposit") != std::string::npos ||
                        nameLower.find("outcrop") != std::string::npos ||
                        nameLower.find("quarry") != std::string::npos ||
                        nameLower.find("stalhrim") != std::string::npos) {
                        isOreVein = true;
                    }
                }
            }

            if (!isOreVein) {
                return false;
            }

            // Жила обнаружена!
            int countToMine = 3;
            RE::BSTSmartPointer<RE::BSScript::Object> scriptObj;

            auto* vm = RE::BSScript::Internal::VirtualMachine::GetSingleton();
            if (vm) {
                auto* policy = vm->GetObjectHandlePolicy();
                if (policy) {
                    auto handle = policy->GetHandleForObject(RE::TESObjectREFR::FORMTYPE, targetRefr);
                    if (handle != policy->EmptyHandle()) {
                        if (!vm->FindBoundObject(handle, "MineOreScript", scriptObj) || !scriptObj) {
                            vm->FindBoundObject(handle, "mineorescript", scriptObj);
                        }
                    }
                    if (!scriptObj && a_refr != targetRefr) {
                        auto hOrig = policy->GetHandleForObject(RE::TESObjectREFR::FORMTYPE, a_refr);
                        if (hOrig != policy->EmptyHandle()) {
                            if (!vm->FindBoundObject(hOrig, "MineOreScript", scriptObj) || !scriptObj) {
                                vm->FindBoundObject(hOrig, "mineorescript", scriptObj);
                            }
                        }
                    }
                }
            }

            if (scriptObj && vm) {
                RE::BSScript::Variable varCur;
                if (vm->GetPropertyValue(scriptObj, "ResourceCountCurrent", varCur) && varCur.IsInt()) {
                    int cur = varCur.GetSInt();
                    if (cur == 0) {
                        s_depletedVeins.insert(targetRefr->GetFormID());
                        s_depletedVeins.insert(a_refr->GetFormID());
                        return false;
                    }
                    if (cur > 0) {
                        countToMine = cur;
                    } else {
                        RE::BSScript::Variable varTotal;
                        if (vm->GetPropertyValue(scriptObj, "ResourceCountTotal", varTotal) && varTotal.IsInt()) {
                            int total = varTotal.GetSInt();
                            if (total > 0) countToMine = total;
                        }
                    }
                }

                // Устанавливаем ResourceCountCurrent в 0, чтобы Papyrus знал, что жила истощена
                RE::BSScript::Variable zeroVar;
                zeroVar.SetSInt(0);
                vm->SetPropertyValue(scriptObj, "ResourceCountCurrent", zeroVar);
            }

            // Помечаем жилу и связанные маркеры как истощенные в кэше
            s_depletedVeins.insert(targetRefr->GetFormID());
            s_depletedVeins.insert(a_refr->GetFormID());
            if (auto* linked = targetRefr->GetLinkedRef(nullptr)) {
                s_depletedVeins.insert(linked->GetFormID());
            }
            if (auto* linked2 = a_refr->GetLinkedRef(nullptr)) {
                s_depletedVeins.insert(linked2->GetFormID());
            }

            // Визуальное истощение жилы (переключение модели на выработанную породу) и установка флага kDestroyed
            targetRefr->DamageObject(50.0f, true);
            targetRefr->formFlags |= RE::TESForm::RecordFlags::kDestroyed;
            if (a_refr != targetRefr) {
                a_refr->DamageObject(50.0f, true);
                a_refr->formFlags |= RE::TESForm::RecordFlags::kDestroyed;
            }

            // Нативно выдаем руду напрямую игроку
            auto* oreItem = GetOreItemForVein(targetRefr, base->As<RE::TESBoundObject>(), scriptObj);
            if (oreItem) {
                a_player->AddObjectToContainer(oreItem, nullptr, countToMine, nullptr);
            }

            // Шанс получить драгоценный камень при выработке жилы (аналог ванильного MineOreScript)
            if (a_cfg.harvestGems) {
                if (rand() % 100 < 20) {
                    auto* gemList = RE::TESForm::LookupByID<RE::TESBoundObject>(0x0001B325);
                    if (gemList) {
                        a_player->AddObjectToContainer(gemList, nullptr, 1, nullptr);
                    }
                }
            }

            std::string oreName = oreItem ? oreItem->GetName() : targetRefr->GetName();
            if (oreName.empty()) oreName = targetRefr->GetName();
            if (oreName.empty()) oreName = base->GetName();
            NotifyLoot(oreName.empty() ? "Ore Vein" : oreName, "ore", countToMine);
            return true;
        }

        // 6. Контейнеры (сундуки, бочки, урны) - тихий сбор без открытия меню
        if (a_cfg.harvestContainers && base->Is(RE::FormType::Container)) {
            // Защита от торговых сундуков (Merchant / Vendor Chests под текстурами и в лавках)
            if (IsMerchantChest(a_refr, base->As<RE::TESBoundObject>())) {
                return false;
            }

            if (a_cfg.protectLockedContainers && a_refr->IsLocked()) {
                return false;
            }

            if (a_cfg.protectBossContainers) {
                auto* extraLocRef = a_refr->extraList.GetByType<RE::ExtraLocationRefType>();
                if (extraLocRef && extraLocRef->locRefType) {
                    if (extraLocRef->locRefType->GetFormID() == 0x000130F8 || 
                        std::string_view(extraLocRef->locRefType->formEditorID.c_str()).find("Boss") != std::string_view::npos) {
                        return false;
                    }
                }
                if (auto* kw = a_refr->As<RE::BGSKeywordForm>()) {
                    if (kw->HasKeywordString("Boss") || kw->HasKeywordString("LocRefTypeBossChest")) {
                        return false;
                    }
                }
            }

            auto inv = a_refr->GetInventory();
            bool lootedAny = false;
            for (const auto& [item, itemData] : inv) {
                if (!item || itemData.first <= 0) continue;
                if (ShouldLootBoundItem(item, itemData.second.get(), a_cfg)) {
                    a_refr->RemoveItem(item, itemData.first, RE::ITEM_REMOVE_REASON::kRemove, nullptr, a_player);
                    lootedAny = true;
                }
            }
            return lootedAny;
        }

        // 7. Отдельно лежащие предметы на земле/столах (СТРОГО предметы, а не существа или контейнеры!)
        if (s_harvestedRefs.contains(a_refr->GetFormID())) {
            return false;
        }

        if (a_cfg.IsWhitelisted(base->GetFormID()) ||
            base->Is(RE::FormType::Misc) ||
            base->Is(RE::FormType::Ingredient) ||
            base->Is(RE::FormType::AlchemyItem) ||
            base->Is(RE::FormType::SoulGem) ||
            base->Is(RE::FormType::Book) ||
            base->Is(RE::FormType::Ammo) ||
            base->Is(RE::FormType::Weapon) ||
            base->Is(RE::FormType::Armor) ||
            base->Is(RE::FormType::KeyMaster) ||
            base->Is(RE::FormType::Scroll) ||
            base->Is(RE::FormType::Light)) {
            if (ShouldLootBoundItem(base->As<RE::TESBoundObject>(), nullptr, a_cfg)) {
                std::string name = a_refr->GetName();
                if (name.empty()) name = base->GetName();

                std::int32_t count = a_refr->extraList.GetCount();
                if (count <= 0) count = 1;

                s_harvestedRefs.insert(a_refr->GetFormID());

                // Забираем предмет напрямую в инвентарь игрока через нативный PickUpObject
                a_player->PickUpObject(a_refr, count, false, false);
                NotifyLoot(name.empty() ? "Item" : name, "misc", count);
                return true;
            }
        }

        return false;
    }

    void HarvestManager::Tick()
    {
        if (!running_.load()) return;

        const auto& cfg = Settings::GetSingleton().GetConfig();
        if (!cfg.enabled) return;

        auto* player = RE::PlayerCharacter::GetSingleton();
        if (!player || !player->Is3DLoaded() || player->IsDead()) return;

        // Отключение в бою
        if (cfg.pauseInCombat && player->IsInCombat()) return;

        // Отключение при обнажении оружия
        if (cfg.pauseWeaponDrawn) {
            if (player->AsActorState()->GetWeaponState() != RE::WEAPON_STATE::kSheathed) {
                return;
            }
        }

        // 1. Проверка нахождения в доме игрока (автоматическое исключение)
        auto* loc = player->GetCurrentLocation();
        auto* cell = player->GetParentCell();
        if (!loc && cell) {
            loc = cell->GetLocation();
        }

        if (cfg.ignorePlayerHouses) {
            for (auto* cur = loc; cur != nullptr; cur = cur->parentLoc) {
                if (cur->HasKeywordString("LocTypePlayerHouse")) {
                    return;
                }
            }
            if (cell) {
                auto* owner = cell->GetOwner();
                if (owner && owner == player->GetBaseObject()) {
                    return;
                }
            }
        }

        // 2. Проверка черного списка локаций / ячеек (СТРОГО в интерьерах / зданиях, не в открытом мире!)
        if (cell && cell->IsInteriorCell()) {
            for (auto* cur = loc; cur != nullptr; cur = cur->parentLoc) {
                if (Settings::GetSingleton().IsLocationExcluded(cur->GetFormID())) {
                    return;
                }
            }
            if (Settings::GetSingleton().IsLocationExcluded(cell->GetFormID())) {
                return;
            }
        }

        auto* tes = RE::TES::GetSingleton();
        if (!tes) return;

        int harvestedCount = 0;
        constexpr int maxPerTick = 6;

        tes->ForEachReferenceInRange(player, cfg.radius, [&](RE::TESObjectREFR& a_refr) {
            if (harvestedCount >= maxPerTick) {
                return RE::BSContainer::ForEachResult::kStop;
            }

            if (ProcessReference(&a_refr, player, cfg)) {
                harvestedCount++;
            }

            return RE::BSContainer::ForEachResult::kContinue;
        });
    }
}
