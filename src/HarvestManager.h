#pragma once

#include "Settings.h"
#include <string>
#include <vector>
#include <atomic>
#include <functional>

namespace EasyHarvest
{
    class HarvestManager
    {
    public:
        static HarvestManager& GetSingleton()
        {
            static HarvestManager instance;
            return instance;
        }

        void Start();
        void Stop();
        void Tick();
        void ClearDepletedCache();
        void InitMerchantChests();

        // Callback for UI notifications
        void SetOnLootCallback(std::function<void(const std::string&, const std::string&, int)> a_callback)
        {
            onLoot_ = a_callback;
        }

    private:
        HarvestManager() = default;

        bool ProcessReference(RE::TESObjectREFR* a_refr, RE::PlayerCharacter* a_player, const Config& a_cfg);
        void NotifyLoot(const std::string& a_name, const std::string& a_category, int a_count);

        std::atomic<bool> running_{ false };
        float timeSinceLastScan_{ 0.0f };
        std::function<void(const std::string&, const std::string&, int)> onLoot_;
    };
}
