add_executable(LoggerTest ${PROJECT_SOURCE_DIR}/tests/test_logger/main.cc)
target_link_libraries(LoggerTest
    PRIVATE
        AuroraCore
        fmt::fmt
        spdlog::spdlog
)

