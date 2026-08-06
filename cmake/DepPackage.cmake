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

        message(STATUS "[Deps] ${PACKAGE_NAME} fetched and built")
    else()
        message(STATUS "[Deps] ${PACKAGE_NAME} found system-wide: ${${PACKAGE_NAME}_DIR}")
    endif()

    if(NOT TARGET ${targetName})
        message(FATAL_ERROR
            "[Deps] Target '${targetName}' not found for package '${packageName}'.\n"
            "Available targets may differ. Check the library's CMakeLists.txt for exported target names."
        )
    endif()
endmacro()

find_or_fetch(fmt fmt::fmt
    "https://github.com/fmtlib/fmt.git"
    "11.2.0"
)

find_or_fetch(spdlog spdlog::spdlog
    "https://github.com/gabime/spdlog.git"
    "1.17.0"
)

find_package(minizip-ng QUIET)

if(TARGET MINIZIP::minizip-ng)
    set(MINIZIP_TARGET MINIZIP::minizip-ng)
elseif(TARGET MINIZIP::minizip)
    set(MINIZIP_TARGET MINIZIP::minizip)
elseif(TARGET minizip)
    set(MINIZIP_TARGET minizip)
else()
    message("NOT FOUND target minizip-ng")
endif()
message("minizip target is ${MINIZIP_TARGET}")

find_or_fetch(nlohmann_json nlohmann_json::nlohmann_json
    "https://github.com/nlohmann/json.git"
    "3.12.0"
)

find_package(OpenSSL REQUIRED)
find_package(CURL REQUIRED)
