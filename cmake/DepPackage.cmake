include(FetchContent)

set(FETCHCONTENT_BASE_DIR ${PROJECT_SOURCE_DIR}/dep)
set(FETCHCONTENT_QUIET OFF)

macro(find_or_fetch packageName targetName gitRepo gitTag)
    string(TOLOWER ${packageName} _lc_name)

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

# qjs: v0.16.1
find_or_fetch(qjs qjs
    "https://github.com/quickjs-ng/quickjs.git"
    "v0.16.1"
)

# minizip-ng: 4.2.2（注意：不带 v 前缀）
find_or_fetch(minizip-ng MINIZIP::minizip-ng
    "https://github.com/zlib-ng/minizip-ng.git"
    "4.2.2"
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

# ============ minizip-ng 目标名兼容处理 ============
# 注意：minizip-ng 的 target 名称可能因版本而异，提前处理防止宏报错
if(NOT TARGET MINIZIP::minizip-ng)
    if(TARGET MINIZIP::minizip)
        set(MINIZIP_TARGET MINIZIP::minizip)
    elseif(TARGET minizip)
        set(MINIZIP_TARGET minizip)
    else()
        message(FATAL_ERROR "[Deps] minizip-ng target not found. Check build output.")
    endif()
else()
    set(MINIZIP_TARGET MINIZIP::minizip-ng)
endif()
message(STATUS "[Deps] minizip target is ${MINIZIP_TARGET}")
