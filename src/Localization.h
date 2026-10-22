#pragma once
#include "I18n.h"

namespace EasyHarvest
{
    using Strings = I18nStrings;

    class Localization
    {
    public:
        static const I18nStrings& Get([[maybe_unused]] const std::string& a_configLang = "")
        {
            return Tr();
        }
    };
}
