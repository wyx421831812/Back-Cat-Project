#include <QApplication>
#include <QFile>
#ifdef USE_QT_WEBVIEW
#include <QtWebView>
#endif
#include "petwidget.h"
#include "appconfig.h"
#include "components/localfileserver.h"
#include "preference/thememanager.h"
#include "preference/preferencewindow.h"
#include "preference/pages/catpage.h"

int main(int argc, char *argv[])
{
    // 高 DPI 支持
    QApplication::setHighDpiScaleFactorRoundingPolicy(
        Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);
    QApplication app(argc, argv);

#ifdef USE_QT_WEBVIEW
    // 初始化 Qt WebView (Windows 上加载 WebView2 后端，必须在使用 WebView 前调用)
    // WebEngine 路径下不需要此调用
    QtWebView::initialize();
#endif

    // 启动本地文件服务器 (解决 WebView2 下 file:/// 被 CORS 阻止的问题)
    LocalFileServer::instance()->start();

    // 设置应用信息
    app.setApplicationName("BackPet");
    app.setApplicationVersion("0.1.0");
    app.setOrganizationName("BackPet");

    // 加载配置
    AppConfig::instance().load();

    // 应用主题 (light/dark/auto, 默认 auto)
    ThemeManager::instance()->applyFromConfig(AppConfig::instance().themeMode());

    // 装配偏好设置页
    PreferenceWindow::instance()->setPageWidget(PreferenceWindow::CatPage, new CatPage);

    // 创建主窗口
    PetWidget w;
    w.show();

    return app.exec();
}
