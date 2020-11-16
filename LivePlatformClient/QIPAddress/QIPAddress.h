#pragma once


#ifndef QIPADDRESS_H
#define QIPADDRESS_H

/**
 * IP地址输入框控件 作者:feiyangqingyun(QQ:517216493) 2017-8-11
 * 1:可设置IP地址,自动填入框
 * 2:可清空IP地址
 * 3:支持按下小圆点自动切换
 * 4:支持退格键自动切换
 * 5:支持IP地址过滤
 * 6:可设置背景色/边框颜色/边框圆角角度
 */

#include <QLineEdit>
#include <QLabel>

class QIPAddress : public QWidget
{
    Q_OBJECT
	Q_PROPERTY(QString ip READ getIP WRITE setIP)
	Q_PROPERTY(QString bgColor READ getBgColor WRITE setBgColor)
	Q_PROPERTY(QString borderColor READ getBorderColor WRITE setBorderColor)
	Q_PROPERTY(int borderRadius READ getBorderRadius WRITE setBorderRadius)

public:
    explicit QIPAddress(QWidget* parent = 0);

protected:
    bool eventFilter(QObject* watched, QEvent* event);

private:
    QLabel* labDot1;    //第一个小圆点
    QLabel* labDot2;    //第二个小圆点
    QLabel* labDot3;    //第三个小圆点

    QLineEdit* txtIP1;  //IP地址网段输入框1
    QLineEdit* txtIP2;  //IP地址网段输入框2
    QLineEdit* txtIP3;  //IP地址网段输入框3
    QLineEdit* txtIP4;  //IP地址网段输入框4

    QString ip;         //IP地址
    QString bgColor;    //背景颜色
    QString borderColor;//边框颜色
    int borderRadius;   //边框圆角角度


    QFrame* frameIP;

private slots:
    void textChanged(const QString& text);

public:
    //获取IP地址
    QString getIP()                 const;

    QSize sizeHint()                const;
    QSize minimumSizeHint()         const;

public Q_SLOTS:
    //设置IP地址
    void setIP(const QString& ip);
    //清空
    void clear();

    //设置背景颜色
    void setBgColor(const QString& bgColor);
    QString getBgColor()const;
    //设置边框颜色
    void setBorderColor(const QString& borderColor);
    QString getBorderColor()const;
    //设置边框圆角角度
    void setBorderRadius(int borderRadius);
    int getBorderRadius()const;

    void applyStyleSheet();

};

#endif // QIPADDRESS_H
