<div align="center">

<img src="assets/banner.svg" alt="EasyHarvest" width="100%">

<p>
  <a href="#english">🇬🇧 English</a> ·
  <a href="#русский">🇷🇺 Русский</a>
</p>

<p>
  <img src="https://img.shields.io/github/v/release/PerfLite/EasyHarvest?style=flat-square" alt="release">
  <img src="https://img.shields.io/github/license/PerfLite/EasyHarvest?style=flat-square" alt="license">
  <img src="https://img.shields.io/github/languages/top/PerfLite/EasyHarvest?style=flat-square" alt="language">
  <img src="https://img.shields.io/github/stars/PerfLite/EasyHarvest?style=flat-square" alt="stars">
  <img src="https://img.shields.io/github/downloads/PerfLite/EasyHarvest/total?style=flat-square" alt="downloads">
</p>

<p>
  <a href="https://github.com/PerfLite/EasyHarvest/releases/latest"><b>⬇️ Download / Скачать</b></a> ·
  <a href="https://github.com/PerfLite/EasyHarvest/releases">📦 All releases / Все релизы</a>
</p>

</div>

## ⬇️ Download / Скачать

| File / Файл | Description / Описание | Link / Ссылка |
|---|---|---|
| `EasyHarvest-v1.2.0.zip` | Main build (SKSE64 Plugin + Config + JSON Translations) · Основная сборка (Плагин SKSE64 + Конфиг + Переводы) | [Download](https://github.com/PerfLite/EasyHarvest/releases/latest/download/EasyHarvest-v1.2.0.zip) |

> Older versions and changelogs: [Releases](https://github.com/PerfLite/EasyHarvest/releases) ·
> Старые версии и список изменений: [Релизы](https://github.com/PerfLite/EasyHarvest/releases)

---

## English

A modern, high-performance, crash-free auto-harvesting SKSE64 plugin for Skyrim Special Edition and Anniversary Edition with Dear ImGui in-game menu and JSON localization.

### Features

- **⚡ Fast & Safe Native Looting:** Written in modern C++23. Direct engine calls ensure instant collection without UI stutter or Papyrus script lag.
- **🎯 Granular Category Filtering:** Individual toggles for flora, critters, coins, lockpicks, keys, gems, ingredients, potions, food, drinks, soul gems, spell tomes, skill books, scrolls, ores, ingots, animal parts, ammo, weapons, and armor.
- **⛏️ Instant Ore Vein Mining:** Automatically mines and depletes nearby ore veins in a single interaction (supports vanilla veins, outcrops, and custom modded veins).
- **🕵️ Crime & Stealing Protection:** Configurable modes (Never steal, Steal only in sneak when undetected, or Steal always).
- **🏰 Interior Location Blacklist:** Exclude specific player houses, shops, or dungeons from auto-harvesting.
- **🔍 Custom Whitelist with Search:** Real-time search to force-loot specific items (modded bounty heads, quest tokens) regardless of value or category filters.
- **🌐 JSON Localization:** Live in-game language switching (`Auto` / `English` / `Русский`) and dynamic translation via `EasyHarvestStrings_<LANG>.json`.
- **🐺 Beast & Vampire Lord Form Safety:** Optional automatic pause while transformed into a Werewolf or Vampire Lord.
- **⚙️ SKSE Menu Framework Overlay:** Native in-game Dear ImGui configuration menu (`F1`), no slow MCM scripts required.

### Install

Install via Mod Organizer 2 or Vortex, or manually copy the contents to your game directory:

```
Data/
└── SKSE/
    └── Plugins/
        ├── EasyHarvest.dll
        ├── EasyHarvest.json
        ├── EasyHarvestStrings.json
        ├── EasyHarvestStrings_EN.json
        └── EasyHarvestStrings_RU.json
```

**Requirements:**
- Skyrim Special Edition (1.5.97) or Anniversary Edition (1.6.x+)
- [SKSE64](https://skse.silverlock.org/)
- [Address Library for SKSE Plugins](https://www.nexusmods.com/skyrimspecialedition/mods/32444)
- [SKSE Menu Framework](https://www.nexusmods.com/skyrimspecialedition/mods/120352)

### Build

Built with [xmake](https://xmake.io/) and [CommonLibSSE-NG](https://github.com/CharmedBaryon/CommonLibSSE-NG):

```bash
git clone https://github.com/PerfLite/EasyHarvest.git
cd EasyHarvest
xmake f -m release -y
xmake -y
```

### Credits

Inspired by and based on ideas from [AutoHarvestSE](https://www.nexusmods.com/skyrimspecialedition/mods/17622) by **kapaer** and **towawot** ([GitHub](https://github.com/towawot/AutoHarvestSE)).

### License

MIT — see [LICENSE](LICENSE).

<p align="right"><a href="#english">↑ Back to top</a> · <a href="#русский">Русский ↓</a></p>

---

## Русский

Современный, легковесный и стабильный SKSE64-плагин автосбора для Skyrim Special Edition и Anniversary Edition с внутриигровым меню Dear ImGui и локализацией через JSON.

### Возможности

- **⚡ Быстрый и безопасный нативный сбор:** Написан на современном C++23. Прямые вызовы функций движка обеспечивают мгновенный сбор без фризов интерфейса и задержек Papyrus-скриптов.
- **🎯 Детальная настройка категорий:** Раздельные переключатели для флоры, мелких существ (бабочки, рыбы), золота, отмычек, ключей, драгоценных камней, ингредиентов, зелий, еды, напитков, камней душ, томов заклинаний, книг навыков, свитков, руды, слитков, шкур, стрел, оружия и брони.
- **⛏️ Мгновенная выработка рудных жил:** Автоматическая добыча руды при приближении к жиле за одно действие (поддерживаются как ванильные жилы, так и кастомные выходы руды из модов).
- **🕵️ Защита от кражи:** 3 режима (Не воровать частное, Воровать только в скрытности если не видят, Воровать всегда).
- **🏰 Чёрный список локаций:** Блокировка сбора в домах игрока, тавернах, магазинах или конкретных подземельях.
- **🔍 Белый список с живым поиском:** Возможность принудительного сбора любых предметов (трофеи, головы по контрактам) в обход фильтров цены и категорий.
- **🌐 Локализация через JSON:** Переключение языка на лету (`Auto` / `English` / `Русский`) и поддержка пользовательских переводов через файлы `EasyHarvestStrings_<ЯЗЫК>.json`.
- **🐺 Защита в облике зверя и вампира-лорда:** Опция автоматического отключения сбора в форме оборотня или лорда-вампира, не нарушающая механику питания трупами и погружение.
- **⚙️ Меню SKSE Menu Framework:** Удобный внутриигровой интерфейс на Dear ImGui (по умолчанию клавиша `F1`), не требующий медленного MCM.

### Установка

Установите через Mod Organizer 2 или Vortex, либо вручную скопируйте файлы в папку игры:

```
Data/
└── SKSE/
    └── Plugins/
        ├── EasyHarvest.dll
        ├── EasyHarvest.json
        ├── EasyHarvestStrings.json
        ├── EasyHarvestStrings_EN.json
        └── EasyHarvestStrings_RU.json
```

**Требования:**
- Skyrim Special Edition (1.5.97) или Anniversary Edition (1.6.x+)
- [SKSE64](https://skse.silverlock.org/)
- [Address Library for SKSE Plugins](https://www.nexusmods.com/skyrimspecialedition/mods/32444)
- [SKSE Menu Framework](https://www.nexusmods.com/skyrimspecialedition/mods/120352)

### Сборка

Сборка осуществляется через [xmake](https://xmake.io/) и [CommonLibSSE-NG](https://github.com/CharmedBaryon/CommonLibSSE-NG):

```bash
git clone https://github.com/PerfLite/EasyHarvest.git
cd EasyHarvest
xmake f -m release -y
xmake -y
```

### Благодарности

Основано на идеях и архитектуре [AutoHarvestSE](https://www.nexusmods.com/skyrimspecialedition/mods/17622) от авторов **kapaer** и **towawot** ([GitHub](https://github.com/towawot/AutoHarvestSE)).

### Лицензия

MIT — см. [LICENSE](LICENSE).

<p align="right"><a href="#русский">↑ Наверх</a> · <a href="#english">English ↑</a></p>
