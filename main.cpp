#include "cutterwidget.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    CutterWidget w;
    w.show();
    return a.exec();
}
