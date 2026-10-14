cmake_minimum_required(VERSION 3.21)

project(FlashBarCSandbox VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 6.2 REQUIRED COMPONENTS Core Widgets Concurrent)

set(APP_FINDER_SOURCES
    src/repositories/InstalledAppFinder.h
    src/infrastructure/InstalledAppFinderFactory.cpp
    src/infrastructure/InstalledAppFinderFactory.h
)
set(APP_FINDER_LIBRARIES)
if(WIN32)
    list(APPEND APP_FINDER_SOURCES
        src/infrastructure/windows/StartMenuAppFinder.cpp
        src/infrastructure/windows/StartMenuAppFinder.h
    )
    set(APP_FINDER_LIBRARIES ole32 shell32 uuid)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    list(APPEND APP_FINDER_SOURCES
        src/infrastructure/linux/DesktopEntryFinder.cpp
        src/infrastructure/linux/DesktopEntryFinder.h
        src/infrastructure/linux/DesktopEntryParser.cpp
        src/infrastructure/linux/DesktopEntryParser.h
    )
endif()

add_executable(FlashBarCSandbox
    src/main.cpp
    src/domain/AppEntry.h
    src/repositories/AppCatalogRepository.h
    src/services/AppLauncher.h
    src/services/SearchApps.cpp
    src/services/SearchApps.h
    src/services/LaunchApp.cpp
    src/services/LaunchApp.h
    src/infrastructure/JsonAppCatalogRepository.cpp
    src/infrastructure/JsonAppCatalogRepository.h
    src/infrastructure/ProcessAppLauncher.cpp
    src/infrastructure/ProcessAppLauncher.h
    src/ui/MainWindow.cpp
    src/ui/MainWindow.h
    ${APP_FINDER_SOURCES}
)

target_link_libraries(FlashBarCSandbox PRIVATE Qt6::Widgets Qt6::Concurrent ${APP_FINDER_LIBRARIES})

include(CTest)
if(BUILD_TESTING)
    add_executable(FlashBarCSandboxTests
        tests/AppArchitectureTests.cpp
        src/domain/AppEntry.h
        src/repositories/AppCatalogRepository.h
        src/services/AppLauncher.h
        src/services/SearchApps.cpp
        src/services/SearchApps.h
        src/services/LaunchApp.cpp
        src/services/LaunchApp.h
        src/infrastructure/JsonAppCatalogRepository.cpp
        src/infrastructure/JsonAppCatalogRepository.h
        src/infrastructure/linux/DesktopEntryParser.cpp
        src/infrastructure/linux/DesktopEntryParser.h
    )
    target_link_libraries(FlashBarCSandboxTests PRIVATE Qt6::Core)
    add_test(NAME FlashBarCSandboxTests COMMAND FlashBarCSandboxTests)
endif()

configure_file(apps.json apps.json COPYONLY)

set_target_properties(FlashBarCSandbox PROPERTIES
    WIN32_EXECUTABLE ON
    MACOSX_BUNDLE ON
)

include(GNUInstallDirs)
install(TARGETS FlashBarCSandbox RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(FILES apps.json DESTINATION ${CMAKE_INSTALL_BINDIR})
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    install(FILES packaging/FlashBarCSandbox.desktop DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)
    install(FILES packaging/flashbarcsandbox.png
        DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/256x256/apps
        RENAME flashbarcsandbox.png)
endif()