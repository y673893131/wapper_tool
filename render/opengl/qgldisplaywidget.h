#ifndef QGLDISPLAYWIDGET_H
#define QGLDISPLAYWIDGET_H

#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QOpenGLShader>
#include <QOpenGLTexture>
#include <QOpenGLBuffer>
#include "video/videoframe.h"

struct _texture_obj_{
    void init(){
        texture = new QOpenGLTexture(QOpenGLTexture::Target2D);
        texture->create();
        id = texture->textureId();
        glBindTexture(GL_TEXTURE_2D, id);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    }

    QOpenGLTexture* texture;
    uint id;
};

class QGLDisplayWidget : public QOpenGLWidget, public QOpenGLFunctions
{
    Q_OBJECT
    enum enum_pragram_attr_index
    {
        ATTR_VERTEX_IN = 0,
        ATTR_TEXTURE_IN,
    };

    enum enum_texture_index
    {
        TEXTURE_Y = 0,
        TEXTURE_U,
        TEXTURE_V,
        TEXTURE_IMG,

        TEXTURE_MAX
    };

public:
    QGLDisplayWidget(QWidget *parent = nullptr);
    virtual ~QGLDisplayWidget();

    void setDisplaySize(int w, int h);
    // QOpenGLWidget interface
signals:
    void modifyFile(const QString&);
    void flushPoint(const QPoint&);
    void playOver();
public slots:
    void setSpeed(int);
    void onAppendFrame(void*);

protected:
    void initializeGL();
    void resizeGL(int w, int h);
    void paintGL();

    void initProgram(const QString&);
private:
    QOpenGLShader* m_vShader,* m_fShader;
    QOpenGLShaderProgram* m_program;
    int m_location[TEXTURE_MAX];
    GLfloat m_vertexVertices[8], m_textureVertices[8];
    _texture_obj_ m_texture[TEXTURE_MAX];
    QPoint m_center;
    _video_frame_* m_pFrame;


    long long m_nStart;
    int m_nDelay;
};

#endif // QGLDISPLAYWIDGET_H
