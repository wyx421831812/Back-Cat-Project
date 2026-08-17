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
    m_cover = QPixmap(":/assets/bongo/cover.png");

    // 按键 vkCode -> 名称映射
    static const QMap<int, QString> keyMap = {
        {0x41, "KeyA"}, {0x42, "KeyB"}, {0x43, "KeyC"}, {0x44, "KeyD"},
        {0x45, "KeyE"}, {0x46, "KeyF"}, {0x47, "KeyG"}, {0x51, "KeyQ"},
        {0x52, "KeyR"}, {0x53, "KeyS"}, {0x54, "KeyT"}, {0x56, "KeyV"},
        {0x57, "KeyW"}, {0x58, "KeyX"}, {0x5A, "KeyZ"},
        {0x31, "Num1"}, {0x32, "Num2"}, {0x33, "Num3"},
        {0x34, "Num4"}, {0x35, "Num5"},
        {0x20, "Space"}, {0x0D, "Return"}, {0x10, "Shift"},
        {0x11, "Control"}, {0x12, "Alt"},
    };

    // 按键左右手分类表，参照原版 BongoCat 的 left-keys / right-keys 目录划分
    // 左手区：键盘左半边的字母 + 修饰键 + 空格（由左手按）
    static const QSet<int> leftHandKeys = {
        0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47,  // A-G
        0x51, 0x52, 0x53, 0x54, 0x56, 0x57, 0x58, 0x5A,  // Q R S T V W X Z
        0x10, 0x11, 0x12, 0x20                       // Shift Control Alt Space
    };
    // 右手区：数字键 + 回车
    static const QSet<int> rightHandKeys = {
        0x31, 0x32, 0x33, 0x34, 0x35, 0x0D           // 1-5 + Return
    };

    for (auto it = keyMap.begin(); it != keyMap.end(); ++it) {
        QPixmap pix(QString(":/assets/bongo/keys/%1.png").arg(it.value()));
        if (!pix.isNull()) {
            m_keyPixmaps[it.key()] = pix;

            // 建立按键 -> 左右手映射
            if (leftHandKeys.contains(it.key())) {
                m_keyHandMap[it.key()] = HandSide::Left;
            } else if (rightHandKeys.contains(it.key())) {
                m_keyHandMap[it.key()] = HandSide::Right;
            }
        }
    }
}

void BongoCatWidget::onShow() { installHook(); }

void BongoCatWidget::onHide()
{
    removeHook();
    clearActiveKeys();
}

void BongoCatWidget::clearActiveKeys()
{
    m_activeKey[static_cast<int>(HandSide::Left)] = {};
    m_activeKey[static_cast<int>(HandSide::Right)] = {};
    update();
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

        // 只处理有贴图的按键
        if (s_instance->m_keyPixmaps.contains(vk)) {
            if (isDown) {
                // 查按键属于哪只手，默认归左手（未分类的按键）
                HandSide side = s_instance->m_keyHandMap.value(vk, HandSide::Left);

                // ★ 关键修复：同一只手同一时刻只保留一个按键贴图
                // 对应原版 BongoCat useModel.ts handlePress 的互斥逻辑
                // （path.split(sep).last(-2) === 'left-keys'/'right-keys'）
                // 如果同手之前按的是别的键，新键按下后直接覆盖旧键
                s_instance->m_activeKey[static_cast<int>(side)] = {
                    vk,
                    s_instance->m_keyPixmaps.value(vk),
                    true
                };
                s_instance->update();
            } else if (isUp) {
                HandSide side = s_instance->m_keyHandMap.value(vk, HandSide::Left);
                // 只释放对应手的按键，且只在该手当前激活的就是这个键时才清
                if (s_instance->m_activeKey[static_cast<int>(side)].vk == vk) {
                    s_instance->m_activeKey[static_cast<int>(side)] = {};
                }
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
    if (m_cover.isNull()) return;

    QSize scaledSize = m_cover.size().scaled(targetRect.size(), Qt::KeepAspectRatio);
    QRect drawRect(
        (targetRect.width() - scaledSize.width()) / 2,
        (targetRect.height() - scaledSize.height()) / 2,
        scaledSize.width(), scaledSize.height()
    );

    // 1. 画底图（猫咪身体 + 键盘）
    painter.drawPixmap(drawRect, m_cover);

    // 2. ★ 修复：每只手最多叠加一张按键贴图，杜绝多手叠加
    // 对应原版 BongoCat main/index.vue 的 <img v-for="path in modelStore.pressedKeys">
    // pressedKeys 经过 handlePress 互斥后，每只手最多一个键
    for (int side = 0; side < 2; ++side) {
        const ActiveKey &ak = m_activeKey[side];
        if (ak.valid && !ak.pixmap.isNull()) {
            painter.drawPixmap(drawRect, ak.pixmap);
        }
    }
}
