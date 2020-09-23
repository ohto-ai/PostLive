#pragma execution_character_set("utf-8")

#include "LiveLoginUnit.h"


LiveLoginUnit::LiveLoginUnit(QWidget* parent)
    : QMainWindow(parent)
{
    ui.setupUi(this);
    setWindowFlags(Qt::FramelessWindowHint);
    setAttribute(Qt::WA_TranslucentBackground);

    // set tool buttons
    ui.minimizeToolButton->setIcon(style()->standardPixmap(QStyle::SP_TitleBarMinButton));
    ui.closeToolButton->setIcon(style()->standardPixmap(QStyle::SP_TitleBarCloseButton));
    connect(ui.minimizeToolButton, &QToolButton::clicked, this, &QMainWindow::showMinimized);
    connect(ui.closeToolButton, &QToolButton::clicked, this, &QMainWindow::close);

    // test avatar
    ui.avatarLabel->setPixmap(thatboy::utils::roundedPixmap(QPixmap{ "avatar-test.png" }));
    //ui.avatarLabel->setPixmap(roundPixmap(QPixmap{ R"(E:\Documents\Downloads\c180974952931b1433afc32c6e8522cf.jpg)" }));
}

void LiveLoginUnit::mousePressEvent(QMouseEvent* event)
{
    if (Qt::LeftButton == event->button() && !isMaximized())
    {
        pLast = event->globalPos();
        event->ignore();
    }
    mouseIsPressed = true;
}

void LiveLoginUnit::mouseReleaseEvent(QMouseEvent* event)
{
    QApplication::restoreOverrideCursor();
    event->ignore();

    mouseIsPressed = false;

}

void LiveLoginUnit::mouseMoveEvent(QMouseEvent* event)
{
    if (mouseIsPressed && !isMaximized() && (event->buttons() & Qt::LeftButton))
    {
        move(event->globalPos() + pos() - pLast);
        pLast = event->globalPos();
    }
}
