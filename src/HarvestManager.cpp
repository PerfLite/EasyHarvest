#include "pch.h"
#include "HarvestManager.h"
#include <cctype>
#include <chrono>
#include <unordered_map>
#include <unordered_set>
#include <RE/I/IObjectHandlePolicy.h>
#include <RE/V/VirtualMachine.h>
#include <RE/P/PackUnpack.h>

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

    static std::unordered_set<RE::FormID> s_depletedVeins;

    void HarvestManager::ClearDepletedCache()
    {
        s_depletedVeins.clear();
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

        // 1. Min Value / Weight Ratio filter
        if (a_cfg.minValPerWeight > 0.0f) {
            float weight = a_item->GetWeight();
            int gold = a_item->GetGoldValue();
            if (a_entry) {
                weight = a_entry->GetWeight();
                gold = a_entry->GetValue();
            }
            if (weight > 0.0f && (static_cast<float>(gold) / weight) < a_cfg.minValPerWeight) {
                return false;
            }
        }

        // 2. Gold / Septims
        if (a_cfg.harvestCoins && a_item->Is(RE::FormType::Misc) && a_item->GetFormID() == 0x0000000F) {
            return true;
        }

        // 3. Lockpicks & Keys
        if (a_cfg.harvestLockpicks && a_item->GetFormID() == 0x0000000A) {
            return true;
        }

        auto* kw = a_item->As<RE::BGSKeywordForm>();

        // Keys (KeyMaster form type, or VendorItemKey)
        if (a_cfg.harvestKeys && (a_item->Is(RE::FormType::KeyMaster) || (kw && kw->HasKeywordString("VendorItemKey")))) {
            return true;
        }

        // 4. Gems
        if (a_cfg.harvestGems && a_item->Is(RE::FormType::Misc) && kw && kw->HasKeywordString("VendorItemGem")) {
            return true;
        }

        // 5. Ingredients
        if (a_cfg.harvestIngredients && a_item->Is(RE::FormType::Ingredient)) {
            return true;
        }

        // 6. Potions, Poisons, Food & Drinks
        if (auto* alch = a_item->As<RE::AlchemyItem>()) {
            if (alch->IsFood()) {
                bool isDrink = (alch->data.consumptionSound && alch->data.consumptionSound->GetFormID() == 0x000B6435) ||
                               (kw && kw->HasKeywordString("VendorItemDrink"));
                if (isDrink) {
                    return a_cfg.harvestDrinks;
                } else {
                    return a_cfg.harvestFood;
                }
            } else {
                return a_cfg.harvestPotions;
            }
        }

        // 7. Soul Gems
        if (a_cfg.harvestSoulGems && a_item->Is(RE::FormType::SoulGem)) {
            return true;
        }

        // 8. Books (Spell Tomes, Skill Books, Notes & Books)
        if (auto* book = a_item->As<RE::TESObjectBOOK>()) {
            if (!book->CanBeTaken()) return false;
            if (book->TeachesSpell()) {
                return a_cfg.harvestSpellbooks;
            }
            if (book->TeachesSkill()) {
                return a_cfg.harvestSkillbooks;
            }
            return a_cfg.harvestBooks;
        }

        // Scrolls
        if (a_cfg.harvestScrolls && (a_item->Is(RE::FormType::Scroll) || (kw && kw->HasKeywordString("VendorItemScroll")))) {
            return true;
        }

        // 9. Ore & Metal Ingots
        if (a_cfg.harvestOreIngots && a_item->Is(RE::FormType::Misc) && kw && kw->HasKeywordString("VendorItemOreIngot")) {
            return true;
        }

        // 10. Animal Pelts, Hides & Parts
        if (a_cfg.harvestAnimalParts && a_item->Is(RE::FormType::Misc) && kw) {
            if (kw->HasKeywordString("VendorItemAnimalHide") || kw->HasKeywordString("VendorItemAnimalPart")) {
                return true;
            }
        }

        // 11. Ammo
        if (a_cfg.harvestAmmo && a_item->Is(RE::FormType::Ammo)) {
            return true;
        }

        // 12. Weapons (Enchanted & Standard)
        if (auto* weapon = a_item->As<RE::TESObjectWEAP>()) {
            if (!weapon->GetPlayable()) return false;
            bool isEnchanted = (weapon->formEnchanting != nullptr) || (a_entry && a_entry->IsEnchanted());
            if (isEnchanted) {
                return a_cfg.harvestEnchantedWeapons || a_cfg.harvestWeapons;
            } else {
                return a_cfg.harvestWeapons;
            }
        }

        // 13. Armor & Jewelry (Rings, Amulets, Enchanted & Standard Armor)
        if (auto* armor = a_item->As<RE::TESObjectARMO>()) {
            if (!armor->GetPlayable()) return false;
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

        // 14. Miscellaneous & Modded items (Crafting components, claws, tokens, trophies, torches)
        if (a_cfg.harvestMisc) {
            if (a_item->Is(RE::FormType::Misc)) {
                if (a_item->formFlags & 0x04) return false; // kNonPlayable
                return true;
            }
            if (a_item->Is(RE::FormType::Light)) {
                auto* light = a_item->As<RE::TESObjectLIGH>();
                if (light && light->CanBeCarried()) return true;
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

        auto* base = a_refr->GetBaseObject();
        if (!base) return false;

        // 2. Флора и деревья с урожаем (цветы, грибы, мох, корни нирна)
        if (a_cfg.harvestFlora) {
            if (base->Is(RE::FormType::Flora) || base->Is(RE::FormType::Tree)) {
                if (!(a_refr->formFlags & RE::TESObjectREFR::RecordFlags::kHarvested)) {
                    // Проверяем урожай (produce item): если это дрова и не в белом списке — пропускаем
                    if (auto* produce = base->As<RE::TESProduceForm>()) {
                        if (produce->produceItem && IsFirewood(produce->produceItem)) {
                            if (!a_cfg.IsWhitelisted(produce->produceItem->GetFormID()) &&
                                !a_cfg.IsWhitelisted(base->GetFormID())) {
                                return false;
                            }
                        }
                    }

                    // Проверяем имя поленницы / кучи дров
                    std::string name = a_refr->GetName();
                    if (name.empty()) name = base->GetName();
                    std::string nameLower = ToLowerUtf8(name);
                    if (nameLower.find("\xd0\xbf\xd0\xbe\xd0\xbb\xd0\xb5\xd0\xbd") != std::string::npos ||     // полен (поленница)
                        nameLower.find("\xd0\xbf\xd0\xbe\xd0\xbb\xd0\xb5\xd1\x88\xd0\xba") != std::string::npos || // полешк
                        nameLower.find("firewood") != std::string::npos ||
                        nameLower.starts_with("\xd0\xb4\xd1\x80\xd0\xbe\xd0\xb2") ||                               // дров... (дрова)
                        nameLower.find(" \xd0\xb4\xd1\x80\xd0\xbe\xd0\xb2") != std::string::npos) {                // ... дров
                        if (!a_cfg.IsWhitelisted(base->GetFormID())) {
                            return false;
                        }
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
            if (a_refr->formFlags & RE::TESForm::RecordFlags::kDestroyed) return false;

            const char* edid = base->GetFormEditorID();
            std::string s = edid ? edid : "";
            const char* n = base->GetName();
            if (n) s += n;
            for (auto& c : s) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));

            if (s.find("mineore") != std::string::npos ||
                s.find("orevein") != std::string::npos ||
                s.find("mine_ore") != std::string::npos ||
                s.find("stalhrim") != std::string::npos ||
                s.find("\xd0\xb6\xd0\xb8\xd0\xbb\xd0\xb0") != std::string::npos || // жила
                s.find("\xd0\xb7\xd0\xb0\xd0\xbb\xd0\xb5\xd0\xb6") != std::string::npos || // залеж
                s.find("\xd0\xba\xd0\xb0\xd1\x80\xd1\x8c\xd0\xb5\xd1\x80") != std::string::npos) { // карьер
                
                if (s_depletedVeins.contains(a_refr->GetFormID())) {
                    return false;
                }

                auto* vm = RE::BSScript::Internal::VirtualMachine::GetSingleton();
                if (vm) {
                    auto* policy = vm->GetObjectHandlePolicy();
                    if (policy) {
                        auto handle = policy->GetHandleForObject(RE::TESObjectREFR::FORMTYPE, a_refr);
                        if (handle != policy->EmptyHandle()) {
                            RE::BSTSmartPointer<RE::BSScript::Object> scriptObj;
                            if (!vm->FindBoundObject(handle, "MineOreScript", scriptObj) || !scriptObj) {
                                vm->FindBoundObject(handle, "mineorescript", scriptObj);
                            }

                            // Если это мебель (Furniture), привязанная к активатору жилы, или наоборот
                            if (!scriptObj && a_refr->GetLinkedRef(nullptr)) {
                                auto* linked = a_refr->GetLinkedRef(nullptr);
                                if (s_depletedVeins.contains(linked->GetFormID())) {
                                    s_depletedVeins.insert(a_refr->GetFormID());
                                    return false;
                                }
                                auto linkedHandle = policy->GetHandleForObject(RE::TESObjectREFR::FORMTYPE, linked);
                                if (linkedHandle != policy->EmptyHandle()) {
                                    if (!vm->FindBoundObject(linkedHandle, "MineOreScript", scriptObj) || !scriptObj) {
                                        vm->FindBoundObject(linkedHandle, "mineorescript", scriptObj);
                                    }
                                    if (scriptObj) {
                                        handle = linkedHandle;
                                    }
                                }
                            }

                            int countToMine = 3;
                            if (scriptObj) {
                                RE::BSScript::Variable varCur;
                                if (vm->GetPropertyValue(scriptObj, "ResourceCountCurrent", varCur) && varCur.IsInt()) {
                                    int cur = varCur.GetSInt();
                                    if (cur == 0) {
                                        // Жила уже истощена — добавляем в кэш и не спамим!
                                        s_depletedVeins.insert(a_refr->GetFormID());
                                        if (auto* linked = a_refr->GetLinkedRef(nullptr)) {
                                            s_depletedVeins.insert(linked->GetFormID());
                                        }
                                        return false;
                                    }
                                    if (cur > 0) {
                                        countToMine = cur;
                                    } else {
                                        // cur == -1 (нетронутая жила), проверяем ResourceCountTotal
                                        RE::BSScript::Variable varTotal;
                                        if (vm->GetPropertyValue(scriptObj, "ResourceCountTotal", varTotal) && varTotal.IsInt()) {
                                            int total = varTotal.GetSInt();
                                            if (total > 0) countToMine = total;
                                        }
                                    }
                                }

                                // Заглушаем нативный DepletedMessage ("Рудная жила истощена"),
                                // чтобы игра не спамила системным уведомлением
                                RE::BSScript::Variable nullMsg;
                                nullMsg.SetNone();
                                vm->SetPropertyValue(scriptObj, "DepletedMessage", nullMsg);
                            }

                            // Помечаем жилу как истощенную в кэше EasyHarvest (и жилу, и связанную мебель)
                            s_depletedVeins.insert(a_refr->GetFormID());
                            if (auto* linked = a_refr->GetLinkedRef(nullptr)) {
                                s_depletedVeins.insert(linked->GetFormID());
                            }

                            // Добываем ровно оставшееся количество порций руды (без лишних вызовов giveOre)
                            for (int i = 0; i < countToMine; ++i) {
                                RE::BSTSmartPointer<RE::BSScript::IStackCallbackFunctor> callback;
                                auto* args = RE::MakeFunctionArguments();
                                vm->DispatchMethodCall(handle, "mineorescript", "giveOre", args, callback);
                            }

                            std::string oreName = a_refr->GetName();
                            if (oreName.empty()) oreName = base->GetName();
                            NotifyLoot(oreName.empty() ? "Ore Vein" : oreName, "ore", countToMine);
                            return true;
                        }
                    }
                }
            }
        }

        // 6. Контейнеры (сундуки, бочки, урны) - тихий сбор без открытия меню
        if (a_cfg.harvestContainers && base->Is(RE::FormType::Container)) {
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

                // Для книг: НЕ вызываем ActivateRef, иначе Скайрим откроет меню чтения книги!
                // Используем нативный метод Actor::PickUpObject, чтобы тихо забрать книгу в инвентарь без открытия интерфейса чтения
                if (base->Is(RE::FormType::Book)) {
                    std::int32_t count = a_refr->extraList.GetCount();
                    if (count <= 0) count = 1;
                    a_player->PickUpObject(a_refr, count, false, false);
                    NotifyLoot(name.empty() ? "Book" : name, "book", count);
                    return true;
                }

                a_refr->ActivateRef(a_player, 0, nullptr, 1, false);
                NotifyLoot(name.empty() ? "Item" : name, "misc", 1);
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
