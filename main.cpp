#include "mainwindow.h"

#include <locale>
#include <QApplication>

int main(int argc, char *argv[])
{
    std::locale::global(std::locale(".UTF-8"));
    QApplication a(argc, argv);
    MainWindow w;
    w.show();
    return a.exec();
}
