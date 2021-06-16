#ifndef QRENDERWIDGET_H
#define QRENDERWIDGET_H

#include <QWidget>

class QRenderWidget : public QObject
{
    Q_OBJECT
public:
    explicit QRenderWidget(QWidget *parent = nullptr);

    QWidget* renderWidget();

signals:
    void appendFrame(void*);

public slots:

private:
    QWidget* m_renderWidget;
};

#endif // QRENDERWIDGET_H
