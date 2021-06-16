#include "videocontrol.h"
#include "videoframe.h"
#include <QApplication>
#include <QFileInfo>
#include <thread>
#include <QTime>
#include <QDebug>

QVideoControl::QVideoControl(QObject* parent)
    : QObject(parent)
    , m_index(-1)
    , m_frameCount(0)
    , m_seekPos(0)
{
    qDebug() << qApp->applicationDirPath();
    m_core = new video_player_core(qApp->applicationDirPath().toStdString());
    m_core->_init();
    m_core->_setCallBack(this);

    m_seekTimer = new QTimer;
    m_seekTimer->setInterval(200);
    m_seekTimer->setSingleShot(true);
    connect(m_seekTimer, &QTimer::timeout, this, &QVideoControl::onDoSeekPos, Qt::QueuedConnection);

    m_frameRateTimer = new QTimer;
    m_frameRateTimer->setInterval(1000);
    connect(m_frameRateTimer, &QTimer::timeout, this, &QVideoControl::onStatFrameRate, Qt::QueuedConnection);
    connect(this, &QVideoControl::start, m_frameRateTimer, static_cast<void(QTimer::*)()>(&QTimer::start), Qt::QueuedConnection);
    connect(this, &QVideoControl::end, m_frameRateTimer, &QTimer::stop, Qt::QueuedConnection);
}

void QVideoControl::onStatFrameRate()
{
    emit frameRate(m_frameCount);
    m_frameCount = 0;
}

void QVideoControl::onStart(const QString &filename)
{
    if(filename.isEmpty())
    {
        return ;
    }

    if(m_core && m_index >= 0)
    {
        if(m_core->_getState(m_index) == video_player_core::state_paused)
        {
            qDebug() << "[countinue]";
            m_core->_continue(m_index);
            return;
        }
    }

    waittingStoped();

    auto file = filename.toUtf8().toStdString();
    m_core->_setSrc(file);

    qDebug() << "[play]" << filename;
    m_core->_play();
    ++m_index;
}

void QVideoControl::onStoped()
{
    m_core->_stop(m_index);
}

void QVideoControl::onPause()
{
    m_core->_pause(m_index);
    qDebug() << "[pause]" << m_index;
}

void QVideoControl::onContinue()
{
    m_core->_continue(m_index);
    qDebug() << "[continue]" << m_index;
}

void QVideoControl::onSeekPos(int value)
{
    qDebug() << __FUNCTION__ << value;
    m_core->_seek(m_index, value);
//    m_seekPos = value;
//    m_seekTimer->stop();
//    m_seekTimer->start();
}

void QVideoControl::onDoSeekPos()
{
    m_core->_seek(m_index, m_seekPos);
}

void QVideoControl::onSeekPosImg(int value)
{
    m_core->_get_seek_img(m_index, value);
}

void QVideoControl::onSetVol(int value)
{
    m_core->_setVol(m_index, value);
}

void QVideoControl::onSetMute(bool bMute)
{
    m_core->_setMute(m_index, bMute);
}

void QVideoControl::onActiveChannel(int channel, int index)
{
    m_core->_setStreamChannel(m_index, channel, index);
}

void QVideoControl::onSetDecodeType(int type)
{
    m_core->_setDecodeType(m_index, static_cast<video_player_core::enum_decode_type>(type));
}

void QVideoControl::waittingStoped()
{
    int index = m_index;
    if(index < 0)
        return;
    m_core->_stop(index);
    int state = m_core->_state(index);
    qDebug() << "[waitting] index:" << index << "state:" << state;
    while(state != video_player_core::state_uninit
           && state != video_player_core::state_stopped)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        state = m_core->_state(index);
    }
}

void QVideoControl::totalTime(const int64_t t)
{
    auto tm = QTime::fromMSecsSinceStartOfDay(static_cast<int>(t / 1000));
    qDebug() << "total:" << tm.toString("HH:mm:ss");
    emit total(static_cast<int>(t / 1000));
}

void QVideoControl::posChange(const int64_t t)
{
    emit setPos(static_cast<int>(t));
}

void QVideoControl::setVideoSize(int width, int height)
{
    emit videoSizeChanged(width, height);
}

void QVideoControl::displayCall(void *data, int width, int height)
{
    auto frame = new _video_frame_(reinterpret_cast<unsigned char*>(data), width, height);
    emit appendFrame(frame);
    ++m_frameCount;
}

void QVideoControl::displayStreamChannelInfo(enum_stream_channel channel, const std::vector<_stream_channel_info_ *>& infos, int defalut)
{
    QStringList list;
    int n = 0, select = 0;
    for(auto it : infos)
    {
        if(it->sTitle.empty())
            it->sTitle = tr("default").toStdString();
        if(it->sLanguage.empty())
            it->sLanguage = tr("default").toStdString();
        list << QString("%1 - [%2]").arg(QString::fromUtf8(it->sTitle.c_str())).arg(QString::fromUtf8(it->sLanguage.c_str()));
        if(it->index != defalut)
            ++n;
        else
            select = n;
    }

    emit streamInfo(list, static_cast<int>(channel), select);
}

void QVideoControl::displaySubTitleCall(char * str, unsigned int index)
{
//    qDebug() << QString::fromUtf8(str) << index;
    emit subtitle(QString::fromUtf8(str), index);
}

void QVideoControl::previewDisplayCall(void *data, int width, int height)
{
//    emit m_toolbar->_preview(data, width, height);
}

void QVideoControl::startCall(int index)
{
    qDebug() << "[video] start:" << index;
    emit start(index);
}

void QVideoControl::endCall(int index)
{
    qDebug() << "[video] end:" << index;
    emit end(index);
}
