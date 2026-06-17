/*
 * ==========================================================================
 *  dutywindow.cpp
 *  值日表窗口
 * ==========================================================================
 */

#include "dutywindow.h"
#include "config.h"

DutyWindow *g_dutyWin = nullptr;

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QDateTime>
#include <QApplication>
#include <QScreen>
#include <QPainter>
#include <QPainterPath>   // 路径 = 可组合的几何形状 (圆角矩形等)
#include <QTimer>
#include <QMouseEvent>

#ifdef HAS_LAYER_SHELL
#include <LayerShellQt/Window>
#endif

// =========================================================================
//  构造函数
// =========================================================================

DutyWindow::DutyWindow(QWidget *parent)
    : QWidget(parent)
{
    auto flags = Qt::FramelessWindowHint | Qt::Tool;
    if (DUTY_ALWAYS_ON_TOP) flags |= Qt::WindowStaysOnTopHint;
    setWindowFlags(flags);
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_NoSystemBackground);

    setupUi();
    initPlatform();

    refresh();
    m_timer = new QTimer(this);
    connect(m_timer, &QTimer::timeout, this, &DutyWindow::refresh);
    m_timer->start(60 * 1000);  // 值日表不需要秒级刷新, 60秒一次即可
}

int DutyWindow::todayIndex()
{
    int dow = QDate::currentDate().dayOfWeek();
    if (dow >= 6) return -1;
    return dow - 1;
}

// =========================================================================
//  paintEvent — 画圆角矩形背景
// =========================================================================

#include <QImage>

// 真正的盒式模糊 (水平+垂直分离, 近似高斯)
static QImage blurImage(const QImage &src, int radius)
{
    if (src.isNull() || radius < 1) return src;
    int w = src.width(), h = src.height();
    QImage tmp(w, h, QImage::Format_ARGB32);
    QImage dst(w, h, QImage::Format_ARGB32);

    // 水平模糊
    for (int y = 0; y < h; ++y) {
        const QRgb *line = reinterpret_cast<const QRgb *>(src.constScanLine(y));
        QRgb *out = reinterpret_cast<QRgb *>(tmp.scanLine(y));
        for (int x = 0; x < w; ++x) {
            int r = 0, g = 0, b = 0, a = 0, cnt = 0;
            for (int dx = -radius; dx <= radius; ++dx) {
                int sx = qBound(0, x + dx, w - 1);
                QRgb p = line[sx];
                r += qRed(p); g += qGreen(p); b += qBlue(p); a += qAlpha(p); ++cnt;
            }
            out[x] = qRgba(r/cnt, g/cnt, b/cnt, a/cnt);
        }
    }
    // 垂直模糊
    for (int y = 0; y < h; ++y) {
        QRgb *out = reinterpret_cast<QRgb *>(dst.scanLine(y));
        for (int x = 0; x < w; ++x) {
            int r = 0, g = 0, b = 0, a = 0, cnt = 0;
            for (int dy = -radius; dy <= radius; ++dy) {
                int sy = qBound(0, y + dy, h - 1);
                QRgb p = reinterpret_cast<const QRgb *>(tmp.constScanLine(sy))[x];
                r += qRed(p); g += qGreen(p); b += qBlue(p); a += qAlpha(p); ++cnt;
            }
            out[x] = qRgba(r/cnt, g/cnt, b/cnt, a/cnt);
        }
    }
    return dst;
}

void DutyWindow::captureBlurBg()
{
    QScreen *sc = QApplication::primaryScreen();
    if (!sc) return;
    QPoint g = mapToGlobal(QPoint(0, 0));
    QRect area(g.x(), g.y(), width(), height());
    QPixmap raw = sc->grabWindow(0, area.x(), area.y(), area.width(), area.height());
    if (raw.isNull()) return;

    QImage img = raw.toImage().convertToFormat(QImage::Format_ARGB32);
    bool valid = false;
    for (int y = 0; y < img.height() && !valid; y += 8) {
        auto *line = reinterpret_cast<const QRgb *>(img.constScanLine(y));
        for (int x = 0; x < img.width() && !valid; x += 8)
            if (qRed(line[x]) > 10 || qGreen(line[x]) > 10 || qBlue(line[x]) > 10) valid = true;
    }
    if (!valid) { m_blurBg = QPixmap(); return; }
    m_blurBg = QPixmap::fromImage(blurImage(img, 20));
}

void DutyWindow::moveEvent(QMoveEvent *)  { QTimer::singleShot(120, this, [this]{ captureBlurBg(); update(); }); }
void DutyWindow::showEvent(QShowEvent *)  { QTimer::singleShot(250, this, [this]{ captureBlurBg(); update(); }); }

void DutyWindow::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    QPainterPath path;
    path.addRoundedRect(rect().adjusted(1, 1, -1, -1), 16, 16);

    // 1. 真模糊桌面背景 (Wayland 截屏可用时)
    if (!m_blurBg.isNull()) {
        painter.setClipPath(path);
        painter.drawPixmap(0, 0, m_blurBg);
        painter.setClipping(false);
    }

    // 2. 深黑底色 — 与 TopBar 一致的纯黑基调
    painter.setPen(QPen(QColor(255, 255, 255, 15), 1.0));
    QLinearGradient bg(0, 0, 0, height());
    for (int i = 0; i <= 8; ++i) {
        double t = i / 8.0;
        int a = m_blurBg.isNull() ? 220 - (int)(t * 30) : 170 - (int)(t * 20);
        bg.setColorAt(t, QColor(0, 0, 0, a));
    }
    painter.setBrush(bg);
    painter.drawPath(path);

    // 3. 顶部高光
    QPainterPath sheen;
    sheen.addRoundedRect(QRectF(2, 2, width()-4, height()/2), 14, 14);
    QLinearGradient sg(0, 2, 0, height()/2);
    sg.setColorAt(0.0, QColor(255, 255, 255, 8));
    sg.setColorAt(1.0, QColor(255, 255, 255, 0));
    painter.setPen(Qt::NoPen);
    painter.setBrush(sg);
    painter.drawPath(sheen);
}

// =========================================================================
//  initPlatform — Wayland layer-shell 或手动定位
// =========================================================================

void DutyWindow::initPlatform()
{
    setFixedSize(300, 440);

    // 可移动模式: 不使用 LayerShell, 用普通窗口定位
    if (DUTY_MOVABLE) {
        setMouseTracking(true);
        setCursor(Qt::OpenHandCursor);
        positionAtRight();
        return;
    }

#ifdef HAS_LAYER_SHELL
    if (qEnvironmentVariableIsSet("WAYLAND_DISPLAY")) {
        winId();
        if (auto *sw = LayerShellQt::Window::get(windowHandle())) {
            sw->setLayer(DUTY_ALWAYS_ON_TOP
                ? LayerShellQt::Window::LayerTop
                : LayerShellQt::Window::LayerBottom);
            using A = LayerShellQt::Window::Anchor;
            sw->setAnchors(
                LayerShellQt::Window::Anchors(A::AnchorTop) | A::AnchorRight);
            sw->setMargins(QMargins(0, 130, 30, 0));
            sw->setKeyboardInteractivity(
                LayerShellQt::Window::KeyboardInteractivityNone);
            m_usingLayerShell = true;
            return;
        }
    }
#endif
    positionAtRight();
    if (!DUTY_ALWAYS_ON_TOP)
        setWindowFlag(Qt::WindowStaysOnTopHint, false);
}

void DutyWindow::applySettings()
{
    QPoint savedPos = pos();
    bool wasVisible = isVisible();
    if (wasVisible) hide();

    // 重建窗口标志以匹配当前设置
    auto flags = Qt::FramelessWindowHint | Qt::Tool;
    if (DUTY_ALWAYS_ON_TOP) flags |= Qt::WindowStaysOnTopHint;
    setWindowFlags(flags);

    // Wayland: 重新初始化 LayerShell 或普通窗口
#ifdef HAS_LAYER_SHELL
    if (qEnvironmentVariableIsSet("WAYLAND_DISPLAY")) {
        // 销毁旧 native handle
        if (m_usingLayerShell) {
            destroy();
            m_usingLayerShell = false;
            create();
            winId();
        }

        if (!DUTY_MOVABLE) {
            // 固定模式: 使用 LayerShell 锚定右上角
            if (auto *sw = LayerShellQt::Window::get(windowHandle())) {
                sw->setLayer(DUTY_ALWAYS_ON_TOP
                    ? LayerShellQt::Window::LayerTop
                    : LayerShellQt::Window::LayerBottom);
                using A = LayerShellQt::Window::Anchor;
                sw->setAnchors(
                    LayerShellQt::Window::Anchors(A::AnchorTop) | A::AnchorRight);
                sw->setMargins(QMargins(0, 130, 30, 0));
                sw->setKeyboardInteractivity(
                    LayerShellQt::Window::KeyboardInteractivityNone);
                m_usingLayerShell = true;
            }
            setCursor(Qt::ArrowCursor);
        } else {
            setCursor(Qt::OpenHandCursor);
        }
    }
#endif

    if (!m_usingLayerShell && !DUTY_MOVABLE)
        positionAtRight();

    if (wasVisible) show();
}

void DutyWindow::mousePressEvent(QMouseEvent *e)
{
    if (!DUTY_MOVABLE || m_usingLayerShell) {
        QWidget::mousePressEvent(e);
        return;
    }
    if (e->button() == Qt::LeftButton) {
        m_dragPos = e->globalPosition().toPoint() - frameGeometry().topLeft();
        setCursor(Qt::ClosedHandCursor);
    }
}

void DutyWindow::mouseMoveEvent(QMouseEvent *e)
{
    if (!DUTY_MOVABLE || m_usingLayerShell) {
        QWidget::mouseMoveEvent(e);
        return;
    }
    if (e->buttons() & Qt::LeftButton) {
        move(e->globalPosition().toPoint() - m_dragPos);
    }
}

void DutyWindow::mouseReleaseEvent(QMouseEvent *e)
{
    if (DUTY_MOVABLE && !m_usingLayerShell)
        setCursor(Qt::OpenHandCursor);
    QWidget::mouseReleaseEvent(e);
}

// =========================================================================
//  setupUi — 构建值日表卡片
// =========================================================================

void DutyWindow::setupUi()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(20, 16, 20, 14);
    mainLayout->setSpacing(6);

    // ---- 标题: 英文星期缩写 (大) + "值日表" (小), 类似 TopBar 日期 ----
    m_titleLabel = new QLabel();
    m_titleLabel->setAlignment(Qt::AlignCenter);
    m_titleLabel->setTextFormat(Qt::RichText);
    m_titleLabel->setStyleSheet("padding: 2px;");
    mainLayout->addWidget(m_titleLabel);

    // ---- 分隔线 ----
    QWidget *divider = new QWidget();
    divider->setFixedHeight(1);
    divider->setStyleSheet("background: rgba(255,255,255,0.12);");
    mainLayout->addWidget(divider);

    // ---- 5 行值日条目: 左职责 / 右姓名, 间隔分割线 ----
    for (int i = 0; i < 5; ++i) {
        if (i > 0) {
            QWidget *line = new QWidget();
            line->setFixedHeight(1);
            line->setStyleSheet("background: rgba(255,255,255,0.08);");
            mainLayout->addWidget(line);
        }

        QWidget *row = new QWidget();
        row->setStyleSheet("background: transparent;");
        QHBoxLayout *hl = new QHBoxLayout(row);
        hl->setContentsMargins(4, 9, 4, 9);
        hl->setSpacing(16);

        m_roleLabels[i] = new QLabel(DUTY_ROLES[i]);
        m_roleLabels[i]->setStyleSheet(R"(
            font-size: 18px; color: #9898a8; font-weight: 400;
            background: transparent;
        )");

        m_nameLabels[i] = new QLabel();
        m_nameLabels[i]->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        m_nameLabels[i]->setStyleSheet(R"(
            font-size: 24px; color: #e8e8f0; font-weight: 500;
            background: transparent;
        )");

        hl->addWidget(m_roleLabels[i]);
        hl->addStretch();
        hl->addWidget(m_nameLabels[i]);

        mainLayout->addWidget(row);
    }

    mainLayout->addStretch();

    // ---- 底部日期 ----
    m_dateLabel = new QLabel();
    m_dateLabel->setAlignment(Qt::AlignCenter);
    m_dateLabel->setStyleSheet("font-size: 13px; color: #8888a0;");
    mainLayout->addWidget(m_dateLabel);
}

// =========================================================================
//  positionAtRight — 手动定位到屏幕右侧 (非 Wayland 回退)
// =========================================================================

void DutyWindow::positionAtRight()
{
    QScreen *screen = QApplication::primaryScreen();
    if (!screen) {
        move(1500, 140);  // 预设值
        return;
    }
    QRect geo = screen->geometry();
    // 右上角: x = 屏幕右边界 - 窗口宽度 - 30px 留白
    //         y = 屏幕顶部 + 130px (在顶栏下方)
    move(geo.right() - width() - 30, geo.top() + 130);
    // 用 move 而不是 setGeometry, 因为大小已经通过 setFixedSize 设好了
}

// =========================================================================
//  refresh — 更新值日内容
// =========================================================================

void DutyWindow::refresh()
{
    QDate today = QDate::currentDate();
    static const QStringList engDays = {"Mon","Tue","Wed","Thu","Fri","Sat","Sun"};

    int g = DUTY_CALENDAR.value(today, -1);

    // 标题: 大号英文缩写 + 小号"值日表"
    QString dayAbbr = engDays[today.dayOfWeek() - 1];
    if (g >= 1 && g <= 9 && g - 1 < DUTY_GROUPS.size()) {
        m_titleLabel->setText(
            QString("<span style='font-size:28px;color:#e0e0e8;font-weight:300;'>%1</span>"
                    " <span style='font-size:19px;color:#9898a8;font-weight:400;'>值日表</span>"
                    " <span style='font-size:16px;color:#8888a0;'>· %2</span>")
                .arg(dayAbbr, groupName(g)));

        const auto &names = DUTY_GROUPS[g - 1];
        for (int i = 0; i < 5; ++i) {
            m_roleLabels[i]->setText(DUTY_ROLES[i]);
            m_nameLabels[i]->setText(i < names.size() ? names[i] : "—");
            m_roleLabels[i]->setVisible(true);
            m_nameLabels[i]->setVisible(true);
        }
    } else {
        m_titleLabel->setText(
            QString("<span style='font-size:28px;color:#e0e0e8;font-weight:300;'>%1</span>"
                    " <span style='font-size:19px;color:#9898a8;font-weight:400;'>值日表</span>")
                .arg(dayAbbr));
        for (int i = 0; i < 5; ++i) {
            m_roleLabels[i]->setText(DUTY_ROLES[i]);
            m_nameLabels[i]->setText("—");
        }
    }

    m_dateLabel->setText(today.toString("yyyy年M月d日  dddd"));
}
