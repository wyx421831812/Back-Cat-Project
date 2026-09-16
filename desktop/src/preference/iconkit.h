#pragma once

#include <QColor>
#include <QIcon>
#include <QPixmap>
#include <QSize>

// SVG 图标工具: 读取 :/assets/icons/<name>.svg, 将单色占位 __COLOR__
// 替换为指定颜色后渲染为位图 (自动适配高 DPI)。
class IconKit
{
public:
    // 以指定逻辑尺寸和颜色渲染
    static QPixmap pixmap(const QString &name, const QSize &size,
                          const QColor &color);
    static QPixmap pixmap(const QString &name, int size, const QColor &color);

    // 生成 QIcon (单态, 调用方自行按主题重建)
    static QIcon icon(const QString &name, const QSize &size,
                      const QColor &color);

    // 全彩图标 (如 logo.svg, 不含 __COLOR__ 占位)
    static QPixmap colorPixmap(const QString &name, const QSize &size);

private:
    IconKit() = delete;
};
