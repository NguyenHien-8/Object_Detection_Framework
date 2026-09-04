add_executable(odf_unit_tests test_main.cpp)
target_link_libraries(odf_unit_tests PRIVATE odf_core odf_torch_core odf_paddle_core)
target_compile_definitions(odf_unit_tests PRIVATE ODF_SOURCE_DIR="${PROJECT_SOURCE_DIR}")
odf_enable_warnings(odf_unit_tests)
add_test(NAME odf_unit_tests COMMAND odf_unit_tests)

