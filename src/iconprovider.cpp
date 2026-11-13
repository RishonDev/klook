#include "iconprovider.h"

#include <QIcon>

IconProvider::IconProvider()
    : QQuickImageProvider(QQuickImageProvider::Pixmap)
{
}

QPixmap IconProvider::requestPixmap(const QString &id, QSize *size, const QSize &requestedSize)
{
    const QSize target = requestedSize.isValid() ? requestedSize : QSize(256, 256);
    QIcon icon = QIcon::fromTheme(id);
    if (icon.isNull())
        icon = QIcon::fromTheme(QStringLiteral("unknown"));

    const QPixmap pixmap = icon.pixmap(target);
    if (size)
        *size = pixmap.size();
    return pixmap;
}
