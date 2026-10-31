#include "ImageLabel.h"

#include <QPainter>
#include <QResizeEvent>

ImageLabel::ImageLabel(QWidget* parent)
	: QLabel(parent)
{
}

void ImageLabel::resizeEvent(QResizeEvent* event)
{
	QWidget::resizeEvent(event);
	if (m_pixmap.isNull())
		return;
	updateScaledPixmap();
}

void ImageLabel::paintEvent(QPaintEvent* event)
{
	QWidget::paintEvent(event);
	if (m_scaledPixmap.isNull())
		return;
	QPainter painter(this);
	painter.drawPixmap(m_pixmapPos, m_scaledPixmap);
}

bool ImageLabel::covered() const
{
	return m_covered;
}

QPixmap ImageLabel::pixmap() const
{
	return m_pixmap;
}

void ImageLabel::setPixmap(const QPixmap& pixmap)
{
	m_pixmap = pixmap;
	updateScaledPixmap();
}

void ImageLabel::setCovered(bool covered)
{
	m_covered = covered;
	updateScaledPixmap();
}

void ImageLabel::setMargin(int margin)
{
	m_margin = margin;
	updateScaledPixmap();
}

void ImageLabel::updateScaledPixmap()
{
	// 根据 alignment、pixmap 尺寸以及窗口尺寸，计算缩放后的 pixmap 与位置
	if (m_pixmap.isNull())
		return;

	QSize pixmapSize = m_pixmap.size();
	QSize labelSize = size();
	labelSize.setWidth(labelSize.width() - 2 * m_margin);
	labelSize.setHeight(labelSize.height() - 2 * m_margin);
	if (labelSize.width() <= 0 || labelSize.height() <= 0)
		return;

	if (m_scaledContents)
	{
		if (m_covered)
		{
			// 保持比例，使图片完全覆盖 label（object-fit: cover）
			if (pixmapSize.width() * labelSize.height() > pixmapSize.height() * labelSize.width())
				m_scaledPixmap = m_pixmap.scaledToHeight(labelSize.height(), Qt::SmoothTransformation);
			else
				m_scaledPixmap = m_pixmap.scaledToWidth(labelSize.width(), Qt::SmoothTransformation);
		}
		else
		{
			m_scaledPixmap = m_pixmap.scaled(labelSize, Qt::KeepAspectRatio);
		}
	}
	else
	{
		m_scaledPixmap = m_pixmap;
	}

	if (m_alignment & Qt::AlignLeft)
		m_pixmapPos.setX(0);
	else if (m_alignment & Qt::AlignRight)
		m_pixmapPos.setX(labelSize.width() - m_scaledPixmap.width());
	else
		m_pixmapPos.setX((labelSize.width() - m_scaledPixmap.width()) / 2);

	if (m_alignment & Qt::AlignTop)
		m_pixmapPos.setY(0);
	else if (m_alignment & Qt::AlignBottom)
		m_pixmapPos.setY(labelSize.height() - m_scaledPixmap.height());
	else
		m_pixmapPos.setY((labelSize.height() - m_scaledPixmap.height()) / 2);

	m_pixmapPos += QPoint(m_margin, m_margin);
	update();
}
