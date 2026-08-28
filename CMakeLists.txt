cmake_minimum_required(VERSION 3.15)
project(CrashSmokeScaler C)

# Path to the extracted official X-Plane SDK.
set(XPSDK_PATH "" CACHE PATH "Path to the extracted X-Plane SDK folder")

if(NOT XPSDK_PATH)
    message(FATAL_ERROR
        "Set -DXPSDK_PATH=/path/to/extracted/SDK when running cmake. "
        "Download the SDK from https://developer.x-plane.com/sdk/plugin-sdk-downloads/")
endif()

add_library(CrashSmokeScaler MODULE CrashSmokeScaler.c)

target_include_directories(CrashSmokeScaler PRIVATE
    ${XPSDK_PATH}/CHeaders/XPLM
    ${XPSDK_PATH}/CHeaders/Widgets
    ${XPSDK_PATH}/CHeaders/Wrappers
)

target_compile_definitions(CrashSmokeScaler PRIVATE
    XPLM400 XPLM303 XPLM301 XPLM300 XPLM210 XPLM200
)

set_target_properties(CrashSmokeScaler PROPERTIES PREFIX "")

if(WIN32)
    set_target_properties(CrashSmokeScaler PROPERTIES SUFFIX ".xpl")
    target_link_libraries(CrashSmokeScaler
        ${XPSDK_PATH}/Libraries/Win/XPLM_64.lib
        ${XPSDK_PATH}/Libraries/Win/XPWidgets_64.lib
    )
    target_compile_definitions(CrashSmokeScaler PRIVATE IBM=1)
elseif(APPLE)
    set_target_properties(CrashSmokeScaler PROPERTIES SUFFIX ".xpl")
    target_compile_definitions(CrashSmokeScaler PRIVATE APL=1)
    target_link_options(CrashSmokeScaler PRIVATE
        "-F${XPSDK_PATH}/Libraries/Mac"
        "-framework" "XPLM"
        "-framework" "XPWidgets"
        "-undefined" "dynamic_lookup"
    )
else()
    set_target_properties(CrashSmokeScaler PROPERTIES SUFFIX ".xpl")
    target_compile_definitions(CrashSmokeScaler PRIVATE LIN=1)
    target_link_options(CrashSmokeScaler PRIVATE "-Wl,--no-undefined")
endif()

if(WIN32)
    set(XP_PLATFORM_DIR "win_x64")
elseif(APPLE)
    set(XP_PLATFORM_DIR "mac_x64")
else()
    set(XP_PLATFORM_DIR "lin_x64")
endif()

set_target_properties(CrashSmokeScaler PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/CrashSmokeScaler/${XP_PLATFORM_DIR}"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/CrashSmokeScaler/${XP_PLATFORM_DIR}"
)
