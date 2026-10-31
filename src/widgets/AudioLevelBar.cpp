#include "AudioLevelBar.h"

#include <QPainter>
#include <QTimer>

AudioLevelBar::AudioLevelBar(QWidget* parent)
	: QWidget(parent)
{
	setMinimumHeight(14);
	setMaximumHeight(18);

	// Decay ballistics: fall 4% per tick so brief peaks stay visible.
	auto* decayTimer = new QTimer(this);
	connect(decayTimer, &QTimer::timeout, this, [this]
		{
			if (m_level > 0.001f)
			{
				m_level = qMax(0.0f, m_level - 0.04f);
				update();
			}
		});
	decayTimer->start(50);
}

void AudioLevelBar::setLevel(float level)
{
	m_level = qBound(0.0f, level, 1.0f);
	update();
}

void AudioLevelBar::paintEvent(QPaintEvent* event)
{
	QPainter painter(this);
	painter.setRenderHint(QPainter::Antialiasing);

	painter.setPen(Qt::NoPen);
	painter.setBrush(QColor(45, 64, 72));
	painter.drawRoundedRect(rect(), 4, 4);

	const qreal width = qreal(rect().width()) * m_level;
	if (width <= 0)
		return;

	QLinearGradient gradient(0, 0, qreal(rect().width()), 0);
	gradient.setColorAt(0.0, QColor(80, 200, 120));
	gradient.setColorAt(0.7, QColor(220, 200, 80));
	gradient.setColorAt(1.0, QColor(220, 80, 60));
	painter.setBrush(gradient);
	painter.drawRoundedRect(QRectF(0, 0, width, qreal(rect().height())), 4, 4);
}
