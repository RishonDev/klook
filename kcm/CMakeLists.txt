# System Settings page (Apps & Windows → Quick Look) for klook's appearance.
find_package(Qt6 6.8 REQUIRED COMPONENTS Qml Quick)
find_package(KF6 REQUIRED COMPONENTS CoreAddons KCMUtils Config)

# A loadable plugin (KDE projects usually get these two from KDECMakeSettings).
set(BUILD_SHARED_LIBS ON)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

kcmutils_add_qml_kcm(kcm_klook)
set_target_properties(kcm_klook PROPERTIES PREFIX "")
target_sources(kcm_klook PRIVATE
    klookkcm.cpp klookkcm.h
    ../src/settings.cpp ../src/settings.h
)
target_include_directories(kcm_klook PRIVATE ../src)
target_link_libraries(kcm_klook PRIVATE
    Qt6::Qml Qt6::Quick
    KF6::CoreAddons KF6::ConfigCore KF6::KCMUtilsQuick
)
