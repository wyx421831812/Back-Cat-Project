#include "live2dwidget.h"
#include "localfileserver.h"
#include <QVBoxLayout>
#include <QFile>
#include <QCoreApplication>
#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#if defined(USE_QT_WEBENGINE)
#include <QWebEngineSettings>
#include <QWebEnginePage>
#elif defined(USE_QT_WEBVIEW)
#include <QQuickItem>
#endif

#if defined(USE_QT_WEBENGINE)
// QWebEnginePage 子类: 将 JS console.log/warn/error 转发到 Qt 日志
// (javaScriptConsoleMessage 是 protected 虚函数，必须重写才能拦截)
class ConsoleLoggingPage : public QWebEnginePage
{
public:
    explicit ConsoleLoggingPage(QObject *parent = nullptr)
        : QWebEnginePage(parent) {}

protected:
    void javaScriptConsoleMessage(JavaScriptConsoleMessageLevel level,
                                  const QString &message,
                                  int lineNumber,
                                  const QString &sourceID) override
    {
        const QString src = sourceID.isEmpty()
            ? QString()
            : QStringLiteral(" (%1:%2)").arg(sourceID).arg(lineNumber);
        switch (level) {
        case InfoMessageLevel:
            qDebug().noquote() << "[JS]" << message << src;
            break;
        case WarningMessageLevel:
            qWarning().noquote() << "[JS WARN]" << message << src;
            break;
        case ErrorMessageLevel:
            qWarning().noquote() << "[JS ERROR]" << message << src;
            break;
        }
    }
};
#endif

// JS 通过 document.title 发送的事件前缀, 格式:
//   "__bongocat__:<json-payload>:<timestamp>"
static const QString kTitlePrefix = QStringLiteral("__bongocat__:");

Live2DWidget::Live2DWidget(QWidget *parent)
    : QWidget(parent)
{
    setObjectName("Live2DWidget");
    setAttribute(Qt::WA_TranslucentBackground);
    setAutoFillBackground(false);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

#if defined(USE_QT_WEBENGINE)
    // MSVC/Windows: 使用 QWebEngineView
    m_webView = new QWebEngineView(this);
    m_webView->setStyleSheet("background: transparent;");
    m_webView->setAttribute(Qt::WA_TranslucentBackground);
    m_webView->setAutoFillBackground(false);
    QPalette pal = m_webView->palette();
    pal.setBrush(QPalette::Base, Qt::transparent);
    m_webView->setPalette(pal);

    // 启用透明背景和必要的设置
    QWebEngineSettings *settings = m_webView->settings();
    settings->setAttribute(QWebEngineSettings::ShowScrollBars, false);
    settings->setAttribute(QWebEngineSettings::JavascriptEnabled, true);
    settings->setAttribute(QWebEngineSettings::LocalContentCanAccessRemoteUrls, true);
    settings->setAttribute(QWebEngineSettings::LocalContentCanAccessFileUrls, true);
    settings->setAttribute(QWebEngineSettings::AllowRunningInsecureContent, true);

    // 设置页面背景透明，并使用自定义 Page 转发 JS console 消息
    m_webView->setPage(new ConsoleLoggingPage(m_webView));
    m_webView->page()->setBackgroundColor(Qt::transparent);

    layout->addWidget(m_webView);

    // 监听 JS 通过 document.title 发出的事件
    connect(m_webView, &QWebEngineView::titleChanged,
            this, &Live2DWidget::onWebViewTitleChanged);

    // 页面加载完成后注入 HTML
    connect(m_webView, &QWebEngineView::loadFinished, this, [this](bool ok) {
        if (ok) {
            qDebug() << "Live2D WebEngine page loaded";
        } else {
            qWarning() << "Live2D WebEngine page failed to load";
        }
    });

    // 直接 setHtml 加载完整页面（不要先 load about:blank，否则会产生竞争）
    loadCombinedHtml();

#elif defined(USE_QT_WEBVIEW)
    // MinGW: 使用 QQuickWidget 加载 QML WebView
    m_quickWidget = new QQuickWidget(this);
    m_quickWidget->setStyleSheet("background: transparent;");
    m_quickWidget->setAttribute(Qt::WA_TranslucentBackground);
    m_quickWidget->setResizeMode(QQuickWidget::SizeRootObjectToView);

    m_quickWidget->setSource(QUrl("qrc:/resources/live2d-webview.qml"));

    layout->addWidget(m_quickWidget);

    connect(m_quickWidget, &QQuickWidget::statusChanged,
            this, &Live2DWidget::onQuickWidgetStatusChanged);
#endif
}

Live2DWidget::~Live2DWidget()
{
#if defined(USE_QT_WEBENGINE)
    if (m_webView) {
        runJavaScript("if(window.Live2DAPI) window.Live2DAPI.destroy();");
    }
#elif defined(USE_QT_WEBVIEW)
    if (m_webViewObj) {
        runJavaScript("if(window.Live2DAPI) window.Live2DAPI.destroy();");
    }
#endif
}

#if defined(USE_QT_WEBVIEW)
void Live2DWidget::onQuickWidgetStatusChanged(QQuickWidget::Status status)
{
    if (status == QQuickWidget::Ready) {
        qDebug() << "Live2D QML loaded successfully";

        QObject *rootObj = m_quickWidget->rootObject();
        if (rootObj) {
            m_webViewObj = rootObj->findChild<QObject*>("live2dWebView");
            if (m_webViewObj) {
                qDebug() << "Found QML WebView object";

                // 监听 JS 通过 document.title 发出的事件
                connect(m_webViewObj, SIGNAL(titleChanged(QString)),
                        this, SLOT(onWebViewTitleChanged(QString)));

                // 加载合并后的 HTML
                loadCombinedHtml();
            } else {
                qWarning() << "Could not find live2dWebView in QML";
            }
        }
    } else if (status == QQuickWidget::Error) {
        qWarning() << "Live2D QML failed to load";
    }
}
#endif

void Live2DWidget::onWebViewTitleChanged(const QString &title)
{
    // 只处理带前缀的事件消息, 其它 title 变化忽略
    if (!title.startsWith(kTitlePrefix)) return;

    // 格式: "__bongocat__:<json-payload>:<timestamp>"
    QString payload = title.mid(kTitlePrefix.size());

    // 去除末尾的 ":<timestamp>"
    int lastColon = payload.lastIndexOf(':');
    if (lastColon > 0) {
        payload.chop(payload.size() - lastColon);
    }

    QJsonParseError parseErr;
    QJsonDocument doc = QJsonDocument::fromJson(payload.toUtf8(), &parseErr);
    if (parseErr.error != QJsonParseError::NoError || !doc.isObject()) {
        qWarning() << "[Live2D bridge] failed to parse title payload:" << payload
                   << "err:" << parseErr.errorString();
        return;
    }

    QJsonObject obj = doc.object();
    QString eventName = obj.value("event").toString();
    QJsonObject data = obj.value("data").toObject();
    handleJsEvent(eventName, data);
}

void Live2DWidget::handleJsEvent(const QString &eventName, const QJsonObject &data)
{
    if (eventName == QLatin1String("ready")) {
        qDebug() << "[Live2D bridge] JS ready";
        setReady(true);
        if (!m_currentModelPath.isEmpty()) {
            loadModel(m_currentModelPath);
        }
    } else if (eventName == QLatin1String("modelLoaded")) {
        int width = data.value("width").toInt();
        int height = data.value("height").toInt();
        QString mode = data.value("mode").toString();
        qDebug() << "Live2D model loaded:" << width << "x" << height << "mode:" << mode;
        emit modelLoaded(width, height);
    } else if (eventName == QLatin1String("error")) {
        QString msg = data.value("message").toString();
        qWarning() << "Live2D JavaScript error:" << msg;
        emit errorOccurred(msg);
    } else {
        qWarning() << "[Live2D bridge] unknown event:" << eventName;
    }
}

void Live2DWidget::setReady(bool ready)
{
    if (m_ready == ready) return;
    m_ready = ready;
    emit readyChanged(m_ready);
}

void Live2DWidget::loadCombinedHtml()
{
    QFile htmlFile(":/resources/live2d-view.html");
    if (!htmlFile.open(QIODevice::ReadOnly)) {
        qWarning() << "Failed to load live2d-view.html from resources";
        return;
    }
    QString html = QString::fromUtf8(htmlFile.readAll());

    auto readJs = [](const QString &path) -> QString {
        QFile f(path);
        if (!f.open(QIODevice::ReadOnly)) {
            qWarning() << "Failed to load JS from" << path;
            return QString();
        }
        return QString::fromUtf8(f.readAll());
    };

    // 按依赖顺序内联: pixi.js -> pixi-live2d-display.js -> live2d.min.js -> live2dcubismcore.min.js
    QStringList inlineScripts;
    inlineScripts << "<script>" << readJs(":/resources/js/pixi.min.js") << "</script>";
    inlineScripts << "<script>" << readJs(":/resources/js/pixi-live2d-display.js") << "</script>";
    inlineScripts << "<script>" << readJs(":/resources/js/live2d.min.js") << "</script>";
    inlineScripts << "<script>" << readJs(":/resources/js/live2dcubismcore.min.js") << "</script>";

    QString allScripts = inlineScripts.join("\n");
    html.replace("/*__INLINE_SCRIPTS__*/", allScripts);

    // baseUrl 使用本地 HTTP 服务器, 确保页面 origin 为 http://127.0.0.1,
    // 避免因 origin=null 或 qrc:/ 导致的 CORS 限制
    quint16 port = LocalFileServer::instance()->port();
    QUrl baseUrl(QStringLiteral("http://127.0.0.1:%1/").arg(port));

#if defined(USE_QT_WEBENGINE)
    // WebEngine 直接使用 setHtml
    m_webView->setHtml(html, baseUrl);
#elif defined(USE_QT_WEBVIEW)
    if (!m_webViewObj) return;
    QMetaObject::invokeMethod(m_webViewObj, "loadHtml",
                              Q_ARG(QString, html),
                              Q_ARG(QUrl, baseUrl));
#endif

    qDebug() << "Combined HTML with inline JS loaded ("
             << allScripts.size() << "bytes of JS)";
}

bool Live2DWidget::loadModel(const QString &modelPath)
{
    m_currentModelPath = modelPath;

    if (!m_ready) {
        qDebug() << "Live2D engine not ready, will load model when ready";
        return false;
    }

    QString urlPath = LocalFileServer::instance()->toUrl(modelPath);

    QString jsCode = QString(
        "if(window.Live2DAPI) {"
        "    window.Live2DAPI.loadModel('%1');"
        "}"
    ).arg(urlPath);

    runJavaScript(jsCode);
    return true;
}

bool Live2DWidget::playMotion(const QString &group, int index)
{
    if (!m_ready) return false;

    QString jsCode = QString(
        "if(window.Live2DAPI) {"
        "    window.Live2DAPI.playMotion('%1', %2);"
        "}"
    ).arg(group).arg(index);

    runJavaScript(jsCode);
    return true;
}

bool Live2DWidget::playExpression(int index)
{
    if (!m_ready) return false;

    QString jsCode = QString(
        "if(window.Live2DAPI) {"
        "    window.Live2DAPI.playExpression(%1);"
        "}"
    ).arg(index);

    runJavaScript(jsCode);
    return true;
}

bool Live2DWidget::setParameter(const QString &paramId, double value)
{
    if (!m_ready) return false;

    QString jsCode = QString(
        "if(window.Live2DAPI) {"
        "    window.Live2DAPI.setParameter('%1', %2);"
        "}"
    ).arg(paramId).arg(value, 0, 'f', 6);

    runJavaScript(jsCode);
    return true;
}

void Live2DWidget::handleKeyDown(const QString &keyName)
{
    if (!m_ready) return;

    QString jsCode = QString(
        "if(window.Live2DAPI) {"
        "    window.Live2DAPI.handleKeyDown('%1');"
        "}"
    ).arg(keyName);

    runJavaScript(jsCode);
}

void Live2DWidget::handleKeyUp(const QString &keyName)
{
    if (!m_ready) return;

    QString jsCode = QString(
        "if(window.Live2DAPI) {"
        "    window.Live2DAPI.handleKeyUp('%1');"
        "}"
    ).arg(keyName);

    runJavaScript(jsCode);
}

void Live2DWidget::handleMouseMove(double x, double y)
{
    if (!m_ready) return;

    QString jsCode = QString(
        "if(window.Live2DAPI) {"
        "    window.Live2DAPI.handleMouseMove(%1, %2);"
        "}"
    ).arg(x, 0, 'f', 4).arg(y, 0, 'f', 4);

    runJavaScript(jsCode);
}

void Live2DWidget::handleMouseDown(int button)
{
    if (!m_ready) return;

    QString jsCode = QString(
        "if(window.Live2DAPI) {"
        "    window.Live2DAPI.handleMouseDown(%1);"
        "}"
    ).arg(button);

    runJavaScript(jsCode);
}

void Live2DWidget::handleMouseUp(int button)
{
    if (!m_ready) return;

    QString jsCode = QString(
        "if(window.Live2DAPI) {"
        "    window.Live2DAPI.handleMouseUp(%1);"
        "}"
    ).arg(button);

    runJavaScript(jsCode);
}

void Live2DWidget::handleGamepadAxis(double leftX, double leftY, double rightX, double rightY)
{
    if (!m_ready) return;

    QString jsCode = QString(
        "if(window.Live2DAPI) {"
        "    window.Live2DAPI.handleGamepadAxis(%1, %2, %3, %4);"
        "}"
    ).arg(leftX, 0, 'f', 4).arg(leftY, 0, 'f', 4)
     .arg(rightX, 0, 'f', 4).arg(rightY, 0, 'f', 4);

    runJavaScript(jsCode);
}

void Live2DWidget::handleGamepadButtonDown(int button)
{
    if (!m_ready) return;

    QString jsCode = QString(
        "if(window.Live2DAPI) {"
        "    window.Live2DAPI.handleGamepadButtonDown(%1);"
        "}"
    ).arg(button);

    runJavaScript(jsCode);
}

void Live2DWidget::handleGamepadButtonUp(int button)
{
    if (!m_ready) return;

    QString jsCode = QString(
        "if(window.Live2DAPI) {"
        "    window.Live2DAPI.handleGamepadButtonUp(%1);"
        "}"
    ).arg(button);

    runJavaScript(jsCode);
}

void Live2DWidget::resetParameters()
{
    if (!m_ready) return;

    QString jsCode = "if(window.Live2DAPI) { window.Live2DAPI.resetParameters(); }";
    runJavaScript(jsCode);
}

void Live2DWidget::setAutoReleaseDelay(int ms)
{
    if (!m_ready) return;

    QString jsCode = QString(
        "if(window.Live2DAPI) { window.Live2DAPI.setAutoReleaseDelay(%1); }"
    ).arg(ms);

    runJavaScript(jsCode);
}

static QString toFileUrl(const QString &localPath)
{
    if (localPath.isEmpty()) return QString();
    if (localPath.startsWith("http:") || localPath.startsWith("qrc:")) return localPath;
    return LocalFileServer::instance()->toUrl(localPath);
}

void Live2DWidget::setBackgroundImage(const QString &path)
{
    if (!m_ready) return;
    QString url = toFileUrl(path);
    if (url.isEmpty()) return;
    QString jsCode = QString(
        "if(window.Live2DAPI) { window.Live2DAPI.setBackground('%1'); }"
    ).arg(url.replace("'", "\\'"));
    runJavaScript(jsCode);
}

void Live2DWidget::setKeyImage(const QString &keyName, const QString &path)
{
    if (!m_ready || keyName.isEmpty()) return;
    QString url = toFileUrl(path);
    QString jsCode;
    if (url.isEmpty()) {
        jsCode = QString(
            "if(window.Live2DAPI) { delete window.Live2DAPI._keyImages['%1']; }"
        ).arg(keyName);
    } else {
        jsCode = QString(
            "if(window.Live2DAPI) { window.Live2DAPI._keyImages['%1'] = '%2'; }"
        ).arg(keyName).arg(url.replace("'", "\\'"));
    }
    runJavaScript(jsCode);
}

void Live2DWidget::clearKeyImage(const QString &keyName)
{
    if (!m_ready) return;
    QString jsCode = QString(
        "if(window.Live2DAPI) { delete window.Live2DAPI._keyImages['%1']; }"
    ).arg(keyName);
    runJavaScript(jsCode);
}

void Live2DWidget::clearAllKeyImages()
{
    if (!m_ready) return;
    runJavaScript("if(window.Live2DAPI) { window.Live2DAPI._keyImages = {}; }");
}

void Live2DWidget::runJavaScript(const QString &code)
{
#if defined(USE_QT_WEBENGINE)
    if (!m_webView) {
        qWarning() << "Cannot run JavaScript: WebEngine view not available";
        return;
    }
    m_webView->page()->runJavaScript(code);
#elif defined(USE_QT_WEBVIEW)
    if (!m_webViewObj) {
        qWarning() << "Cannot run JavaScript: WebView not available";
        return;
    }

    bool ok = QMetaObject::invokeMethod(
        m_webViewObj,
        "runJavaScript",
        Qt::QueuedConnection,
        Q_ARG(QString, code)
    );

    if (!ok) {
        qWarning() << "Failed to invoke runJavaScript on WebView";
    }
#endif
}
