file(GLOB AURORA_SRC
        # ${PROJECT_SOURCE_DIR}/modules/*.cppm
        ${PROJECT_SOURCE_DIR}/src/base/*.cc
        ${PROJECT_SOURCE_DIR}/src/base/**/*.cc
        ${PROJECT_SOURCE_DIR}/src/auth/*.cc
        ${PROJECT_SOURCE_DIR}/src/assets/*.cc
        ${PROJECT_SOURCE_DIR}/src/io/*.cc
        ${PROJECT_SOURCE_DIR}/src/jvm/*.cc
        ${PROJECT_SOURCE_DIR}/src/launch/*.cc
        ${PROJECT_SOURCE_DIR}/src/loader/*.cc
        ${PROJECT_SOURCE_DIR}/src/process/*.cc
        ${PROJECT_SOURCE_DIR}/src/version/*.cc
)

message(${AURORA_SRC})

add_library(AuroraCore SHARED)

target_sources(AuroraCore
    PUBLIC
        FILE_SET CXX_MODULES
        BASE_DIRS ${PROJECT_SOURCE_DIR}/modules/
        FILES
        # 第一阶段，base&io
            launcher.base.types.cppm
            launcher.base.platform.cppm
            launcher.base.error.cppm
            launcher.base.build.cppm
            launcher.base.config.cppm
            launcher.base.logger.cppm
            launcher.base.cppm

            launcher.io.cppm

        # 第二阶段， version -> assets -> jvm
            launcher.version.cppm

            launcher.assets.cppm

            launcher.jvm.cppm

        # 第三阶段， process -> launch
            launcher.process.cppm

            launcher.launch.cppm

        # 第四阶段， auth
            launcher.auth.cppm

        # 第五阶段， loader,模组等等扩展
            launcher.loader.cppm

            launcher.cppm
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
        MINIZIP::minizip-ng
        OpenSSL::SSL
        OpenSSL::Crypto
)
