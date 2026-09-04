set(ONNXRUNTIME_ROOT "" CACHE PATH "Root of an ONNX Runtime C/C++ distribution")

find_path(ONNXRUNTIME_INCLUDE_DIR
    NAMES onnxruntime_cxx_api.h
    HINTS ${ONNXRUNTIME_ROOT}
    PATH_SUFFIXES include include/onnxruntime/core/session)
find_library(ONNXRUNTIME_LIBRARY
    NAMES onnxruntime
    HINTS ${ONNXRUNTIME_ROOT}
    PATH_SUFFIXES lib lib64)

if(NOT ONNXRUNTIME_INCLUDE_DIR OR NOT ONNXRUNTIME_LIBRARY)
    message(FATAL_ERROR
        "ODF_ENABLE_ONNXRUNTIME=ON but ONNX Runtime was not found. "
        "Set ONNXRUNTIME_ROOT to an official C/C++ distribution, or disable the option.")
endif()

add_library(odf_backend_onnxruntime onnxruntime_backend.cpp)
add_library(odf::backend_onnxruntime ALIAS odf_backend_onnxruntime)
set_target_properties(odf_backend_onnxruntime PROPERTIES EXPORT_NAME backend_onnxruntime)
target_include_directories(odf_backend_onnxruntime
    PUBLIC
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
    PRIVATE ${ONNXRUNTIME_INCLUDE_DIR})
target_link_libraries(odf_backend_onnxruntime PUBLIC odf_core PRIVATE ${ONNXRUNTIME_LIBRARY})
target_compile_features(odf_backend_onnxruntime PUBLIC cxx_std_17)
odf_enable_warnings(odf_backend_onnxruntime)
install(TARGETS odf_backend_onnxruntime EXPORT odfTargets)
