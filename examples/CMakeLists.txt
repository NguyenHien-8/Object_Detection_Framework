add_executable(odf_registry_inspect registry_inspect.cpp)
target_link_libraries(odf_registry_inspect PRIVATE odf_core)
target_compile_definitions(odf_registry_inspect PRIVATE
    ODF_DEFAULT_MODEL_ROOT="${PROJECT_SOURCE_DIR}/models")
odf_enable_warnings(odf_registry_inspect)

if(TARGET odf_app_support AND TARGET odf_backend_onnxruntime)
    add_subdirectory(image_detection)
    add_subdirectory(camera_detection)
endif()
