add_library(odf_paddle_core
    picodet/picodet_adapter.cpp
    register_adapters.cpp
)
add_library(odf::paddle_core ALIAS odf_paddle_core)
set_target_properties(odf_paddle_core PROPERTIES EXPORT_NAME paddle_core)

target_include_directories(odf_paddle_core PUBLIC
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
target_link_libraries(odf_paddle_core PUBLIC odf_core)
target_compile_features(odf_paddle_core PUBLIC cxx_std_17)
odf_enable_warnings(odf_paddle_core)
install(TARGETS odf_paddle_core EXPORT odfTargets)
