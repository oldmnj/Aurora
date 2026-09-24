include(GoogleTest)

add_executable(logger_tests)

target_link_libraries(logger_tests
    PRIVATE
        AuroraCore
        GTest::gmock_main
        ${DEPS_MODULE_CORE}
)

target_sources(logger_tests
    PRIVATE
        test_logger/main.cc
)

gtest_discover_tests(logger_tests)
