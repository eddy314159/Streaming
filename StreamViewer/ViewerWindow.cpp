#include "ViewerWindow.h"
#include <thread>

#include "ClientConn.h"

int index = 0;

ViewerWindow::ViewerWindow(std::string ip, std::string port, std::string token, QWidget *parent)
    : QMainWindow(parent), ip(std::move(ip)), token(std::move(token)), port(std::move(port)),
    viewer(Viewer([this](unsigned int width, unsigned int height, std::vector<char> data)
        {
            std::lock_guard<std::mutex> lock(mutex);
            screen = std::move(data);

            signalSetScreen();
        }))
{
    screen.resize(1920 * 1080 * 3);

    ui.setupUi(this);

    connect(this, &ViewerWindow::signalSetScreen, this, &ViewerWindow::setScreen);

    viewer.connect(this->ip, this->port, this->token);
}

ViewerWindow::~ViewerWindow()
{
    viewer.disconnect();
}

void ViewerWindow::setScreen()
{
    std::lock_guard<std::mutex> lock(mutex);
    //Sleep(10);
    QImage image((uchar*)screen.data(), 1920, 1080, QImage::Format_BGR888);
    QImage copie = image.copy(); // Copie nécessaire car le buffer sera réutilisé

    // Affichage dans le QLabel
    ui.screen->setPixmap(QPixmap::fromImage(copie));
}
