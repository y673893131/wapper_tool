#include "widget.h"

#include <QApplication>
#include <QDir>
#include <QTranslator>
#include <QFile>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    a.setWindowIcon(QIcon(":/logo"));
    QTranslator trans;
    trans.load(":/tr/wapper_tool.qm");
    a.installTranslator(&trans);
    QDir::setCurrent(a.applicationDirPath());
    Widget w;
    w.show();

    QFile file(":/rc/rc.qss");
    if(file.open(QFile::ReadOnly))
    {
        auto qss = file.readAll();
        a.setStyleSheet(qss);
        file.close();
    }

    return a.exec();
}
