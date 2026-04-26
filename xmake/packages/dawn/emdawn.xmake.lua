target("emdawn")
    set_languages("cxx20")
    set_kind("static")
    add_files("webgpu/src/**.cpp")
    add_installfiles("webgpu/src/**.js", {prefixdir = "lib"})
    add_headerfiles("webgpu/include/**.h")
    add_headerfiles("webgpu_cpp/include/**.h")
    add_includedirs("webgpu/include")
    add_includedirs("webgpu_cpp/include")

    add_ldflags("--closure=1")
    add_ldflags("--js-library=webgpu/src/library_webgpu_enum_tables.js")
    add_ldflags("--js-library=webgpu/src/library_webgpu_generated_sig_info.js")
    add_ldflags("--js-library=webgpu/src/library_webgpu_generated_struct_info.js")
    add_ldflags("--js-library=webgpu/src/library_webgpu.js", {force = true})
    add_ldflags("--closure-args=--externs=webgpu/src/webgpu-externs.js")

    -- enable webgpu support
    -- add_cxflags("-sUSE_WEBGPU=1")
    add_ldflags("-sUSE_WEBGPU=1")

    -- enable async
    add_ldflags("-sASYNCIFY", {force = true})

    add_defines("WGPU_IMPLEMENTATION")
    add_defines("DAWN_ENABLE_BACKEND_WGPU")
