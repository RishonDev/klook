cmake_minimum_required(VERSION 3.21)
project(klook VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 6.8 REQUIRED COMPONENTS Gui Quick)
find_package(KF6WindowSystem REQUIRED)
find_package(KF6SyntaxHighlighting REQUIRED)

qt_standard_project_setup(REQUIRES 6.8)

qt_add_executable(klook
    src/main.cpp
    src/iconprovider.h src/iconprovider.cpp
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
    SOURCES
        src/highlighter.h src/highlighter.cpp
)

target_include_directories(klook PRIVATE src)

target_link_libraries(klook PRIVATE
    Qt6::Gui Qt6::Quick
    KF6::WindowSystem KF6::SyntaxHighlighting
)

include(GNUInstallDirs)
install(TARGETS klook RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(FILES klook.desktop DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)
