/*
* ==========================================================================
 *  topbarwindow.h
 *  顶栏声明
 * ==========================================================================
 */

#ifndef TOPBARWINDOW_H
#define TOPBARWINDOW_H

#include <QWidget>
#include <QLabel>
#include <QTimer>
#include <QVector>
#include <QChar>
#include <QGraphicsOpacityEffect>

// =========================================================================
//  PulseRing
// =========================================================================
class PulseRing : public QWidget {
    Q_OBJECT
public:
    explicit PulseRing(QWidget *parent = nullptr);
protected:
    void paintEvent(QPaintEvent *) override;
};

// =========================================================================
//  FlipClock
// =========================================================================
class FlipClock : public QWidget {
    Q_OBJECT
public:
    explicit FlipClock(int pixelSize = 38, QWidget *parent = nullptr);
    void setText(const QString &hhmmss, const QString &ms);
    void setPixelSize(int ps);
    int pixelSize() const { return m_pixelSize; }

protected:
    void paintEvent(QPaintEvent *) override;

private:
    struct Slot { QChar ch, oldCh; double anim = 1.0; int dur = 400;
                  bool sep = false; double x = 0, w = 0; };
    Slot m_slots[12];
    QString m_fullText;
    int m_pixelSize;
    void recalcLayout();
};

// =========================================================================
//  WobbleButton — timer 驱动悬停缩放, 无晃动
//
//  用独立 QTimer @ 8ms 推进 m_hover (0.001 → 1.0),
//  paintEvent 中 ease-out 插值: alpha 130→255, scale 1.0→1.15x。
//  m_hover 初始 0.001 而非 0, 避免第一帧"黑闪"。
// =========================================================================
class WobbleButton : public QWidget {
    Q_OBJECT
public:
    WobbleButton(int iconType, const QString &tooltip,
                 QWidget *parent = nullptr);
signals:
    void clicked();
    void hoverIn();
    void hoverOut();

protected:
protected:
    void paintEvent(QPaintEvent *) override;
    void enterEvent(QEnterEvent *) override;
    void leaveEvent(QEvent *) override;
    void mousePressEvent(QMouseEvent *) override;
    void mouseReleaseEvent(QMouseEvent *) override;
private:
    void tick();
    double m_hover  = 0.001;
    double m_target = 0.001;
    int m_iconType;
    bool m_pressed = false;
    QTimer *m_timer;
};

// =========================================================================
//  QuickButtons — 快捷按钮组, 自包含 hover-tip 逻辑
// =========================================================================
class QuickButtons : public QWidget {
    Q_OBJECT
public:
    QuickButtons(QLabel *hoverTip, QGraphicsOpacityEffect *tipOpacity,
                 QWidget *parent = nullptr);
signals:
    void settingsRequested();
    void browserRequested();
    void fileManagerRequested();
    void toggleClockRequested();
};

// =========================================================================
//  ExamWindow — 全屏考试时钟, 独立窗口, 带动画
// =========================================================================
class ExamWindow : public QWidget {
    Q_OBJECT
    Q_PROPERTY(double bgAlpha READ bgAlpha WRITE setBgAlpha)
public:
    explicit ExamWindow(QWidget *parent = nullptr);
    void animateIn(QRect clockGlobalRect);
    void animateOut();

    double bgAlpha() const { return m_bgAlpha; }
    void setBgAlpha(double a) { m_bgAlpha = a; update(); }

signals:
    void dismissed();

protected:
    void paintEvent(QPaintEvent *) override;
    void mousePressEvent(QMouseEvent *) override;

public:
    struct RiseTri {
        QPointF pts[3];
        QRectF prevRect;       // 上一帧包围盒, 用于脏区擦除
        double opacity = 0.0;
        double x, y;
        double size;
        double peakOpacity;
        double life = 0.0;
        double lifeStep;
    };

private:
    void tick();
    QVector<RiseTri> m_triangles;
    int m_spawnCd = 0;

    FlipClock *m_clock;
    PulseRing *m_ring;
    QWidget *m_content;
    QTimer *m_timer;
    QRect m_fromRect;
    QRect m_targetRect;
    double m_bgAlpha = 0.0;
};

// =========================================================================
//  TopBarWindow
// =========================================================================
class SettingsWindow;

class TopBarWindow : public QWidget {
    Q_OBJECT
public:
    explicit TopBarWindow(QWidget *parent = nullptr);
    void applySettings();   // 即时应用 独占区块 / 置顶 设置
protected:
    void paintEvent(QPaintEvent *) override;
    void closeEvent(QCloseEvent *) override;
    bool eventFilter(QObject *obj, QEvent *ev) override;

private:
    void setupUi();
    void initPlatform();
    void positionAtTop();
    void refresh();
    void enterExamMode();
    static int todayIndex();
    void findNextClasses(int &nextIdx, int &next2Idx) const;

    QLabel *m_dateLabel      = nullptr;
    QLabel *m_countdownLabel = nullptr;

    QWidget *m_centerPanel = nullptr;      // 用于点击检测
    PulseRing *m_pulseRing = nullptr;
    FlipClock *m_flipClock = nullptr;
    QuickButtons *m_quickBtns = nullptr;

    QLabel *m_nextLabel  = nullptr;
    QLabel *m_next2Label = nullptr;
    QLabel *m_schoolLabel = nullptr;
    QLabel *m_classLabel  = nullptr;
    QLabel *m_logoLabel   = nullptr;

    QLabel *m_hoverTip = nullptr;
    QGraphicsOpacityEffect *m_tipOpacity = nullptr;

    SettingsWindow *m_settings = nullptr;
    ExamWindow *m_examWindow = nullptr;
    QTimer *m_timer        = nullptr;
    bool m_usingLayerShell = false;
};

#endif // TOPBARWINDOW_H
