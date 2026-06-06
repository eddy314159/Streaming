#include "ViewerWindow.h"
#include <QtWidgets/QApplication>
#include <iostream>

int main(int argc, char *argv[])
{
    std::string l;
    std::cin >> l;

    std::string token;
    for (int i = 0; i < 16; i++)
        token += l;

    QApplication a(argc, argv);
    //Viewer w("localhost", "12345", token);
    ViewerWindow w("noelfic.fr", "12345", token);
    w.show();
    return a.exec();
}
