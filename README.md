# EasyHarvest

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)
[![Platform](https://img.shields.io/badge/Platform-Skyrim%20SE%20%7C%20AE-green.svg)](https://store.steampowered.com/app/489830/)
[![Language](https://img.shields.io/badge/Language-C%2B%2B23-orange.svg)]()
[![Build System](https://img.shields.io/badge/Build-xmake-blueviolet.svg)](https://xmake.io/)

**EasyHarvest** is a modern, lightweight, crash-free auto-harvesting SKSE plugin for *The Elder Scrolls V: Skyrim Special Edition* and *Anniversary Edition*.

Built with **CommonLibSSE-NG** and powered by **SKSE Menu Framework**, EasyHarvest offers native in-game configuration through a responsive Dear ImGui overlay (accessible via `F1`), eliminating the need for slow Papyrus MCM scripts or external web runtimes.

---

## ✨ Features

- **⚡ Fast & Safe Native Looting:**
  - Written in modern C++23.
  - Direct engine calls ensure instant collection without UI stutter or Papyrus script lag.
  - Silent book collection (never triggers book reading menus).
  - Clean container & dead body looting with full equipment unequip safety.

- **🎯 Granular Category Filtering:**
  - **Flora & Harvesting:** Flowers, plants, hanging ingredients, mushrooms, nirnroot.
  - **Critters:** Flying insects (butterflies, bees, dragonflies, fireflies) and salmon.
  - **Valuables:** Gold/Septims, Lockpicks, Keys, and Gems.
  - **Consumables:** Alchemy ingredients, potions, poisons, food, and drinks.
  - **Magic & Knowledge:** Soul gems, Spell tomes, Skill books, and Scrolls (regular clutter books disabled by default).
  - **Crafting & Parts:** Ore, ingots, animal pelts, leather, hides, and parts.
  - **Combat Gear:** Ammo (arrows & bolts), weapons, and armor (with individual filters for enchanted gear).
  - **Containers & Bodies:** Chests, urns, barrels, dead bodies, and ash piles (with configurable lock and boss chest protection).

- **⛏️ Instant Ore Vein Mining:**
  - Automatically mines ore veins upon proximity.
  - Depletes the entire vein in a single instant interaction (supports both vanilla 3-ore veins and RFAB/modded 6-ore veins).
  - Automatically respects depleted state (`kDestroyed`) with a built-in cooldown.

- **🕵️ Crime & Stealing Protection:**
  - **Mode 0 (Never steal):** Owned/private items and containers are strictly protected.
  - **Mode 1 (Steal in sneak only):** Automatically loots owned items only when the player is sneaking and undetected.
  - **Mode 2 (Steal always):** Loots everything regardless of ownership.

- **🏰 Interior Location Blocker:**
  - Exclude specific player houses, shops, taverns, or dungeons from auto-harvesting.
  - Guarded to operate strictly within interiors, preventing accidental blocking of the open world.

- **🔍 Custom Whitelist with Search Bar:**
  - Force-harvest special items (modded bounty heads, quest trophies, custom tokens) regardless of other category filters.
  - In-game search bar with real-time UTF-8 filtering (supports Russian and English).
  - Smart prioritization: modded items automatically appear at the top of candidate list.

- **🌐 In-Game Configuration & Localization:**
  - Modern overlay menu via **SKSE Menu Framework** (`F1`).
  - Native bilingual support (English and Russian with auto-detection).
  - Quick toggle key (default: `G`) to instantly enable or pause auto-harvesting on the fly.

---

## 📋 Requirements

1. **The Elder Scrolls V: Skyrim Special Edition** (1.5.97) or **Anniversary Edition** (1.6.x+)
2. **[SKSE64](https://skse.silverlock.org/)** matching your game version
3. **[Address Library for SKSE Plugins](https://www.nexusmods.com/skyrimspecialedition/mods/32444)**
4. **[SKSE Menu Framework](https://www.nexusmods.com/skyrimspecialedition/mods/120352)**

---

## 📦 Installation

### Mod Organizer 2 / Vortex
1. Download the latest release archive.
2. Install via your mod manager of choice.
3. Ensure **SKSE Menu Framework** is installed and active.
4. Launch Skyrim through `skse64_loader.exe`.

### Manual Installation
Copy the contents of the archive into your Skyrim `Data` directory:
```
Data/
└── SKSE/
    └── Plugins/
        ├── EasyHarvest.dll
        └── EasyHarvest.json
```

---

## 🛠️ Building from Source

EasyHarvest uses [xmake](https://xmake.io/) and [CommonLibSSE-NG](https://github.com/CharmedBaryon/CommonLibSSE-NG).

### Prerequisites
- Visual Studio 2022 (MSVC v143 toolset with C++23 support)
- [xmake](https://xmake.io/) (v2.8.2 or newer)
- Git

### Build Steps
```bash
# Clone the repository
git clone https://github.com/PerfLite/EasyHarvest.git
cd EasyHarvest

# Build release binary
xmake build -y
```

The compiled binary will be located in:
`build/windows/x64/release/EasyHarvest.dll`

---

## ⚙️ Configuration (`EasyHarvest.json`)

Settings can be changed live in-game via the `F1` menu, or edited directly in `Data/SKSE/Plugins/EasyHarvest.json`:

```json
{
  "enabled": true,
  "radius": 400.0,
  "interval": 0.2,
  "minValPerWeight": 0.0,
  "harvestFlora": true,
  "harvestCritters": true,
  "harvestCoins": true,
  "harvestLockpicks": true,
  "harvestKeys": true,
  "harvestGems": true,
  "harvestIngredients": true,
  "harvestPotions": true,
  "harvestFood": true,
  "harvestDrinks": true,
  "harvestSoulGems": true,
  "harvestSpellbooks": true,
  "harvestSkillbooks": true,
  "harvestBooks": false,
  "harvestScrolls": true,
  "harvestOreIngots": true,
  "harvestOreVeins": true,
  "harvestAnimalParts": true,
  "harvestAmmo": true,
  "harvestMisc": false,
  "harvestJewelry": true,
  "harvestEnchantedWeapons": true,
  "harvestWeapons": true,
  "harvestEnchantedArmor": true,
  "harvestArmor": true,
  "harvestContainers": true,
  "protectLockedContainers": true,
  "protectBossContainers": false,
  "harvestDeadBodies": true,
  "harvestAshPiles": true,
  "pauseInCombat": false,
  "pauseWeaponDrawn": false,
  "stealingMode": 0,
  "toggleKey": 34,
  "toggleKeyName": "G",
  "language": "auto",
  "ignorePlayerHouses": true,
  "excludedLocations": [],
  "customWhitelist": []
}
```

---

## 🇷🇺 Описание (Русский)

**EasyHarvest** — современный, быстрый и стабильный SKSE-плагин для автоматического сбора предметов в *The Elder Scrolls V: Skyrim Special Edition* и *Anniversary Edition*.

Плагин написан на C++23 с использованием **CommonLibSSE-NG** и настраивается прямо во время игры через удобное оверлей-меню на **Dear ImGui** (по клавише `F1`, требует **SKSE Menu Framework**). Не использует тяжёлые Papyrus-скрипты, интерфейсные веб-движки или медленные таймеры, благодаря чему сбор происходит мгновенно и без просадок FPS.

### Основные возможности:
- **Мгновенный и безопасный сбор:** прямой перенос предметов в инвентарь игрока без открытия меню чтения книг, без зависаний интерфейса и без вылетов при луте трупов.
- **Гибкие категории сбора:**
  - Флора (цветы, травы, грибы, корни нирна);
  - Насекомые и рыба (бабочки, светлячки, стрекозы, пчёлы, лосось);
  - Золото, отмычки, ключи и драгоценные камни;
  - Ингредиенты, зелья, яды, еда и напитки;
  - Камни душ, тома заклинаний, книги навыков, свитки (обычные книги выключены по умолчанию);
  - Руда, слитки, шкуры, кожа и части животных;
  - Боеприпасы (стрелы и болты), оружие и броня (с отдельным фильтром для зачарованных вещей);
  - Контейнеры (сундуки, урны, бочки), трупы и кучи пепла (с настраиваемой защитой замков и сундуков боссов).
- **Авто-добыча руды из жил:** моментально опустошает жилу целиком за одно касание без необходимости стоять и ждать анимацию (поддерживает как ванильные жилы на 3 руды, так и жилы на 6 руды из RFAB).
- **Умная защита от кражи:**
  - *Режим 0:* Никогда не воровать (чужие вещи и контейнеры не трогаются);
  - *Режим 1:* Воровать только в скрытности, если персонажа никто не видит;
  - *Режим 2:* Забирать всё подряд независимо от владельца.
- **Блокировщик локаций:** возможность исключить автосбор в конкретных домах, магазинах или подземельях. Работает строго в интерьерах, защищая открытый мир от случайной блокировки.
- **Белый список предметов с поиском:** гарантированный сбор любых предметов (квестовых голов, трофеев, модовых вещей) в обход любых фильтров. Встроен живой поиск по названию (на русском и английском) с авто-сортировкой модовых вещей в самый верх списка.
- **Горячая клавиша:** мгновенное включение и отключение автосбора нажатием клавиши `G` (настраивается).

---

## 📄 License

This project is licensed under the **MIT License**.  
See the [LICENSE](LICENSE) file for details.
