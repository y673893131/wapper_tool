#include "widget.h"
#include <Windows.h>
#include <QDebug>
#include <QAction>
#include <QSystemTrayIcon>
#include <QStyle>
#include <QMenu>
#include <QApplication>
#include <QDesktopWidget>
#include <QFileDialog>
#include <QStandardPaths>
#include <QActionGroup>
#include <QTimer>
#include <QLabel>
#include <QMovie>
#include <QKeyEvent>
#include <QWidgetAction>
#include <QSlider>
#include "render/qrenderwidget.h"
#include "video/videocontrol.h"
#include "qdatamodel.h"

HHOOK mouseHook=nullptr;
HHOOK keyHook=nullptr;
LRESULT CALLBACK mouseProc(int nCode,WPARAM wParam,LPARAM lParam)
{
    if(nCode == HC_ACTION)
    {
        switch (wParam) {
        case WM_LBUTTONDOWN:
        {
            POINT pt;
            ::GetCursorPos(&pt);
            Widget::instance()->setCenter(pt.x, pt.y);
        }break;
        case WM_KEYUP:
        case WM_SYSKEYDOWN:
        case WM_SYSKEYUP:
        {
            Widget::instance()->setKeyDown(lParam);
        }break;
        }
    }

    return CallNextHookEx(mouseHook,nCode,wParam,lParam);
}

HWND _workerw = nullptr;
inline BOOL CALLBACK EnumWindowsProc(_In_ HWND tophandle, _In_ LPARAM topparamhandle)
{
    HWND defview = FindWindowEx(tophandle, 0, L"SHELLDLL_DefView", nullptr);
    if (defview != nullptr)
    {
        _workerw = FindWindowEx(0, tophandle, L"WorkerW", 0);
    }
    return true;
}

HWND getworkWnd(){
    int result;
    HWND windowHandle = FindWindow(L"Progman", nullptr);
    //使用 0x3e8 命令分割出两个 WorkerW
    SendMessageTimeout(windowHandle, 0x052c, 0 ,0, SMTO_NORMAL, 0x3e8,(PDWORD_PTR)&result);
    //遍历窗体获得窗口句柄
    EnumWindows(EnumWindowsProc,(LPARAM)nullptr);
    // 显示WorkerW
    ShowWindow(_workerw,SW_HIDE);
    return windowHandle;
}

Widget* Widget::m_this=nullptr;
Widget::Widget(QWidget *parent)
    : QFrameLessWidget(parent)
    , m_bUserEnd(false)
    , m_tray(nullptr)
{
    auto work = getworkWnd();
    SetParent((HWND)this->winId(), work);
    m_render = new QRenderWidget(this);
    m_control = new QVideoControl(this);
//    m_label = new QLabel(this);
    m_data = new QDataModel(this);
    resize(qApp->desktop()->size());
    move(0, 0);
    init();

    m_this = this;
    mouseHook = SetWindowsHookEx(WH_MOUSE_LL, mouseProc, GetModuleHandle(NULL),NULL);
    keyHook = SetWindowsHookEx(WH_KEYBOARD_LL, mouseProc, GetModuleHandle(NULL),NULL);

    connect(this, &Widget::setUrl, m_control, &QVideoControl::onStart);
    connect(this, &Widget::setUrl, m_data, &QDataModel::onAddUrl);
    connect(m_control, &QVideoControl::appendFrame, m_render, &QRenderWidget::appendFrame, Qt::QueuedConnection);
    connect(m_control, &QVideoControl::end, this, &Widget::onEnd, Qt::DirectConnection);
    connect(this, &Widget::prev, this, &Widget::onPrev);
    connect(this, &Widget::next, this, &Widget::onNext);
}

Widget *Widget::instance()
{
    return m_this;
}

void Widget::setCenter(int x, int y)
{
//    m_label->update();
//    qDebug() << __FUNCTION__ << x << y;
}

void Widget::setKeyDown(int lp)
{
    auto p = (PKBDLLHOOKSTRUCT)lp;
    if((GetAsyncKeyState(VK_CONTROL) & 0x8000) != 0)
    {
        if(p->vkCode == VK_LEFT)
            emit prev();
        else if(p->vkCode == VK_RIGHT)
            emit next();
    }
}

Widget::~Widget()
{
    if(m_tray) m_tray->hide();
    if(mouseHook) UnhookWindowsHookEx(mouseHook);
    if(keyHook) UnhookWindowsHookEx(keyHook);
}

#include "QDataModel.h"

void Widget::init()
{
    initTray();
    m_data->load();
}

void Widget::initTray()
{
    m_tray = new QSystemTrayIcon(QApplication::desktop());
    m_tray->setParent(this);
    m_tray->setObjectName("tary1");
    m_tray->setIcon(QIcon(":/logo"));
    m_tray->setToolTip(tr(""));
    m_tray->show();

    auto menu = new QMenu();
    menu->setObjectName("menu");
    m_tray->setContextMenu(menu);

    m_playList = menu->addMenu(tr("play_list"));
    m_playList->setObjectName("menu");
    m_deleteList = menu->addMenu(tr("delete_list"));
    m_deleteList->setObjectName("menu");
    auto action_stop = menu->addAction(tr("stop"));
    auto action_quit = menu->addAction(tr("quit"));
    auto action_add = m_playList->addAction(tr("add..."));

    connect(action_stop, &QAction::triggered, [=]{
        m_bUserEnd = true;
        m_control->onStoped();
    });

    connect(action_quit, &QAction::triggered, [=]{
        m_tray->hide();
        qApp->quit();
    });

    connect(action_add, &QAction::triggered, [=](){
        auto url = QFileDialog::getOpenFileUrl(nullptr, QString(),
                                    QUrl(QStandardPaths::writableLocation(QStandardPaths::DesktopLocation)),
                                    tr("All Files (*.*);;mp4 (*.mp4)"));
        addUrl(url.toLocalFile());
        setUserEnd(true);
        emit setUrl(url.toLocalFile());
        setUserEnd(false);
    });

    connect(m_data, &QDataModel::loadsuccess, [=](const QVector<QStringList>& info)
    {
        m_deleteList->clear();
        auto actions = m_playList->actions();
        actions.erase(actions.begin());
        QString lastAcTitle;
        QString lastAcUrl;
        for(auto it : actions)
        {
            if(it->isChecked())
            {
                lastAcTitle = it->text();
                lastAcUrl = it->data().toString();
            }

            m_playList->removeAction(it);
        }

        for(auto it : info)
        {
            auto ac = m_playList->addAction(it[0]);
            ac->setCheckable(true);
            if(lastAcTitle == it[0] && lastAcUrl == it[1])
            {
                ac->setChecked(true);
            }
            ac->setData(it[1]);
            m_deleteList->addAction(it[0]);
        }
    });

    connect(m_playList, &QMenu::triggered, [=](QAction* ac){
        auto index = m_playList->actions().indexOf(ac);
        if(!index)
        {
            return;
        }

        for(auto it : m_playList->actions())
        {
            it->setChecked(false);
        }

        ac->setChecked(true);
        setUserEnd(true);
        emit setUrl(ac->data().toString());
        setUserEnd(false);
    });

    connect(m_deleteList, &QMenu::triggered, [=](QAction* ac){
        auto index = m_deleteList->actions().indexOf(ac);
        auto indexEx = m_playList->actions()[index + 1];
        m_deleteList->removeAction(ac);
        m_playList->removeAction(indexEx);
        m_data->onRemove(indexEx->data().toString());
    });
}

void Widget::resizeEvent(QResizeEvent *event)
{
    __super::resizeEvent(event);
    m_render->renderWidget()->resize(size());
}


void Widget::keyPressEvent(QKeyEvent *event)
{
    qDebug() << event->key();
    __super::keyPressEvent(event);
}

void Widget::addUrl(const QString& fileUrl)
{
    addUrl(fileUrl, m_playList);
    addUrl(fileUrl, m_deleteList);
}

void Widget::addUrl(const QString& fileUrl, QMenu* menu)
{
    auto actions = menu->actions();
//    actions.erase(actions.begin());
    bool bFinder = false;
    auto file = fileUrl;
    for(auto it : actions)
    {
        if(it->data().toString() == file)
        {
            it->setChecked(true);
            bFinder = true;
        }
        else
        {
            it->setChecked(false);
        }
    }

    if(!bFinder)
    {
        auto name = file.mid(file.lastIndexOf('/') + 1);
        auto ac = menu->addAction(name);
        ac->setData(file);
    }
}

void Widget::onEnd(int index)
{
    if(m_bUserEnd)
    {
        setUserEnd(false);
        return;
    }

    onNext();
}

void Widget::onNext()
{
    auto actions = m_playList->actions();
    actions.erase(actions.begin());
    bool bChecked = false;
    QAction* nextAc = nullptr;
    for(auto ac : actions)
    {
        if(bChecked)
        {
            nextAc = ac;
            break;
        }

        if(ac->isChecked())
        {
            bChecked = true;
            ac->setChecked(false);
        }
    }

    if(!actions.isEmpty() && !nextAc)
    {
        nextAc = actions[0];
    }

    if(!nextAc)
        return;

    nextAc->setChecked(true);
    setUserEnd(true);
    emit setUrl(nextAc->data().toString());
    setUserEnd(false);
}

void Widget::onPrev()
{
    auto actions = m_playList->actions();
    actions.erase(actions.begin());
    QAction* lastAc = nullptr;
    for(auto ac : actions)
    {
        if(ac->isChecked())
        {
            ac->setChecked(false);
            break;
        }
        else
        {
            lastAc = ac;
        }
    }

    if(!actions.isEmpty())
    {
        if(!lastAc)
        {
            lastAc = actions[0];
        }
    }

    if(!lastAc)
        return;

    lastAc->setChecked(true);
    setUserEnd(true);
    emit setUrl(lastAc->data().toString());
    setUserEnd(false);
}

void Widget::setUserEnd(bool bUserEnd)
{
    m_bUserEnd = bUserEnd;
}
