#include "qgldisplaywidget.h"
#include <QDebug>
#include <time.h>
#include <QTimer>

QGLDisplayWidget::QGLDisplayWidget(QWidget *parent)
    :QOpenGLWidget(parent)
    ,m_nDelay(1000)
    ,m_vShader(nullptr),m_fShader(nullptr)
    ,m_program(nullptr),m_pFrame(nullptr)
{
    m_movie = new QMovie();
    m_movie->setCacheMode(QMovie::CacheAll);
    connect(m_movie, &QMovie::frameChanged, this, [=]{ update(); });
}

QGLDisplayWidget::~QGLDisplayWidget()
{
    qDebug() << "gl video widget quit.";
}

void QGLDisplayWidget::setDisplaySize(int w, int h)
{
    resize(w, h);
}

void QGLDisplayWidget::setSpeed(int speed)
{
    if(speed)
        m_nDelay = speed;
}

void QGLDisplayWidget::setSrcFile(const QString& file)
{
    m_movie->stop();
    m_movie->setFileName(file);
    m_movie->start();
}

void QGLDisplayWidget::onAppendFrame(void* frame)
{
    if(m_pFrame) delete m_pFrame;
    m_pFrame = reinterpret_cast<_video_frame_*>(frame);
    update();
}

void QGLDisplayWidget::initializeGL()
{
    initializeOpenGLFunctions();
    glEnable(GL_DEPTH_TEST);

    initProgram(":/custom.fsh");
}

void QGLDisplayWidget::resizeGL(int w, int h)
{
    glViewport(0, 0, w, h);
}

void QGLDisplayWidget::paintGL()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glClearColor(0.0,0.0,0.0, 0);

    auto time = clock() - m_nStart;

    auto frame = m_pFrame;
    if(frame)
    {
        m_program->bind();
#ifdef FRAME_RGB
        glActiveTexture(GL_TEXTURE0 + TEXTURE_IMG);
        glBindTexture(GL_TEXTURE_2D, m_texture[TEXTURE_IMG].id);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, frame->w, frame->h, 0, GL_RGB, GL_UNSIGNED_BYTE, frame->framebuffer);
//        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, m_movie->frameRect().width(), m_movie->frameRect().height(), 0, GL_RGBA, GL_UNSIGNED_BYTE, m_movie->currentImage().rgbSwapped().bits());
        glUniform1i(m_location[TEXTURE_IMG], TEXTURE_IMG);
#else
        for (int index = TEXTURE_Y; index <= TEXTURE_V; ++index) {
            glActiveTexture(static_cast<GLenum>(GL_TEXTURE0 + index));
            glBindTexture(GL_TEXTURE_2D, m_texture[index].id);
            unsigned int w = 0, h = 0;
            auto data = frame->data(index, w, h);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, static_cast<int>(w), static_cast<int>(h), 0, GL_RED, GL_UNSIGNED_BYTE, data);
            glUniform1i(m_location[index], index);
        }
#endif
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        m_program->release();
    }
}

void QGLDisplayWidget::initProgram(const QString & fragment)
{
    if(m_program) {m_program->release(); delete m_program;}
    // program
    if(!m_vShader)
    {
        m_vShader = new QOpenGLShader(QOpenGLShader::Vertex, this);
        auto bCompile = m_vShader->compileSourceFile(":/img.vsh");
        if(!bCompile){
            qWarning() << "vertex compile failed.";
        }
    }

    if(m_fShader) delete m_fShader;
    m_fShader = new QOpenGLShader(QOpenGLShader::Fragment, this);
    auto bCompile = m_fShader->compileSourceFile(fragment);
    if(!bCompile){
        qWarning() <<  "fragment compile failed.";
    }

    m_program = new QOpenGLShaderProgram(this);
    m_program->addShader(m_fShader);
    m_program->addShader(m_vShader);
    m_program->bindAttributeLocation("vertexIn", ATTR_VERTEX_IN);
    m_program->bindAttributeLocation("textureIn", ATTR_TEXTURE_IN);
    m_program->link();

    m_location[TEXTURE_Y] = m_program->uniformLocation("tex_y");
    m_location[TEXTURE_U] = m_program->uniformLocation("tex_u");
    m_location[TEXTURE_V] = m_program->uniformLocation("tex_v");
    m_location[TEXTURE_IMG] = m_program->uniformLocation("tex_img");

    // vertex/texture vertices value
    GLfloat vertexVertices[] = {
        -1.0f, -1.0f,
         1.0f, -1.0f,
         -1.0f, 1.0f,
         1.0f, 1.0f,
    };

    memcpy(m_vertexVertices, vertexVertices, sizeof(vertexVertices));
    glVertexAttribPointer(ATTR_VERTEX_IN, 2, GL_FLOAT, 0, 0, m_vertexVertices);
    glEnableVertexAttribArray(ATTR_VERTEX_IN);

    GLfloat textureVertices[] = {
        0.0f,  1.0f,
        1.0f,  1.0f,
        0.0f,  0.0f,
        1.0f,  0.0f,
    };

    memcpy(m_textureVertices, textureVertices, sizeof(textureVertices));
    glVertexAttribPointer(ATTR_TEXTURE_IN, 2, GL_FLOAT, 0, 0, m_textureVertices);
    glEnableVertexAttribArray(ATTR_TEXTURE_IN);
    // texture obj
    for (int n = 0; n < TEXTURE_MAX; ++n)
        m_texture[n].init();
}
