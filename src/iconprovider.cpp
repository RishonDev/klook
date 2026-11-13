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

    QPixmap pixmap = icon.pixmap(target);
    // No icon theme at all (minimal systems, headless tests): a transparent
    // placeholder keeps the layout intact instead of a QML load error.
    if (pixmap.isNull()) {
        pixmap = QPixmap(target);
        pixmap.fill(Qt::transparent);
    }
    if (size)
        *size = pixmap.size();
    return pixmap;
}
