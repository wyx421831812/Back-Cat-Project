#include "bongocatwidget.h"
#include <QPainter>
#include <QPaintEvent>
#include <QMouseEvent>
#include <QGuiApplication>
#include <QScreen>
#include <QCursor>
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
        m_live2dWidget->show();

        // 连接信号
        connect(m_live2dWidget, &Live2DWidget::errorOccurred,
                this, [this](const QString &err) {
            qWarning() << "Live2D error:" << err;
            // 出错时回退到静态图片
            switchToStaticImage();
        });

        // 初始时隐藏，直到模型加载完成
        m_live2dWidget->hide();
    }

    // 加载Live2D模型
    if (!m_model.live2dModelFile.isEmpty()) {
        m_live2dWidget->loadModel(m_model.live2dModelFile);
        m_live2dWidget->show();
        m_useLive2D = true;
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
    // 功能键
    switch (vkCode) {
    case VK_SPACE: return "Space";
    case VK_RETURN: return "Return";
    case VK_SHIFT: return "Shift";
    case VK_CONTROL: return "Control";
    case VK_MENU: return "Alt";
    case VK_TAB: return "Tab";
    case VK_BACK: return "Backspace";
    case VK_ESCAPE: return "Escape";
    case VK_CAPITAL: return "CapsLock";
    case VK_LEFT: return "Left";
    case VK_RIGHT: return "Right";
    case VK_UP: return "Up";
    case VK_DOWN: return "Down";
    case VK_LWIN:
    case VK_RWIN: return "Meta";
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
            m_live2dWidget->handleMouseDown(button);
        }
#endif
    }

    update();
}

void BongoCatWidget::handleMouseUp(bool isLeft)
{
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

    // 手部状态在按下后保持一小段时间，产生动画效果
    qint64 handHoldMs = 100;
    if (!leftPressed && m_leftHandActive && (now - m_leftHandPressTime > handHoldMs)) {
        m_leftHandActive = false;
    }
    if (!rightPressed && m_rightHandActive && (now - m_rightHandPressTime > handHoldMs)) {
        m_rightHandActive = false;
    }

    // 更新鼠标位置
    updateMouseFollow(QCursor::pos());

    // 将输入状态传递给Live2D (如果正在使用)
    if (m_useLive2D) {
#ifdef HAS_LIVE2D_SUPPORT
        if (m_live2dWidget && m_live2dWidget->isReady()) {
            // 传递鼠标移动
            QPoint globalPos = QCursor::pos();
            QScreen *screen = QGuiApplication::screenAt(globalPos);
            if (screen) {
                QRect geo = screen->geometry();
                double nx = (globalPos.x() - geo.x()) / (double)geo.width();
                double ny = (globalPos.y() - geo.y()) / (double)geo.height();
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

void BongoCatWidget::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::TextAntialiasing);

    QRect targetRect = rect();
    painter.setCompositionMode(QPainter::CompositionMode_SourceOver);

    // === 计算呼吸/浮动参数 ===
    qreal timeSec = m_bobPhase;
    qreal bobOffset = qSin(timeSec * 2.0) * 2.5;
    qreal breatheScale = 1.0 + qSin(timeSec * 1.5) * 0.02; // 呼吸缩放
    int offsetX = static_cast<int>(m_mouseOffset.x() * 5.0);
    int offsetY = static_cast<int>(m_mouseOffset.y() * 5.0 + bobOffset);

    // 按下时的抖动
    if (!m_pressedKeys.isEmpty() || m_leftMouseDown || m_rightMouseDown) {
        offsetX += qSin(timeSec * 25.0) * 1.5;
        offsetY += qCos(timeSec * 22.0) * 1.0;
    }

    // === 第一层: 键盘背景布局 ===
    if (!m_model.backgroundImage.isNull()) {
        painter.drawPixmap(targetRect,
                           m_model.backgroundImage.scaled(
                               targetRect.size(),
                               Qt::IgnoreAspectRatio,
                               Qt::SmoothTransformation));
    } else {
        // 绘制简洁的渐变背景
        QLinearGradient bgGrad(targetRect.topLeft(), targetRect.bottomRight());
        bgGrad.setColorAt(0, QColor(255, 248, 220));
        bgGrad.setColorAt(1, QColor(255, 240, 200));
        painter.fillRect(targetRect, bgGrad);
    }

    // === 第二层: 绘制按键高亮(如果有背景图) ===
    if (!m_model.backgroundImage.isNull() && !m_pressedKeys.isEmpty()) {
        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor(255, 100, 100, 60));

        for (const QString &keyName : m_pressedKeys) {
            // 在背景图上查找按键位置并高亮
            auto it = m_model.keyImages.find(keyName);
            if (it != m_model.keyImages.end() && !it.value().isNull()) {
                // 按键图片叠加
                painter.setOpacity(0.5 + 0.5 * qSin(timeSec * 8.0));
                painter.drawPixmap(targetRect, it.value());
                painter.setOpacity(1.0);
            }
        }
    }

    // === 计算猫咪绘制区域 ===
    QSize catSize = m_model.coverImage.isNull()
                    ? QSize(targetRect.width() * 3 / 4, targetRect.height() * 3 / 4)
                    : m_model.coverImage.size();

    // 应用呼吸缩放
    int scaledW = static_cast<int>(catSize.scaled(targetRect.size(), Qt::KeepAspectRatio).width() * breatheScale);
    int scaledH = static_cast<int>(catSize.scaled(targetRect.size(), Qt::KeepAspectRatio).height() * breatheScale);

    QRect catRect(
        (targetRect.width() - scaledW) / 2 + offsetX,
        (targetRect.height() - scaledH) / 2 + offsetY,
        scaledW,
        scaledH
    );

    // === 第三层: 猫咪主体 ===
    if (!m_model.coverImage.isNull()) {
        painter.drawPixmap(catRect, m_model.coverImage);
    } else {
        // 绘制可爱的默认猫咪 (使用QPainter矢量绘制)
        drawDefaultCat(painter, catRect);
    }

    // === 第四层: 按下的按键图片叠加 ===
    for (const QString &keyName : m_pressedKeys) {
        QPixmap keyPix = m_model.getKeyImage(keyName);
        if (!keyPix.isNull()) {
            // 带脉冲效果的按键叠加
            qreal pulse = 0.7 + 0.3 * qSin(timeSec * 10.0);
            painter.setOpacity(pulse);
            painter.drawPixmap(catRect, keyPix);
            painter.setOpacity(1.0);
        }
    }

    // === 第五层: 手部按下动画 ===
    drawHandPressEffect(painter, catRect, scaledW, scaledH);

    // === 第六层: 按键涟漪/冲击波效果 ===
    if (!m_pressedKeys.isEmpty()) {
        drawKeyPressRipple(painter, catRect, timeSec);
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
