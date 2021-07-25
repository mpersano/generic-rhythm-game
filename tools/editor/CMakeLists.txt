find_package(Qt5 COMPONENTS Widgets Multimedia REQUIRED)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(editor_SOURCES
    main.cpp
    editorwindow.cpp
    editorwindow.h
    track.cpp
    track.h
    trackview.cpp
    trackview.h
    trackinfowidget.cpp
    trackinfowidget.h
    waveformrenderer.cpp
    waveformrenderer.h
)

add_executable(editor
    ${editor_SOURCES}
)

target_link_libraries(editor
    Qt5::Widgets
    Qt5::Multimedia
)
