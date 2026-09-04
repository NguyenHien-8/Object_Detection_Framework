cmake_minimum_required(VERSION 3.24)

project(ObjectDetectionFramework VERSION 0.1.0 LANGUAGES CXX)

option(ODF_BUILD_DESKTOP "Build the Qt 6 desktop application" OFF)
option(ODF_BUILD_TESTS "Build unit tests" ON)
option(ODF_BUILD_EXAMPLES "Build command-line examples" ON)
option(ODF_ENABLE_BENCHMARK "Build benchmark support" ON)
option(ODF_ENABLE_NCNN "Build the ncnn backend" OFF)
option(ODF_ENABLE_ONNXRUNTIME "Build the ONNX Runtime backend" OFF)
option(ODF_ENABLE_OPENVINO "Build the OpenVINO backend" OFF)
option(ODF_ENABLE_TENSORRT "Build the TensorRT backend" OFF)
option(ODF_ENABLE_PADDLE "Build the Paddle Inference backend" OFF)
option(ODF_ENABLE_CUDA "Enable CUDA execution where supported" OFF)
option(ODF_ENABLE_VULKAN "Enable Vulkan execution where supported" OFF)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

function(odf_enable_warnings target)
    if(MSVC)
        target_compile_options(${target} PRIVATE /W4 /permissive- /Zc:__cplusplus)
    else()
        target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wshadow)
    endif()
endfunction()

add_subdirectory(core)
add_subdirectory(frameworks/torch)
add_subdirectory(frameworks/paddle)

if(ODF_ENABLE_ONNXRUNTIME)
    add_subdirectory(backends/onnxruntime)
endif()

foreach(planned_backend IN ITEMS NCNN OPENVINO TENSORRT PADDLE)
    if(ODF_ENABLE_${planned_backend})
        message(FATAL_ERROR
            "ODF_ENABLE_${planned_backend}=ON, but this backend is not implemented in ODF 0.1. "
            "Disable it; see docs/backends.md for the current truthful feature matrix.")
    endif()
endforeach()

if(ODF_BUILD_DESKTOP)
    message(FATAL_ERROR
        "ODF_BUILD_DESKTOP=ON, but the desktop client is not part of the verified 0.1 build. "
        "Disable it; see docs/backends.md and docs/build.md.")
endif()

if(ODF_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

if(ODF_BUILD_TESTS)
    include(CTest)
    enable_testing()
    add_subdirectory(tests)
endif()

configure_package_config_file(
    ${PROJECT_SOURCE_DIR}/cmake/odfConfig.cmake.in
    ${PROJECT_BINARY_DIR}/odfConfig.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/odf)
write_basic_package_version_file(
    ${PROJECT_BINARY_DIR}/odfConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion)
install(EXPORT odfTargets
    FILE odfTargets.cmake
    NAMESPACE odf::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/odf)
install(FILES
    ${PROJECT_BINARY_DIR}/odfConfig.cmake
    ${PROJECT_BINARY_DIR}/odfConfigVersion.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/odf)
install(DIRECTORY ${PROJECT_SOURCE_DIR}/models/
    DESTINATION ${CMAKE_INSTALL_DATADIR}/odf/models
    FILES_MATCHING PATTERN "model.json" PATTERN "*.txt")
