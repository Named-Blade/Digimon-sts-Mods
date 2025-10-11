cmake_minimum_required(VERSION 3.15)

# Set project name and language
project(KZMods LANGUAGES CXX ASM_MASM)

# Specify the C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Disable iterator debugging (equivalent to /D_ITERATOR_DEBUG_LEVEL=0)
add_definitions(-D_ITERATOR_DEBUG_LEVEL=0)

# Find the required libraries
set(SHLWAPI_LIB shlwapi.lib)
set(MINHOOK_LIB ${CMAKE_BINARY_DIR}/minhook/Release/minhook.x64.lib)
set(ZYCORE_LIB ${CMAKE_BINARY_DIR}/../dependencies/CallHook/thirdparty/zydis/include/Zydis/Zycore/Zycore.lib)
set(ZYDIS_LIB ${CMAKE_BINARY_DIR}/../dependencies/CallHook/thirdparty/zydis/include/Zydis/Zydis.lib)

add_subdirectory(dependencies/minhook minhook)

# Add include directories
include_directories(${CMAKE_SOURCE_DIR}/utils)
include_directories(${CMAKE_SOURCE_DIR}/dependencies/CallHook/include)
include_directories(${CMAKE_SOURCE_DIR}/dependencies/minhook/include)
include_directories(${CMAKE_SOURCE_DIR}/dependencies/Pattern16/include)
include_directories(${CMAKE_SOURCE_DIR}/dependencies/mINI/src)
include_directories(${CMAKE_SOURCE_DIR}/dependencies/PointerChain)

#build asm file
function(add_asm_source ASM_SRC ASM_OBJ)
    add_custom_command(
        OUTPUT ${ASM_OBJ}
        COMMAND ml64 /c /nologo /Fo ${ASM_OBJ} ${ASM_SRC}
        DEPENDS ${ASM_SRC}
    )
endfunction()

#build mod
function(build_mod MOD_NAME MOD_SOURCE)
set (MOD_ASM ${ARGN})
    if(NOT DEFINED MOD_ASM)
        set(SRC_FILES mods/${MOD_NAME}/${MOD_SOURCE}.cpp)
    else()
        set(ASM_SRC ${CMAKE_SOURCE_DIR}/mods/${MOD_NAME}/${MOD_ASM}.asm)
        set(ASM_OBJ ${CMAKE_SOURCE_DIR}/build/${MOD_ASM}.obj)
        add_asm_source(${ASM_SRC} ${ASM_OBJ})
        set(SRC_FILES mods/${MOD_NAME}/${MOD_SOURCE}.cpp ${ASM_OBJ})
    endif()

    add_library(${MOD_NAME} SHARED ${SRC_FILES})
    add_dependencies(${MOD_NAME} minhook)
    target_link_libraries(${MOD_NAME} PRIVATE ${MINHOOK_LIB} ${ZYCORE_LIB} ${ZYDIS_LIB} ${SHLWAPI_LIB})
endfunction()

#build Mods
build_mod(testMod testMod)
build_mod(logMod logMod)
build_mod(fieldGuideMod fieldGuideMod)