#include "main_window.h"

MainWindow::MainWindow(QWidget* parent /* = nullptr*/)
    : QMainWindow(parent)
    , m_ui(new Ui::MainWindow)
{
    m_ui->setupUi(this);

}
