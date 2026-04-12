cmake_minimum_required(VERSION 3.21)
project(SCRYBE LANGUAGES C)

option(SCRYBE_ENABLE_ASAN "Enable AddressSanitizer" ON)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src")
set(INC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/header")

file(GLOB_RECURSE ENGINE_SRC CONFIGURE_DEPENDS
    "${SRC_DIR}/*.c"
)

file(GLOB_RECURSE VENDOR_SRC CONFIGURE_DEPENDS
    "${SRC_DIR}/vendor/*.c"
)

list(REMOVE_ITEM ENGINE_SRC ${VENDOR_SRC})

add_executable(${PROJECT_NAME}
    ${ENGINE_SRC}
    ${VENDOR_SRC}
)

find_package(glfw3 REQUIRED)
find_package(cglm REQUIRED)
find_package(json-c REQUIRED)
find_package(Threads REQUIRED)

target_include_directories(${PROJECT_NAME} PRIVATE
    ${INC_DIR}
)

target_include_directories(${PROJECT_NAME} SYSTEM PRIVATE
    "${INC_DIR}/vendor"
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    glfw
    cglm::cglm
    json-c::json-c
    Threads::Threads
)

target_compile_options(${PROJECT_NAME} PRIVATE
    -Wall
    -Wextra
    -Wpedantic
    -Wconversion
    -Wsign-conversion
    -Wdouble-promotion
    -Wfloat-equal
    -Wredundant-decls
    -Wshadow
    -Wswitch
    -Wswitch-enum
    -Wswitch-default
    -Wimplicit-fallthrough
    -Wvla
    -Walloca
    -Wcast-align
    -Wcast-qual
    -Wpointer-arith
    -Wnull-dereference
    -Wuninitialized
    -Wmissing-field-initializers
    -Wundef
    -Wunused-macros
    -Wno-unused-macros
    -Wformat=2
    -Wformat-security
    -Wwrite-strings
    -Wno-empty-translation-unit
    -Wbad-function-cast
    -Werror=implicit-function-declaration
    -Wmissing-prototypes
    -Wstrict-prototypes
    -Wold-style-definition
    -Wnested-externs
    -Wno-declaration-after-statement
    $<$<C_COMPILER_ID:Clang>:-Wno-gcc-compat>
    $<$<CONFIG:Debug>:-O1 -g -fno-omit-frame-pointer>
    $<$<CONFIG:Release>:-O3>
)

set_source_files_properties(${VENDOR_SRC}
    PROPERTIES COMPILE_OPTIONS "-w"
)

target_link_options(${PROJECT_NAME} PRIVATE
    $<$<CONFIG:Release>:-flto>
)

if (SCRYBE_ENABLE_ASAN)
    target_compile_options(${PROJECT_NAME} PRIVATE -fsanitize=address,undefined)
    target_link_options(${PROJECT_NAME} PRIVATE -fsanitize=address,undefined)
else()
    target_compile_options(${PROJECT_NAME} PRIVATE -fno-sanitize=all)
endif()

add_custom_target(run
    COMMAND ${PROJECT_NAME}
    DEPENDS ${PROJECT_NAME}
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/res
        $<TARGET_FILE_DIR:${PROJECT_NAME}>/res
)
