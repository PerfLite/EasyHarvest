set_xmakever("2.8.2")

set_project("EasyHarvest")
set_version("1.0.0")
set_license("GPL-3.0")

set_languages("c++23")
set_warnings("allextra")

set_policy("package.requires_lock", true)

add_rules("mode.release")
add_rules("plugin.vsxmake.autoupdate")

add_requires("commonlibsse-ng")

target("EasyHarvest")
    set_kind("shared")
    add_packages("commonlibsse-ng")

    add_files("src/*.cpp")
    add_headerfiles("src/**.h")
    add_includedirs("src")
    set_pcxxheader("src/pch.h")
