/*
 * ==========================================================================
 *  main.cpp
 *  主程序
 *  version 1.0.0 (2026/6/6)
 * ==========================================================================
 */

#include <QApplication>    // 图形界面程序的核心类
#include <QIcon>
#include <QFont>            // 字体描述
#include <QFontDatabase>    // 查询系统已安装的字体
#include <QSurfaceFormat>   // 控制窗口表面的像素格式 (alpha通道等)

#include "topbarwindow.h"
#include "dutywindow.h"
#include "config.h"

int main(int argc, char *argv[]) {
    // ---------------------------------------------------------------
    //  设置全局表面格式
    // ---------------------------------------------------------------
    QSurfaceFormat fmt = QSurfaceFormat::defaultFormat();
    fmt.setAlphaBufferSize(8);               //  8bit / 256 级透明度
    QSurfaceFormat::setDefaultFormat(fmt);

    // ---------------------------------------------------------------
    //  创建 QApplication
    // ---------------------------------------------------------------
    QApplication app(argc, argv);
    app.setWindowIcon(QIcon(":/appico"));

    loadConfig();   // 从 AppData 读取持久化配置

    // ---------------------------------------------------------------
    //  设置全局字体
    // ---------------------------------------------------------------
#ifdef Q_OS_WIN
    const char *fontFamilies[] = {
        "Century Gothic", "Microsoft YaHei", "SimHei", nullptr};
#else
    const char *fontFamilies[] = {
        "Century Gothic", "Noto Sans CJK SC", "Noto Sans CJK JP", nullptr};
#endif
    QFont globalFont;
    for (const char **name = fontFamilies; *name; ++name) {
        if (QFontDatabase::hasFamily(*name)) {
            globalFont = QFont(*name, 16);
            break;
        }
    }
    if (!globalFont.pixelSize())
        globalFont = QFont("sans-serif", 16);
    globalFont.setStyleStrategy(QFont::PreferAntialias);
    app.setFont(globalFont);

    // ---------------------------------------------------------------
    //  创建窗口
    // ---------------------------------------------------------------
    TopBarWindow topBar;    // 顶栏
    topBar.show();

    DutyWindow dutyWin;     // 值日简表
    g_dutyWin = &dutyWin;
    if (DUTY_VISIBLE) dutyWin.show();

    // ---------------------------------------------------------------
    //  进入事件循环
    // ---------------------------------------------------------------
    return app.exec();
}
