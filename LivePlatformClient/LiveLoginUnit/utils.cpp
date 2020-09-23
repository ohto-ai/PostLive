#include "utils.h"

QPixmap thatboy::utils::roundedPixmap(const QPixmap& src)
{
    if (src.isNull()) {
        return QPixmap();
    }
    int radius = std::max(src.width(), src.height());
    QSize size(2 * radius, 2 * radius);
    QBitmap mask(size);
    QPainter painter(&mask);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);
    painter.fillRect(0, 0, size.width(), size.height(), Qt::white);
    painter.setBrush(QColor(0, 0, 0));
    painter.drawRoundedRect(0, 0, size.width(), size.height(), radius * 2 - 1, radius * 2 - 1);
    QPixmap image = src.scaled(size);
    image.setMask(mask);
    return image;
}
