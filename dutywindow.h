/*
 * ==========================================================================
 *  dutywindow.h
 *  值日表窗口声明
 * ==========================================================================
 */

#ifndef DUTYWINDOW_H
#define DUTYWINDOW_H

#include <QWidget>
#include <QLabel>
#include <QTimer>
#include <QPixmap>

class DutyWindow : public QWidget {
    Q_OBJECT

public:
    explicit DutyWindow(QWidget *parent = nullptr);
    void applySettings();   // 即时应用置顶/可移动设置

protected:
    void paintEvent(QPaintEvent *) override;
    void moveEvent(QMoveEvent *) override;
    void showEvent(QShowEvent *) override;
    void mousePressEvent(QMouseEvent *) override;
    void mouseMoveEvent(QMouseEvent *) override;
    void mouseReleaseEvent(QMouseEvent *) override;

private:
    void setupUi();
    void initPlatform();
    void positionAtRight();
    void refresh();
    void captureBlurBg();
    static int todayIndex();

    QLabel *m_titleLabel = nullptr;
    QLabel *m_roleLabels[5] = {};
    QLabel *m_nameLabels[5] = {};
    QLabel *m_dateLabel    = nullptr;
    QTimer *m_timer        = nullptr;
    bool m_usingLayerShell = false;
    QPoint m_dragPos;  // 拖拽起始偏移

    QPixmap m_blurBg;
};

extern DutyWindow *g_dutyWin;  // 全局指针, 供设置窗口显隐

#endif // DUTYWINDOW_H
