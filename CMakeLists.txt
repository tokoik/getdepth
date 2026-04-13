cmake_minimum_required(VERSION 3.14)
project(getdepth CXX)

# C++ Standard Requirements
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Platform specific compile definitions
if(WIN32)
    add_compile_definitions(_USE_MATH_DEFINES NOMINMAX)
endif()

# 古い CMake ポリシーを使用する依存ライブラリ（librealsense2等）のための回避策
set(CMAKE_POLICY_VERSION_MINIMUM 3.5 CACHE STRING "Workaround for old policies in dependencies" FORCE)
set(CMAKE_POLICY_DEFAULT_CMP0148 OLD) # For PythonInterp deprecation in OpenCV etc

include(FetchContent)
# ---------------------------------------------------------
# 1. 自動ダウンロード： GLFW
# ---------------------------------------------------------
message(STATUS "Fetching GLFW...")
FetchContent_Declare(
    glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG        3.4
)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(glfw)

# ---------------------------------------------------------
# 2. 自動ダウンロード： librealsense2 (Rs400)
#   ※ OpenCV より先に FetchContent することで uninstall ターゲットの衝突を回避します
# ---------------------------------------------------------
message(STATUS "Fetching librealsense2...")
FetchContent_Declare(
    librealsense2
    GIT_REPOSITORY https://github.com/IntelRealSense/librealsense.git
    GIT_TAG        v2.54.2
)
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(BUILD_GRAPHICAL_EXAMPLES OFF CACHE BOOL "" FORCE)
set(BUILD_PYTHON_BINDINGS OFF CACHE BOOL "" FORCE)
set(BUILD_WITH_OPENMP OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(librealsense2)

# ---------------------------------------------------------
# 3. 自動ダウンロード： OpenCV
# ---------------------------------------------------------
message(STATUS "Fetching OpenCV...")
FetchContent_Declare(
    opencv
    GIT_REPOSITORY https://github.com/opencv/opencv.git
    GIT_TAG        4.10.0
)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(BUILD_PERF_TESTS OFF CACHE BOOL "" FORCE)
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(BUILD_opencv_apps OFF CACHE BOOL "" FORCE)
set(BUILD_opencv_python2 OFF CACHE BOOL "" FORCE)
set(BUILD_opencv_python3 OFF CACHE BOOL "" FORCE)
set(BUILD_opencv_java OFF CACHE BOOL "" FORCE)
set(WITH_GTK ON CACHE BOOL "" FORCE) # Required for Linux GUI
set(WITH_WIN32UI ON CACHE BOOL "" FORCE) # Required for Windows GUI
FetchContent_MakeAvailable(opencv)

# ---------------------------------------------------------
# ソースファイルの定義
# ---------------------------------------------------------
set(SOURCES
    main.cpp
    getdepth.cpp
    gg.cpp
    DepthCamera.cpp
    KinectV1.cpp
    KinectV2.cpp
    Ds325.cpp
    Rs400.cpp
)

# ---------------------------------------------------------
# 固有SDK (Kinect v1/v2, DepthSense) のチェックと追加（Windows専用）
# ---------------------------------------------------------
if(WIN32)
    # これらはMSIインストーラなどでシステムにインストールされている必要があります。
    # ヘッダが見つかればソースコードをビルド対象に加えます。

    # Kinect V1
    find_path(KINECT_V1_INC "NuiApi.h" PATHS "C:/Program Files/Microsoft SDKs/Kinect/v1.8/inc" "C:/Program Files/Microsoft SDKs/Kinect/v1.8/inc")
    if(KINECT_V1_INC)
        message(STATUS "Found Kinect V1 SDK")
        list(APPEND SOURCES KinectV1.cpp)
        include_directories(${KINECT_V1_INC})
        link_directories("C:/Program Files/Microsoft SDKs/Kinect/v1.8/lib/amd64")
        set(KINECT_V1_LIBS "Kinect10.lib")
    endif()

    # Kinect V2
    find_path(KINECT_V2_INC "Kinect.h" PATHS "C:/Program Files/Microsoft SDKs/Kinect/v2.0_1409/inc")
    if(KINECT_V2_INC)
        message(STATUS "Found Kinect V2 SDK")
        list(APPEND SOURCES KinectV2.cpp)
        include_directories(${KINECT_V2_INC})
        link_directories("C:/Program Files/Microsoft SDKs/Kinect/v2.0_1409/Lib/x64")
        set(KINECT_V2_LIBS "Kinect20.lib")
    endif()

    # DepthSense (DS325)
    find_path(DS325_INC "DepthSense.hxx" PATHS "C:/Program Files/SoftKinetic/DepthSenseSDK/include" "C:/Program Files (x86)/SoftKinetic/DepthSenseSDK/include")
    if(DS325_INC)
        message(STATUS "Found DepthSense SDK")
        list(APPEND SOURCES Ds325.cpp)
        include_directories(${DS325_INC})
        link_directories("C:/Program Files/SoftKinetic/DepthSenseSDK/lib") # AMD64 / x86 depend on target
        set(DS325_LIBS "DepthSense.lib")
    endif()
endif()

# テクスチャ等をコンパイル後のバイナリに含めるか、実行時に見つけるために
# 現在のディレクトリにアクセスできるようにします
add_executable(getdepth ${SOURCES})

target_include_directories(getdepth PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# ---------------------------------------------------------
# ライブラリのリンク
# ---------------------------------------------------------
# FetchContentで取得したライブラリは、通常ターゲットとしてリンクできます。
target_link_libraries(getdepth PRIVATE 
    glfw
    opencv_core opencv_highgui opencv_videoio opencv_imgproc
    realsense2
)

# Windows固有の SDK ライブラリ
if(WIN32)
    if(KINECT_V1_INC)
        target_link_libraries(getdepth PRIVATE ${KINECT_V1_LIBS})
    endif()
    if(KINECT_V2_INC)
        target_link_libraries(getdepth PRIVATE ${KINECT_V2_LIBS})
    endif()
    if(DS325_INC)
        target_link_libraries(getdepth PRIVATE ${DS325_LIBS})
    endif()
endif()

# OSに応じたシステムレベルのグラフィックス系ライブラリのリンク
if(WIN32)
    target_link_libraries(getdepth PRIVATE opengl32 user32 gdi32 shell32)
elseif(APPLE)
    find_library(COCOA_LIBRARY Cocoa)
    find_library(OPENGL_LIBRARY OpenGL)
    find_library(IOKIT_LIBRARY IOKit)
    find_library(COREVIDEO_LIBRARY CoreVideo)
    target_link_libraries(getdepth PRIVATE ${COCOA_LIBRARY} ${OPENGL_LIBRARY} ${IOKIT_LIBRARY} ${COREVIDEO_LIBRARY})
else()
    # Linux (Ubuntu) の場合
    # X11, GL, pthread 等が必要な場合があります
    target_link_libraries(getdepth PRIVATE GL X11 pthread)
endif()
