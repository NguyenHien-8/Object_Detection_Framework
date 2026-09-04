add_executable(odf_unit_tests test_main.cpp)
target_link_libraries(odf_unit_tests PRIVATE odf_core odf_torch_core odf_paddle_core)
target_compile_definitions(odf_unit_tests PRIVATE ODF_SOURCE_DIR="${PROJECT_SOURCE_DIR}")
odf_enable_warnings(odf_unit_tests)
add_test(NAME odf_unit_tests COMMAND odf_unit_tests)

if(TARGET odf_app_support)
    add_executable(odf_app_tests app_test_main.cpp)
    target_link_libraries(odf_app_tests PRIVATE odf_app_support)
    target_compile_definitions(odf_app_tests PRIVATE ODF_SOURCE_DIR="${PROJECT_SOURCE_DIR}")
    odf_enable_warnings(odf_app_tests)
    odf_stage_runtime_dependencies(odf_app_tests)
    add_test(NAME odf_app_tests COMMAND odf_app_tests)
endif()

if(TARGET odf_desktop)
    find_package(Qt6 6.5 REQUIRED COMPONENTS Widgets)
    qt_add_executable(odf_desktop_tests
        desktop_test_main.cpp
        "${PROJECT_SOURCE_DIR}/desktop/Qt/bridge/QtImageBridge.cpp"
        "${PROJECT_SOURCE_DIR}/desktop/Qt/bridge/QtImageBridge.hpp"
        "${PROJECT_SOURCE_DIR}/desktop/Qt/widgets/ClassFilterPanel.cpp"
        "${PROJECT_SOURCE_DIR}/desktop/Qt/widgets/ClassFilterPanel.hpp")
    target_include_directories(odf_desktop_tests PRIVATE "${PROJECT_SOURCE_DIR}/desktop/Qt")
    target_link_libraries(odf_desktop_tests PRIVATE odf_app_support Qt6::Widgets)
    set_target_properties(odf_desktop_tests PROPERTIES AUTOMOC ON)
    odf_enable_warnings(odf_desktop_tests)
    odf_stage_runtime_dependencies(odf_desktop_tests)
    add_test(NAME odf_desktop_tests COMMAND odf_desktop_tests)
    get_target_property(odf_test_qmake Qt6::qmake IMPORTED_LOCATION)
    get_filename_component(odf_test_qt_bin "${odf_test_qmake}" DIRECTORY)
    get_filename_component(odf_test_qt_root "${odf_test_qt_bin}" DIRECTORY)
    set_tests_properties(odf_desktop_tests PROPERTIES
        ENVIRONMENT "QT_QPA_PLATFORM=offscreen;QT_PLUGIN_PATH=${odf_test_qt_root}/plugins"
        ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:${odf_test_qt_bin}")

    set(odf_missing_model_root "${CMAKE_CURRENT_BINARY_DIR}/missing-models")
    file(MAKE_DIRECTORY
        "${odf_missing_model_root}/yolo26/yolo26n"
        "${odf_missing_model_root}/labels")
    configure_file(
        "${PROJECT_SOURCE_DIR}/models/yolo26/yolo26n/model.json"
        "${odf_missing_model_root}/yolo26/yolo26n/model.json" COPYONLY)
    configure_file(
        "${PROJECT_SOURCE_DIR}/models/labels/coco80.txt"
        "${odf_missing_model_root}/labels/coco80.txt" COPYONLY)
    add_test(NAME odf_desktop_smoke
        COMMAND odf_desktop --models "${PROJECT_SOURCE_DIR}/models" --odf-smoke-test)
    set_tests_properties(odf_desktop_smoke PROPERTIES
        ENVIRONMENT "QT_QPA_PLATFORM=offscreen;ODF_DESKTOP_SMOKE_EXIT_MS=1200"
        TIMEOUT 15)
    add_test(NAME odf_desktop_missing_model_smoke
        COMMAND odf_desktop --models "${odf_missing_model_root}" --odf-smoke-test)
    set_tests_properties(odf_desktop_missing_model_smoke PROPERTIES
        ENVIRONMENT "QT_QPA_PLATFORM=offscreen;ODF_DESKTOP_SMOKE_EXIT_MS=1200"
        TIMEOUT 15)
endif()

if(ODF_ENABLE_REAL_MODEL_TESTS)
    if(NOT TARGET odf_app_support)
        message(FATAL_ERROR "ODF_ENABLE_REAL_MODEL_TESTS requires ODF_ENABLE_ONNXRUNTIME=ON")
    endif()
    if(NOT ODF_TEST_MODEL_ROOT OR NOT IS_DIRECTORY "${ODF_TEST_MODEL_ROOT}")
        message(FATAL_ERROR "Set ODF_TEST_MODEL_ROOT to a model registry containing yolo26n")
    endif()
    if(NOT ODF_TEST_IMAGE OR NOT EXISTS "${ODF_TEST_IMAGE}")
        message(FATAL_ERROR "Set ODF_TEST_IMAGE to a real JPG or PNG containing COCO objects")
    endif()
    add_executable(odf_real_model_test real_model_test.cpp)
    target_link_libraries(odf_real_model_test PRIVATE odf_app_support)
    target_compile_definitions(odf_real_model_test PRIVATE
        ODF_TEST_MODEL_ROOT="${ODF_TEST_MODEL_ROOT}"
        ODF_TEST_IMAGE="${ODF_TEST_IMAGE}")
    odf_enable_warnings(odf_real_model_test)
    odf_stage_runtime_dependencies(odf_real_model_test)
    add_test(NAME odf_real_yolo26n COMMAND odf_real_model_test)
endif()
