# ==========================================================================
#  HibiBoard — CMake 构建配置
#
#  【CMake 在 Qt 项目中的角色】
#  Qt 项目不能只用 g++ main.cpp, 因为 Qt 需要三个额外的构建工具:
#
#    1. MOC  (Meta-Object Compiler) — 处理 Q_OBJECT 宏
#    2. UIC  (UI Compiler)          — 处理 .ui 界面文件 (本项目未使用)
#    3. RCC  (Resource Compiler)    — 处理 .qrc 资源文件 (本项目未使用)
#
#  CMake 通过 AUTOMOC/AUTORCC/AUTOUIC 自动调用这些工具,
#  你只需要写普通的 CMakeLists.txt, 不用手动调用 moc。
#
#  【Qt 的模块结构】
#  Qt 不是一个大库, 而是按功能拆成多个模块:
#    Qt::Core     — 基础 (字符串、容器、事件循环、定时器)
#    Qt::Gui      — 绘图、字体、窗口系统集成
#    Qt::Widgets  — 控件 (按钮、标签、布局等经典桌面 UI)
#  还有更多模块 (Qt::Network, Qt::Sql, Qt::Qml...), 按需添加。
# ==========================================================================

cmake_minimum_required(VERSION 4.3)
project(HibiBoard)

# ---- C++ 标准 ----
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ---- 开启 Qt 自动工具 ----
# AUTOMOC: 自动扫描头文件中的 Q_OBJECT, 调用 moc 生成元对象代码
# AUTORCC: 自动编译 .qrc 资源文件
# AUTOUIC: 自动编译 .ui 界面文件
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# ---- 查找 Qt6 模块 ----
# find_package 会搜索 Qt6 的 CMake 配置文件,
# 找到后提供 Qt::Core, Qt::Gui, Qt::Widgets 等 imported target。
# REQUIRED: 找不到就报错退出 (没有 Qt 编译不了)
find_package(Qt6 COMPONENTS
        Core      # QTimer, QString, QVector, QDateTime ...
        Gui       # QPainter, QFont, QSurfaceFormat ...
        Widgets   # QWidget, QLabel, QHBoxLayout ...
        Network   # QNetworkAccessManager (GitHub 头像)
        REQUIRED)

# ---- 可执行文件 ----
add_executable(HibiBoard WIN32
        main.cpp
        topbarwindow.cpp
        topbarwindow.h
        dutywindow.cpp
        dutywindow.h
        settingswindow.cpp
        settingswindow.h
        config.h
        resources.qrc
)
if(WIN32)
    target_sources(HibiBoard PRIVATE resources.rc)
endif()

# ---- 链接 Qt 库 ----
# PRIVATE: 这些库只被 HibiBoard 使用, 不会传递给依赖 HibiBoard 的项目
# (本项目中 HibiBoard 是最终产物, PUBLIC/PRIVATE 区别不大)
target_link_libraries(HibiBoard PRIVATE
        Qt::Core
        Qt::Gui
        Qt::Widgets
        Qt::Network
)

# ---- 可选: Wayland layer-shell 支持 ----
# QUIET: 找不到也不报错, 安静跳过
# 找到 → 定义 HAS_LAYER_SHELL 宏, 启用 Wayland 面板模式 (避免平铺 WM 缩放窗口)
# 找不到 → 回退到手动 setGeometry 定位
find_package(LayerShellQt QUIET)
if(LayerShellQt_FOUND)
    target_link_libraries(HibiBoard PRIVATE LayerShellQt::Interface)
    target_compile_definitions(HibiBoard PRIVATE HAS_LAYER_SHELL)
    message(STATUS "LayerShellQt found — Wayland panel mode enabled")
else()
    message(STATUS "LayerShellQt not found — fallback to regular window mode")
endif()

# ---- 安装规则 ----
# 可执行文件
install(TARGETS HibiBoard RUNTIME DESTINATION .)

# Qt 运行时 DLL（仅 MinGW 交叉编译时）
if(WIN32)
    set(MINGW_BIN_DIR "${CMAKE_FIND_ROOT_PATH}/bin")
    set(MINGW_PLUGINS_DIR "${CMAKE_FIND_ROOT_PATH}/lib/qt6/plugins")

    # Qt 模块 DLL（真实路径在 lib/qt6/bin, bin/ 下是 symlink）
    foreach(qt_mod Core Gui Widgets Network)
        install(FILES "${MINGW_BIN_DIR}/../lib/qt6/bin/Qt6${qt_mod}.dll" DESTINATION .)
    endforeach()

    # 运行时依赖（含传递依赖闭包）
    foreach(dll libstdc++-6 libgcc_s_seh-1 libwinpthread-1
                libfreetype-6 libharfbuzz-0 libpng16-16
                libpcre2-16-0 libpcre2-8-0
                libbrotlidec libbrotlicommon
                libzstd zlib1 libbz2-1 libssp-0
                libglib-2.0-0 libintl-8 libiconv-2
                libgraphite2)
        install(FILES "${MINGW_BIN_DIR}/${dll}.dll" DESTINATION .)
    endforeach()

    # Qt 平台插件
    install(FILES "${MINGW_PLUGINS_DIR}/platforms/qwindows.dll" DESTINATION platforms)
endif()

# ---- CPack NSIS 安装包 ----
set(CPACK_PACKAGE_NAME "HibiBoard")
set(CPACK_PACKAGE_VERSION "1.2")
set(CPACK_PACKAGE_VENDOR "HibiBoard")
set(CPACK_GENERATOR "NSIS")
set(CPACK_NSIS_INSTALL_ROOT "$PROGRAMFILES64")
set(CPACK_NSIS_DISPLAY_NAME "HibiBoard")
set(CPACK_NSIS_CONTACT "hibiboard@example.com")
set(CPACK_NSIS_MENU_LINKS "HibiBoard.exe" "HibiBoard")
set(CPACK_NSIS_MUI_ICON "${CMAKE_SOURCE_DIR}/assets/HibiBoard.ico")
set(CPACK_NSIS_MUI_UNIICON "${CMAKE_SOURCE_DIR}/assets/HibiBoard.ico")
set(CPACK_NSIS_INSTALLER_ICON "${CMAKE_SOURCE_DIR}/assets/HibiBoard.ico")
set(CPACK_NSIS_CREATE_ICONS_EXTRA
    "CreateShortCut '$SMPROGRAMS\\\\$STARTMENU_FOLDER\\\\HibiBoard.lnk' '$INSTDIR\\\\HibiBoard.exe'")
set(CPACK_NSIS_DELETE_ICONS_EXTRA
    "Delete '$SMPROGRAMS\\\\$START_MENU\\\\HibiBoard.lnk'")

include(CPack)
