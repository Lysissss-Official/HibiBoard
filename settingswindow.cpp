/*
* ==========================================================================
 *  settingswindow.cpp
 *  设置窗口
 * ==========================================================================
 */

#include "settingswindow.h"
#include "topbarwindow.h"
#include "dutywindow.h"
#include "config.h"

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QScrollArea>
#include <QLabel>
#include <QPainter>
#include <QPainterPath>
#include <QApplication>
#include <QScreen>
#include <QPushButton>
#include <QtMath>
#include <QCalendarWidget>
#include <QDesktopServices>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QPainter>

SettingsWindow::SettingsWindow(TopBarWindow *topbar, QWidget *parent)
    : QWidget(parent), m_topbar(topbar)
{
    setWindowFlags(Qt::FramelessWindowHint | Qt::Tool | Qt::WindowStaysOnTopHint);
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_NoSystemBackground);
    setFixedSize(720, 520);

    buildUi();

    m_animTimer = new QTimer(this);
    connect(m_animTimer, &QTimer::timeout, this, &SettingsWindow::tick);
    setVisible(false);
}

void SettingsWindow::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    double s = 0.7 + 0.3 * m_progress;
    p.setOpacity(m_progress);
    QPointF c = rect().center();
    p.translate(c); p.scale(s, s); p.translate(-c);

    // 背景: 与 TopBar 一致的深黑渐变
    QPainterPath bg;
    bg.addRoundedRect(rect().adjusted(1,1,-1,-1), 18, 18);
    QLinearGradient grad(0, 0, 0, height());
    for (int i = 0; i <= 8; ++i) {
        double t = i / 8.0;
        grad.setColorAt(t, QColor(8, 8, 22, 248 - (int)(t * 30)));
    }
    p.setBrush(grad);
    p.setPen(QPen(QColor(255,255,255,18), 1.0));
    p.drawPath(bg);

    // 顶部高光
    QPainterPath sh;
    sh.addRoundedRect(QRectF(2,2,width()-4,height()/2), 16,16);
    QLinearGradient gl(0,2,0,height()/2);
    gl.setColorAt(0,QColor(255,255,255,8)); gl.setColorAt(1,QColor(255,255,255,0));
    p.setPen(Qt::NoPen); p.setBrush(gl); p.drawPath(sh);
}

void SettingsWindow::tick()
{
    double step = 8.0/200.0;
    if (m_target > m_progress) {
        m_progress = qMin(m_progress+step, m_target);
        if (m_progress >= 1.0) m_animTimer->stop();
    } else {
        m_progress = qMax(m_progress-step, m_target);
        if (m_progress <= 0.0) { m_animTimer->stop(); hide(); deleteLater(); return; }
    }
    update();
}

void SettingsWindow::popIn()
{
    QScreen *sc = QApplication::primaryScreen();
    if (sc) { QRect g = sc->geometry(); move((g.width()-width())/2, (g.height()-height())/2); }
    m_target = 1.0; show(); raise();
    if (!m_animTimer->isActive()) m_animTimer->start(8);
}

void SettingsWindow::popOut()
{
    saveConfig();
    m_target = 0.0;
    if (!m_animTimer->isActive()) m_animTimer->start(8);
}

void SettingsWindow::setActiveNav(int idx)
{
    m_activeNav = idx;
    for (int i = 0; i < m_navBtns.size(); ++i) m_navBtns[i]->setChecked(i == idx);
    rebuildPage(idx);
    m_stack->setCurrentIndex(idx);
}

// 销毁旧页面内所有子控件 + 布局, 但不销毁 page 本身 (保持 stack index 不变)
static void clearPage(QWidget *page)
{
    // 递归删除所有子控件
    const auto kids = page->findChildren<QWidget *>(Qt::FindDirectChildrenOnly);
    for (auto *w : kids) { w->setParent(nullptr); delete w; }
    // 删除布局
    delete page->layout();
}

void SettingsWindow::rebuildPage(int idx)
{
    QWidget *page = m_stack->widget(idx);
    clearPage(page);
    buildContent(idx);
}

void SettingsWindow::buildContent(int idx)
{
    switch (idx) {
    case 0: buildSchedulePage(); break;
    case 1: buildGroupPage(); break;
    case 2:
        m_calYear  = QDate::currentDate().year();
        m_calMonth = QDate::currentDate().month();
        refreshCalendar();
        break;
    case 3: buildSemesterPage(); break;
    case 4: buildOtherPage(); break;
    case 5: buildAboutPage(); break;
    }
}

void SettingsWindow::buildUi()
{
    QHBoxLayout *ml = new QHBoxLayout(this);
    ml->setContentsMargins(16,16,16,16); ml->setSpacing(12);

    // ---- 左侧导航 ----
    QWidget *nav = new QWidget(); nav->setFixedWidth(120);
    QVBoxLayout *nl = new QVBoxLayout(nav); nl->setContentsMargins(8,8,8,8); nl->setSpacing(6);

    QLabel *tt = new QLabel("HibiBoard"); tt->setAlignment(Qt::AlignCenter);
    tt->setStyleSheet("font-size: 18px; color: #e0e0e8; font-weight: 300; padding: 8px;"
                      "border-bottom: 1px solid rgba(255,255,255,0.08); margin-bottom: 4px;");
    nl->addWidget(tt);

    QStringList items = {"课程表","值日组","值日日历","学期设置","其他","关于","退出程序"};
    for (int i = 0; i < items.size(); ++i) {
        // "退出" 用警示色
        bool isQuit = (i == 6);
        auto *b = new QPushButton(items[i]); b->setCheckable(!isQuit); b->setCursor(Qt::PointingHandCursor);
        b->setStyleSheet(QString(R"(
            QPushButton{text-align:left;padding:10px 14px;font-size:15px;color:%1;
                background:transparent;border:none;border-radius:10px;}
            QPushButton:hover{color:%2;background:rgba(255,255,255,0.06);}
            QPushButton:checked{color:#e0e0e8;background:rgba(255,255,255,0.08);font-weight:bold;}
        )").arg(isQuit ? "#d04545" : "#9898a8",
                isQuit ? "#ff6060" : "#e0e0e0"));
        connect(b, &QPushButton::clicked, this, [this,i](){
            if (i == 6) { saveCurrent(); saveConfig(); popOut(); qApp->quit(); }
            else { saveCurrent(); setActiveNav(i); }
        });
        nl->addWidget(b); m_navBtns.append(b);
    }
    nl->addStretch();
    ml->addWidget(nav);

    // ---- 右侧 Stack ----
    m_stack = new QStackedWidget();
    m_stack->setStyleSheet("background: rgba(0,0,0,0.25); border-radius: 12px;");
    m_schedulePage = new QWidget();
    m_groupPage    = new QWidget();
    m_calendarPage = new QWidget();
    m_semesterPage = new QWidget();
    m_otherPage    = new QWidget();
    m_aboutPage    = new QWidget();
    m_stack->addWidget(m_schedulePage);
    m_stack->addWidget(m_groupPage);
    m_stack->addWidget(m_calendarPage);
    m_stack->addWidget(m_semesterPage);
    m_stack->addWidget(m_otherPage);
    m_stack->addWidget(m_aboutPage);
    buildSemesterPage();
    ml->addWidget(m_stack, 1);
    setActiveNav(0);

    // 右上角 X 关闭按钮
    auto *closeBtn = new QPushButton("✕", this);
    closeBtn->setCursor(Qt::PointingHandCursor);
    closeBtn->setFixedSize(32, 32);
    closeBtn->move(width() - 42, 10);
    closeBtn->setStyleSheet(R"(
        QPushButton{font-size:16px;color:#8888a0;background:transparent;border:none;border-radius:8px;}
        QPushButton:hover{color:#e0e0e8;background:rgba(255,255,255,0.08);}
    )");
    connect(closeBtn, &QPushButton::clicked, this, [this](){ popOut(); });
}

// ==================== 课程表 ====================
void SettingsWindow::buildSchedulePage()
{
    auto *out = new QVBoxLayout(m_schedulePage); out->setContentsMargins(12,10,12,10);

    auto *sc = new QScrollArea(); sc->setWidgetResizable(true);
    sc->setStyleSheet("QScrollArea{border:none;background:transparent;}QScrollBar:vertical{width:6px;background:transparent;}QScrollBar::handle:vertical{background:rgba(255,255,255,0.15);border-radius:3px;}");
    auto *ct = new QWidget(); ct->setStyleSheet("background:transparent;");
    auto *cl = new QVBoxLayout(ct); cl->setSpacing(6);

    // ---- 课程时间 (12行, 名称+开始+结束) ----
    QLabel *timeTitle = new QLabel("课程时间");
    timeTitle->setStyleSheet("font-size:16px;color:#ccc;font-weight:bold;");
    cl->addWidget(timeTitle);

    // 表头
    auto *hdr = new QWidget(); auto *hl = new QHBoxLayout(hdr); hl->setContentsMargins(0,0,0,0); hl->setSpacing(6);
    for (auto *s : {new QLabel("名称"), new QLabel("开始"), new QLabel("结束")}) {
        s->setStyleSheet("font-size:12px;color:#777;"); s->setFixedWidth(s->text()=="名称"?70:55);
        hl->addWidget(s);
    }
    hl->addStretch(); cl->addWidget(hdr);

    auto sty = "QLineEdit{font-size:13px;color:#ddd;padding:3px 6px;background:rgba(255,255,255,0.04);border:1px solid rgba(255,255,255,0.08);border-radius:5px;}QLineEdit:focus{border-color:rgba(255,255,255,0.25);}";
    for (int p=0; p<PERIOD_TIMES.size(); ++p) {
        auto *rw = new QWidget(); auto *rl = new QHBoxLayout(rw); rl->setContentsMargins(0,0,0,0); rl->setSpacing(6);
        auto *nm = new QLineEdit(PERIOD_TIMES[p].name);  nm->setFixedWidth(70);
        auto *st = new QLineEdit(PERIOD_TIMES[p].start); st->setFixedWidth(55); st->setInputMask("00:00");
        auto *ed = new QLineEdit(PERIOD_TIMES[p].end);   ed->setFixedWidth(55); ed->setInputMask("00:00");
        for (auto *e : {nm,st,ed}) e->setStyleSheet(sty);
        connect(nm,&QLineEdit::textChanged,[p,nm](){PERIOD_TIMES[p].name=nm->text();});
        connect(st,&QLineEdit::textChanged,[p,st](){PERIOD_TIMES[p].start=st->text();});
        connect(ed,&QLineEdit::textChanged,[p,ed](){PERIOD_TIMES[p].end=ed->text();});
        rl->addWidget(nm); rl->addWidget(st); rl->addWidget(ed); rl->addStretch();
        cl->addWidget(rw);
    }

    // 分隔
    auto *sep = new QWidget(); sep->setFixedHeight(1);
    sep->setStyleSheet("background:rgba(255,255,255,0.08);");
    cl->addWidget(sep);

    // ---- 课程安排 (周一~周五, 每行一个时段) ----
    QLabel *schedTitle = new QLabel("课程安排");
    schedTitle->setStyleSheet("font-size:16px;color:#ccc;font-weight:bold;margin-top:6px;");
    cl->addWidget(schedTitle);

    // 表头
    auto *hdr2 = new QWidget(); auto *hl2 = new QHBoxLayout(hdr2); hl2->setContentsMargins(0,0,0,0); hl2->setSpacing(8);
    QLabel *lbBlank = new QLabel(""); lbBlank->setFixedWidth(110);
    hl2->addWidget(lbBlank);
    for (auto *s : {new QLabel("科目"), new QLabel("教师")}) {
        s->setStyleSheet("font-size:12px;color:#777;"); s->setFixedWidth(80);
        hl2->addWidget(s);
    }
    hl2->addStretch(); cl->addWidget(hdr2);

    static const char *dn[] = {"周一","周二","周三","周四","周五"};
    auto sty2 = "QLineEdit{font-size:13px;color:#ddd;padding:3px 6px;background:rgba(255,255,255,0.04);border:1px solid rgba(255,255,255,0.08);border-radius:5px;}QLineEdit:focus{border-color:rgba(255,255,255,0.25);}";
    for (int d=0; d<5; ++d) {
        QLabel *day = new QLabel(dn[d]);
        day->setStyleSheet("font-size:15px;color:#ddd;font-weight:bold;margin-top:4px;");
        cl->addWidget(day);

        for (int p=0; p<PERIOD_TIMES.size(); ++p) {
            auto *rw = new QWidget(); auto *rl = new QHBoxLayout(rw); rl->setContentsMargins(0,0,0,0); rl->setSpacing(8);
            QLabel *lb = new QLabel(PERIOD_TIMES[p].name + "  " + PERIOD_TIMES[p].start);
            lb->setFixedWidth(110); lb->setStyleSheet("font-size:12px;color:#888;");
            rl->addWidget(lb);

            auto *sub = new QLineEdit(); sub->setFixedWidth(80); sub->setStyleSheet(sty2);
            if (d<WEEK_SUBJECT.size() && p<WEEK_SUBJECT[d].size()) sub->setText(WEEK_SUBJECT[d][p]);
            connect(sub,&QLineEdit::textChanged,[d,p,sub](){if(d<WEEK_SUBJECT.size()&&p<WEEK_SUBJECT[d].size())WEEK_SUBJECT[d][p]=sub->text();});

            auto *tch = new QLineEdit(); tch->setFixedWidth(80); tch->setStyleSheet(sty2);
            if (d<WEEK_TEACHER.size() && p<WEEK_TEACHER[d].size()) tch->setText(WEEK_TEACHER[d][p]);
            connect(tch,&QLineEdit::textChanged,[d,p,tch](){if(d<WEEK_TEACHER.size()&&p<WEEK_TEACHER[d].size())WEEK_TEACHER[d][p]=tch->text();});

            rl->addWidget(sub); rl->addWidget(tch); rl->addStretch();
            cl->addWidget(rw);
        }
    }
    cl->addStretch(); sc->setWidget(ct); out->addWidget(sc);
}

// ==================== 值日组编辑 ====================
void SettingsWindow::buildGroupPage()
{
    auto *out = new QVBoxLayout(m_groupPage); out->setContentsMargins(12,10,12,10);

    auto *sc = new QScrollArea(); sc->setWidgetResizable(true);
    sc->setStyleSheet("QScrollArea{border:none;background:transparent;}QScrollBar:vertical{width:6px;background:transparent;}QScrollBar::handle:vertical{background:rgba(255,255,255,0.15);border-radius:3px;}");
    auto *ct = new QWidget(); ct->setStyleSheet("background:transparent;");
    auto *cl = new QVBoxLayout(ct); cl->setSpacing(8);

    for (int g=0; g<9; ++g) {
        QLabel *gl = new QLabel(groupName(g+1));
        gl->setStyleSheet("font-size: 16px; color: #ccc; font-weight: bold;");
        cl->addWidget(gl);

        for (int r=0; r<5; ++r) {
            auto *rw = new QWidget(); auto *rl = new QHBoxLayout(rw); rl->setContentsMargins(0,0,0,0);
            QLabel *lb = new QLabel(DUTY_ROLES[r]); lb->setFixedWidth(60);
            lb->setStyleSheet("font-size:13px;color:#999;");
            rl->addWidget(lb);
            auto *e = new QLineEdit();
            e->setStyleSheet("QLineEdit{font-size:13px;color:#ddd;padding:4px 8px;background:rgba(255,255,255,0.04);border:1px solid rgba(255,255,255,0.08);border-radius:5px;}QLineEdit:focus{border-color:rgba(255,255,255,0.25);}");
            if (g<DUTY_GROUPS.size() && r<DUTY_GROUPS[g].size()) e->setText(DUTY_GROUPS[g][r]);
            connect(e,&QLineEdit::textChanged,this,[g,r,e](){if(g<DUTY_GROUPS.size()&&r<DUTY_GROUPS[g].size())DUTY_GROUPS[g][r]=e->text();});
            rl->addWidget(e); cl->addWidget(rw);
        }
    }
    cl->addStretch(); sc->setWidget(ct); out->addWidget(sc);
}

// ==================== 值日月历 ====================
void SettingsWindow::refreshCalendar()
{
    clearPage(m_calendarPage);
    auto *out = new QVBoxLayout(m_calendarPage); out->setContentsMargins(12,10,12,10);

    // 月导航
    QWidget *nav = new QWidget(); nav->setStyleSheet("background:transparent;");
    QHBoxLayout *hl = new QHBoxLayout(nav); hl->setContentsMargins(0,0,0,0);
    auto *prev = new QPushButton("◀"); auto *next = new QPushButton("▶");
    for (auto *b : {prev,next}) b->setStyleSheet("QPushButton{font-size:20px;color:#ccc;background:transparent;border:none;padding:6px 12px;}QPushButton:hover{color:#fff;}");
    m_calTitle = new QLabel(); m_calTitle->setAlignment(Qt::AlignCenter);
    m_calTitle->setStyleSheet("font-size:18px;color:#e0e0e0;font-weight:bold;");
    hl->addWidget(prev); hl->addWidget(m_calTitle,1); hl->addWidget(next);
    out->addWidget(nav);

    connect(prev,&QPushButton::clicked,this,[this](){if(--m_calMonth<1){m_calMonth=12;--m_calYear;}refreshCalendar();});
    connect(next,&QPushButton::clicked,this,[this](){if(++m_calMonth>12){m_calMonth=1;++m_calYear;}refreshCalendar();});
    m_calTitle->setText(QString("%1年 %2月").arg(m_calYear).arg(m_calMonth));

    // 星期头
    QWidget *header = new QWidget(); header->setStyleSheet("background:transparent;");
    QHBoxLayout *hl2 = new QHBoxLayout(header); hl2->setContentsMargins(0,0,0,0); hl2->setSpacing(2);
    QStringList dow = {"日","一","二","三","四","五","六"};
    for (const auto &d : dow) {
        QLabel *l = new QLabel(d); l->setAlignment(Qt::AlignCenter);
        l->setFixedHeight(26);
        l->setStyleSheet("font-size:13px;color:#888;");
        hl2->addWidget(l);
    }
    out->addWidget(header);

    // 42 格日历
    QDate first(m_calYear, m_calMonth, 1);
    int startDow = first.dayOfWeek() % 7;  // Mon=1→1, Sun=7→0

    QGridLayout *grid = new QGridLayout(); grid->setSpacing(2);
    QDate today = QDate::currentDate();

    for (int cell=0; cell<42; ++cell) {
        int row = cell / 7, col = cell % 7;
        int day = cell - startDow + 1;
        QDate date(m_calYear, m_calMonth, day);
        bool valid = (date.month() == m_calMonth);

        auto *btn = new QPushButton();
        btn->setFixedHeight(42);
        btn->setCursor(valid ? Qt::PointingHandCursor : Qt::ArrowCursor);

        if (valid) {
            int g = DUTY_CALENDAR.value(date, 0);
            QString t = QString("%1").arg(day);
            if (g > 0) t += QString("\n%1").arg(groupName(g));
            btn->setText(t);

            // 今天高亮
            QString extra = (date == today) ? "border:2px solid rgba(255,200,100,0.6);" : "";
            btn->setStyleSheet(QString(
                "QPushButton{font-size:12px;color:#ddd;background:rgba(255,255,255,0.04);"
                "border:1px solid rgba(255,255,255,0.06);border-radius:6px;%1}"
                "QPushButton:hover{background:rgba(255,255,255,0.12);}").arg(extra));

            connect(btn,&QPushButton::clicked,this,[this,date,btn](){
                int cur = DUTY_CALENDAR.value(date, 0);
                cur = (cur + 1) % 10;  // cycle 0-9, 0=无组
                DUTY_CALENDAR[date] = cur;
                if (cur == 0)
                    btn->setText(QString::number(date.day()));
                else
                    btn->setText(QString("%1\n%2").arg(date.day()).arg(groupName(cur)));
            });
        } else {
            btn->setStyleSheet("QPushButton{font-size:12px;color:#444;background:transparent;border:none;}");
        }
        grid->addWidget(btn, row, col);
    }
    out->addLayout(grid);
    out->addStretch();
}

// ==================== 学期设置 ====================
void SettingsWindow::buildSemesterPage()
{
    auto *out = new QVBoxLayout(m_semesterPage); out->setContentsMargins(20,20,20,20); out->setSpacing(16);

    auto mk = [](const QString &l){
        auto *w=new QWidget();auto *ll=new QHBoxLayout(w);ll->setContentsMargins(0,0,0,0);
        auto *lb=new QLabel(l);lb->setFixedWidth(100);lb->setStyleSheet("font-size:16px;color:#ccc;");
        ll->addWidget(lb);return std::make_pair(w,ll);
    };
    {
        auto[r,l]=mk("班级名称");
        m_classEdit=new QLineEdit(CLASS_NAME);
        m_classEdit->setStyleSheet("QLineEdit{font-size:16px;color:#ddd;padding:8px 12px;background:rgba(255,255,255,0.04);border:1px solid rgba(255,255,255,0.08);border-radius:8px;}QLineEdit:focus{border-color:rgba(255,255,255,0.25);}");
        connect(m_classEdit,&QLineEdit::textChanged,this,[](const QString&t){CLASS_NAME=t;});
        l->addWidget(m_classEdit);out->addWidget(r);
    }
    {
        auto[r,l]=mk("学期结束");
        m_semesterEdit=new QDateEdit(SEMESTER_END); m_semesterEdit->setCalendarPopup(true);
        m_semesterEdit->setStyleSheet("QDateEdit{font-size:16px;color:#ddd;padding:8px 12px;background:rgba(255,255,255,0.04);border:1px solid rgba(255,255,255,0.08);border-radius:8px;}QDateEdit:focus{border-color:rgba(255,255,255,0.25);}QDateEdit::drop-down{subcontrol-origin:padding;subcontrol-position:top right;width:24px;border-left:1px solid rgba(255,255,255,0.1);}");
        connect(m_semesterEdit,&QDateEdit::dateChanged,this,[](const QDate&d){SEMESTER_END=d;});
        l->addWidget(m_semesterEdit);out->addWidget(r);
    }
    out->addStretch();
}

// ==================== 其他设置 ====================
void SettingsWindow::buildOtherPage()
{
    auto *out = new QVBoxLayout(m_otherPage); out->setContentsMargins(0,0,0,0);

    auto *sc = new QScrollArea(); sc->setWidgetResizable(true);
    sc->setStyleSheet("QScrollArea{border:none;background:transparent;}QScrollBar:vertical{width:6px;background:transparent;}QScrollBar::handle:vertical{background:rgba(255,255,255,0.15);border-radius:3px;}");
    auto *ct = new QWidget(); ct->setStyleSheet("background:transparent;");
    auto *in = new QVBoxLayout(ct); in->setContentsMargins(20,20,20,20); in->setSpacing(14);
    out->addWidget(sc);

    auto mkToggle = [&](const QString &title, const QString &desc, bool initial) {
        auto *cb = new QCheckBox(title);
        cb->setChecked(initial);
        cb->setStyleSheet(R"(
            QCheckBox{font-size:16px;color:#ddd;spacing:10px;}
            QCheckBox::indicator{width:44px;height:24px;border-radius:12px;
                border:2px solid rgba(255,255,255,0.3);background:rgba(255,255,255,0.06);}
            QCheckBox::indicator:checked{background:rgba(120,180,255,0.5);
                border-color:rgba(120,180,255,0.7);}
        )");
        auto *lb = new QLabel(desc);
        lb->setStyleSheet("font-size:12px;color:#888;");
        lb->setWordWrap(true);
        in->addWidget(cb);
        in->addWidget(lb);
        return cb;
    };

    in->addWidget(new QLabel("顶部栏行为"));  // will be restyled below
    if (auto *l = qobject_cast<QLabel *>(in->itemAt(in->count()-1)->widget()))
        l->setStyleSheet("font-size:18px;color:#e0e0e8;font-weight:bold;");

    m_exclusiveZoneCb = mkToggle("占据独立屏幕区块",
        "开启后顶部栏独占屏幕顶部 70px, 其他窗口自动避让。\n"
        "关闭后顶部栏悬浮于其他窗口之上, 不占用布局空间。\n"
        "（重启程序后生效）",
        TOPBAR_EXCLUSIVE_ZONE);
    connect(m_exclusiveZoneCb, &QCheckBox::toggled, this, [this](bool v) {
        TOPBAR_EXCLUSIVE_ZONE = v;
        if (m_topbar) m_topbar->applySettings();
    });

    m_alwaysOnTopCb = mkToggle("窗口置顶",
        "开启后顶部栏始终位于所有窗口最上层。\n"
        "关闭后其他窗口可覆盖顶部栏。\n"
        "（重启程序后生效）",
        TOPBAR_ALWAYS_ON_TOP);
    connect(m_alwaysOnTopCb, &QCheckBox::toggled, this, [this](bool v) {
        TOPBAR_ALWAYS_ON_TOP = v;
        if (m_topbar) m_topbar->applySettings();
    });

    // ---- 值日栏行为 ----
    auto *dutyTitle = new QLabel("值日栏行为");
    dutyTitle->setStyleSheet("font-size:18px;color:#e0e0e8;font-weight:bold;margin-top:8px;");
    in->addWidget(dutyTitle);

    auto *dutyTopCb = mkToggle("值日栏置顶",
        "开启后值日栏始终位于窗口最上层。\n"
        "（重启程序后生效）",
        DUTY_ALWAYS_ON_TOP);
    connect(dutyTopCb, &QCheckBox::toggled, [](bool v) {
        DUTY_ALWAYS_ON_TOP = v;
    });

    auto *dutyMovCb = mkToggle("允许拖动值日栏",
        "开启后可用鼠标拖拽值日栏到任意位置。\n"
        "关闭后值日栏固定在屏幕右上角。\n"
        "（重启程序后生效）",
        DUTY_MOVABLE);
    connect(dutyMovCb, &QCheckBox::toggled, [](bool v) {
        DUTY_MOVABLE = v;
    });

    auto *dutyVisCb = mkToggle("显示值日栏",
        "关闭后隐藏值日栏窗口。",
        DUTY_VISIBLE);
    connect(dutyVisCb, &QCheckBox::toggled, [](bool v) {
        DUTY_VISIBLE = v;
        if (g_dutyWin) g_dutyWin->setVisible(v);
    });

    in->addStretch();
    sc->setWidget(ct);
}

// ==================== 关于 ====================
void SettingsWindow::buildAboutPage()
{
    auto *out = new QVBoxLayout(m_aboutPage);
    out->setContentsMargins(40, 30, 40, 30);
    out->setSpacing(10);

    // 标题
    auto *name = new QLabel("HibiBoard");
    name->setAlignment(Qt::AlignCenter);
    name->setStyleSheet("font-size: 28px; color: #e0e0e8; font-weight: 200;");

    auto *ver = new QLabel("v0.1.0b");
    ver->setAlignment(Qt::AlignCenter);
    ver->setStyleSheet("font-size: 14px; color: #9898a8;");

    auto *desc = new QLabel("校园桌面组件栏 | 课程表 | 值日表 | 倒计时 | 考试模式");
    desc->setAlignment(Qt::AlignCenter);
    desc->setWordWrap(true);
    desc->setStyleSheet("font-size: 14px; color: #8888a0; margin-top: 4px;");

    // 分隔线
    auto *sep = new QWidget();
    sep->setFixedHeight(1);
    sep->setStyleSheet("background: rgba(255,255,255,0.08);");

    // GitHub 行: 链接 + 头像
    auto *ghRow = new QWidget();
    auto *ghLayout = new QHBoxLayout(ghRow);
    ghLayout->setContentsMargins(0, 0, 0, 0);
    ghLayout->setSpacing(10);

    auto *github = new QLabel(
        "<a href='https://github.com/Lysissss-Official' "
        "style='color:#7ec8e3;text-decoration:none;'>"
        "@Lysissss-Official</a>");
    github->setTextFormat(Qt::RichText);
    github->setOpenExternalLinks(true);
    github->setStyleSheet("font-size: 14px;");

    // 头像 — 从 GitHub 加载, 失败则显示圆形字母占位
    auto *avatar = new QLabel();
    avatar->setFixedSize(28, 28);
    // 占位图标 (加载期间或失败时显示)
    auto setPlaceholder = [avatar]() {
        QPixmap av(28, 28);
        av.fill(Qt::transparent);
        QPainter p(&av);
        p.setRenderHint(QPainter::Antialiasing);
        p.setBrush(QColor(55, 65, 85));
        p.setPen(Qt::NoPen);
        p.drawEllipse(0, 0, 28, 28);
        QFont f;
        f.setPixelSize(13);
        f.setWeight(QFont::DemiBold);
        p.setFont(f);
        p.setPen(QColor(210, 215, 230));
        p.drawText(QRect(0, 0, 28, 28), Qt::AlignCenter, "L");
        avatar->setPixmap(av);
    };
    setPlaceholder();

    // 异步加载 GitHub 头像
    auto *mgr = new QNetworkAccessManager(avatar);
    QUrl url("https://github.com/Lysissss-Official.png");
    auto *reply = mgr->get(QNetworkRequest(url));
    QObject::connect(reply, &QNetworkReply::finished, avatar, [avatar, reply]() {
        if (reply->error() == QNetworkReply::NoError) {
            QPixmap pm;
            if (pm.loadFromData(reply->readAll())) {
                // 圆形裁剪
                QPixmap round(28, 28);
                round.fill(Qt::transparent);
                QPainter pr(&round);
                pr.setRenderHint(QPainter::Antialiasing);
                QPainterPath path;
                path.addEllipse(0, 0, 28, 28);
                pr.setClipPath(path);
                pr.drawPixmap(0, 0, 28, 28,
                    pm.scaled(28, 28, Qt::KeepAspectRatioByExpanding,
                              Qt::SmoothTransformation));
                avatar->setPixmap(round);
            }
        }
        reply->deleteLater();
    });

    ghLayout->addStretch();
    ghLayout->addWidget(github);
    ghLayout->addWidget(avatar);
    ghLayout->addStretch();

    auto *tech = new QLabel("C++ · Qt6 · Windows · Linux");
    tech->setAlignment(Qt::AlignCenter);
    tech->setStyleSheet("font-size: 12px; color: #707088;");

    auto *copy = new QLabel("© 2026  Lysissss-Official");
    copy->setAlignment(Qt::AlignCenter);
    copy->setStyleSheet("font-size: 12px; color: #606078; margin-top: 8px;");

    out->addStretch();
    out->addWidget(name);
    out->addWidget(ver);
    out->addWidget(desc);
    out->addSpacing(6);
    out->addWidget(sep);
    out->addSpacing(6);
    out->addWidget(ghRow);
    out->addWidget(tech);
    out->addWidget(copy);
    out->addStretch();
}

void SettingsWindow::saveCurrent() {}
