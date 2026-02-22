cmake_minimum_required(VERSION 3.19)
project(NetworkSimulator LANGUAGES CXX)

# Find Qt6 with all needed components
find_package(Qt6 6.5 REQUIRED COMPONENTS Core Widgets Gui)

qt_standard_project_setup()

# Add the executable with src/ paths
qt_add_executable(NetworkSimulator
    MACOSX_BUNDLE
    src/main.cpp
    src/netsim_window.cpp
    src/netsim_classes.h
    src/netsim.ui
)

# Link necessary Qt modules
target_link_libraries(NetworkSimulator
    PRIVATE
        mingw32
        Qt6::Core
        Qt6::Widgets
        Qt6::Gui
)

# Set C++ standard
set_target_properties(NetworkSimulator PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)

# Install targets
include(GNUInstallDirs)
install(TARGETS NetworkSimulator
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

# Deploy script
qt_generate_deploy_app_script(
    TARGET NetworkSimulator
    OUTPUT_SCRIPT deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
)

if (WIN32)
    get_target_property(QT_QMAKE_EXECUTABLE Qt6::qmake IMPORTED_LOCATION)
    get_filename_component(QT_BIN_DIR "${QT_QMAKE_EXECUTABLE}" DIRECTORY)
    
    add_custom_command(TARGET NetworkSimulator POST_BUILD
        # First copy the exe to a deploy folder
        COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_SOURCE_DIR}/deploy"
        COMMAND ${CMAKE_COMMAND} -E copy "$<TARGET_FILE:NetworkSimulator>" "${CMAKE_SOURCE_DIR}/deploy/"
        # Run windeployqt on the deploy folder copy
        COMMAND "${QT_BIN_DIR}/windeployqt.exe"
                --no-translations
                --no-system-d3d-compiler
                "${CMAKE_SOURCE_DIR}/deploy/NetworkSimulator.exe"
        COMMENT "Deploying to deploy/ folder..."
    )
endif()