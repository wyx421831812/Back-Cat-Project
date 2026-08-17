#include "bongocatwidget.h"
#include <QPainter>
#include <QPaintEvent>
#include <QDir>
#include <QFileInfo>
#include <QCoreApplication>

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

void BongoCatWidget::scanKeyDirectory(const QString &dirPath, HandSide side,
                                       const QMap<int, QString> &nameToVk)
{
    QDir dir(dirPath);
    if (!dir.exists()) return;

    // 建一个反向映射：文件名（不含扩展名） -> vkCode
    QMap<QString, int> fileToVk;
    for (auto it = nameToVk.begin(); it != nameToVk.end(); ++it) {
        fileToVk[it.value()] = it.key();
    }

    // 扫描目录下所有 png 文件
    const QStringList filters = {"*.png", "*.jpg"};
    for (const QFileInfo &fi : dir.entryInfoList(filters, QDir::Files)) {
        QString baseName = fi.completeBaseName();   // "KeyA" from "KeyA.png"
        if (!fileToVk.contains(baseName)) continue;

        int vk = fileToVk[baseName];
        QPixmap pix(fi.absoluteFilePath());
        if (pix.isNull()) continue;

        m_keyPixmaps[vk] = pix;
        m_keyHandMap[vk] = side;
    }
}

void BongoCatWidget::loadImages()
{
    // 按键 vkCode -> 名称映射（与原版 BongoCat left-keys 目录文件名一致）
    static const QMap<int, QString> keyMap = {
        // 字母键
        {0x41, "KeyA"}, {0x42, "KeyB"}, {0x43, "KeyC"}, {0x44, "KeyD"},
        {0x45, "KeyE"}, {0x46, "KeyF"}, {0x47, "KeyG"}, {0x48, "KeyH"},
        {0x49, "KeyI"}, {0x4A, "KeyJ"}, {0x4B, "KeyK"}, {0x4C, "KeyL"},
        {0x4D, "KeyM"}, {0x4E, "KeyN"}, {0x4F, "KeyO"}, {0x50, "KeyP"},
        {0x51, "KeyQ"}, {0x52, "KeyR"}, {0x53, "KeyS"}, {0x54, "KeyT"},
        {0x55, "KeyU"}, {0x56, "KeyV"}, {0x57, "KeyW"}, {0x58, "KeyX"},
        {0x59, "KeyY"}, {0x5A, "KeyZ"},
        // 数字键
        {0x30, "Num0"}, {0x31, "Num1"}, {0x32, "Num2"}, {0x33, "Num3"},
        {0x34, "Num4"}, {0x35, "Num5"}, {0x36, "Num6"}, {0x37, "Num7"},
        {0x38, "Num8"}, {0x39, "Num9"},
        // 符号键
        {0xBA, "Semicolon"}, {0xBB, "Equal"}, {0xBD, "Minus"},
        {0xDB, "BracketLeft"}, {0xDD, "BracketRight"}, {0xDC, "Backslash"},
        {0xBE, "Period"}, {0xBF, "Slash"}, {0xC0, "Quote"}, {0xE2, "BackQuote"},
        // 功能键
        {0x08, "Backspace"}, {0x09, "Tab"}, {0x0D, "Return"}, {0x20, "Space"},
        {0x1B, "Escape"}, {0x2E, "Delete"},
        {0x10, "Shift"}, {0xA0, "ShiftLeft"}, {0xA1, "ShiftRight"},
        {0x11, "Control"}, {0xA2, "ControlLeft"}, {0xA3, "ControlRight"},
        {0x12, "Alt"}, {0x5B, "Meta"}, {0x14, "CapsLock"},
        // 方向键
        {0x25, "LeftArrow"}, {0x26, "UpArrow"}, {0x27, "RightArrow"}, {0x28, "DownArrow"}
    };

    // ★ 优先从文件系统加载真实资源目录
    // 路径优先级：可执行文件同级 desktop/resources/models/keyboard/resources/
    //           → 工作目录 desktop/resources/models/keyboard/resources/
    //           → qrc 资源回退
    QStringList searchPaths = {
        QCoreApplication::applicationDirPath() + "/desktop/resources/models/keyboard/resources",
        QDir::currentPath() + "/desktop/resources/models/keyboard/resources",
        QDir::currentPath() + "/resources/models/keyboard/resources",
        ":/assets/bongo"  // qrc 回退路径
    };

    for (const QString &resPath : searchPaths) {
        QString coverPath = resPath + "/cover.png";
        if (m_cover.isNull() && QFileInfo::exists(coverPath)) {
            m_cover = QPixmap(coverPath);
        }

        // 扫描 left-keys 子目录
        QString leftKeysDir = resPath + "/left-keys";
        if (QDir(leftKeysDir).exists()) {
            scanKeyDirectory(leftKeysDir, HandSide::Left, keyMap);
        }

        // 扫描 right-keys 子目录
        QString rightKeysDir = resPath + "/right-keys";
        if (QDir(rightKeysDir).exists()) {
            scanKeyDirectory(rightKeysDir, HandSide::Right, keyMap);
        }

        // qrc 回退：扁平 keys/ 目录，默认归左手
        if (resPath == ":/assets/bongo") {
            for (auto it = keyMap.begin(); it != keyMap.end(); ++it) {
                if (m_keyPixmaps.contains(it.key())) continue;
                QPixmap pix(QString(":/assets/bongo/keys/%1.png").arg(it.value()));
                if (!pix.isNull()) {
                    m_keyPixmaps[it.key()] = pix;
                    // qrc 回退时没有目录分类，默认归左手
                    if (!m_keyHandMap.contains(it.key())) {
                        m_keyHandMap[it.key()] = HandSide::Left;
                    }
                }
            }
        }

        // 如果已经加载到足够资源，提前退出
        if (!m_cover.isNull() && m_keyPixmaps.size() > 0) break;
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
                // 查按键属于哪只手，未分类的按键默认归左手
                HandSide side = s_instance->m_keyHandMap.value(vk, HandSide::Left);

                // ★ 关键修复：同一只手同一时刻只保留一个按键贴图
                // 对应原版 BongoCat useModel.ts handlePress 的互斥逻辑
                // 同手新键按下后直接覆盖旧键
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
