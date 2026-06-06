#ifndef VIEWER_WINDOW_H
#define VIEWER_WINDOW_H

#include "Viewer.h"

#include <QtWidgets/QMainWindow>
#include "ui_Viewer.h"

class ViewerWindow : public QMainWindow
{
    Q_OBJECT

signals:
    void signalSetScreen();

private slots:
    void setScreen();

public:
    ViewerWindow() = default;
    ViewerWindow(std::string ip, std::string port, std::string token, QWidget *parent = nullptr);
    ~ViewerWindow();

private:
    Ui::ViewerClass ui;

    Viewer viewer;

    std::vector<char> screen;

    std::mutex mutex;

    std::string ip;
    std::string port;
    std::string token;

    constexpr static int WIDTH = 1920;
    constexpr static int HEIGHT = 1080;
};

#endif
