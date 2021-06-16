#ifndef WIDGET_H
#define WIDGET_H

#include "framelesswidget/framelesswidget.h"
#include <QSystemTrayIcon>
class QRenderWidget;
class QLabel;
class QVideoControl;
class QDataModel;
class QMenu;
class Widget : public QFrameLessWidget
{
    Q_OBJECT

public:
    Widget(QWidget *parent = nullptr);
    static Widget* instance();
    void setCenter(int x, int y);
    void setKeyDown(int code);
    ~Widget();
signals:
    void setUrl(const QString&);
    void prev();
    void next();

private slots:
    void addUrl(const QString&);
    void addUrl(const QString&, QMenu*);
    void onEnd(int);
    void onNext();
    void onPrev();

private:
    void init();
    void initTray();
    void setUserEnd(bool);
private:
    bool m_bUserEnd;
    QMenu* m_playList, *m_deleteList;
    QSystemTrayIcon* m_tray;
    QRenderWidget* m_render;
    QVideoControl* m_control;
//    QLabel* m_label;
    QDataModel* m_data;
    static Widget* m_this;
    // QWidget interface
protected:
    void resizeEvent(QResizeEvent *event);

    // QWidget interface
protected:
    void keyPressEvent(QKeyEvent *event);
};
#endif // WIDGET_H
