#include "bongocatwidget.h"
#include <QPainter>
#include <QPaintEvent>

#ifdef Q_OS_WIN
BongoCatWidget *BongoCatWidget::s_instance = nullptr;
HHOOK BongoCatWidget::s_hook = nullptr;
#endif

BongoCatWidget::BongoCatWidget(QWidget *parent)
    : ComponentBase(parent)
{
    setupTransparentBackground();
    setAttribute(Qt::WA_TranslucentBackground);

#ifdef Q_OS_WIN
    s_instance = this;
#endif

    loadImages();
}

BongoCatWidget::~BongoCatWidget()
{
    removeHook();
#ifdef Q_OS_WIN
    if (s_instance == this)
        s_instance = nullptr;
#endif
}

void BongoCatWidget::loadImages()
{
    // 加载角色底图
    m_cover = QPixmap(":/assets/bongo/cover.png");

    // VK码 -> 按键图片名映射
    static const QMap<int, QString> keyMap = {
        // 字母键
        {0x41, "KeyA"}, {0x42, "KeyB"}, {0x43, "KeyC"}, {0x44, "KeyD"},
        {0x45, "KeyE"}, {0x46, "KeyF"}, {0x47, "KeyG"}, {0x51, "KeyQ"},
        {0x52, "KeyR"}, {0x53, "KeyS"}, {0x54, "KeyT"}, {0x56, "KeyV"},
        {0x57, "KeyW"}, {0x58, "KeyX"}, {0x5A, "KeyZ"},
        // 数字键
        {0x31, "Num1"}, {0x32, "Num2"}, {0x33, "Num3"},
        {0x34, "Num4"}, {0x35, "Num5"},
        // 特殊键
        {0x20, "Space"}, {0x0D, "Return"}, {0x10, "Shift"},
        {0x11, "Control"}, {0x12, "Alt"},
    };

    for (auto it = keyMap.begin(); it != keyMap.end(); ++it) {
        QPixmap pix(QString(":/assets/bongo/keys/%1.png").arg(it.value()));
        if (!pix.isNull()) {
            m_keyPixmaps[it.key()] = pix;
        }
    }
}

void BongoCatWidget::onShow()
{
    installHook();
}

void BongoCatWidget::onHide()
{
    removeHook();
    m_pressedKeys.clear();
}

void BongoCatWidget::installHook()
{
#ifdef Q_OS_WIN
    if (!s_hook && s_instance) {
        s_hook = SetWindowsHookEx(WH_KEYBOARD_LL, hookProc,
                                   GetModuleHandle(nullptr), 0);
    }
#endif
}

void BongoCatWidget::removeHook()
{
#ifdef Q_OS_WIN
    if (s_hook) {
        UnhookWindowsHookEx(s_hook);
        s_hook = nullptr;
    }
#endif
}

#ifdef Q_OS_WIN
LRESULT CALLBACK BongoCatWidget::hookProc(int nCode, WPARAM wParam, LPARAM lParam)
{
    if (nCode >= 0 && s_instance) {
        KBDLLHOOKSTRUCT *kb = reinterpret_cast<KBDLLHOOKSTRUCT *>(lParam);
        int vk = static_cast<int>(kb->vkCode);

        bool isDown = (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN);
        bool isUp = (wParam == WM_KEYUP || wParam == WM_SYSKEYUP);

        if (s_instance->m_keyPixmaps.contains(vk)) {
            if (isDown) {
                s_instance->m_pressedKeys.insert(vk);
                s_instance->update();
            } else if (isUp) {
                s_instance->m_pressedKeys.remove(vk);
                s_instance->update();
            }
        }
    }
    return CallNextHookEx(s_hook, nCode, wParam, lParam);
}
#endif

void BongoCatWidget::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);
    painter.setRenderHint(QPainter::Antialiasing);

    QRect targetRect = rect();

    if (m_cover.isNull())
        return;

    // 等比缩放底图
    QSize scaledSize = m_cover.size().scaled(targetRect.size(),
                                               Qt::KeepAspectRatio);
    QRect drawRect(
        (targetRect.width() - scaledSize.width()) / 2,
        (targetRect.height() - scaledSize.height()) / 2,
        scaledSize.width(),
        scaledSize.height()
    );

    // 绘制角色底图
    painter.drawPixmap(drawRect, m_cover);

    // 叠加按下的按键图片 (与底图同尺寸)
    for (int vk : m_pressedKeys) {
        auto it = m_keyPixmaps.find(vk);
        if (it != m_keyPixmaps.end() && !it.value().isNull()) {
            painter.drawPixmap(drawRect, it.value());
        }
    }
}
