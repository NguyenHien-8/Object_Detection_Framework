add_library(odf_torch_core
    nanodet/nanodet_adapter.cpp
    yolo26/yolo26_adapter.cpp
    register_adapters.cpp
)
add_library(odf::torch_core ALIAS odf_torch_core)
set_target_properties(odf_torch_core PROPERTIES EXPORT_NAME torch_core)

target_include_directories(odf_torch_core PUBLIC
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
target_link_libraries(odf_torch_core PUBLIC odf_core)
target_compile_features(odf_torch_core PUBLIC cxx_std_17)
odf_enable_warnings(odf_torch_core)
install(TARGETS odf_torch_core EXPORT odfTargets)
