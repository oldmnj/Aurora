file(GLOB AURORA_SRC
        ${CMAKE_SOURCE_DIR}/src/impl/base/*.cc
        ${CMAKE_SOURCE_DIR}/src/impl/service/*.cc
        ${CMAKE_SOURCE_DIR}/src/impl/core/*.cc
        ${CMAKE_SOURCE_DIR}/src/impl/runtime/*.cc
)

message(${AURORA_SRC})

add_library(AuroraCore SHARED)

target_sources(AuroraCore
    PRIVATE
        FILE_SET CXX_MODULES
        BASE_DIRS ${CMAKE_SOURCE_DIR}/src/
        FILES
        # 第一阶段，base&io
            launcher/base/types.cppm
            launcher/base/platform.cppm
            launcher/base/error.cppm
            launcher/base/build.cppm
            launcher/base/config.cppm
            launcher/base/base.cppm

            aurora/service/random.cppm
            aurora/service/uuid.cppm
            aurora/service/logger.cppm
            aurora/service/io.cppm
            aurora/service/network.cppm
            aurora/service/crypto.cppm
            aurora/service/download.cppm
            aurora/service/archive.cppm
            aurora/service/cache.cppm
            aurora/service/quickjs.cppm
            aurora/service/auth.cppm
            aurora/service/service.cppm

            aurora/core/pool.cppm
            aurora/core/scheduler.cppm
            aurora/core/context.cppm
            aurora/core/task.cppm
            aurora/core/event.cppm
            aurora/core/core.cppm

            aurora/runtime/version.cppm
            aurora/runtime/assets.cppm
            aurora/runtime/jvm.cppm
            aurora/runtime/process.cppm
            aurora/runtime/launch.cppm
            aurora/runtime/loader.cppm
            aurora/runtime/plugin.cppm
            aurora/runtime/runtime.cppm
            
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
