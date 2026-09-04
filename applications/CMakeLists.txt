find_package(OpenCV 4 QUIET COMPONENTS core imgproc imgcodecs videoio)
if(NOT OpenCV_FOUND)
    message(FATAL_ERROR
        "ODF inference applications require OpenCV 4 (core, imgproc, imgcodecs, videoio). "
        "Set OpenCV_DIR to the directory containing OpenCVConfig.cmake.")
endif()

add_library(odf_app_support
    common/cli_options.cpp
    common/logging_options.cpp
    common/model_root.cpp
    common/opencv_bridge.cpp
    common/runtime_catalog.cpp
)
add_library(odf::app_support ALIAS odf_app_support)
target_include_directories(odf_app_support PUBLIC
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/applications/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
target_link_libraries(odf_app_support
    PUBLIC odf_core odf_torch_core odf_paddle_core ${OpenCV_LIBS})
target_include_directories(odf_app_support SYSTEM PUBLIC ${OpenCV_INCLUDE_DIRS})
if(TARGET odf_backend_onnxruntime)
    target_link_libraries(odf_app_support PUBLIC odf_backend_onnxruntime)
    target_compile_definitions(odf_app_support PUBLIC ODF_WITH_ONNXRUNTIME=1)
endif()
target_compile_definitions(odf_app_support PRIVATE
    ODF_INSTALLED_MODEL_DIR="${CMAKE_INSTALL_FULL_DATADIR}/odf/models")
target_compile_features(odf_app_support PUBLIC cxx_std_17)
odf_enable_warnings(odf_app_support)

if(WIN32 AND TARGET opencv_world)
    string(TOUPPER "${CMAKE_BUILD_TYPE}" odf_opencv_configuration)
    get_target_property(odf_opencv_runtime opencv_world
        "IMPORTED_LOCATION_${odf_opencv_configuration}")
    if(NOT odf_opencv_runtime)
        get_target_property(odf_opencv_runtime opencv_world IMPORTED_LOCATION_RELEASE)
    endif()
    if(odf_opencv_runtime)
        set_property(TARGET odf_app_support PROPERTY ODF_OPENCV_RUNTIME_DLL
            "${odf_opencv_runtime}")
    endif()
endif()

function(odf_stage_runtime_dependencies target)
    if(NOT WIN32)
        return()
    endif()
    get_property(odf_opencv_dll TARGET odf_app_support PROPERTY ODF_OPENCV_RUNTIME_DLL)
    if(odf_opencv_dll)
        add_custom_command(TARGET ${target} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${odf_opencv_dll}" $<TARGET_FILE_DIR:${target}>
            VERBATIM)
    endif()
    if(TARGET odf_backend_onnxruntime)
        get_property(odf_ort_dll TARGET odf_backend_onnxruntime PROPERTY ODF_RUNTIME_DLL)
        if(odf_ort_dll)
            add_custom_command(TARGET ${target} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "${odf_ort_dll}" $<TARGET_FILE_DIR:${target}>
                VERBATIM)
        endif()
    endif()
endfunction()
