#include "main_window.h"

MainWindow::MainWindow(QWidget* parent /* = nullptr */)
    : QMainWindow(parent)
    , m_ui(new Ui::MainWindow)
{
    m_ui->setupUi(this);
    m_ui->creatorWidget->setProtocolSerializer(&m_ps);
    m_ui->dataVisualizationEdit->setWordWrapMode(QTextOption::WrapMode::NoWrap);
    m_ui->visualizationEdit->setWordWrapMode(QTextOption::WrapMode::NoWrap);

    connect(m_ui->creatorWidget, &CreatorWidget::protocolSerializerChanged, this, [this]()
    {
        m_ui->visualizationEdit->clear();
        m_ui->dataVisualizationEdit->clear();

        m_ui->visualizationEdit->append(QString(m_ps.getVisualization().c_str()));
        m_ui->dataVisualizationEdit->append(QString(m_ps.getDataVisualization().c_str()));
    });
}
