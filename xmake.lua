set_project("dvdbr3o.wgpu")
set_version("0.1.0")

set_policy("package.requires_lock", true)
add_rules("mode.debug", "mode.release")
add_rules("plugin.compile_commands.autoupdate")

includes("xmake")

add_requires("dawn", {alias = "dawn"})
add_requires("slang")
add_requires("libsdl3")
add_requires("fastgltf", {
    configs = {
        cxx_standard = "20",
    },
})
add_requires("spdlog")
add_requires("glm")
add_requires("entt")
add_requires("nlohmann_json")
add_requires("argparse")
add_requires("stdexec", {
    configs = {
        asio = true, 
        asio_implementation = "standalone"
    },
})
add_requires("unordered_dense")
add_requires("stb")
add_requires("tl_expected")

target("dvdbr3o.shaders.lib")
    set_kind("static")
    add_packages("slang")
    add_rules("slang", {target_kind = "none"})
    add_files("src/dvdbr3o/Shaders/Uniform.slang")
    add_files("src/dvdbr3o/Shaders/Vertex.slang")

target("dvdbr3o.shaders")
    set_kind("static")
    add_packages("slang")
    add_rules("slang", {target_kind = "wgsl"})
    add_deps("dvdbr3o.shaders.lib")
    add_files("src/dvdbr3o/Shaders/Ascii.slang")
    add_files("src/dvdbr3o/Shaders/Mtoon.slang")

target("dvdbr3o.wgpu")
    set_kind("binary")
    set_languages("cxx20")
    add_rules("utils.bin2obj", {
        extensions = {
            ".wgsl", 
            ".json", 
            ".spv", 
            ".slang", 
            ".png", 
            ".jpg", 
            ".gltf",
        },
        symbol_prefix = "dvdbr3o_embed_",
        -- zeroend = true,
    })

    add_packages("dawn", {public = true})
    add_packages("slang", {public = true})
    add_packages("libsdl3", {public = true})
    add_packages("fastgltf", {public = true})
    add_packages("spdlog", {public = true})
    add_packages("glm", {public = true})
    add_packages("entt", {public = true})
    add_packages("nlohmann_json", {public = true})
    add_packages("argparse", {public = true})
    add_packages("stdexec", {public = true})
    add_packages("unordered_dense", {public = true})
    add_packages("stb", {public = true})
    add_packages("tl_expected", {public = true})
    add_deps("dvdbr3o.shaders")

    add_files("assets/**")
    add_files("src/dvdbr3o/Shaders/**.wgsl")
    add_files("src/dvdbr3o/Shaders/**.json")
    add_files("src/**.cpp", {public = true})
    add_headerfiles("src/**.hpp", {public = true})
    add_includedirs("src", {public = true})

    if is_plat("windows") then
        add_syslinks("dxguid", "onecore")
        add_defines("DVDBR3O_WGPU_PLAT_WINDOWS")
    elseif is_plat("linux") then
        add_defines("DVDBR3O_WGPU_PLAT_LINUX")
    elseif is_plat("macos") then 
        add_defines("DVDBR3O_WGPU_PLAT_MACOS")
    elseif is_plat("android") then 
        add_defines("DVDBR3O_WGPU_PLAT_ANDROID")
    elseif is_plat("iphoneos") then 
        add_defines("DVDBR3O_WGPU_PLAT_IOS")
    elseif is_plat("wasm") then 
        add_defines("DVDBR3O_WGPU_PLAT_WASM")
        add_defines("WEBGPU_BACKEND_WASM")
    end

    if is_mode("debug") then
        add_defines("DVDBR3O_WGPU_MODE_DEBUG")
    elseif is_mode("release") then
        add_defines("DVDBR3O_WGPU_MODE_RELEASE")
    elseif is_mode("profile") then
        add_defines("DVDBR3O_WGPU_MODE_PROFILE")
    end

