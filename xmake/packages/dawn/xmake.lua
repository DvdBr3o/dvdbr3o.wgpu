package("dawn")
    set_homepage("https://github.com/google/dawn")
    set_description("Dawn is an open-source implementation of WebGPU")

    if is_plat("windows") then
        if is_mode("release") then
            add_urls("https://github.com/google/dawn/releases/download/v20260410.140140/Dawn-f8178d47e2351a11830639a549b9ca476c426bf6-windows-latest-Release.tar.gz", {verify = false})
            add_versions("20260410.140140", "89d6fe623cf9dce44270b84fa4ef38306cb4d160f136974e7af9a2486566b82f")
        elseif is_mode("debug") then
            add_urls("https://github.com/google/dawn/releases/download/v20260410.140140/Dawn-f8178d47e2351a11830639a549b9ca476c426bf6-windows-latest-Debug.tar.gz", {verify = false})
            add_versions("20260410.140140", "8420061e7f84e705ffc09e54b7162af59e7762db42c1867b1a66cef16744b012")
        end
    elseif is_plat("linux") then
        if is_mode("release") then
            add_urls("https://github.com/google/dawn/releases/download/v20260410.140140/Dawn-f8178d47e2351a11830639a549b9ca476c426bf6-ubuntu-latest-Release.tar.gz")
            add_versions("20260410.140140", "30055bc1e8b42b0db53c1865dfc608f26fdef5055b858f94a9f6519a2184642b")
        elseif is_mode("debug") then
            add_urls("https://github.com/google/dawn/releases/download/v20260410.140140/Dawn-f8178d47e2351a11830639a549b9ca476c426bf6-ubuntu-latest-Debug.tar.gz")
            add_versions("20260410.140140", "261f9158f81e9693795c94b6ce660da02df8e181c474cc6e6f6e96620ff87082")
        end
    elseif is_plat("wasm") then
        add_urls("https://github.com/google/dawn/releases/download/v20260410.140140/emdawnwebgpu_pkg-v20260410.140140.zip")
        add_versions("20260410.140140", "db299748107c6fa6f8f219b7e2487bf74276223c04ac7d49bd42a13d6a8c16c5")
    end

    on_install("windows", "linux", function (package)
        os.cp("bin", package:installdir())
        os.cp("include", package:installdir())
        
        package:add("bindirs", "bin")
        package:add("linkdirs", "bin")
        package:add("includedirs", "include")
        if package:is_plat("windows") then
            os.cp("lib", package:installdir())
            package:add("linkdirs", "lib")
        elseif package:is_plat("linux") then
            os.cp("lib64", package:installdir())
            package:add("linkdirs", "lib64")
        end

        if package:is_plat("windows") then
            package:add("syslinks", "dxguid", "onecore")
        end
    end)

    on_install("wasm", function (package)
        os.cp(path.join(os.scriptdir(), "emdawn.xmake.lua"), "xmake.lua")

        import("package.tools.xmake").install(package)

        os.cp("webgpu", package:installdir())
        os.cp("webgpu_cpp", package:installdir())
        package:add("includedirs", path.join("webgpu", "include"))
        package:add("includedirs", path.join("webgpu_cpp", "include"))

        local lib_dir = package:installdir("lib")
        local js_files = {
            "library_webgpu_enum_tables.js",
            "library_webgpu_generated_sig_info.js",
            "library_webgpu_generated_struct_info.js",
            "library_webgpu.js"
        }

        local flags = {}
        for _, jsfile in ipairs(js_files) do
            table.insert(flags, "--js-library=" .. path.join(lib_dir, jsfile))
        end

        local externs_path = path.join(lib_dir, "webgpu-externs.js")
        table.insert(flags, "--closure-args=--externs=" .. externs_path)

        -- why `table.concat`? 
        -- because single ldflag `--js-libray=library_webgpu.js` will fail and 
        -- be ignored by xmake (it depends other js lib), the all `--js-library`
        -- flag must be passed at once.
        package:add("ldflags", table.concat(flags, " "), {force = true})

        package:add("ldflags", "-sASYNCIFY=1")
        package:add("ldflags", "-sJSPI=1")
    end)
