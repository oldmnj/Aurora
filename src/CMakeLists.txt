file(GLOB AURORA_SRC
        # ${PROJECT_SOURCE_DIR}/modules/*.cppm
        ${PROJECT_SOURCE_DIR}/src/base/*.cc
        # ${PROJECT_SOURCE_DIR}/src/base/**/*.cc
        ${PROJECT_SOURCE_DIR}/src/aurora/service/*.cc
        ${PROJECT_SOURCE_DIR}/src/aurora/*.cc
)

message(${AURORA_SRC})

add_library(AuroraCore SHARED)

target_sources(AuroraCore
    PUBLIC
        FILE_SET CXX_MODULES
        BASE_DIRS ${PROJECT_SOURCE_DIR}/src/
        FILES
        # 第一阶段，base&io
            launcher.base/launcher.base.types.cppm
            launcher.base/launcher.base.platform.cppm
            launcher.base/launcher.base.error.cppm
            launcher.base/launcher.base.build.cppm
            launcher.base/launcher.base.config.cppm
            launcher.base/launcher.base.uuid.cppm
            launcher.base/launcher.base.cppm

            aurora.service/aurora.service.logger.cppm
            aurora.service/aurora.service.io.cppm
            aurora.service/aurora.service.network.cppm
            aurora.service/aurora.service.crypto.cppm
            aurora.service/aurora.service.archive.cppm
            aurora.service/aurora.service.cache.cppm
            aurora.service/aurora.service.auth.cppm
            aurora.service/aurora.service.cppm

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

target_link_libraries(AuroraCore
    PUBLIC
        nlohmann_json::nlohmann_json

    PRIVATE
        fmt::fmt
        spdlog::spdlog
        CURL::libcurl
        ${MINIZIP_TARGET}
        OpenSSL::SSL
        OpenSSL::Crypto
)

