cmake_minimum_required(VERSION 3.21)
project(klook VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 6.8 REQUIRED COMPONENTS Gui Quick DBus)
find_package(KF6WindowSystem REQUIRED)
find_package(KF6SyntaxHighlighting REQUIRED)
find_package(KF6Archive REQUIRED)
find_package(KF6DBusAddons REQUIRED)
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
)

target_include_directories(klook PRIVATE src)

target_link_libraries(klook PRIVATE
    Qt6::Gui Qt6::Quick Qt6::DBus
    KF6::WindowSystem KF6::SyntaxHighlighting KF6::Archive KF6::DBusAddons
    LayerShellQt::Interface
)

include(GNUInstallDirs)
install(TARGETS klook RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(FILES data/klook.desktop DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)

# Optionally start the resident instance at login so even the first preview is
# instant. System installs use XDG autostart; user installs (prefix under $HOME)
# get a per-user entry, since <prefix>/etc/xdg isn't on the autostart path.
option(KLOOK_AUTOSTART "Start klook --daemon at login" OFF)
if(KLOOK_AUTOSTART)
    if(CMAKE_INSTALL_PREFIX MATCHES "^$ENV{HOME}")
        set(KLOOK_AUTOSTART_DIR "$ENV{HOME}/.config/autostart")
    else()
        set(KLOOK_AUTOSTART_DIR "${CMAKE_INSTALL_FULL_SYSCONFDIR}/xdg/autostart")
    endif()
    install(FILES data/klook-autostart.desktop DESTINATION ${KLOOK_AUTOSTART_DIR} RENAME klook.desktop)
endif()
# KF6 only loads service menus that are executable.
install(FILES data/klook-servicemenu.desktop
        DESTINATION ${CMAKE_INSTALL_DATADIR}/kio/servicemenus
        RENAME klook.desktop
        PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
