#include "bongocatwidget.h"
#include "appconfig.h"
#include <QPainter>
#include <QPaintEvent>
#include <QMouseEvent>
#include <QGuiApplication>
#include <QScreen>
#include <QCursor>
#include <QFileInfo>
#include <QDir>
#include <QtMath>

#ifdef Q_OS_WIN
BongoCatWidget *BongoCatWidget::s_instance = nullptr;
HHOOK BongoCatWidget::s_keyboardHook = nullptr;
HHOOK BongoCatWidget::s_mouseHook = nullptr;
#endif

BongoCatWidget::BongoCatWidget(QWidget *parent)
    : ComponentBase(parent)
    , m_animationTimer(new QTimer(this))
{
    setupTransparentBackground();
    setAttribute(Qt::WA_TranslucentBackground);
    setMouseTracking(true);

#ifdef Q_OS_WIN
    s_instance = this;
#endif

    // 加载模型管理器中的模型
    BongoModelManager::instance().loadModels();
    reloadCurrentModel();

    // 监听模型变化
    connect(&BongoModelManager::instance(), &BongoModelManager::modelChanged,
            this, [this](const QString &) {
        reloadCurrentModel();
        update();
    });

    // 动画定时器 - 约60fps
    connect(m_animationTimer, &QTimer::timeout, this, &BongoCatWidget::onAnimationTick);

    // L-105: 从配置加载自动释放延迟
    m_autoReleaseDelay = AppConfig::instance().autoReleaseDelay();

    m_elapsedTimer.start();
}

BongoCatWidget::~BongoCatWidget()
{
    removeHook();
#ifdef Q_OS_WIN
    if (s_instance == this)
        s_instance = nullptr;
#endif
}

void BongoCatWidget::refresh()
{
    reloadCurrentModel();
    update();
}

void BongoCatWidget::onShow()
{
    installHook();
    m_animationTimer->start(16); // ~60fps
    m_elapsedTimer.restart();
    update(); // 立即触发首次绘制
}

void BongoCatWidget::onHide()
{
    removeHook();
    m_animationTimer->stop();
    m_pressedKeys.clear();
    m_pressedVkCodes.clear();
    m_leftMouseDown = false;
    m_rightMouseDown = false;
}

void BongoCatWidget::loadModel(const BongoModel &model)
{
    m_model = model;
    m_pressedKeys.clear();
    m_pressedVkCodes.clear();

    // 根据模型类型选择渲染方式
    if (shouldUseLive2D()) {
        switchToLive2D();
    } else {
        switchToStaticImage();
    }

    update();
}

void BongoCatWidget::reloadCurrentModel()
{
    loadModel(BongoModelManager::instance().currentModel());
}

BongoModel BongoCatWidget::currentModel() const
{
    return m_model;
}

void BongoCatWidget::setMouseFollowEnabled(bool enabled)
{
    m_mouseFollowEnabled = enabled;
    if (!enabled) {
        m_targetOffset = QPointF(0, 0);
    }
}

bool BongoCatWidget::isMouseFollowEnabled() const
{
    return m_mouseFollowEnabled;
}

bool BongoCatWidget::hasLive2DSupport() const
{
#ifdef HAS_LIVE2D_SUPPORT
    return true;
#else
    return false;
#endif
}

bool BongoCatWidget::isUsingLive2D() const
{
    return m_useLive2D;
}

// L-104: 游戏手柄开关
void BongoCatWidget::setGamepadEnabled(bool enabled)
{
    m_gamepadEnabled = enabled;
    if (!enabled) {
        // 关闭时重置手柄状态，避免卡住
        m_lastGamepadButtons = 0;
        m_lastStickLX = m_lastStickLY = m_lastStickRX = m_lastStickRY = 0;
    }
}

bool BongoCatWidget::isGamepadEnabled() const
{
    return m_gamepadEnabled;
}

bool BongoCatWidget::isGamepadConnected() const
{
    return m_gamepadConnected;
}

void BongoCatWidget::setAutoReleaseDelay(int ms)
{
    m_autoReleaseDelay = qMax(0, ms);
    AppConfig::instance().setAutoReleaseDelay(m_autoReleaseDelay);
    AppConfig::instance().save();
#ifdef HAS_LIVE2D_SUPPORT
    if (m_live2dWidget && m_live2dWidget->isReady()) {
        m_live2dWidget->setAutoReleaseDelay(m_autoReleaseDelay);
    }
#endif
}

int BongoCatWidget::autoReleaseDelay() const
{
    return m_autoReleaseDelay;
}

bool BongoCatWidget::shouldUseLive2D() const
{
#ifdef HAS_LIVE2D_SUPPORT
    // 只有当模型有Live2D模型文件时才使用Live2D
    return !m_model.live2dModelFile.isEmpty();
#else
    return false;
#endif
}

void BongoCatWidget::switchToLive2D()
{
#ifdef HAS_LIVE2D_SUPPORT
    if (!m_live2dWidget) {
        m_live2dWidget = new Live2DWidget(this);
        m_live2dWidget->setGeometry(rect());

        // 连接信号
        connect(m_live2dWidget, &Live2DWidget::errorOccurred,
                this, [this](const QString &err) {
            qWarning() << "Live2D error:" << err;
            // 出错时回退到静态图片
            switchToStaticImage();
        });
    }

    // 加载Live2D模型
    if (!m_model.live2dModelFile.isEmpty()) {
        m_live2dWidget->loadModel(m_model.live2dModelFile);
        m_live2dWidget->show();
        m_live2dWidget->lower();          // 放到父窗口底层，让 paintEvent 画的 background/按键图可见
        m_useLive2D = true;

        // L-105: 传递自动释放延迟配置
        connect(m_live2dWidget, &Live2DWidget::readyChanged, this, [this](bool ready) {
            if (ready) {
                m_live2dWidget->setAutoReleaseDelay(m_autoReleaseDelay);
                // 引擎就绪后，推送背景图和按键图路径到 HTML
                pushImagesToLive2D();
            }
        });

        // 如果已经 ready，立即推送
        if (m_live2dWidget->isReady()) {
            pushImagesToLive2D();
        }
    }
#else
    m_useLive2D = false;
#endif
}

void BongoCatWidget::switchToStaticImage()
{
#ifdef HAS_LIVE2D_SUPPORT
    if (m_live2dWidget) {
        m_live2dWidget->hide();
    }
#endif
    m_useLive2D = false;
    update();
}

#ifdef HAS_LIVE2D_SUPPORT
void BongoCatWidget::pushImagesToLive2D()
{
    if (!m_live2dWidget || !m_live2dWidget->isReady()) return;

    // m_model.path 是模型目录 (文件系统路径 或 qrc:/...)
    // m_model.live2dModelFile 是 cat.model3.json 的完整路径
    QString modelDir = m_model.path;
    if (modelDir.isEmpty() && !m_model.live2dModelFile.isEmpty()) {
        QFileInfo fi(m_model.live2dModelFile);
        modelDir = fi.absolutePath();
    }

    // 背景图: <modelDir>/resources/background.png
    QString bgPath = modelDir + "/resources/background.png";
    if (QFile::exists(bgPath)) {
        m_live2dWidget->setBackgroundImage(bgPath);
    } else {
        m_live2dWidget->setBackgroundImage(QString());
    }

    // 清空旧的按键图注册表
    m_live2dWidget->clearAllKeyImages();

    // 按键图: <modelDir>/resources/left-keys/*.png 和 right-keys/*.png
    // 文件系统路径用 QDir 枚举; qrc 路径用硬编码列表
    bool fromFileSystem = !modelDir.startsWith(":/") && !modelDir.startsWith("qrc:");
    if (fromFileSystem) {
        static const QStringList subDirs = {"left-keys", "right-keys", "keys"};
        for (const QString &sd : subDirs) {
            QString dirPath = modelDir + "/resources/" + sd;
            QDir dir(dirPath);
            if (!dir.exists()) continue;
            QStringList filters;
            filters << "*.png" << "*.jpg" << "*.jpeg";
            const auto files = dir.entryInfoList(filters, QDir::Files);
            for (const QFileInfo &fi : files) {
                QString keyName = fi.baseName();
                m_live2dWidget->setKeyImage(keyName, fi.absoluteFilePath());
            }
        }
    } else {
        // qrc 路径: 直接使用 qrc URL
        static const QStringList allKeys = {
            "Alt","AltGr","BackQuote","Backspace","CapsLock","Control","ControlLeft","ControlRight",
            "Delete","Escape","Fn","KeyA","KeyB","KeyC","KeyD","KeyE","KeyF","KeyG","KeyH","KeyI",
            "KeyJ","KeyK","KeyL","KeyM","KeyN","KeyO","KeyP","KeyQ","KeyR","KeyS","KeyT","KeyU",
            "KeyV","KeyW","KeyX","KeyY","KeyZ","Meta","Num0","Num1","Num2","Num3","Num4","Num5",
            "Num6","Num7","Num8","Num9","Return","Shift","ShiftLeft","ShiftRight","Slash","Space","Tab",
            "DownArrow","LeftArrow","RightArrow","UpArrow",
            "DPadDown","DPadLeft","DPadRight","DPadUp","LeftTrigger","LeftTrigger2",
            "East","North","RightTrigger","RightTrigger2","South","West"
        };
        for (const QString &key : allKeys) {
            for (const QString &sd : {"left-keys", "right-keys"}) {
                QString p = modelDir + "/resources/" + sd + "/" + key + ".png";
                if (QFile::exists(p)) {
                    m_live2dWidget->setKeyImage(key, p);
                    break;
                }
            }
        }
    }
}
#endif

QString BongoCatWidget::vkToKeyName(int vkCode) const
{
    // 字母键 A-Z
    if (vkCode >= 0x41 && vkCode <= 0x5A) {
        return QString("Key%1").arg(QChar(vkCode));
    }
    // 数字键 0-9 (主键盘)
    if (vkCode >= 0x30 && vkCode <= 0x39) {
        return QString("Num%1").arg(vkCode - 0x30);
    }

#ifdef Q_OS_WIN
    // 小键盘数字 - 暂时映射到主键盘数字
    if (vkCode >= VK_NUMPAD0 && vkCode <= VK_NUMPAD9) {
        return QString("Num%1").arg(vkCode - VK_NUMPAD0);
    }
    // 功能键 / 修饰键 / 方向键 / OEM 符号键
    switch (vkCode) {
    case VK_SPACE:    return "Space";
    case VK_RETURN:   return "Return";
    case VK_TAB:      return "Tab";
    case VK_BACK:     return "Backspace";
    case VK_ESCAPE:   return "Escape";
    case VK_CAPITAL:  return "CapsLock";
    case VK_DELETE:   return "Delete";

    // 修饰键：区分左右变体，便于 HTML 判定左右手
    case VK_SHIFT:    return "Shift";
    case VK_LSHIFT:   return "ShiftLeft";
    case VK_RSHIFT:   return "ShiftRight";
    case VK_CONTROL:  return "Control";
    case VK_LCONTROL: return "ControlLeft";
    case VK_RCONTROL: return "ControlRight";
    case VK_MENU:     return "Alt";
    case VK_LMENU:    return "AltLeft";
    case VK_RMENU:    return "AltRight";
    case VK_LWIN:     return "MetaLeft";
    case VK_RWIN:     return "MetaRight";
    case VK_APPS:     return "ContextMenu";

    // 方向键 (HTML normalizeKey 会将 Left/Right/Up/Down 映射为 ArrowLeft/...)
    case VK_LEFT:  return "Left";
    case VK_RIGHT: return "Right";
    case VK_UP:    return "Up";
    case VK_DOWN:  return "Down";

    // F1-F12
    case VK_F1:  return "F1";
    case VK_F2:  return "F2";
    case VK_F3:  return "F3";
    case VK_F4:  return "F4";
    case VK_F5:  return "F5";
    case VK_F6:  return "F6";
    case VK_F7:  return "F7";
    case VK_F8:  return "F8";
    case VK_F9:  return "F9";
    case VK_F10: return "F10";
    case VK_F11: return "F11";
    case VK_F12: return "F12";

    // OEM 符号键
    case VK_OEM_1: return "Semicolon";    // ;:
    case VK_OEM_2: return "Slash";        // /?
    case VK_OEM_3: return "BackQuote";    // `~
    case VK_OEM_4: return "BracketLeft";  // [{
    case VK_OEM_5: return "Backslash";    // \|
    case VK_OEM_6: return "BracketRight"; // ]}
    case VK_OEM_7: return "Quote";        // '"
    case VK_OEM_PLUS:   return "Equal";   // =+
    case VK_OEM_COMMA:  return "Comma";   // ,<
    case VK_OEM_MINUS:  return "Minus";   // -_
    case VK_OEM_PERIOD: return "Period";  // .>

    default:
        // 尝试转换为字符
        UINT scanCode = MapVirtualKeyW(vkCode, MAPVK_VK_TO_VSC);
        if (scanCode) {
            wchar_t chars[16] = {0};
            BYTE keyboardState[256] = {0};
            GetKeyboardState(keyboardState);
            if (ToUnicode(vkCode, scanCode, keyboardState, chars, 15, 0) > 0) {
                return QString("Key%1").arg(QString::fromWCharArray(chars).toUpper());
            }
        }
        return QString();
    }
#else
    // 非Windows平台的简化映射
    switch (vkCode) {
    case 0x20: return "Space";
    case 0x0D: return "Return";
    case 0x09: return "Tab";
    case 0x08: return "Backspace";
    case 0x1B: return "Escape";
    default:
        return QString();
    }
#endif
}

void BongoCatWidget::handleKeyDown(int vkCode)
{
    m_pressedVkCodes.insert(vkCode);

    QString keyName = vkToKeyName(vkCode);
    if (!keyName.isEmpty()) {
        m_pressedKeys.insert(keyName);
    }

#ifdef Q_OS_WIN
    // 判断左右手(简化：左边的键用左手，右边的键用右手)
    static const QSet<int> leftHandKeys = {
        0x41, 0x53, 0x44, 0x46, 0x47,
        0x51, 0x57, 0x45, 0x52, 0x54,
        0x5A, 0x58, 0x43, 0x56,
        0x31, 0x32, 0x33, 0x34, 0x35,
        VK_TAB, VK_CAPITAL, VK_LSHIFT, VK_LCONTROL, VK_LMENU, VK_OEM_3
    };

    static const QSet<int> rightHandKeys = {
        0x48, 0x4A, 0x4B, 0x4C, 0x4D,
        0x59, 0x55, 0x49, 0x4F, 0x50,
        0x42, 0x4E,
        0x36, 0x37, 0x38, 0x39, 0x30,
        VK_SPACE, VK_RETURN, VK_RSHIFT, VK_RCONTROL, VK_RMENU,
        VK_OEM_1, VK_OEM_2, VK_OEM_4, VK_OEM_5, VK_OEM_6, VK_OEM_7
    };

    if (leftHandKeys.contains(vkCode) || m_pressedVkCodes.contains(VK_LSHIFT)) {
        m_leftHandActive = true;
        m_leftHandPressTime = m_elapsedTimer.elapsed();
    }
    if (rightHandKeys.contains(vkCode) || m_pressedVkCodes.contains(VK_RSHIFT)) {
        m_rightHandActive = true;
        m_rightHandPressTime = m_elapsedTimer.elapsed();
    }

    if (!m_leftHandActive && !m_rightHandActive) {
        if (vkCode < 0x50) {
            m_leftHandActive = true;
            m_leftHandPressTime = m_elapsedTimer.elapsed();
        } else {
            m_rightHandActive = true;
            m_rightHandPressTime = m_elapsedTimer.elapsed();
        }
    }
#else
    // 非Windows平台: 简单左/右手分配
    if (vkCode < 0x50) {
        m_leftHandActive = true;
        m_leftHandPressTime = m_elapsedTimer.elapsed();
    } else {
        m_rightHandActive = true;
        m_rightHandPressTime = m_elapsedTimer.elapsed();
    }
#endif

    // 传递按键事件给Live2D
    if (m_useLive2D && !keyName.isEmpty()) {
#ifdef HAS_LIVE2D_SUPPORT
        if (m_live2dWidget && m_live2dWidget->isReady()) {
            m_live2dWidget->handleKeyDown(keyName);
        }
#endif
    }

    update();
}

void BongoCatWidget::handleKeyUp(int vkCode)
{
    m_pressedVkCodes.remove(vkCode);

    QString keyName = vkToKeyName(vkCode);
    if (!keyName.isEmpty()) {
        m_pressedKeys.remove(keyName);
    }

    // 传递按键释放事件给Live2D
    if (m_useLive2D && !keyName.isEmpty()) {
#ifdef HAS_LIVE2D_SUPPORT
        if (m_live2dWidget && m_live2dWidget->isReady()) {
            m_live2dWidget->handleKeyUp(keyName);
        }
#endif
    }

    update();
}

void BongoCatWidget::handleMouseDown(bool isLeft)
{
    qDebug() << "[BongoCat] handleMouseDown isLeft=" << isLeft << "useLive2D=" << m_useLive2D;
    if (isLeft) {
        m_leftMouseDown = true;
        m_leftHandActive = true;
        m_leftHandPressTime = m_elapsedTimer.elapsed();
    } else {
        m_rightMouseDown = true;
        m_rightHandActive = true;
        m_rightHandPressTime = m_elapsedTimer.elapsed();
    }

    // 传递鼠标事件给Live2D
    if (m_useLive2D) {
#ifdef HAS_LIVE2D_SUPPORT
        if (m_live2dWidget && m_live2dWidget->isReady()) {
            int button = isLeft ? 0 : 2; // 0=左键, 2=右键
            qDebug() << "[BongoCat] -> Live2D handleMouseDown button=" << button;
            m_live2dWidget->handleMouseDown(button);
        } else {
            qDebug() << "[BongoCat] Live2D not ready or null";
        }
#endif
    }

    update();
}

void BongoCatWidget::handleMouseUp(bool isLeft)
{
    qDebug() << "[BongoCat] handleMouseUp isLeft=" << isLeft << "useLive2D=" << m_useLive2D;
    if (isLeft) {
        m_leftMouseDown = false;
    } else {
        m_rightMouseDown = false;
    }

    // 传递鼠标释放事件给Live2D
    if (m_useLive2D) {
#ifdef HAS_LIVE2D_SUPPORT
        if (m_live2dWidget && m_live2dWidget->isReady()) {
            int button = isLeft ? 0 : 2;
            qDebug() << "[BongoCat] -> Live2D handleMouseUp button=" << button;
            m_live2dWidget->handleMouseUp(button);
        }
#endif
    }

    update();
}

void BongoCatWidget::updateMouseFollow(const QPoint &globalPos)
{
    if (!m_mouseFollowEnabled) return;

    QScreen *screen = QGuiApplication::screenAt(globalPos);
    if (!screen) {
        screen = QGuiApplication::primaryScreen();
    }

    QRect geo = screen->availableGeometry();
    qreal xRatio = (globalPos.x() - geo.x()) / static_cast<qreal>(geo.width());
    qreal yRatio = (globalPos.y() - geo.y()) / static_cast<qreal>(geo.height());

    // 映射到 -1.0 ~ 1.0
    m_targetOffset.setX((xRatio - 0.5) * 2.0);
    m_targetOffset.setY((yRatio - 0.5) * 2.0);
}

void BongoCatWidget::onAnimationTick()
{
    qint64 now = m_elapsedTimer.elapsed();

    // 呼吸/浮动效果
    m_bobPhase = static_cast<qreal>(now) / 1000.0;

    // 平滑鼠标跟随
    qreal smoothFactor = 0.08;
    m_mouseOffset.setX(m_mouseOffset.x() + (m_targetOffset.x() - m_mouseOffset.x()) * smoothFactor);
    m_mouseOffset.setY(m_mouseOffset.y() + (m_targetOffset.y() - m_mouseOffset.y()) * smoothFactor);

    // 手部按下后自动恢复(如果没有持续按键)
    bool leftPressed = false;
    bool rightPressed = false;

#ifdef Q_OS_WIN
    // 检查修饰键
    if (m_pressedVkCodes.contains(VK_LSHIFT) || m_pressedVkCodes.contains(VK_LCONTROL) ||
        m_pressedVkCodes.contains(VK_LMENU)) {
        leftPressed = true;
    }
    if (m_pressedVkCodes.contains(VK_RSHIFT) || m_pressedVkCodes.contains(VK_RCONTROL) ||
        m_pressedVkCodes.contains(VK_RMENU)) {
        rightPressed = true;
    }

    // 检查普通按键
    static const QSet<int> leftHandKeys = {
        0x41, 0x53, 0x44, 0x46, 0x47,
        0x51, 0x57, 0x45, 0x52, 0x54,
        0x5A, 0x58, 0x43, 0x56,
        0x31, 0x32, 0x33, 0x34, 0x35,
        VK_TAB, VK_CAPITAL, VK_OEM_3
    };

    static const QSet<int> rightHandKeys = {
        0x48, 0x4A, 0x4B, 0x4C, 0x4D,
        0x59, 0x55, 0x49, 0x4F, 0x50,
        0x42, 0x4E,
        0x36, 0x37, 0x38, 0x39, 0x30,
        VK_SPACE, VK_RETURN
    };

    for (int vk : m_pressedVkCodes) {
        if (leftHandKeys.contains(vk)) leftPressed = true;
        if (rightHandKeys.contains(vk)) rightPressed = true;
    }
#endif

    if (m_leftMouseDown) leftPressed = true;
    if (m_rightMouseDown) rightPressed = true;

    // 手部状态在按下后保持一小段时间，产生动画效果 (L-105: 可配置延迟)
    qint64 handHoldMs = m_autoReleaseDelay;
    if (!leftPressed && m_leftHandActive && (now - m_leftHandPressTime > handHoldMs)) {
        m_leftHandActive = false;
    }
    if (!rightPressed && m_rightHandActive && (now - m_rightHandPressTime > handHoldMs)) {
        m_rightHandActive = false;
    }

    // 更新鼠标位置
    updateMouseFollow(QCursor::pos());

    // L-104: 轮询游戏手柄状态 (Windows XInput)
    if (m_gamepadEnabled) {
        pollGamepadState();
    }

    // 将输入状态传递给Live2D (如果正在使用)
    if (m_useLive2D) {
#ifdef HAS_LIVE2D_SUPPORT
        if (m_live2dWidget && m_live2dWidget->isReady()) {
            // 传递鼠标移动 (归一化到 [-1, 1])
            QPoint globalPos = QCursor::pos();
            QScreen *screen = QGuiApplication::screenAt(globalPos);
            if (screen) {
                QRect geo = screen->geometry();
                double nx = (globalPos.x() - geo.x()) / (double)geo.width() * 2.0 - 1.0;
                double ny = (globalPos.y() - geo.y()) / (double)geo.height() * 2.0 - 1.0;
                m_live2dWidget->handleMouseMove(nx, ny);
            }
        }
#endif
    }

    update();
}

bool BongoCatWidget::isLeftHandDown() const
{
    return m_leftHandActive;
}

bool BongoCatWidget::isRightHandDown() const
{
    return m_rightHandActive;
}

void BongoCatWidget::installHook()
{
#ifdef Q_OS_WIN
    if (!s_keyboardHook && s_instance) {
        s_keyboardHook = SetWindowsHookEx(WH_KEYBOARD_LL, keyboardHookProc,
                                           GetModuleHandle(nullptr), 0);
    }
    if (!s_mouseHook && s_instance) {
        s_mouseHook = SetWindowsHookEx(WH_MOUSE_LL, mouseHookProc,
                                        GetModuleHandle(nullptr), 0);
    }
#endif
}

void BongoCatWidget::removeHook()
{
#ifdef Q_OS_WIN
    if (s_keyboardHook) {
        UnhookWindowsHookEx(s_keyboardHook);
        s_keyboardHook = nullptr;
    }
    if (s_mouseHook) {
        UnhookWindowsHookEx(s_mouseHook);
        s_mouseHook = nullptr;
    }
#endif
}

#ifdef Q_OS_WIN
LRESULT CALLBACK BongoCatWidget::keyboardHookProc(int nCode, WPARAM wParam, LPARAM lParam)
{
    if (nCode >= 0 && s_instance) {
        KBDLLHOOKSTRUCT *kb = reinterpret_cast<KBDLLHOOKSTRUCT *>(lParam);
        int vk = static_cast<int>(kb->vkCode);

        if (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN) {
            s_instance->handleKeyDown(vk);
        } else if (wParam == WM_KEYUP || wParam == WM_SYSKEYUP) {
            s_instance->handleKeyUp(vk);
        }
    }
    return CallNextHookEx(s_keyboardHook, nCode, wParam, lParam);
}

LRESULT CALLBACK BongoCatWidget::mouseHookProc(int nCode, WPARAM wParam, LPARAM lParam)
{
    if (nCode >= 0 && s_instance) {
        MSLLHOOKSTRUCT *mouse = reinterpret_cast<MSLLHOOKSTRUCT *>(lParam);
        Q_UNUSED(mouse);

        switch (wParam) {
        case WM_LBUTTONDOWN:
            s_instance->handleMouseDown(true);
            break;
        case WM_LBUTTONUP:
            s_instance->handleMouseUp(true);
            break;
        case WM_RBUTTONDOWN:
            s_instance->handleMouseDown(false);
            break;
        case WM_RBUTTONUP:
            s_instance->handleMouseUp(false);
            break;
        default:
            break;
        }
    }
    return CallNextHookEx(s_mouseHook, nCode, wParam, lParam);
}
#endif

void BongoCatWidget::resizeEvent(QResizeEvent *)
{
#ifdef HAS_LIVE2D_SUPPORT
    if (m_live2dWidget) {
        m_live2dWidget->setGeometry(rect());
    }
#endif
}

void BongoCatWidget::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::TextAntialiasing);

    QRect targetRect = rect();
    bool hasModelImage = !m_model.coverImage.isNull();
    bool hasBackgroundImage = !m_model.backgroundImage.isNull();

    // === 计算呼吸/浮动参数 ===
    qreal timeSec = m_bobPhase;
    qreal bobOffset = qSin(timeSec * 2.0) * 2.5;
    int offsetX = static_cast<int>(m_mouseOffset.x() * 5.0);
    int offsetY = static_cast<int>(m_mouseOffset.y() * 5.0 + bobOffset);

    // 按下时的抖动
    if (!m_pressedKeys.isEmpty() || m_leftMouseDown || m_rightMouseDown) {
        offsetX += qSin(timeSec * 25.0) * 1.5;
        offsetY += qCos(timeSec * 22.0) * 1.0;
    }

    // === Live2D 模式：不通过 paintEvent 绘制，全部由 HTML 渲染 ===
    // 渲染层次 (与 BongoCat 官方一致): background → Live2D canvas → 按键图
    // 这些图层都在 Live2D HTML 中，C++ 通过 setBackground/setKeyImage 传入
    if (m_useLive2D) {
        return;
    }

    if (hasModelImage) {
        // === 图片模式: 参考BongoCat项目的渲染顺序 ===
        // BongoCat原项目: background → Live2D canvas → key images
        // 我们没有Live2D,用cover.png作为静态猫咪叠加层
        // 关键: cover和background底部对齐(键盘和鼠标垫在画面底部)

        qreal timeSec = m_bobPhase;

        // 第一层: 键盘背景 (全窗口, IgnoreAspectRatio)
        if (hasBackgroundImage) {
            painter.drawPixmap(targetRect,
                               m_model.backgroundImage.scaled(
                                   targetRect.size(),
                                   Qt::IgnoreAspectRatio,
                                   Qt::SmoothTransformation));
        } else {
            QLinearGradient bgGrad(targetRect.topLeft(), targetRect.bottomRight());
            bgGrad.setColorAt(0, QColor(255, 248, 220));
            bgGrad.setColorAt(1, QColor(255, 240, 200));
            painter.fillRect(targetRect, bgGrad);
        }

        // 第二层: 猫咪主体 (底部对齐, KeepAspectRatio)
        // cover.png中有键盘/鼠标垫等底部元素,必须与background的底部对齐
        QSize coverSize = m_model.coverImage.size();
        QSize scaledCover = coverSize.scaled(targetRect.size(), Qt::KeepAspectRatioByExpanding);
        // 如果cover比窗口宽,按比例缩放让宽度匹配
        if (scaledCover.width() > targetRect.width()) {
            scaledCover = QSize(targetRect.width(),
                                coverSize.height() * targetRect.width() / coverSize.width());
        }
        if (scaledCover.height() > targetRect.height()) {
            scaledCover = QSize(coverSize.width() * targetRect.height() / coverSize.height(),
                                targetRect.height());
        }
        // 水平居中, 底部对齐
        QRect coverRect(
            (targetRect.width() - scaledCover.width()) / 2,
            targetRect.bottom() - scaledCover.height(),
            scaledCover.width(),
            scaledCover.height()
        );
        painter.drawPixmap(coverRect, m_model.coverImage.scaled(
            scaledCover, Qt::IgnoreAspectRatio, Qt::SmoothTransformation));

        // 第三层: 按下的按键图片 (全窗口, 与背景同尺寸对齐)
        for (const QString &keyName : m_pressedKeys) {
            QPixmap keyPix = m_model.getKeyImage(keyName);
            if (!keyPix.isNull()) {
                painter.drawPixmap(targetRect, keyPix.scaled(
                    targetRect.size(), Qt::IgnoreAspectRatio, Qt::SmoothTransformation));
            }
        }

        // 第四层: 鼠标按下反馈 (图片模式下模拟手部按压效果)
        // 左键按下 → 键盘区域（左手）按压遮罩
        if (isLeftHandDown() && m_pressedKeys.isEmpty()) {
            painter.setPen(Qt::NoPen);
            painter.setBrush(QColor(0, 150, 255, 50));
            qreal pulse = 0.7 + 0.3 * qSin(timeSec * 15.0);
            painter.setOpacity(pulse);
            // 左手位置: 画面左下偏中（对应键盘区）
            int pressW = targetRect.width() / 3;
            int pressH = targetRect.height() / 4;
            QRect pressRect(
                targetRect.x() + targetRect.width() * 45 / 100,
                targetRect.y() + targetRect.height() * 58 / 100,
                pressW, pressH
            );
            painter.drawRoundedRect(pressRect, 8, 8);
            painter.setOpacity(1.0);
        }
        // 右键按下 → 右手/空格区域按压反馈
        if (isRightHandDown()) {
            painter.setPen(Qt::NoPen);
            painter.setBrush(QColor(0, 150, 255, 50));
            qreal pulse = 0.7 + 0.3 * qSin(timeSec * 15.0);
            painter.setOpacity(pulse);
            int pressW = targetRect.width() / 4;
            int pressH = targetRect.height() / 5;
            QRect pressRect(
                targetRect.x() + targetRect.width() * 62 / 100,
                targetRect.y() + targetRect.height() * 72 / 100,
                pressW, pressH
            );
            painter.drawRoundedRect(pressRect, 8, 8);
            painter.setOpacity(1.0);
        }

    } else {
        // === 矢量绘制模式: 无模型图片时使用 ===

        // 绘制渐变背景
        QLinearGradient bgGrad(targetRect.topLeft(), targetRect.bottomRight());
        bgGrad.setColorAt(0, QColor(255, 248, 220));
        bgGrad.setColorAt(1, QColor(255, 240, 200));
        painter.fillRect(targetRect, bgGrad);

        // 计算猫咪绘制区域
        QSize catSize(targetRect.width() * 3 / 4, targetRect.height() * 3 / 4);
        qreal breatheScale = 1.0 + qSin(timeSec * 1.5) * 0.02;

        int scaledW = static_cast<int>(catSize.scaled(targetRect.size(), Qt::KeepAspectRatio).width() * breatheScale);
        int scaledH = static_cast<int>(catSize.scaled(targetRect.size(), Qt::KeepAspectRatio).height() * breatheScale);

        QRect catRect(
            (targetRect.width() - scaledW) / 2 + offsetX,
            (targetRect.height() - scaledH) / 2 + offsetY,
            scaledW,
            scaledH
        );

        // 绘制默认猫咪
        drawDefaultCat(painter, catRect);

        // 按键高亮 (矢量模式下使用粉色半透明效果)
        if (!m_pressedKeys.isEmpty()) {
            painter.setPen(Qt::NoPen);
            painter.setBrush(QColor(255, 100, 100, 60));
            for (const QString &keyName : m_pressedKeys) {
                auto it = m_model.keyImages.find(keyName);
                if (it != m_model.keyImages.end() && !it.value().isNull()) {
                    painter.setOpacity(0.5 + 0.5 * qSin(timeSec * 8.0));
                    painter.drawPixmap(catRect, it.value());
                    painter.setOpacity(1.0);
                }
            }
        }

        // 矢量模式下的手部效果(粉色爪子)
        drawHandPressEffect(painter, catRect, scaledW, scaledH);

        // 矢量模式下的涟漪效果
        if (!m_pressedKeys.isEmpty()) {
            drawKeyPressRipple(painter, catRect, timeSec);
        }
    }
}

// === 绘制默认猫咪 ===
void BongoCatWidget::drawDefaultCat(QPainter &painter, const QRect &catRect)
{
    painter.save();

    int w = catRect.width();
    int h = catRect.height();
    int cx = catRect.center().x();
    int cy = catRect.center().y();

    // 调整绘制区域到猫咪中心
    painter.translate(cx, cy);

    // 计算猫咪各部分尺寸
    int bodyW = w * 70 / 100;
    int bodyH = h * 55 / 100;
    int headW = w * 55 / 100;
    int headH = h * 40 / 100;
    int earW = w * 18 / 100;
    int earH = h * 15 / 100;

    // === 身体 ===
    painter.setPen(Qt::NoPen);
    QRadialGradient bodyGrad(0, bodyH / 6, bodyW / 2);
    bodyGrad.setColorAt(0, QColor(255, 230, 180));
    bodyGrad.setColorAt(1, QColor(240, 200, 140));
    painter.setBrush(bodyGrad);
    painter.drawEllipse(-bodyW / 2, -bodyH / 6, bodyW, bodyH);

    // === 头部 ===
    QRadialGradient headGrad(0, -headH / 4, headW / 2);
    headGrad.setColorAt(0, QColor(255, 235, 190));
    headGrad.setColorAt(1, QColor(245, 205, 145));
    painter.setBrush(headGrad);
    painter.drawEllipse(-headW / 2, -headH / 2 - h * 0.05, headW, headH);

    // === 耳朵 ===
    painter.setBrush(QColor(245, 205, 145));
    // 左耳
    QPolygon leftEar;
    leftEar << QPoint(-headW / 2 + w * 0.02, -headH / 2 - h * 0.05)
            << QPoint(-headW / 2 - earW / 4, -headH / 2 - earH - h * 0.05)
            << QPoint(-headW / 4, -headH / 2 - h * 0.02);
    painter.drawPolygon(leftEar);
    // 右耳
    QPolygon rightEar;
    rightEar << QPoint(headW / 2 - w * 0.02, -headH / 2 - h * 0.05)
             << QPoint(headW / 2 + earW / 4, -headH / 2 - earH - h * 0.05)
             << QPoint(headW / 4, -headH / 2 - h * 0.02);
    painter.drawPolygon(rightEar);

    // 耳朵内侧
    painter.setBrush(QColor(255, 180, 180));
    QPolygon leftInnerEar;
    leftInnerEar << QPoint(-headW / 3, -headH / 2 - h * 0.04)
                 << QPoint(-headW / 2 - earW / 8, -headH / 2 - earH / 2 - h * 0.04)
                 << QPoint(-headW / 4, -headH / 2 - h * 0.01);
    painter.drawPolygon(leftInnerEar);
    QPolygon rightInnerEar;
    rightInnerEar << QPoint(headW / 3, -headH / 2 - h * 0.04)
                  << QPoint(headW / 2 + earW / 8, -headH / 2 - earH / 2 - h * 0.04)
                  << QPoint(headW / 4, -headH / 2 - h * 0.01);
    painter.drawPolygon(rightInnerEar);

    // === 眼睛 ===
    qreal eyeY = -headH / 8 - h * 0.05;
    qreal eyeSpacing = headW / 4;
    qreal eyeR = w * 0.04;
    qreal blinkPhase = qSin(m_bobPhase * 3.0);
    bool isBlinking = blinkPhase > 0.95;
    qreal blinkScale = isBlinking ? 0.1 : 1.0;

    // 左眼白
    painter.setBrush(Qt::white);
    painter.setPen(QPen(QColor(80, 80, 80), 1.5));
    painter.drawEllipse(QPointF(-eyeSpacing, eyeY), eyeR, eyeR * blinkScale);
    // 右眼白
    painter.drawEllipse(QPointF(eyeSpacing, eyeY), eyeR, eyeR * blinkScale);

    if (!isBlinking) {
        // 瞳孔(跟随鼠标偏移)
        qreal pupilOffsetX = m_mouseOffset.x() * eyeR * 0.3;
        qreal pupilOffsetY = m_mouseOffset.y() * eyeR * 0.3;

        painter.setBrush(QColor(50, 50, 50));
        painter.setPen(Qt::NoPen);
        painter.drawEllipse(QPointF(-eyeSpacing + pupilOffsetX, eyeY + pupilOffsetY),
                           eyeR * 0.6, eyeR * 0.6 * blinkScale);
        painter.drawEllipse(QPointF(eyeSpacing + pupilOffsetX, eyeY + pupilOffsetY),
                           eyeR * 0.6, eyeR * 0.6 * blinkScale);

        // 眼睛高光
        painter.setBrush(Qt::white);
        painter.drawEllipse(QPointF(-eyeSpacing + pupilOffsetX - eyeR * 0.2,
                                    eyeY + pupilOffsetY - eyeR * 0.2),
                           eyeR * 0.2, eyeR * 0.2);
        painter.drawEllipse(QPointF(eyeSpacing + pupilOffsetX - eyeR * 0.2,
                                    eyeY + pupilOffsetY - eyeR * 0.2),
                           eyeR * 0.2, eyeR * 0.2);
    }

    // === 鼻子 ===
    painter.setBrush(QColor(200, 120, 120));
    painter.setPen(Qt::NoPen);
    QPolygon nose;
    nose << QPoint(0, -h * 0.02)
         << QPoint(-w * 0.015, h * 0.01)
         << QPoint(w * 0.015, h * 0.01);
    painter.drawPolygon(nose);

    // === 嘴巴 ===
    painter.setPen(QPen(QColor(150, 80, 80), 2));
    painter.setBrush(Qt::NoBrush);
    qreal mouthY = h * 0.03;
    // 左半边嘴
    painter.drawArc(QRect(-w * 0.04, mouthY - h * 0.02, w * 0.04, h * 0.04),
                    180 * 16, -180 * 8);
    // 右半边嘴
    painter.drawArc(QRect(0, mouthY - h * 0.02, w * 0.04, h * 0.04),
                    0, -180 * 8);

    // === 胡须 ===
    painter.setPen(QPen(QColor(150, 120, 100), 1));
    qreal whiskY = mouthY + h * 0.005;
    // 左边胡须
    painter.drawLine(-w * 0.06, whiskY, -w * 0.02, whiskY);
    painter.drawLine(-w * 0.06, whiskY + h * 0.01, -w * 0.02, whiskY + h * 0.005);
    // 右边胡须
    painter.drawLine(w * 0.02, whiskY, w * 0.06, whiskY);
    painter.drawLine(w * 0.02, whiskY + h * 0.005, w * 0.06, whiskY + h * 0.01);

    // === 腮红 ===
    painter.setBrush(QColor(255, 150, 150, 80));
    painter.setPen(Qt::NoPen);
    painter.drawEllipse(QPointF(-eyeSpacing - eyeR * 0.8, eyeY + eyeR),
                        eyeR * 0.7, eyeR * 0.4);
    painter.drawEllipse(QPointF(eyeSpacing + eyeR * 0.8, eyeY + eyeR),
                        eyeR * 0.7, eyeR * 0.4);

    painter.restore();
}

// === 绘制手部按下效果 ===
void BongoCatWidget::drawHandPressEffect(QPainter &painter, const QRect &catRect,
                                          int scaledW, int scaledH)
{
    qreal timeSec = m_bobPhase;

    // 左手按下
    if (isLeftHandDown()) {
        if (!m_model.leftHandDownImage.isNull()) {
            painter.drawPixmap(catRect, m_model.leftHandDownImage);
        } else {
            int pawW = scaledW / 4;
            int pawH = scaledH / 7;
            int pawX = catRect.x() + catRect.width() * 15 / 100;
            int pawY = catRect.y() + catRect.height() * 65 / 100;

            // 按下的爪子阴影
            painter.setPen(Qt::NoPen);
            painter.setBrush(QColor(0, 0, 0, 30));
            painter.drawEllipse(QRect(pawX + 3, pawY + 5, pawW, pawH));

            // 粉色肉垫
            QRadialGradient pawGrad(pawX + pawW / 2, pawY + pawH / 2, pawW / 2);
            pawGrad.setColorAt(0, QColor(255, 200, 180));
            pawGrad.setColorAt(1, QColor(255, 150, 130));
            painter.setBrush(pawGrad);
            painter.setPen(QPen(QColor(200, 100, 100), 2));
            painter.drawEllipse(QRect(pawX, pawY, pawW, pawH));

            // 手指肉垫
            painter.setBrush(QColor(255, 200, 180));
            painter.setPen(Qt::NoPen);
            for (int i = 0; i < 3; i++) {
                int fx = pawX + pawW * (2 + i * 3) / 10;
                int fy = pawY - pawH / 6;
                painter.drawEllipse(QPoint(fx, fy), pawW / 12, pawH / 12);
            }

            // 按下时的挤压效果(闪烁)
            qreal squeeze = 0.3 + 0.3 * qSin(timeSec * 15.0);
            painter.setBrush(QColor(255, 255, 255, int(squeeze * 100)));
            painter.setPen(Qt::NoPen);
            painter.drawEllipse(QRect(pawX, pawY, pawW, pawH));
        }
    }

    // 右手按下
    if (isRightHandDown()) {
        if (!m_model.rightHandDownImage.isNull()) {
            painter.drawPixmap(catRect, m_model.rightHandDownImage);
        } else {
            int pawW = scaledW / 4;
            int pawH = scaledH / 7;
            int pawX = catRect.x() + catRect.width() * 65 / 100;
            int pawY = catRect.y() + catRect.height() * 65 / 100;

            painter.setPen(Qt::NoPen);
            painter.setBrush(QColor(0, 0, 0, 30));
            painter.drawEllipse(QRect(pawX + 3, pawY + 5, pawW, pawH));

            QRadialGradient pawGrad(pawX + pawW / 2, pawY + pawH / 2, pawW / 2);
            pawGrad.setColorAt(0, QColor(255, 200, 180));
            pawGrad.setColorAt(1, QColor(255, 150, 130));
            painter.setBrush(pawGrad);
            painter.setPen(QPen(QColor(200, 100, 100), 2));
            painter.drawEllipse(QRect(pawX, pawY, pawW, pawH));

            painter.setBrush(QColor(255, 200, 180));
            painter.setPen(Qt::NoPen);
            for (int i = 0; i < 3; i++) {
                int fx = pawX + pawW * (2 + i * 3) / 10;
                int fy = pawY - pawH / 6;
                painter.drawEllipse(QPoint(fx, fy), pawW / 12, pawH / 12);
            }

            qreal squeeze = 0.3 + 0.3 * qSin(timeSec * 15.0);
            painter.setBrush(QColor(255, 255, 255, int(squeeze * 100)));
            painter.setPen(Qt::NoPen);
            painter.drawEllipse(QRect(pawX, pawY, pawW, pawH));
        }
    }
}

// === 按键按下涟漪效果 ===
void BongoCatWidget::drawKeyPressRipple(QPainter &painter, const QRect &catRect,
                                         qreal timeSec)
{
    painter.save();

    int rippleCount = 2;
    qreal basePhase = timeSec * 4.0;

    for (int i = 0; i < rippleCount; i++) {
        qreal phase = basePhase + i * 0.5;
        qreal radius = (fmod(phase, 2.0)) / 2.0; // 0~1 循环
        int rippleR = int(radius * catRect.width() / 3);

        int alpha = int((1.0 - radius) * 50);
        if (alpha <= 0) continue;

        painter.setPen(QPen(QColor(255, 150, 150, alpha), 2));
        painter.setBrush(Qt::NoBrush);
        painter.drawEllipse(catRect.center(), rippleR, rippleR);
    }

    painter.restore();
}

void BongoCatWidget::mouseMoveEvent(QMouseEvent *event)
{
    updateMouseFollow(event->globalPosition().toPoint());
    ComponentBase::mouseMoveEvent(event);
}

void BongoCatWidget::enterEvent(QEnterEvent *event)
{
    Q_UNUSED(event);
}

void BongoCatWidget::leaveEvent(QEvent *event)
{
    Q_UNUSED(event);
}

// =============================================
// L-104: 游戏手柄轮询 (Windows XInput)
// 参考 BongoCat: src-tauri/src/core/gamepad.rs
// =============================================
void BongoCatWidget::pollGamepadState()
{
#ifdef Q_OS_WIN
    // 动态加载 XInput 库（避免链接依赖）
    typedef DWORD(WINAPI *XInputGetStateFn)(DWORD, void *);
    static HMODULE hXInput = nullptr;
    static XInputGetStateFn pXInputGetState = nullptr;
    static bool xinputResolved = false;

    if (!xinputResolved) {
        xinputResolved = true;
        const wchar_t *dllNames[] = {
            L"xinput1_4.dll",  // Windows 8+
            L"xinput1_3.dll",  // DirectX SDK
            L"xinput9_1_0.dll" // Windows Vista+
        };
        for (auto name : dllNames) {
            hXInput = LoadLibraryW(name);
            if (hXInput) {
                pXInputGetState = (XInputGetStateFn)GetProcAddress(hXInput, "XInputGetState");
                if (pXInputGetState) break;
                FreeLibrary(hXInput);
                hXInput = nullptr;
                pXInputGetState = nullptr;
            }
        }
        if (!pXInputGetState) {
            qWarning() << "[BongoCat] XInput not available, gamepad disabled";
            m_gamepadConnected = false;
            return;
        }
    }

    if (!pXInputGetState) { m_gamepadConnected = false; return; }

    // XINPUT_STATE 结构体 (最小子集)
#pragma pack(push, 1)
    struct XINPUT_GAMEPAD_MINI {
        quint16 wButtons;
        quint8  bLeftTrigger;
        quint8  bRightTrigger;
        qint16  sThumbLX;
        qint16  sThumbLY;
        qint16  sThumbRX;
        qint16  sThumbRY;
    };
    struct XINPUT_STATE_MINI {
        quint32             dwPacketNumber;
        XINPUT_GAMEPAD_MINI Gamepad;
    };
#pragma pack(pop)

    XINPUT_STATE_MINI state;
    bool anyConnected = false;

    for (DWORD user = 0; user < 4; user++) {
        DWORD hr = pXInputGetState(user, &state);
        if (hr == 0) { // ERROR_SUCCESS
            anyConnected = true;
            break;
        }
    }
    m_gamepadConnected = anyConnected;
    if (!anyConnected) return;

    const XINPUT_GAMEPAD_MINI &gp = state.Gamepad;

    // === 摇杆归一化: [-32768, 32767] → [-1.0, 1.0] ===
    // 死区 (XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE = 7849)
    const double DEADZONE = 7849.0 / 32768.0;
    auto normalizeAxis = [&](qint16 raw) -> double {
        double v = raw / 32768.0;
        if (v > 1.0) v = 1.0;
        if (v < -1.0) v = -1.0;
        if (qAbs(v) < DEADZONE) return 0.0;
        return v;
    };
    auto normalizeTrigger = [&](quint8 b) -> double {
        if (b < 30) return 0.0; // XINPUT_GAMEPAD_TRIGGER_THRESHOLD
        return b / 255.0;
    };

    double lx = normalizeAxis(gp.sThumbLX);
    double ly = normalizeAxis(gp.sThumbLY);
    double rx = normalizeAxis(gp.sThumbRX);
    double ry = normalizeAxis(gp.sThumbRY);

    // === 摇杆变化 → 传递给 Live2D ===
    const double eps = 0.01;
    if (qAbs(lx - m_lastStickLX) > eps || qAbs(ly - m_lastStickLY) > eps ||
        qAbs(rx - m_lastStickRX) > eps || qAbs(ry - m_lastStickRY) > eps) {
        m_lastStickLX = lx; m_lastStickLY = ly;
        m_lastStickRX = rx; m_lastStickRY = ry;
#ifdef HAS_LIVE2D_SUPPORT
        if (m_useLive2D && m_live2dWidget && m_live2dWidget->isReady()) {
            m_live2dWidget->handleGamepadAxis(lx, ly, rx, ry);
        }
#endif
    }

    // === 按钮变化检测 (按下/松开) ===
    // BongoCat 按钮索引映射 (标准 Xbox 布局):
    // 0=South(A), 1=East(B), 2=West(X), 3=North(Y),
    // 4=LB, 5=RB, 6=LT(按下阈值), 7=RT,
    // 8=Back, 9=Start, 10=LeftThumb, 11=RightThumb
    // 12=D-Up, 13=D-Down, 14=D-Left, 15=D-Right
    static const QVector<QPair<quint16, int>> buttonMap = {
        {0x1000, 0},  // XINPUT_GAMEPAD_A → South(0)
        {0x2000, 1},  // XINPUT_GAMEPAD_B → East(1)
        {0x4000, 2},  // XINPUT_GAMEPAD_X → West(2)
        {0x8000, 3},  // XINPUT_GAMEPAD_Y → North(3)
        {0x0100, 4},  // XINPUT_GAMEPAD_LEFT_SHOULDER → LB(4)
        {0x0200, 5},  // XINPUT_GAMEPAD_RIGHT_SHOULDER → RB(5)
        // LT/RT 用阈值判断
        {0x0010, 8},  // XINPUT_GAMEPAD_BACK → Back(8)
        {0x0020, 9},  // XINPUT_GAMEPAD_START → Start(9)
        {0x0040, 10}, // XINPUT_GAMEPAD_LEFT_THUMB → LeftThumb(10)
        {0x0080, 11}, // XINPUT_GAMEPAD_RIGHT_THUMB → RightThumb(11)
        {0x0001, 12}, // XINPUT_GAMEPAD_DPAD_UP → D-Up(12)
        {0x0002, 13}, // XINPUT_GAMEPAD_DPAD_DOWN → D-Down(13)
        {0x0004, 14}, // XINPUT_GAMEPAD_DPAD_LEFT → D-Left(14)
        {0x0008, 15}, // XINPUT_GAMEPAD_DPAD_RIGHT → D-Right(15)
    };

    quint32 curBtns = gp.wButtons;
    // 扳机键阈值超过也算作"按键"
    if (normalizeTrigger(gp.bLeftTrigger) > 0.5)  curBtns |= 0x00010000u; // bit 16 作为 LT 虚拟位
    if (normalizeTrigger(gp.bRightTrigger) > 0.5) curBtns |= 0x00020000u; // bit 17 作为 RT 虚拟位

    // 触发/松开按钮事件
    auto fireButton = [&](int idx, bool pressed) {
        // 图片模式下用手柄按键也驱动手部
        if (idx == 0 || idx == 2 || idx == 4 || idx == 6 || idx == 12 || idx == 13 || idx == 14) {
            if (pressed) { m_leftHandActive = true; m_leftHandPressTime = m_elapsedTimer.elapsed(); }
        }
        if (idx == 1 || idx == 3 || idx == 5 || idx == 7 || idx == 8 || idx == 9 || idx == 15) {
            if (pressed) { m_rightHandActive = true; m_rightHandPressTime = m_elapsedTimer.elapsed(); }
        }
#ifdef HAS_LIVE2D_SUPPORT
        if (m_useLive2D && m_live2dWidget && m_live2dWidget->isReady()) {
            if (pressed) m_live2dWidget->handleGamepadButtonDown(idx);
            else         m_live2dWidget->handleGamepadButtonUp(idx);
        }
#else
        Q_UNUSED(pressed)
#endif
    };

    // 检查按钮映射表
    for (const auto &item : buttonMap) {
        bool wasDown = (m_lastGamepadButtons & item.first) != 0;
        bool nowDown = (curBtns & item.first) != 0;
        if (wasDown != nowDown) {
            fireButton(item.second, nowDown);
        }
    }
    // 检查虚拟 LT/RT 位
    quint32 virtualMask = 0x00010000u;
    if (((m_lastGamepadButtons & virtualMask) != 0) != ((curBtns & virtualMask) != 0)) {
        fireButton(6, (curBtns & virtualMask) != 0); // LT → idx 6
    }
    virtualMask = 0x00020000u;
    if (((m_lastGamepadButtons & virtualMask) != 0) != ((curBtns & virtualMask) != 0)) {
        fireButton(7, (curBtns & virtualMask) != 0); // RT → idx 7
    }

    m_lastGamepadButtons = curBtns;

#else
    // 非 Windows 平台: 暂不支持手柄
    m_gamepadConnected = false;
#endif
}
