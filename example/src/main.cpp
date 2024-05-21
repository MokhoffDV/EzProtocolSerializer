#include "gui_elements/main_window.h"

#include <QApplication>

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);

    MainWindow mainWindow;
    mainWindow.setWindowTitle("ez::protocol_serializer example");
    mainWindow.resize(1620, 1024);
    mainWindow.show();

    return app.exec();
}
