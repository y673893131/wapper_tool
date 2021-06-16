#include "qrenderwidget.h"
#include "opengl\qgldisplaywidget.h"

QRenderWidget::QRenderWidget(QWidget *parent) : QObject(parent)
{
    auto gl = new QGLDisplayWidget(parent);
    connect(this, &QRenderWidget::appendFrame, gl, &QGLDisplayWidget::onAppendFrame);

    m_renderWidget = gl;
}

QWidget* QRenderWidget::renderWidget()
{
    return m_renderWidget;
}
