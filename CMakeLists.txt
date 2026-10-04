cmake_minimum_required(VERSION 3.21)
cmake_policy(SET CMP0091 NEW)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
project(PrecacheManager LANGUAGES CXX)

if(NOT WIN32 OR NOT MSVC OR NOT CMAKE_SIZEOF_VOID_P EQUAL 4)
    message(FATAL_ERROR "PrecacheManager requires Windows MSVC x86. Configure with -A Win32.")
endif()
get_property(multi_config GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
if(multi_config)
    set(CMAKE_CONFIGURATION_TYPES "Debug;Release" CACHE STRING "Supported configurations" FORCE)
elseif(NOT CMAKE_BUILD_TYPE MATCHES "^(Debug|Release)$")
    message(FATAL_ERROR "Select Debug or Release.")
endif()

include(cmake/Dependencies.cmake)
precachemanager_prepare_dependencies()

include("${VC_LTL_Root}/VC-LTL helper for cmake.cmake")

# Include directories follow PrecacheManager.vcxproj; only the SDK headers are
# needed, and the plugin links no third-party library.
add_library(precachemanager_settings INTERFACE)
target_include_directories(precachemanager_settings INTERFACE
    src
    "${METAHOOK_SOURCE_PATH}/include"
    "${METAHOOK_SOURCE_PATH}/include/HLSDK/cl_dll"
    "${METAHOOK_SOURCE_PATH}/include/HLSDK/common"
    "${METAHOOK_SOURCE_PATH}/include/HLSDK/engine"
    "${METAHOOK_SOURCE_PATH}/include/HLSDK/pm_shared"
    "${METAHOOK_SOURCE_PATH}/include/HLSDK/public"
    "${METAHOOK_SOURCE_PATH}/include/Interface"
)
# _MBCS mirrors the original MultiByte character set.
target_compile_definitions(precachemanager_settings INTERFACE WIN32 _WINDOWS _USRDLL _MBCS
    $<$<CONFIG:Debug>:_DEBUG>
    $<$<CONFIG:Release>:NDEBUG>)
# Debug used to turn every warning off, Release stayed at Level3 with the SDK's
# known warnings suppressed.
target_compile_options(precachemanager_settings INTERFACE /permissive
    $<$<CONFIG:Debug>:/W0>
    $<$<CONFIG:Release>:/W3>
    $<$<CONFIG:Release>:/wd4311>
    $<$<CONFIG:Release>:/wd4312>
    $<$<CONFIG:Release>:/wd4819>
    $<$<CONFIG:Release>:/wd4996>)

include(cmake/Sources.cmake)
add_library(PrecacheManager SHARED ${PRECACHEMANAGER_SOURCES})
target_link_libraries(PrecacheManager PRIVATE precachemanager_settings)
target_compile_options(PrecacheManager PRIVATE /MP /Zi)
target_link_options(PrecacheManager PRIVATE /DEBUG
    $<$<CONFIG:Debug>:/INCREMENTAL>
    $<$<CONFIG:Release>:/INCREMENTAL:NO>
    $<$<CONFIG:Release>:/OPT:REF>
    $<$<CONFIG:Release>:/OPT:ICF>)
set_target_properties(PrecacheManager PROPERTIES INTERPROCEDURAL_OPTIMIZATION_RELEASE TRUE)

install(TARGETS PrecacheManager RUNTIME DESTINATION svencoop/metahook/plugins)
install(FILES "$<TARGET_PDB_FILE:PrecacheManager>" DESTINATION svencoop/metahook/plugins)

# PrecacheManager ships its own gamedata catalog, pruned to the symbols it
# resolves, in a nested directory the launcher's catalog loader merges
# (metahook/gamedata/precachemanager). The manifest lists those symbols; the
# synchronizer downloads the upstream catalog into a persistent cache, prunes
# it, and publishes the result.
option(PRECACHEMANAGER_SYNC_GAMEDATA "Download and prune PrecacheManager's gamedata catalog before building" ON)
set(PRECACHEMANAGER_GAMEDATA_DIR "${PROJECT_BINARY_DIR}/assets/svencoop/metahook/gamedata/precachemanager" CACHE PATH "PrecacheManager gamedata catalog directory")
if(PRECACHEMANAGER_SYNC_GAMEDATA)
    find_package(Python3 REQUIRED COMPONENTS Interpreter)
    add_custom_target(PrecacheManagerGameData
        COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/scripts/sync-gamedata.py"
            --manifest "${CMAKE_CURRENT_SOURCE_DIR}/scripts/manifests/precachemanager.json"
            --target-dir "${PRECACHEMANAGER_GAMEDATA_DIR}" --temp-root "${PROJECT_BINARY_DIR}/gamedata-sync"
        COMMENT "Synchronizing and validating PrecacheManager gamedata"
        VERBATIM
    )
    add_custom_target(PrecacheManagerGameDataValidate
        COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/scripts/validate-gamedata.py"
            "${PRECACHEMANAGER_GAMEDATA_DIR}" --manifest "${CMAKE_CURRENT_SOURCE_DIR}/scripts/manifests/precachemanager.json"
        COMMENT "Validating PrecacheManager gamedata against the manifest"
        VERBATIM
    )
    add_dependencies(PrecacheManagerGameDataValidate PrecacheManagerGameData)
    add_dependencies(PrecacheManager PrecacheManagerGameDataValidate)
endif()
install(DIRECTORY "${PRECACHEMANAGER_GAMEDATA_DIR}/" DESTINATION svencoop/metahook/gamedata/precachemanager OPTIONAL)

# The aggregate solution owns its single LaunchGame target.
if(CMAKE_SOURCE_DIR STREQUAL PROJECT_SOURCE_DIR)
    option(METAHOOKSV_ENABLE_LAUNCH_GAME "Add the Visual Studio build/deploy/debug target" OFF)
    if(METAHOOKSV_ENABLE_LAUNCH_GAME)
        include(cmake/LaunchGame.cmake)
        metahooksv_add_launch_game()
    endif()
endif()
