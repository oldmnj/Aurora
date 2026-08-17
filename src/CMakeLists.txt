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
            launcher.base/launcher.base.types.cppm
            launcher.base/launcher.base.platform.cppm
            launcher.base/launcher.base.error.cppm
            launcher.base/launcher.base.build.cppm
            launcher.base/launcher.base.config.cppm
            launcher.base/launcher.base.cppm

            aurora.service/aurora.service.random.cppm
            aurora.service/aurora.service.uuid.cppm
            aurora.service/aurora.service.logger.cppm
            aurora.service/aurora.service.io.cppm
            aurora.service/aurora.service.network.cppm
            aurora.service/aurora.service.crypto.cppm
            aurora.service/aurora.service.download.cppm
            aurora.service/aurora.service.archive.cppm
            aurora.service/aurora.service.cache.cppm
            aurora.service/aurora.service.quickjs.cppm
            aurora.service/aurora.service.auth.cppm
            aurora.service/aurora.service.cppm

            aurora.core/aurora.core.pool.cppm
            aurora.core/aurora.core.scheduler.cppm
            aurora.core/aurora.core.context.cppm
            aurora.core/aurora.core.task.cppm
            aurora.core/aurora.core.event.cppm
            aurora.core/aurora.core.cppm

            aurora.runtime/aurora.runtime.version.cppm
            aurora.runtime/aurora.runtime.assets.cppm
            aurora.runtime/aurora.runtime.jvm.cppm
            aurora.runtime/aurora.runtime.process.cppm
            aurora.runtime/aurora.runtime.launch.cppm
            aurora.runtime/aurora.runtime.loader.cppm
            aurora.runtime/aurora.runtime.plugin.cppm
            aurora.runtime/aurora.runtime.cppm
            
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
