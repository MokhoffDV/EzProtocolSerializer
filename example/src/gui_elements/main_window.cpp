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
    
    using vis_type = ez::protocol_serializer::visualization_type;
    m_ui->creatorWidget->appendField("version", 4, vis_type::unsigned_integer);
    m_ui->creatorWidget->appendField("header_len", 4, vis_type::unsigned_integer);
    m_ui->creatorWidget->appendField("service_type", 8, vis_type::unsigned_integer);
    m_ui->creatorWidget->appendField("total_datagram_len", 16, vis_type::unsigned_integer);
    m_ui->creatorWidget->appendField("id_num", 16, vis_type::unsigned_integer);
    m_ui->creatorWidget->appendField("R", 1, vis_type::unsigned_integer);
    m_ui->creatorWidget->appendField("DF", 1, vis_type::unsigned_integer);
    m_ui->creatorWidget->appendField("MF", 1, vis_type::unsigned_integer);
    m_ui->creatorWidget->appendField("frag_offset", 13, vis_type::unsigned_integer);
    m_ui->creatorWidget->appendField("time_to_live", 8, vis_type::unsigned_integer);
    m_ui->creatorWidget->appendField("protocol", 8, vis_type::unsigned_integer);
    m_ui->creatorWidget->appendField("header_checksum", 16, vis_type::unsigned_integer);
    m_ui->creatorWidget->appendField("source_ip", 32, vis_type::unsigned_integer);
    m_ui->creatorWidget->appendField("dest_ip", 32, vis_type::unsigned_integer);
    m_ui->creatorWidget->appendField("float_val_for_fun", 32, vis_type::floating_point);
    m_ui->creatorWidget->submit();
}
