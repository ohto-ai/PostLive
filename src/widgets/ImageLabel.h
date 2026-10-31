#pragma once
// QLabel that implements object-fit painting: "cover" crops the image to
// fill the label, "fit" keeps the aspect ratio. Ported from PostLive.Re
// (PostLiveClient/components/ImageLabel).
#include <QLabel>
#include <QPixmap>

class ImageLabel : public QLabel
{
	Q_OBJECT
	Q_PROPERTY(bool covered READ covered WRITE setCovered)

public:
	ImageLabel(QWidget* parent);

	void resizeEvent(QResizeEvent* event) override;
	void paintEvent(QPaintEvent* event) override;

	bool covered() const;
	QPixmap pixmap() const;

public slots:
	void setPixmap(const QPixmap& pixmap);
	void setCovered(bool covered);
	void setMargin(int margin);

private:
	void updateScaledPixmap();

private:
	QPixmap m_pixmap;
	QPixmap m_scaledPixmap;
	QPoint m_pixmapPos;
	Qt::Alignment m_alignment{ Qt::AlignCenter };
	bool m_scaledContents = true;
	bool m_covered = false;
	int m_margin = 0;
};
