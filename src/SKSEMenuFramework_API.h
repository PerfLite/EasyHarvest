#pragma once

namespace SKSEMenuFramework
{
    using RenderCallback = void (*)();
    using AddSectionItem_t = void (*)(const char*, RenderCallback);
    using AddWindow_t = void (*)(RenderCallback);
    using RegisterHudElement_t = void (*)(RenderCallback);

    struct ImVec2
    {
        float x{ 0.0f };
        float y{ 0.0f };
    };

    using igText_t = void (*)(const char*, ...);
    using igCheckbox_t = bool (*)(const char*, bool*);
    using igSliderFloat_t = bool (*)(const char*, float*, float, float, const char*, int);
    using igButton_t = bool (*)(const char*, ImVec2);
    using igSeparator_t = void (*)();
    using igSameLine_t = void (*)(float, float);
    using igInputText_t = bool (*)(const char*, char*, size_t, int, void*, void*);
    using igInputTextWithHint_t = bool (*)(const char*, const char*, char*, size_t, int, void*, void*);
    using igSetNextItemWidth_t = void (*)(float);

    inline AddSectionItem_t AddSectionItem = nullptr;
    inline AddWindow_t AddWindow = nullptr;
    inline RegisterHudElement_t RegisterHudElement = nullptr;
    inline igText_t igText = nullptr;
    inline igCheckbox_t igCheckbox = nullptr;
    inline igSliderFloat_t igSliderFloat = nullptr;
    inline igButton_t igButton = nullptr;
    inline igSeparator_t igSeparator = nullptr;
    inline igSameLine_t igSameLine = nullptr;
    inline igInputText_t igInputText = nullptr;
    inline igInputTextWithHint_t igInputTextWithHint = nullptr;
    inline igSetNextItemWidth_t igSetNextItemWidth = nullptr;

    inline bool Init()
    {
        auto hMod = GetModuleHandle("SKSEMenuFramework.dll");
        if (!hMod) {
            hMod = LoadLibrary("SKSEMenuFramework.dll");
        }
        if (!hMod) {
            return false;
        }

        AddSectionItem = reinterpret_cast<AddSectionItem_t>(GetProcAddress(hMod, "AddSectionItem"));
        AddWindow = reinterpret_cast<AddWindow_t>(GetProcAddress(hMod, "AddWindow"));
        RegisterHudElement = reinterpret_cast<RegisterHudElement_t>(GetProcAddress(hMod, "RegisterHudElement"));
        igText = reinterpret_cast<igText_t>(GetProcAddress(hMod, "igText"));
        igCheckbox = reinterpret_cast<igCheckbox_t>(GetProcAddress(hMod, "igCheckbox"));
        igSliderFloat = reinterpret_cast<igSliderFloat_t>(GetProcAddress(hMod, "igSliderFloat"));
        igButton = reinterpret_cast<igButton_t>(GetProcAddress(hMod, "igButton"));
        igSeparator = reinterpret_cast<igSeparator_t>(GetProcAddress(hMod, "igSeparator"));
        igSameLine = reinterpret_cast<igSameLine_t>(GetProcAddress(hMod, "igSameLine"));
        igInputText = reinterpret_cast<igInputText_t>(GetProcAddress(hMod, "igInputText"));
        igInputTextWithHint = reinterpret_cast<igInputTextWithHint_t>(GetProcAddress(hMod, "igInputTextWithHint"));
        igSetNextItemWidth = reinterpret_cast<igSetNextItemWidth_t>(GetProcAddress(hMod, "igSetNextItemWidth"));

        return AddSectionItem != nullptr;
    }
}
