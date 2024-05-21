#include "main_window.h"

MainWindow::MainWindow(QWidget* parent /* = nullptr */)
    : QMainWindow(parent)
    , m_ui(new Ui::MainWindow)
{
    m_ui->setupUi(this);
    m_ui->splitter->setStretchFactor(0, 0);
    m_ui->splitter->setStretchFactor(1, 1);
    m_ui->splitter->setStretchFactor(2, 0);

    m_ui->creatorWidget->setProtocolSerializer(&m_ps);
    m_ui->editorWidget->setProtocolSerializer(&m_ps);
    m_ui->visualizerWidget->setProtocolSerializer(&m_ps);
    m_ui->editorWidget->regenerate();
    m_ui->visualizerWidget->visualize();

    connect(m_ui->creatorWidget, &CreatorWidget::protocolSerializerChanged, this, [this]()
    {
        m_ui->editorWidget->regenerate();
        m_ui->visualizerWidget->visualize();
    });

    connect(m_ui->editorWidget, &EditorWidget::fieldValueChanged, this, [this]()
    {
        m_ui->visualizerWidget->visualize();
    });
}
