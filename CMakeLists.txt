cmake_minimum_required(VERSION 3.30.0)
project(AuroraLuncherCore
    LANGUAGES CXX
    VERSION 0.1.0
)

# if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
#     target_compile_options(demo PRIVATE /W4)
# elseif(CMAKE_CXX_COMPILER_ID STREQUAL "LLVM")
#     target_compile_options()
# elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
#     target_compile_options(demo PRIVATE -Wall -Wextra)
# endif()

include(cmake/Settings.cmake)

# if(NOT EXISTS "${PROJECT_SOURCE_DIR}/cmake/xrepo.cmake")
#     message(STATUS "Downloading xrepo.cmake from https://github.com/xmake-io/xrepo-cmake/")
#     file(DOWNLOAD 
#         "https://raw.githubusercontent.com/xmake-io/xrepo-cmake/main/xrepo.cmake"
#         "${PROJECT_BINARY_DIR}/cmake/xrepo.cmake"
#     )
# endif()
# include(${PROJECT_SOURCE_DIR}/cmake/xrepo.cmake)
include(cmake/DepPackage.cmake)

add_subdirectory(src)


add_subdirectory(tests)
