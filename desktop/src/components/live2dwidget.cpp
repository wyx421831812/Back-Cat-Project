#include "live2dwidget.h"
#include <QVBoxLayout>
#include <QFile>
#include <QCoreApplication>
#include <QDir>

Live2DWidget::Live2DWidget(QWidget *parent)
    : QWidget(parent)
{
    setObjectName("Live2DWidget");
    setAttribute(Qt::WA_TranslucentBackground);
    setAutoFillBackground(false);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    // 创建 WebEngineView
    m_webView = new QWebEngineView(this);
    m_webView->setStyleSheet("background: transparent;");
    m_webView->setAttribute(Qt::WA_TranslucentBackground);
    m_webView->page()->setBackgroundColor(Qt::transparent);
    layout->addWidget(m_webView);

    // 设置 WebChannel
    setupWebChannel();

    // 加载 HTML 页面 (从qrc资源)
    m_webView->load(QUrl("qrc:/live2d-view.html"));

    connect(m_webView, &QWebEngineView::loadFinished, this, [this](bool ok) {
        if (ok) {
            qDebug() << "Live2D HTML page loaded successfully";
        } else {
            qWarning() << "Failed to load Live2D HTML page";
        }
    });
}

Live2DWidget::~Live2DWidget()
{
    if (m_webView) {
        runJavaScript("if(window.Live2DAPI) window.Live2DAPI.destroy();");
    }
}

void Live2DWidget::setupWebChannel()
{
    m_webChannel = new QWebChannel(this);
    m_callbackObject = new JsCallbackObject(this);

    // 注册回调对象
    m_webChannel->registerObject("qt", m_callbackObject);

    // 将 WebChannel 绑定到页面
    m_page = m_webView->page();
    m_page->setWebChannel(m_webChannel);
}

void Live2DWidget::onJavaScriptReady()
{
    m_ready = true;
    emit readyChanged(true);
    qDebug() << "Live2D JavaScript engine is ready";

    // 如果有待加载的模型，立即加载
    if (!m_currentModelPath.isEmpty()) {
        loadModel(m_currentModelPath);
    }
}

void Live2DWidget::onModelLoaded(const QString &modelInfoJson)
{
    QJsonDocument doc = QJsonDocument::fromJson(modelInfoJson.toUtf8());
    if (doc.isObject()) {
        QJsonObject obj = doc.object();
        int width = obj.value("width").toInt();
        int height = obj.value("height").toInt();
        emit modelLoaded(width, height);
        qDebug() << "Live2D model loaded:" << width << "x" << height;
    }
}

void Live2DWidget::onJavaScriptError(const QString &errorMsg)
{
    qWarning() << "Live2D JavaScript error:" << errorMsg;
    emit errorOccurred(errorMsg);
}

bool Live2DWidget::loadModel(const QString &modelPath)
{
    m_currentModelPath = modelPath;

    if (!m_ready) {
        qDebug() << "Live2D engine not ready, will load model when ready";
        return false;
    }

    // 将Windows路径转换为file:// URL
    QString urlPath = QUrl::fromLocalFile(modelPath).toString();

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
    ).arg(x, 0, 'f', 2).arg(y, 0, 'f', 2);

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

void Live2DWidget::resetParameters()
{
    if (!m_ready) return;

    QString jsCode = "if(window.Live2DAPI) { window.Live2DAPI.resetParameters(); }";
    runJavaScript(jsCode);
}

void Live2DWidget::runJavaScript(const QString &code)
{
    if (m_webView) {
        m_webView->page()->runJavaScript(code);
    }
}

// === JsCallbackObject 实现 ===

void Live2DWidget::JsCallbackObject::onReady()
{
    if (m_widget) {
        m_widget->onJavaScriptReady();
    }
}

void Live2DWidget::JsCallbackObject::onModelLoaded(const QString &modelInfoJson)
{
    if (m_widget) {
        m_widget->onModelLoaded(modelInfoJson);
    }
}

void Live2DWidget::JsCallbackObject::onError(const QString &errorMsg)
{
    if (m_widget) {
        m_widget->onJavaScriptError(errorMsg);
    }
}
