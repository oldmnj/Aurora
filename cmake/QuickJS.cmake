# cmake/QuickJS-NG.cmake
set(_QJS_DIR ${PROJECT_SOURCE_DIR}/third_party/quickjs-ng)
set(_QJS_BUILD_LIBC ON CACHE BOOL "Build stdlib extension" FORCE)

# 核心源文件
set(_QJS_SOURCES
    ${_QJS_DIR}/quickjs.c
    ${_QJS_DIR}/dtoa.c
    ${_QJS_DIR}/libregexp.c
    ${_QJS_DIR}/libunicode.c
)

if(_QJS_BUILD_LIBC)
    list(APPEND _QJS_SOURCES ${_QJS_DIR}/quickjs-libc.c)
endif()

# 编译定义
set(_QJS_DEFINES _GNU_SOURCE QUICKJS_NG_BUILD)
if(_QJS_BUILD_LIBC)
    list(APPEND _QJS_DEFINES QJS_BUILD_LIBC)
endif()
if(WIN32)
    list(APPEND _QJS_DEFINES WIN32_LEAN_AND_MEAN _WIN32_WINNT=0x0601)
endif()

# 创建库
add_library(qjs STATIC ${_QJS_SOURCES})
target_compile_definitions(qjs PRIVATE ${_QJS_DEFINES})
target_include_directories(qjs PUBLIC ${_QJS_DIR})

# 链接依赖
find_package(Threads REQUIRED)
target_link_libraries(qjs PUBLIC 
    ${CMAKE_DL_LIBS}
    Threads::Threads
)
# 不链接 libm！

# 动态库版本（可选）
# add_library(qjs_shared SHARED ${_QJS_SOURCES})
# target_compile_definitions(qjs_shared PRIVATE ${_QJS_DEFINES})
# target_include_directories(qjs_shared PUBLIC ${_QJS_DIR})
# target_link_libraries(qjs_shared PUBLIC 
#     ${CMAKE_DL_LIBS}
#     Threads::Threads
# )
# set_target_properties(qjs_shared PROPERTIES OUTPUT_NAME quickjs)
