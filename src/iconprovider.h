#pragma once

#include <QQuickImageProvider>

// Serves freedesktop theme icons to QML as image://icon/<icon-name>.
class IconProvider : public QQuickImageProvider
{
public:
    IconProvider();
    QPixmap requestPixmap(const QString &id, QSize *size, const QSize &requestedSize) override;
};
