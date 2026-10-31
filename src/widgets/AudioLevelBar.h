#pragma once
// Horizontal audio level meter (OBS mixer style): setLevel takes the
// instantaneous peak (0..1); the displayed bar decays smoothly towards it.
#include <QWidget>

class AudioLevelBar : public QWidget
{
	Q_OBJECT

public:
	AudioLevelBar(QWidget* parent = nullptr);

	void setLevel(float level); // 0..1

protected:
	void paintEvent(QPaintEvent* event) override;

private:
	float m_level = 0.0f; // displayed (decayed) level
};
