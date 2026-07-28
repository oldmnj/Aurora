set_project("OlLauncherCore")
set_version("1.0.0")
set_languages("cxx23")

add_cxflags("-std=c++23", "-Wall", "-Wextra", "-Wpedantic")
add_cxflags("-fPIC", {tools={"gcc", "clang"}})
add_cxflags("/std::c++23", "/W4", {tools={"msvc"}})

if is_plat("linux") and os.getenv("TERMUX_VERSION") then
    add_cxflags("-stdlib=libc++", "-nostdinc++", {force = true})
    add_cxflags("-I/data/data/com.termux/files/usr/include/c++/v1", {force = true})
    add_cxflags("-I/data/data/com.termux/files/usr/include", {force = true})
    
    add_ldflags("-stdlib=libc++", "-lc++", {force = true})
    add_linkdirs("/data/data/com.termux/files/usr/lib")
end

set_policy("build.c++.modules", true)

set_config("build.c++.modules.reuse", true)
set_config("build.c++.modules.culling", false)
set_config("build.c++.modules.output", "build/.modules") 


add_requires("nlohmann_json", "fmt 11.2.0", "spdlog", "openssl", "libcurl", "minizip-ng")

includes("tests", "examples")

add_includedirs("include/cpp", "include/c", {public=true})

target("launcher_core")
    set_kind("shared")
    set_basename("LauncherCore")
    
    add_files("src/base/*.cc")
    add_files("src/io/*.cc")
    add_files("src/assets/*.cc")
    add_files("src/auth/*.cc")
    add_files("src/jvm/*.cc")
    add_files("src/launch/*.cc")
    add_files("src/loader/*.cc")
    add_files("src/process/*.cc")
    add_files("src/version/*.cc")
    
    add_files("modules/*.cppm")
    
    if is_plat("windows") then
        add_defines("LAUNCHER_CORE_EXPORTS")
        add_links("ws2_32", "crypt_32")
    end
    add_packages("fmt", "nlohmann_json", "spdlog", "openssl", "libcurl", "minizip-ng")
    
    add_includedirs("build/.modules", {public = true})