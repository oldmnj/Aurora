file(GLOB AURORA_SRC
        ${CMAKE_SOURCE_DIR}/src/impl/base/*.cc
        ${CMAKE_SOURCE_DIR}/src/impl/service/*.cc
        ${CMAKE_SOURCE_DIR}/src/impl/core/*.cc
        ${CMAKE_SOURCE_DIR}/src/impl/runtime/*.cc
)

file(GLOB AURORA_MODULES
        ${CMAKE_SOURCE_DIR}/src/aurora/service/*.cppm
        ${CMAKE_SOURCE_DIR}/src/aurora/runtime/*.cppm
        ${CMAKE_SOURCE_DIR}/src/aurora/core/*.cppm
        ${CMAKE_SOURCE_DIR}/src/launcher/base/*.cppm
)

message(${AURORA_SRC})

add_library(AuroraCore SHARED)

target_sources(AuroraCore
    PRIVATE
        FILE_SET CXX_MODULES
        BASE_DIRS ${CMAKE_SOURCE_DIR}
        FILES ${AURORA_MODULES}
)

target_sources(AuroraCore
    PRIVATE
        ${AURORA_SRC}
)

target_include_directories(AuroraCore
    PUBLIC
        ${PROJECT_SOURCE_DIR}/include
        ${PROJECT_SOURCE_DIR}/third_party
)

set(DEPS_MODULE_CORE
        nlohmann_json::nlohmann_json
        fmt::fmt
        spdlog::spdlog
        CURL::libcurl
        ${MINIZIP_TARGET}
        OpenSSL::SSL
        OpenSSL::Crypto
        qjs
#         yaml-cpp::yaml-cpp
)

target_link_libraries(AuroraCore
    PRIVATE
        ${DEPS_MODULE_CORE}
)
