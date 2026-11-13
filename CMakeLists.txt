cmake_minimum_required(VERSION 3.21)
project(klook VERSION 0.5.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# KDE's install layout (before Qt, which would otherwise pull in GNUInstallDirs).
find_package(ECM 6.0 REQUIRED NO_MODULE)
list(APPEND CMAKE_MODULE_PATH ${ECM_MODULE_PATH})
include(KDEInstallDirs)

find_package(Qt6 6.8 REQUIRED COMPONENTS Gui Quick DBus Concurrent)
find_package(KF6WindowSystem REQUIRED)
find_package(KF6SyntaxHighlighting REQUIRED)
find_package(KF6Archive REQUIRED)
find_package(KF6DBusAddons REQUIRED)
find_package(KF6Service REQUIRED)
find_package(KF6Config REQUIRED)
find_package(LayerShellQt REQUIRED)

qt_standard_project_setup(REQUIRES 6.8)

qt_add_executable(klook
    src/main.cpp
    src/iconprovider.h src/iconprovider.cpp
    src/previewer.h src/previewer.cpp
)

qt_add_qml_module(klook
    URI klook
    OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/qml/klook
    # Imports load at run time; scanning would also pick up the settings page's
    # Kirigami imports and warn that they can't be linked into klook.
    NO_IMPORT_SCAN
    VERSION 1.0
    QML_FILES
        qml/Main.qml
        qml/ImageView.qml
        qml/TextView.qml
        qml/PdfView.qml
        qml/MediaView.qml
        qml/OtherView.qml
        qml/MarkdownView.qml
        qml/ArchiveView.qml
    SOURCES
        src/highlighter.h src/highlighter.cpp
        src/textfile.h src/textfile.cpp
        src/archivemodel.h src/archivemodel.cpp
        src/settings.h src/settings.cpp
        src/markdownpolish.h src/markdownpolish.cpp
)

target_include_directories(klook PRIVATE src)
target_compile_definitions(klook PRIVATE KLOOK_VERSION="${PROJECT_VERSION}")

target_link_libraries(klook PRIVATE
    Qt6::Gui Qt6::Quick Qt6::DBus Qt6::Concurrent
    KF6::WindowSystem KF6::SyntaxHighlighting KF6::Archive KF6::DBusAddons KF6::Service KF6::ConfigCore
    LayerShellQt::Interface
)

install(TARGETS klook RUNTIME DESTINATION ${KDE_INSTALL_BINDIR})
install(FILES data/klook.desktop DESTINATION ${KDE_INSTALL_DATADIR}/applications)
install(FILES data/try-me.md DESTINATION ${KDE_INSTALL_DATADIR}/klook)

# Optionally start the resident instance at login so even the first preview is
# instant. System installs use XDG autostart; user installs (prefix under $HOME)
# get a per-user entry, since <prefix>/etc/xdg isn't on the autostart path.
option(KLOOK_AUTOSTART "Start klook --daemon at login" OFF)
if(KLOOK_AUTOSTART)
    if(CMAKE_INSTALL_PREFIX MATCHES "^$ENV{HOME}")
        set(KLOOK_AUTOSTART_DIR "$ENV{HOME}/.config/autostart")
    else()
        set(KLOOK_AUTOSTART_DIR "${KDE_INSTALL_AUTOSTARTDIR}")
    endif()
    install(FILES data/klook-autostart.desktop DESTINATION ${KLOOK_AUTOSTART_DIR} RENAME klook.desktop)
endif()
# KF6 only loads service menus that are executable.
install(FILES data/klook-servicemenu.desktop
        DESTINATION ${KDE_INSTALL_DATADIR}/kio/servicemenus
        RENAME klook.desktop
        PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)

option(KLOOK_KCM "Build the System Settings page" ON)
if(KLOOK_KCM)
    add_subdirectory(kcm)
endif()

include(CTest)
if(BUILD_TESTING)
    find_package(Qt6 REQUIRED COMPONENTS Test)
    add_executable(settingstest tests/settingstest.cpp src/settings.cpp src/settings.h)
    target_include_directories(settingstest PRIVATE src)
    target_link_libraries(settingstest PRIVATE Qt6::Test Qt6::Qml KF6::ConfigCore)
    add_test(NAME settings COMMAND settingstest)
endif()
