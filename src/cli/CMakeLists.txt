add_executable(AuroraCLI)

file(GLOB CLI_SOURCE
    ${CMAKE_SOURCE_DIR}/src/cli/main.cc
    ${CMAKE_SOURCE_DIR}/src/cli/cmd/*.cc
)

target_link_libraries(AuroraCLI
    PRIVATE
        AuroraCore
        ${DEPS_MODULE_CORE}
)

target_include_directories(AuroraCLI
    PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/third_party
)

target_sources(AuroraCLI
    PRIVATE
        FILE_SET CXX_MODULES
        BASE_DIRS ${CMAKE_SOURCE_DIR}/src/cli
        FILES
            CLI11/CLI11.cppm
)

target_sources(AuroraCLI
    PRIVATE
        ${CLI_SOURCE}
)
