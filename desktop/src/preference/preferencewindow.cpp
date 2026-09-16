#include "preferencewindow.h"

#include <QButtonGroup>
#include <QCloseEvent>
#include <QCursor>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QScreen>
#include <QScrollArea>
#include <QStackedWidget>
#include <QVBoxLayout>
#include <QGuiApplication>

#ifdef Q_OS_WIN
#  include <windows.h>
#endif

#include "preference/iconkit.h"
#include "preference/widgets/navitem.h"

PreferenceWindow *PreferenceWindow::instance()
{
    static PreferenceWindow *s = new PreferenceWindow();
    return s;
}

PreferenceWindow::PreferenceWindow()
    : QWidget(nullptr, Qt::Window | Qt::WindowMinimizeButtonHint
                          | Qt::WindowMaximizeButtonHint
                          | Qt::WindowCloseButtonHint)
{
    setObjectName(QStringLiteral("preferenceWindow"));
    setWindowTitle(QStringLiteral("偏好设置 - BongoCat"));
    resize(800, 600);
    setMinimumSize(800, 600);

    auto *root = new QHBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    root->addWidget(buildSidebar());

    m_stack = new QStackedWidget(this);
    m_stack->setObjectName(QStringLiteral("preferenceContent"));
    root->addWidget(m_stack, 1);

    // 六个占位页 (后续任务用 setPageWidget 替换)
    const QStringList placeholders = {
        QStringLiteral("猫咪设置"),   QStringLiteral("通用设置"),
        QStringLiteral("模型管理"),   QStringLiteral("快捷键"),
        QStringLiteral("任务签"),     QStringLiteral("关于"),
    };
    for (const QString &name : placeholders) {
        m_stack->addWidget(buildPlaceholderPage(
            QStringLiteral("「%1」页面即将上线～").arg(name)));
    }

    navigateTo(CatPage);

#ifdef Q_OS_WIN
    // 不显示在任务栏 (WS_EX_TOOLWINDOW), 同时保持原生标题栏
    if (HWND hwnd = reinterpret_cast<HWND>(winId())) {
        LONG_PTR ex = GetWindowLongPtrW(hwnd, GWL_EXSTYLE);
        SetWindowLongPtrW(hwnd, GWL_EXSTYLE,
                          (ex & ~WS_EX_APPWINDOW) | WS_EX_TOOLWINDOW);
    }
#endif
}

QWidget *PreferenceWindow::buildSidebar()
{
    auto *side = new QFrame(this);
    side->setObjectName(QStringLiteral("preferenceSidebar"));
    side->setFixedWidth(120);

    auto *lay = new QVBoxLayout(side);
    lay->setContentsMargins(0, 20, 0, 16);
    lay->setSpacing(8);

    // Logo: 60px 圆角方框
    auto *logoBox = new QFrame(side);
    logoBox->setFixedSize(60, 60);
    logoBox->setStyleSheet(QStringLiteral(
        "QFrame{border:1px solid rgba(139,92,246,35%);border-radius:14px;"
        "background:rgba(255,255,255,65%);}"));
    auto *logoLay = new QVBoxLayout(logoBox);
    logoLay->setContentsMargins(6, 6, 6, 6);
    auto *logo = new QLabel(logoBox);
    logo->setPixmap(IconKit::colorPixmap(QStringLiteral("logo"), QSize(48, 48)));
    logo->setAlignment(Qt::AlignCenter);
    logoLay->addWidget(logo);
    lay->addWidget(logoBox, 0, Qt::AlignHCenter);

    auto *appName = new QLabel(QStringLiteral("BongoCat"), side);
    appName->setObjectName(QStringLiteral("appNameLabel"));
    lay->addWidget(appName, 0, Qt::AlignHCenter);
    lay->addSpacing(10);

    struct NavDef { QString icon; QString label; };
    const QList<NavDef> defs = {
        {QStringLiteral("cat"), QStringLiteral("猫咪设置")},
        {QStringLiteral("settings"), QStringLiteral("通用设置")},
        {QStringLiteral("magic"), QStringLiteral("模型管理")},
        {QStringLiteral("keyboard"), QStringLiteral("快捷键")},
        {QStringLiteral("todo"), QStringLiteral("任务签")},
        {QStringLiteral("info"), QStringLiteral("关于")},
    };

    m_navGroup = new QButtonGroup(this);
    m_navGroup->setExclusive(true);
    for (int i = 0; i < defs.size(); ++i) {
        auto *item = new NavItem(defs[i].icon, defs[i].label, side);
        m_navGroup->addButton(item, i);
        m_navItems.append(item);
        lay->addWidget(item, 0, Qt::AlignHCenter);
    }
    connect(m_navGroup, &QButtonGroup::idClicked, this,
            &PreferenceWindow::navigateTo);

    lay->addStretch(1);
    return side;
}

QScrollArea *PreferenceWindow::buildPlaceholderPage(const QString &text)
{
    auto *scroll = new QScrollArea(this);
    scroll->setObjectName(QStringLiteral("preferenceContent"));
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    auto *page = new QWidget;
    page->setObjectName(QStringLiteral("preferenceContent"));
    auto *lay = new QVBoxLayout(page);
    lay->setContentsMargins(16, 16, 16, 16);
    auto *hint = new QLabel(text, page);
    hint->setAlignment(Qt::AlignCenter);
    QFont f = hint->font();
    f.setPointSize(11);
    hint->setFont(f);
    lay->addStretch(1);
    lay->addWidget(hint);
    lay->addStretch(2);

    scroll->setWidget(page);
    return scroll;
}

void PreferenceWindow::navigateTo(int page)
{
    if (page < 0 || page >= m_stack->count()) {
        return;
    }
    m_stack->setCurrentIndex(page);
    if (auto *btn = m_navGroup->button(page)) {
        btn->setChecked(true);
    }
}

void PreferenceWindow::setPageWidget(Page pageId, QWidget *widget)
{
    auto *oldScroll = qobject_cast<QScrollArea *>(m_stack->widget(pageId));
    if (!oldScroll) {
        return;
    }
    // 记录被替换页是否为当前页: removeWidget 后 current 会漂移到相邻页
    const bool wasCurrent = (m_stack->currentWidget() == oldScroll);
    auto *scroll = new QScrollArea(this);
    scroll->setObjectName(QStringLiteral("preferenceContent"));
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    auto *container = new QWidget;
    container->setObjectName(QStringLiteral("preferenceContent"));
    auto *lay = new QVBoxLayout(container);
    lay->setContentsMargins(16, 16, 16, 16);
    lay->setSpacing(0);
    lay->addWidget(widget);
    lay->addStretch(1);
    scroll->setWidget(container);

    m_stack->removeWidget(oldScroll);
    m_stack->insertWidget(pageId, scroll);
    if (wasCurrent) {
        m_stack->setCurrentIndex(pageId);
    }
    oldScroll->deleteLater();
}

void PreferenceWindow::showWindow(int page)
{
    if (page >= 0) {
        navigateTo(page);
    }
    show();
    raise();
    activateWindow();

    if (m_firstShow) {
        m_firstShow = false;
        // 首次出现时居中于当前屏幕
        if (QScreen *s = QGuiApplication::screenAt(QCursor::pos())) {
            const QRect avail = s->availableGeometry();
            move(avail.center() - QPoint(width() / 2, height() / 2));
        }
    }
}

void PreferenceWindow::closeEvent(QCloseEvent *event)
{
    hide();
    event->ignore();
}
