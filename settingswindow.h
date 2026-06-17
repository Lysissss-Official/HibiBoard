/*
 * ==========================================================================
 *  settingswindow.h
 *  设置窗口声明
 * ==========================================================================
 */

#ifndef SETTINGSWINDOW_H
#define SETTINGSWINDOW_H

#include <QWidget>
#include <QLabel>
#include <QLineEdit>
#include <QDateEdit>
#include <QTextEdit>
#include <QPushButton>
#include <QTimer>
#include <QVector>
#include <QStackedWidget>
#include <QCheckBox>

class TopBarWindow;

class SettingsWindow : public QWidget {
    Q_OBJECT
public:
    explicit SettingsWindow(TopBarWindow *topbar = nullptr,
                            QWidget *parent = nullptr);

    void popIn();   // 弹入动画
    void popOut();  // 弹出动画 (动画结束后 hide + deleteLater)

protected:
    void paintEvent(QPaintEvent *) override;

private:
    void tick();
    void buildUi();
    void buildSchedulePage();
    void buildGroupPage();
    void buildCalendarPage();
    void buildSemesterPage();
    void buildOtherPage();
    void buildAboutPage();
    void saveCurrent();

    // 动画
    double m_progress = 0.0;
    double m_target   = 0.0;
    QTimer *m_animTimer;

    // 左侧导航
    QVector<QPushButton *> m_navBtns;
    int m_activeNav = 0;
    void setActiveNav(int idx);

    // 右侧内容区
    QStackedWidget *m_stack;
    QWidget *m_schedulePage;
    QWidget *m_groupPage;
    QWidget *m_calendarPage;
    QWidget *m_semesterPage;
    QWidget *m_otherPage;
    QWidget *m_aboutPage;

    // 学期设置
    QLineEdit *m_classEdit;
    QDateEdit *m_semesterEdit;

    // 其他设置
    QCheckBox *m_exclusiveZoneCb;
    QCheckBox *m_alwaysOnTopCb;

    TopBarWindow *m_topbar = nullptr;

    // 日历
    int m_calYear = 0, m_calMonth = 0;
    QLabel *m_calTitle;
    void rebuildPage(int idx);       // 销毁旧页面 + 重建
    void buildContent(int idx);      // 按要求填充内容
    void refreshCalendar();
};

#endif
