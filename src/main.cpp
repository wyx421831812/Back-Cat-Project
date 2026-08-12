#include <QApplication>
#include <QFile>
#include "petwidget.h"
#include "appconfig.h"

int main(int argc, char *argv[])
{
    // 高 DPI 支持
    QApplication::setHighDpiScaleFactorRoundingPolicy(
        Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);
    QApplication app(argc, argv);

    // 设置应用信息
    app.setApplicationName("BackPet");
    app.setApplicationVersion("1.0.0");
    app.setOrganizationName("BackPet");

    // 加载配置
    AppConfig::instance().load();

    // 加载样式表
    QFile styleFile(":/assets/styles.qss");
    if (styleFile.open(QFile::ReadOnly)) {
        QString style = QString::fromUtf8(styleFile.readAll());
        app.setStyleSheet(style);
        styleFile.close();
    }

    // 创建主窗口
    PetWidget w;
    w.show();

    return app.exec();
}