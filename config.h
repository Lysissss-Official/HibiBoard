#ifndef CONFIG_H
#define CONFIG_H

#include <QString>
#include <QDate>
#include <QVector>
#include <QMap>
#include <QSettings>
#include <QStringList>

// ---------------------------------------------------------------
//  班级 & 学期
// ---------------------------------------------------------------
inline QString CLASS_NAME = "高一(3)班";
inline QDate SEMESTER_END(2026, 7, 10);

// ---------------------------------------------------------------
//  每节课 — 12 行 (上午 5节+午饭+午休, 下午最多 5节)
//  含名称/开始时间/结束时间
// ---------------------------------------------------------------
struct PeriodDef { QString name, start, end; };
inline QVector<PeriodDef> PERIOD_TIMES = {
    {"早读",  "07:30","08:00"}, {"第一节","08:10","08:50"},
    {"第二节","09:00","09:40"}, {"第三节","10:05","10:50"},
    {"第四节","11:00","12:00"},
    {"午饭",  "12:00","12:20"}, {"午休",  "12:20","13:00"},
    {"第五节","13:00","13:40"}, {"第六节","13:50","14:30"},
    {"第七节","14:45","15:25"}, {"第八节","15:35","16:15"},
    {"第九节","16:25","17:05"}, {"第十节","17:15","17:55"},
};

// ---------------------------------------------------------------
//  课程表 — 科目和教师分成两个字段
//  WEEK_SUBJECT[d][p] + WEEK_TEACHER[d][p], ""=空
// ---------------------------------------------------------------
inline QVector<QStringList> WEEK_SUBJECT = {
    {"升旗仪式","物理","政治","英语","体育专项","","","数学","语文","历史","心理",""},
    {"物理/化学","化学","数学","地理","物理长课","","","体育专项","英语","信息技术","语文",""},
    {"数学","数学","语文","英语","化学长课","","","历史","体育专项","生物学","",""},
    {"语文","语文","信息技术","体育专项","数学长课","","","艺术","英语","化学","",""},
    {"英语","生物学","数学","英语","语文","","","班会","物理","政治","地理",""},
};
inline QVector<QStringList> WEEK_TEACHER = {
    {"---","刘老师","王老师","丛老师","---","","","徐老师","孙老师","张老师","张老师",""},
    {"刘/刘老师","刘老师","徐老师","吴老师","刘老师","","","---","丛老师","吕老师","孙老师",""},
    {"徐老师","徐老师","孙老师","丛老师","刘老师","","","张老师","---","郁老师","",""},
    {"孙老师","孙老师","吕老师","---","徐老师","","","宋老师","丛老师","刘老师","",""},
    {"丛老师","郁老师","徐老师","丛老师","孙老师","","","孙老师","刘老师","王老师","吴老师",""},
};

// ---------------------------------------------------------------
//  值日系统
//
//  DUTY_ROLES: 5 个固定岗位
//  DUTY_GROUPS: 9 个值日组 (1-7 班内组, 8=数竞1, 9=数竞2)
//               每组 5 个学生, 对应 5 个岗位
//  DUTY_CALENDAR: date → 组号 (1-9), 哪天哪组值日
// ---------------------------------------------------------------
inline QStringList DUTY_ROLES = {"黑板1","黑板2","扫地1","扫地2","琐项"};

inline QVector<QStringList> DUTY_GROUPS = {
    {"1A","1B","1C","1D","1E"},  // 第1组
    {"2A","2B","2C","2D","2E"},  // 第2组
    {"3A","3B","3C","3D","4E"},  // 第3组
    {"4A","4B","4C","4D","4E"},  // 第4组
    {"5A","5B","5C","5D","5E"},  // 第5组
    {"6A","6B","6C","6D","6E"},  // 第6组
    {"7A","7B","7C","7D","7E"},  // 第7组
    {"8A","8B","8C","8D","8E"},  // 数竞1 (组号8)
    {"9A","9B","9C","9D","9E"},  // 数竞2 (组号9)
};

// date → 组号 (1-9)
inline QMap<QDate, int> DUTY_CALENDAR;

// ---------------------------------------------------------------
//  顶部栏行为
// ---------------------------------------------------------------
inline bool TOPBAR_EXCLUSIVE_ZONE = true;  // 是否占据独立屏幕区块
inline bool TOPBAR_ALWAYS_ON_TOP = true;   // 是否置顶

// ---------------------------------------------------------------
//  值日栏行为
// ---------------------------------------------------------------
inline bool DUTY_VISIBLE = true;           // 值日栏是否显示
inline bool DUTY_ALWAYS_ON_TOP = true;     // 值日栏是否置顶
inline bool DUTY_MOVABLE = false;          // 值日栏是否可拖动

// ---------------------------------------------------------------
//  顶部栏快捷按钮
// ---------------------------------------------------------------
struct QuickLink { int iconType; QString tooltip; };
inline QVector<QuickLink> QUICK_LINKS = {
    {0,"设置"},{1,"浏览器"},{2,"文件管理"},{3,"时钟显示"},
};

// 工具: 组号 → 显示名称
inline QString groupName(int g) {
    if (g >= 1 && g <= 7) return QString("第%1组").arg(g);
    if (g == 8) return "数竞1组";
    if (g == 9) return "数竞2组";
    return "?";
}

// ---------------------------------------------------------------
//  配置持久化
// ---------------------------------------------------------------

inline void loadConfig()
{
    QSettings s(QSettings::IniFormat, QSettings::UserScope,
                "HibiBoard", "HibiBoard");

    CLASS_NAME  = s.value("class/name", CLASS_NAME).toString();
    SEMESTER_END = s.value("semester/end", SEMESTER_END).toDate();

    TOPBAR_EXCLUSIVE_ZONE = s.value("topbar/exclusiveZone", true).toBool();
    TOPBAR_ALWAYS_ON_TOP  = s.value("topbar/alwaysOnTop",   true).toBool();
    DUTY_VISIBLE          = s.value("duty/visible",         true).toBool();
    DUTY_ALWAYS_ON_TOP    = s.value("duty/alwaysOnTop",     true).toBool();
    DUTY_MOVABLE          = s.value("duty/movable",         false).toBool();

    // 课程时间
    int n = s.beginReadArray("periodTimes");
    if (n > 0) {
        PERIOD_TIMES.clear();
        for (int i = 0; i < n; ++i) {
            s.setArrayIndex(i);
            PERIOD_TIMES.append({
                s.value("name").toString(),
                s.value("start").toString(),
                s.value("end").toString()});
        }
    }
    s.endArray();

    // 课程科目 / 教师
    auto loadStrLists = [&](const QString &key, QVector<QStringList> &target) {
        int cnt = s.beginReadArray(key);
        if (cnt > 0) {
            target.clear();
            for (int i = 0; i < cnt; ++i) {
                s.setArrayIndex(i);
                target.append(s.value("row").toStringList());
            }
        }
        s.endArray();
    };
    loadStrLists("weekSubject", WEEK_SUBJECT);
    loadStrLists("weekTeacher", WEEK_TEACHER);

    // 值日组
    int gn = s.beginReadArray("dutyGroups");
    if (gn > 0) {
        DUTY_GROUPS.clear();
        for (int i = 0; i < gn; ++i) {
            s.setArrayIndex(i);
            DUTY_GROUPS.append(s.value("members").toStringList());
        }
    }
    s.endArray();

    // 值日日历
    int cn = s.beginReadArray("dutyCalendar");
    if (cn > 0) {
        DUTY_CALENDAR.clear();
        for (int i = 0; i < cn; ++i) {
            s.setArrayIndex(i);
            QDate d = QDate::fromString(s.value("date").toString(), Qt::ISODate);
            int g = s.value("group").toInt();
            if (d.isValid()) DUTY_CALENDAR[d] = g;
        }
    }
    s.endArray();
}

inline void saveConfig()
{
    QSettings s(QSettings::IniFormat, QSettings::UserScope,
                "HibiBoard", "HibiBoard");

    s.setValue("class/name", CLASS_NAME);
    s.setValue("semester/end", SEMESTER_END);

    s.setValue("topbar/exclusiveZone", TOPBAR_EXCLUSIVE_ZONE);
    s.setValue("topbar/alwaysOnTop",   TOPBAR_ALWAYS_ON_TOP);
    s.setValue("duty/visible",         DUTY_VISIBLE);
    s.setValue("duty/alwaysOnTop",     DUTY_ALWAYS_ON_TOP);
    s.setValue("duty/movable",         DUTY_MOVABLE);

    // 课程时间
    s.beginWriteArray("periodTimes");
    for (int i = 0; i < PERIOD_TIMES.size(); ++i) {
        s.setArrayIndex(i);
        s.setValue("name",  PERIOD_TIMES[i].name);
        s.setValue("start", PERIOD_TIMES[i].start);
        s.setValue("end",   PERIOD_TIMES[i].end);
    }
    s.endArray();

    // 课程科目 / 教师
    auto saveStrLists = [&](const QString &key, const QVector<QStringList> &src) {
        s.beginWriteArray(key);
        for (int i = 0; i < src.size(); ++i) {
            s.setArrayIndex(i);
            s.setValue("row", src[i]);
        }
        s.endArray();
    };
    saveStrLists("weekSubject", WEEK_SUBJECT);
    saveStrLists("weekTeacher", WEEK_TEACHER);

    // 值日组
    s.beginWriteArray("dutyGroups");
    for (int i = 0; i < DUTY_GROUPS.size(); ++i) {
        s.setArrayIndex(i);
        s.setValue("members", DUTY_GROUPS[i]);
    }
    s.endArray();

    // 值日日历
    s.beginWriteArray("dutyCalendar");
    int idx = 0;
    for (auto it = DUTY_CALENDAR.cbegin(); it != DUTY_CALENDAR.cend(); ++it, ++idx) {
        s.setArrayIndex(idx);
        s.setValue("date",  it.key().toString(Qt::ISODate));
        s.setValue("group", it.value());
    }
    s.endArray();
}

#endif
