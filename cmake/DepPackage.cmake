include(FetchContent)

set(FETCHCONTENT_BASE_DIR ${PROJECT_SOURCE_DIR}/dep)
set(FETCHCONTENT_QUIET OFF)

if(DEFINED VCPKG_TARGET_TRIPLET OR DEFINED VCPKG_INSTALLED_DIR)
    set(VCPKG_ENABLED ON)
    message(STATUS "[Deps] vcpkg environment detected")
else()
    set(VCPKG_ENABLED OFF)
    message(STATUS "[Deps] vcpkg not detected, fallback to system/FetchContent")
endif()

macro(find_or_fetch packageName targetName gitRepo gitTag)
    string(TOLOWER ${packageName} _lc_name)

    if(VCPKG_ENABLED)
        find_package(${packageName} CONFIG REQUIRED)
        message(STATUS "[Deps] ${packageName} via vcpkg ✓")
    endif()

    find_package(${packageName} CONFIG QUIET)

    if(NOT ${packageName}_FOUND)
        find_package(${packageName} QUIET)
    endif()

    if(NOT ${packageName}_FOUND)
        message(STATUS "[Deps] ${packageName} NOT FOUND, Try Fetching From ${gitRepo}")
        FetchContent_Declare(
            ${_lc_name}
            GIT_REPOSITORY ${gitRepo}
            GIT_TAG ${gitTag}
            GIT_SHALLOW TRUE
            GIT_PROGRESS TRUE
        )
        FetchContent_MakeAvailable(${_lc_name})

        message(STATUS "[Deps] ${packageName} fetched and built")
    else()
        message(STATUS "[Deps] ${packageName} found system-wide: ${${packageName}_DIR}")
    endif()

    if(NOT TARGET ${targetName})
        message(FATAL_ERROR
            "[Deps] Target '${targetName}' not found for package '${packageName}'.\n"
            "Available targets may differ. Check the library's CMakeLists.txt for exported target names."
        )
    endif()
endmacro()

# ============ 依赖声明 ============

# fmt: v11.2.0
find_or_fetch(fmt fmt::fmt
    "https://github.com/fmtlib/fmt.git"
    "v11.2.0"
)

# spdlog: v1.17.0
find_or_fetch(spdlog spdlog::spdlog
    "https://github.com/gabime/spdlog.git"
    "v1.17.0"
)

# nlohmann_json: v3.12.0
find_or_fetch(nlohmann_json nlohmann_json::nlohmann_json
    "https://github.com/nlohmann/json.git"
    "v3.12.0"
)

# find_or_fetch(yaml-cpp yaml-cpp::yaml-cpp
#     "https://github.com/jbeder/yaml-cpp.git"
#     "v0.9.0"
# )

# qjs: v0.16.1
find_or_fetch(qjs qjs
    "https://github.com/quickjs-ng/quickjs.git"
    "v0.16.1"
)

# OpenSSL: openssl-3.6.3
find_or_fetch(OpenSSL OpenSSL::SSL
    "https://github.com/openssl/openssl.git"
    "openssl-3.6.3"
)

# CURL: curl-8_21_0
find_or_fetch(CURL CURL::libcurl
    "https://github.com/curl/curl.git"
    "curl-8_21_0"
)

# minizip-ng
message(STATUS "[Deps] Checking minizip-ng...")
find_package(minizip-ng QUIET)

if(NOT minizip-ng_FOUND)
    message(STATUS "[Deps] minizip-ng NOT FOUND, fetching from GitHub...")
    
    FetchContent_Declare(
        minizip-ng
        GIT_REPOSITORY "https://github.com/zlib-ng/minizip-ng.git"
        GIT_TAG "4.0.7"
        GIT_SHALLOW TRUE
        GIT_PROGRESS TRUE
    )
    
    FetchContent_MakeAvailable(minizip-ng)
    message(STATUS "[Deps] minizip-ng fetched and built")
else()
    message(STATUS "[Deps] minizip-ng found system-wide: ${minizip-ng_DIR}")
endif()

if(TARGET minizip-ng::minizip-ng)
    set(MINIZIP_TARGET minizip-ng::minizip-ng)
elseif(TARGET minizip-ng::minizip)
    set(MINIZIP_TARGET minizip-ng::minizip)
elseif(TARGET MINIZIP::minizip-ng)
    set(MINIZIP_TARGET MINIZIP::minizip-ng)
elseif(TARGET MINIZIP::minizip)
    set(MINIZIP_TARGET MINIZIP::minizip)
elseif(TARGET minizip)
    set(MINIZIP_TARGET minizip)
else()
    message(FATAL_ERROR "[Deps] minizip-ng target not found. Available targets: ${minizip-ng_TARGETS}")
endif()

message(STATUS "[Deps] minizip target is ${MINIZIP_TARGET}")
