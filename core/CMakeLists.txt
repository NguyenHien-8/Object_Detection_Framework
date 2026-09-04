add_library(odf_core
    status.cpp
    detection/bounding_box.cpp
    detection/nms.cpp
    image/preprocess.cpp
    model_registry/json.cpp
    model_registry/model_registry.cpp
    pipeline/detector_factory.cpp
    pipeline/detector_pipeline.cpp
    benchmark/statistics.cpp
    benchmark/performance_benchmark.cpp
    camera/latest_frame_buffer.cpp
    utils/logging.cpp
)

add_library(odf::core ALIAS odf_core)
set_target_properties(odf_core PROPERTIES EXPORT_NAME core)

target_include_directories(odf_core
    PUBLIC
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_compile_features(odf_core PUBLIC cxx_std_17)
target_compile_definitions(odf_core
    PUBLIC
        ODF_VERSION_MAJOR=${PROJECT_VERSION_MAJOR}
        ODF_VERSION_MINOR=${PROJECT_VERSION_MINOR}
        ODF_VERSION_PATCH=${PROJECT_VERSION_PATCH}
)
odf_enable_warnings(odf_core)

install(TARGETS odf_core EXPORT odfTargets)
install(DIRECTORY ${PROJECT_SOURCE_DIR}/include/odf
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
