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
// QWebEnginePage 瀛愮被: 灏?JS console.log/warn/error 杞彂鍒?Qt 鏃ュ織
// (javaScriptConsoleMessage 鏄?protected 铏氬嚱鏁帮紝蹇呴』閲嶅啓鎵嶈兘鎷︽埅)
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

// JS 閫氳繃 document.title 鍙戦€佺殑浜嬩欢鍓嶇紑, 鏍煎紡:
//   "__bongocat__:<json-payload>:<timestamp>"
static const QString kTitlePrefix = QStringLiteral("__bongocat__:");

Live2DWidget::Live2DWidget(QWidget *parent)
    : QWidget(parent)
{
    setObjectName("Live2DWidget");
    setAttribute(Qt::WA_TranslucentBackground);
    setAutoFillBackground(false);

    // 閲嶈: 涓嶈浣跨敤 QVBoxLayout!
    // 瀹炴祴鍦ㄥ叏灞€鏍峰紡琛?QStyleSheetStyle)瀛樺湪鏃? 甯﹀竷灞€鐨勭埗鎺т欢浼氳缁樺埗涓嶉€忔槑鑳屾櫙,
    // 瀵艰嚧 WebEngine 椤甸潰閫忔槑鍖哄煙鍦ㄧ獥鍙ｅ悎鎴愭椂鏄剧ず涓虹櫧鑹层€?
    // 鏀圭敤鐩存帴 setGeometry + resizeEvent 淇濇寔瀛愭帶浠跺～婊°€?

#if defined(USE_QT_WEBENGINE)
    // MSVC/Windows: 浣跨敤 QWebEngineView
    m_webView = new QWebEngineView(this);
    m_webView->setStyleSheet("background: transparent;");
    m_webView->setAttribute(Qt::WA_TranslucentBackground);
    m_webView->setAutoFillBackground(false);
    // 绂佺敤 WebEngine 榛樿鍙抽敭鑿滃崟 (杩斿洖/鍒锋柊/鍙﹀瓨涓虹瓑)锛屾瀹犱笉搴斿嚭鐜版祻瑙堝櫒鑿滃崟
    m_webView->setContextMenuPolicy(Qt::NoContextMenu);
    QPalette pal = m_webView->palette();
    pal.setBrush(QPalette::Base, Qt::transparent);
    m_webView->setPalette(pal);

    // 鍚敤閫忔槑鑳屾櫙鍜屽繀瑕佺殑璁剧疆
    QWebEngineSettings *settings = m_webView->settings();
    settings->setAttribute(QWebEngineSettings::ShowScrollBars, false);
    settings->setAttribute(QWebEngineSettings::JavascriptEnabled, true);
    settings->setAttribute(QWebEngineSettings::LocalContentCanAccessRemoteUrls, true);
    settings->setAttribute(QWebEngineSettings::LocalContentCanAccessFileUrls, true);
    settings->setAttribute(QWebEngineSettings::AllowRunningInsecureContent, true);

    // 璁剧疆椤甸潰鑳屾櫙閫忔槑锛屽苟浣跨敤鑷畾涔?Page 杞彂 JS console 娑堟伅
    m_webView->setPage(new ConsoleLoggingPage(m_webView));
    m_webView->page()->setBackgroundColor(Qt::transparent);

    m_webView->setGeometry(0, 0, width(), height());

    // 鐩戝惉 JS 閫氳繃 document.title 鍙戝嚭鐨勪簨浠?
    connect(m_webView, &QWebEngineView::titleChanged,
            this, &Live2DWidget::onWebViewTitleChanged);

    // 椤甸潰鍔犺浇瀹屾垚鍚庢敞鍏?HTML
    connect(m_webView, &QWebEngineView::loadFinished, this, [this](bool ok) {
        if (ok) {
            qDebug() << "Live2D WebEngine page loaded";
            // 鏌愪簺 Qt 鐗堟湰鍦?setHtml/瀵艰埅瀹屾垚鍚庝細鎶婇〉闈㈣儗鏅噸缃负鐧借壊锛?
            // 蹇呴』閲嶆柊搴旂敤閫忔槑鑳屾櫙锛屽惁鍒欐ā鍨嬬殑閫忔槑鍖哄煙浼氭樉绀轰负鐧借壊鍧?
            m_webView->page()->setBackgroundColor(Qt::transparent);
        } else {
            qWarning() << "Live2D WebEngine page failed to load";
        }
    });

    // 鐩存帴 setHtml 鍔犺浇瀹屾暣椤甸潰锛堜笉瑕佸厛 load about:blank锛屽惁鍒欎細浜х敓绔炰簤锛?
    loadCombinedHtml();

#elif defined(USE_QT_WEBVIEW)
    // MinGW: 浣跨敤 QQuickWidget 鍔犺浇 QML WebView
    m_quickWidget = new QQuickWidget(this);
    m_quickWidget->setStyleSheet("background: transparent;");
    m_quickWidget->setAttribute(Qt::WA_TranslucentBackground);
    m_quickWidget->setResizeMode(QQuickWidget::SizeRootObjectToView);

    m_quickWidget->setSource(QUrl("qrc:/resources/live2d-webview.qml"));

    m_quickWidget->setGeometry(0, 0, width(), height());

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

void Live2DWidget::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
#if defined(USE_QT_WEBENGINE)
    if (m_webView)
        m_webView->setGeometry(0, 0, width(), height());
#elif defined(USE_QT_WEBVIEW)
    if (m_quickWidget)
        m_quickWidget->setGeometry(0, 0, width(), height());
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

                // 鐩戝惉 JS 閫氳繃 document.title 鍙戝嚭鐨勪簨浠?
                connect(m_webViewObj, SIGNAL(titleChanged(QString)),
                        this, SLOT(onWebViewTitleChanged(QString)));

                // 鍔犺浇鍚堝苟鍚庣殑 HTML
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
    // 鍙鐞嗗甫鍓嶇紑鐨勪簨浠舵秷鎭? 鍏跺畠 title 鍙樺寲蹇界暐
    if (!title.startsWith(kTitlePrefix)) return;

    // 鏍煎紡: "__bongocat__:<json-payload>:<timestamp>"
    QString payload = title.mid(kTitlePrefix.size());

    // 鍘婚櫎鏈熬鐨?":<timestamp>"
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

    // 鎸変緷璧栭『搴忓唴鑱? pixi.js -> live2d.min.js(Cubism2 杩愯鏃? -> live2dcubismcore.min.js(Cubism4 鏍稿績) -> pixi-live2d-display.js
    // 娉ㄦ剰: pixi-live2d-display 鍦ㄦā鍧楀姞杞芥椂鍗虫鏌?window.Live2DCubismCore锛?
    // 鑻?Cubism 鏍稿績鏅氫簬瀹冨姞杞戒細鐩存帴 throw "Could not find Cubism 4 runtime"锛?
    // 瀵艰嚧 PIXI.live2d.Live2DModel 鏈敞鍐屻€佹ā鍨嬪姞杞芥姤 "reading 'from'" 澶辫触锛?
    // 搴旂敤鍥為€€鍒伴潤鎬佸浘鐗囨ā寮?鐪肩潧/榧犳爣/鍙虫墜鍏ㄩ儴涓嶄細鍔?銆傛牳蹇冨簱蹇呴』鎺掑湪鍓嶉潰銆?
    QStringList inlineScripts;
    inlineScripts << "<script>" << readJs(":/resources/js/pixi.min.js") << "</script>";
    inlineScripts << "<script>" << readJs(":/resources/js/live2d.min.js") << "</script>";
    inlineScripts << "<script>" << readJs(":/resources/js/live2dcubismcore.min.js") << "</script>";
    inlineScripts << "<script>" << readJs(":/resources/js/pixi-live2d-display.js") << "</script>";

    QString allScripts = inlineScripts.join("\n");
    html.replace("/*__INLINE_SCRIPTS__*/", allScripts);

    // baseUrl 浣跨敤鏈湴 HTTP 鏈嶅姟鍣? 纭繚椤甸潰 origin 涓?http://127.0.0.1,
    // 閬垮厤鍥?origin=null 鎴?qrc:/ 瀵艰嚧鐨?CORS 闄愬埗
    quint16 port = LocalFileServer::instance()->port();
    QUrl baseUrl(QStringLiteral("http://127.0.0.1:%1/").arg(port));

#if defined(USE_QT_WEBENGINE)
    // WebEngine 鐩存帴浣跨敤 setHtml
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
