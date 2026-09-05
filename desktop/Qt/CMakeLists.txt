find_package(Qt6 6.5 QUIET COMPONENTS Widgets)
if(NOT Qt6_FOUND)
    message(FATAL_ERROR
        "ODF_BUILD_DESKTOP=ON requires Qt 6 Widgets. Set CMAKE_PREFIX_PATH to a Qt 6 "
        "desktop kit, for example C:/Qt/6.x.x/msvc2022_64.")
endif()

qt_standard_project_setup()

qt_add_executable(odf_desktop WIN32
    app/main.cpp
    app/ApplicationBootstrap.cpp
    app/ApplicationBootstrap.hpp
    bridge/QtImageBridge.cpp
    bridge/QtImageBridge.hpp
    controllers/AppController.cpp
    controllers/AppController.hpp
    services/CameraDeviceService.cpp
    services/CameraDeviceService.hpp
    services/SettingsService.cpp
    services/SettingsService.hpp
    widgets/ClassFilterPanel.cpp
    widgets/ClassFilterPanel.hpp
    widgets/DetectionViewport.cpp
    widgets/DetectionViewport.hpp
    widgets/MainWindow.cpp
    widgets/MainWindow.hpp
    workers/CameraWorker.cpp
    workers/CameraWorker.hpp
    workers/InferenceWorker.cpp
    workers/InferenceWorker.hpp
)
target_include_directories(odf_desktop PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(odf_desktop PRIVATE odf_app_support Qt6::Widgets)
if(WIN32)
    target_link_libraries(odf_desktop PRIVATE ole32 mf mfplat mfuuid)
endif()
target_compile_features(odf_desktop PRIVATE cxx_std_17)
odf_enable_warnings(odf_desktop)
odf_stage_runtime_dependencies(odf_desktop)

if(WIN32)
    get_target_property(odf_qmake Qt6::qmake IMPORTED_LOCATION)
    get_filename_component(odf_qt_bin "${odf_qmake}" DIRECTORY)
    find_program(ODF_WINDEPLOYQT_EXECUTABLE windeployqt HINTS "${odf_qt_bin}" NO_DEFAULT_PATH)
    if(ODF_WINDEPLOYQT_EXECUTABLE)
        add_custom_command(TARGET odf_desktop POST_BUILD
            COMMAND "${ODF_WINDEPLOYQT_EXECUTABLE}" --no-translations
                "$<TARGET_FILE:odf_desktop>"
            VERBATIM)
    else()
        message(WARNING "windeployqt was not found; use scripts/deploy_windows.ps1 before distribution")
    endif()
    get_filename_component(odf_qt_root "${odf_qt_bin}" DIRECTORY)
    set(odf_qoffscreen "${odf_qt_root}/plugins/platforms/qoffscreen.dll")
    if(EXISTS "${odf_qoffscreen}")
        add_custom_command(TARGET odf_desktop POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E make_directory
                "$<TARGET_FILE_DIR:odf_desktop>/platforms"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${odf_qoffscreen}" "$<TARGET_FILE_DIR:odf_desktop>/platforms"
            VERBATIM)
    endif()
endif()

add_custom_target(odf_stage_desktop_models ALL
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${PROJECT_SOURCE_DIR}/models" "$<TARGET_FILE_DIR:odf_desktop>/models"
    DEPENDS odf_desktop
    VERBATIM)

install(TARGETS odf_desktop RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
