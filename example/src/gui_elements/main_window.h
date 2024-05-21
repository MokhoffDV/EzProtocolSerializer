#ifndef MAIN_WINDOW_H
#define MAIN_WINDOW_H

#include "ui_main_window.h"
#include "ez_protocol_serializer.h"
#include <QMainWindow>

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    MainWindow(QWidget* parent = nullptr);

private:
    Ui::MainWindow* m_ui;
    ez::protocol_serializer m_ps;
};

#endif // MAIN_WINDOW_H
