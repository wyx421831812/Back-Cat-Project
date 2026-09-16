#include "iconkit.h"

#include <QBuffer>
#include <QFile>
#include <QGuiApplication>
#include <QPainter>
#include <QSvgRenderer>

static QByteArray loadSvgData(const QString &name)
{
    QFile f(QStringLiteral(":/assets/icons/%1.svg").arg(name));
    if (!f.open(QIODevice::ReadOnly)) {
        qWarning("IconKit: missing icon %s", qPrintable(name));
        return {};
    }
    return f.readAll();
}

static QPixmap renderSvg(const QByteArray &svgData, const QSize &size,
                         const QColor *color)
{
    if (svgData.isEmpty() || !size.isValid() || size.width() <= 0) {
        return {};
    }

    QByteArray data = svgData;
    if (color) {
        data.replace("__COLOR__", color->name().toLatin1());
    }

    const qreal dpr = qApp->devicePixelRatio();
    QImage image(size * dpr, QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::transparent);
    image.setDevicePixelRatio(dpr);

    QSvgRenderer renderer(data);
    if (!renderer.isValid()) {
        qWarning("IconKit: invalid svg data");
        return {};
    }
    QPainter painter(&image);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
    renderer.render(&painter, QRectF(0, 0, size.width(), size.height()));
    painter.end();

    return QPixmap::fromImage(image);
}

QPixmap IconKit::pixmap(const QString &name, const QSize &size,
                        const QColor &color)
{
    return renderSvg(loadSvgData(name), size, &color);
}

QPixmap IconKit::pixmap(const QString &name, int size, const QColor &color)
{
    return pixmap(name, QSize(size, size), color);
}

QIcon IconKit::icon(const QString &name, const QSize &size, const QColor &color)
{
    return QIcon(pixmap(name, size, color));
}

QPixmap IconKit::colorPixmap(const QString &name, const QSize &size)
{
    return renderSvg(loadSvgData(name), size, nullptr);
}
