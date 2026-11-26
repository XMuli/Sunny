cmake_minimum_required(VERSION 3.16)

set(PROJECT_NAME SunnyCapturer)
set(project_version_major 3)
set(project_version_minor 2)
set(project_version_path 3)
set(project_version ${project_version_major}.${project_version_minor}.${project_version_path})
project(${PROJECT_NAME} VERSION ${project_version} LANGUAGES CXX)

add_definitions(-DXBUILD_TIME="2025.03.11")
add_compile_definitions(BUILD_MICROSOFT_STORE)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
# add_compile_definitions(NO_SEND_USER_DATA) # not send user data

if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Default build type: Release." FORCE)
endif ()

# #######################################################################################################################
# dependencies
# #######################################################################################################################
set(BUILD_SHARED_LIBS           OFF CACHE BOOL "Build static libraries")
set(WITH_GFLAGS                 OFF CACHE BOOL "Use gflags for google/glog")
set(PROBE_BUILD_WITH_QT         ON  CACHE BOOL "Build probe with Qt")
set(JSON_BuildTests             OFF CACHE BOOL "Build nlohmann/json tests")
set(QT_DEFAULT_MAJOR_VERSION    6   CACHE STRING "Qt major version to use (5 or 6), default: 6.")
set(QAPPLICATION_CLASS QApplication CACHE STRING "Qt application class to use (QApplication, QGuiApplication, QCoreApplication)")

add_subdirectory(3rdparty/SingleApplication      EXCLUDE_FROM_ALL)
add_subdirectory(3rdparty/Qt-AES                 EXCLUDE_FROM_ALL)
add_subdirectory(3rdparty/QHotkey                EXCLUDE_FROM_ALL)
add_subdirectory(3rdparty/glog                   EXCLUDE_FROM_ALL)
add_subdirectory(3rdparty/fmt                    EXCLUDE_FROM_ALL)
add_subdirectory(3rdparty/probe                  EXCLUDE_FROM_ALL)
add_subdirectory(3rdparty/json                   EXCLUDE_FROM_ALL)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin")         # .exe or executable file (application) -> Output path for .exe and .dll during runtime.

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(qt_moduls Widgets Network PdfWidgets)    # Multimedia WebSockets Xml Svg Network Charts [case sensitive]
find_package(QT NAMES Qt6 REQUIRED COMPONENTS ${qt_moduls} LinguistTools)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS ${qt_moduls} LinguistTools) # LinguistTools No link

if (WIN32)
    # GUI/console
    set(CMAKE_WIN32_EXECUTABLE $<IF:$<CONFIG:DEBUG>,OFF,ON>)

elseif (UNIX AND NOT APPLE)
    find_package(Qt6 6.7 COMPONENTS DBus REQUIRED)
endif ()

# #######################################################################################################################
# Create the executable
# #######################################################################################################################
configure_file(src/version.h.in ${PROJECT_BINARY_DIR}/version.h @ONLY)

file(GLOB_RECURSE SOURCES src/*.h src/*.cpp src/*.ui)

# #######################################################################################################################
# Automatically get the Qt installation path using the `LOCATION` property of `Qt::Core`, which points to the path of the core module library file.
# #######################################################################################################################
if(CMAKE_SIZEOF_VOID_P EQUAL 4)
    set(arch_bit "86")
    set(is_x64bit false)
elseif(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(arch_bit "64")
    set(is_x64bit true)
endif()

get_target_property(QtCore_location Qt${QT_VERSION_MAJOR}::Core LOCATION)
get_filename_component(qt_binray_dir ${QtCore_location} DIRECTORY)
message("Qt installation directory qt_binray_dir: ${qt_binray_dir}")            # C:/Qt/6.7.3/msvc2022_64/bin

if(UNIX)  # using apt install qt?-devtools*

    execute_process(
        COMMAND bash -c "lsb_release -d | awk -F: '{print $2}' | sed 's/^[ \t]*//'"
        OUTPUT_VARIABLE LSB_ID
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    message(STATUS "This linux syste, Linux Distribution Description: [${LSB_ID}]")

    if (${LSB_ID} STREQUAL "Arch Linux")
        set(qt_binray_dir "/usr/lib/qt6/bin/")
        message("archLinux qt_binray_dir: ${qt_binray_dir}")            # /usr/lib/qt6/bin/qmake
        set(lupdate_path "${qt_binray_dir}/lupdate")
        set(lrelease_path "${qt_binray_dir}/lrelease")
    else()
        set(lupdate_path "${qt_binray_dir}/../bin/lupdate")
        set(lrelease_path "${qt_binray_dir}/../bin/lrelease")
    endif()

else()
    set(lupdate_path "${qt_binray_dir}/lupdate")
    set(lrelease_path "${qt_binray_dir}/lrelease")

    if(WIN32)
        set(windeployqt "${qt_binray_dir}/windeployqt6")
    elseif(APPLE)
        # set(windeployqt "${qt_binray_dir}/macdeployqt")
    elseif(UNIX)
        # custom deployqt
    endif()
endif()

# #######################################################################################################################
# copy resources some file
# #######################################################################################################################
set(folders_to_copy "licenses" "icons/logo" "linux_script")
set(source_root "${CMAKE_SOURCE_DIR}/src/resources")
set(traget_root "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/resources")

foreach(it ${folders_to_copy})
    set(copy_source_dir "${source_root}/${it}")
    file(COPY ${copy_source_dir} DESTINATION ${traget_root})
endforeach()

# #######################################################################################################################
# update .ts, and generate .qm
# #######################################################################################################################
set(ts_dir "${CMAKE_CURRENT_SOURCE_DIR}/translations")
if(APPLE)
    set(qm_dir ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${project_name}.app/Contents/MacOS/translations)
else()
    set(qm_dir ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/translations)
endif()

file(MAKE_DIRECTORY "${ts_dir}" "${qm_dir}")
file(GLOB ts_files "${ts_dir}/*.ts")

set(ts_files_name "")    # Get the filename part (excluding the path).
foreach(it ${ts_files})
    get_filename_component(ts_filename ${it} NAME_WE)
    list(APPEND ts_files_name ${ts_filename})
endforeach()

message("lrelease_path:  ${lrelease_path}")
message("ts_dir: ${ts_dir}")


foreach(it ${ts_files_name})
    # execute_process(COMMAND "${lupdate_path}" -locations none -no-obsolete "${CMAKE_CURRENT_SOURCE_DIR}/src" -ts "${ts_dir}/${it}.ts")
    execute_process(COMMAND "${lrelease_path}" "${ts_dir}/${it}.ts" -qm "${qm_dir}/${it}.qm")
endforeach()

# file(GLOB_RECURSE qm_files "${qm_dir}/*.qm")

add_executable(${PROJECT_NAME}
        ${SOURCES}
        src/capturer.qrc
        # ${qm_files}
        $<$<PLATFORM_ID:Windows>:src/resources/ico_win32.rc>
)

target_compile_options(${PROJECT_NAME}
        PRIVATE
        $<$<CXX_COMPILER_ID:MSVC>:/W4 /utf-8 /DUNICODE /D_UNICODE /DNOMINMAX /Zc:preprocessor /Zc:__cplusplus /wd5054>
        $<$<CXX_COMPILER_ID:MSVC>:/D_DISABLE_CONSTEXPR_MUTEX_CONSTRUCTOR> # VS 17.10: https://github.com/microsoft/STL/issues/4730
        $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra -Wpedantic -Wno-deprecated-enum-enum-conversion>
)

set(qt_version_moduls)
foreach(it ${qt_moduls})
    list(APPEND qt_version_moduls Qt${QT_VERSION_MAJOR}::${it})
endforeach()
target_link_libraries(${PROJECT_NAME}
        PRIVATE
        ${qt_version_moduls}
        SingleApplication::SingleApplication
        QtAES::QtAES
        QHotkey::QHotkey
        glog::glog
        fmt::fmt
        probe::probe
        nlohmann_json::nlohmann_json
        $<$<PLATFORM_ID:Windows>:Winmm>
        $<$<PLATFORM_ID:Linux>:Qt6::DBus>
)

target_include_directories(${PROJECT_NAME}
        PRIVATE
        src/
        src/common
        src/widgets
        src/preview
        src/snipping
        src/setting
        src/process/network/version_updater
        src/process/ocr/online_ocr
        ${PROJECT_BINARY_DIR} # Generated header files
)

include_directories("${CMAKE_CURRENT_SOURCE_DIR}/src/ui/ocr")  # Fix: linux build failed

# Get the number of CPU cores and enable IDE multi-core compilation.
include(ProcessorCount)
ProcessorCount(N)
message("Number of available processor cores: ${N}")

if(WIN32)
    if(MSVC)
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /MP")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP")
    elseif(MINGW)
        set(CMAKE_MAKE_PROGRAM "${CMAKE_MAKE_PROGRAM} -j${N}")
    endif()
elseif(UNIX)
    add_compile_options(-j${N})
endif()

if(MSVC)
    # Set default start project, Fix: Visual Studio 2022 remove Release/Debug folder
    set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${PROJECT_NAME})

    set_target_properties(${PROJECT_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
    set_target_properties(${PROJECT_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
    set_target_properties(${PROJECT_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
    set_target_properties(${PROJECT_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
    set_target_properties(${PROJECT_NAME} PROPERTIES ARCHIVE_OUTPUT_DIRECTORY_DEBUG "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
    set_target_properties(${PROJECT_NAME} PROPERTIES ARCHIVE_OUTPUT_DIRECTORY_RELEASE "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
    set_target_properties(${PROJECT_NAME} PROPERTIES ARCHIVE_OUTPUT_DIRECTORY_MINSIZEREL "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
    set_target_properties(${PROJECT_NAME} PROPERTIES ARCHIVE_OUTPUT_DIRECTORY_RELWITHDEBINFO "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")

    target_compile_definitions(${PROJECT_NAME} PRIVATE
        WIN32_LEAN_AND_MEAN                                                                  # Header files containing only the common APIs
        _CRT_SECURE_NO_WARNINGS                                                              # Unsafe functions such as strcpy, scanf, etc. are allowed)
    )
endif()

# #######################################################################################################################
# # Define some variables that can be used in *.cpp
# #######################################################################################################################
target_compile_definitions(${PROJECT_NAME} PRIVATE XPROJECT_NAME="${PROJECT_NAME}")
target_compile_definitions(${PROJECT_NAME} PRIVATE XPROJECT_VERSION="${project_version}")
target_compile_definitions(${PROJECT_NAME} PRIVATE XARCH_BIT="${arch_bit}")
target_compile_definitions(${PROJECT_NAME} PRIVATE XLINUX_DISTRO="${linux_distro}")
target_compile_definitions(${PROJECT_NAME} PRIVATE XCOMPILER="${CMAKE_HOST_SYSTEM_PROCESSOR}")  # i386/i686/x86_64/unknown  Win: AMD64
target_compile_definitions(${PROJECT_NAME} PRIVATE XCOMPILER_ID="${CMAKE_CXX_COMPILER_ID}")     # Clang/GCC/MSVC or GNU

# #######################################################################################################################
# copy OpenSSL and qt base .dll
# #######################################################################################################################
if(APPLE)
elseif(UNIX)
elseif(WIN32)

    set(ssl_dir "C:/Qt/Tools/OpenSSLv3/Win_x64/bin")
    set(ssl_file "libssl-3-x64.dll" "libcrypto-3-x64.dll")
    foreach(it ${ssl_file})
        file(COPY "${ssl_dir}/${it}" DESTINATION "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
        message("OpenSSL dll it: ${ssl_dir}/${it}")
    endforeach()

    message("windeployqt: ${windeployqt}")
    message("${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${PROJECT_NAME}.exe")
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD COMMAND "${windeployqt}" "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${PROJECT_NAME}.exe" --no-opengl-sw --no-translations)
endif()

# #######################################################################################################################
# Creator Linux .deb and .AppImage
# #######################################################################################################################

include(GNUInstallDirs)
include (FeatureSummary)  # using  set_package_properties
if (UNIX)
    set(CMAKE_INSTALL_PREFIX /usr)
    message("CMAKE_INSTALL_DATADIR: ${CMAKE_INSTALL_DATADIR}")

    # /opt/SunnyCapturer/lib/
    install (DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/linux_qt6_dependency/lib
             DESTINATION "/opt/${PROJECT_NAME}/"
             )

    # /opt/SunnyCapturer/plugins/
    install (DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/linux_qt6_dependency/plugins
             DESTINATION "/opt/${PROJECT_NAME}/"
             )

    # /opt/SunnyCapturer/translations/*.qm
    install (DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/translations
             DESTINATION "/opt/${PROJECT_NAME}/"
             FILES_MATCHING PATTERN "*.qm"
             )

    # /usr/share/applications/sunnycapturer.desktop
    install (FILES ${CMAKE_CURRENT_SOURCE_DIR}/src/resources/linux_script/sunnycapturer.desktop
        DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/applications"
    )


    # /etc/ld.so.conf.d/sunnycapture.conf
    # install (FILES ${CMAKE_CURRENT_SOURCE_DIR}/src/resources/linux_script/sunnycapturer.conf
    #    DESTINATION "/etc/ld.so.conf.d"
    # )

    # /usr/share/icons/hicolor/scalable/apps/sunnycapturer.svg
    install (FILES ${CMAKE_CURRENT_SOURCE_DIR}/src/resources/icons/logo/logo.svg
        DESTINATION "${CMAKE_INSTALL_DATADIR}/icons/hicolor/scalable/apps"
        RENAME sunnycapturer.svg
    )

    # /usr/bin/SunnyCapturer
    install (TARGETS ${PROJECT_NAME}
         RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
         LIBRARY DESTINATION "/usr/lib"
         ARCHIVE DESTINATION "/usr/lib" COMPONENT Devel
         )

     # CPACK: General Settings
     set(CPACK_GENERATOR "DEB")                                # set CPack General DEB
     set(CPACK_PACKAGE_NAME "${PROJECT_NAME}")
     set(CPACK_PACKAGE_VERSION "${project_version}")
     set(CPACK_PACKAGE_VENDOR "https://github.com/XMuli")
     set(CPACK_PACKAGE_CONTACT "https://sunnycapturer.xmuli.tech")
     set(CPACK_DEBIAN_PACKAGE_MAINTAINER "XMuli <xmulitech@gmail.com>")
     set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "SunnyCapturer is a simple and beautiful cross-platform screenshot software that supports OCR to extract text from images, image translation, and pinning images to the screen, among other features.")
     set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}_${CPACK_PACKAGE_VERSION}")

     # Set dependencies for Debian packages
     # set(CPACK_DEBIAN_PACKAGE_SHILIBDEPS ON)
     # set(CPACK_DEBIAN_PACKAGE_DEPENDS "libqt5x11extras5, libqt5widgets5, libqt5xml5, libqt5svg5, libqt5network5")

     # Ensure CPack is included
     include(CPack)

endif()


if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(${PROJECT_NAME})
endif()
