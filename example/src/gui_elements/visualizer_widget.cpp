#include "visualizer_widget.h"
#include <QScrollBar>

VisualizerWidget::VisualizerWidget(QWidget* parent)
    : QWidget(parent)
    , m_ui(new Ui::VisualizerWidget)
{
    m_ui->setupUi(this);

    m_ui->getVisualizationOutputEdit->setWordWrapMode(QTextOption::WrapMode::NoWrap);
    connect(m_ui->drawHeaderCheckbox, &QCheckBox::stateChanged, this, [this]() { visualize(); });
    connect(m_ui->firstLineNumSpinbox, &QSpinBox::valueChanged, this, [this]() { visualize(); });
    connect(m_ui->horizontalBitMarginSpinbox, &QSpinBox::valueChanged, this, [this]() { visualize(); });
    connect(m_ui->namesLinesCountSpinbo, &QSpinBox::valueChanged, this, [this]() { visualize(); });
    connect(m_ui->printValuesCheckbox, &QCheckBox::stateChanged, this, [this]() { visualize(); });

    m_ui->getDataVisualizationOutputEdit->setWordWrapMode(QTextOption::WrapMode::NoWrap);
    connect(m_ui->firstLineNum2Spinbox, &QSpinBox::valueChanged, this, [this]() { visualize(); });
    connect(m_ui->bytesPerLineSpinbox, &QSpinBox::valueChanged, this, [this]() { visualize(); });
    connect(m_ui->baseCombobox, &QComboBox::currentIndexChanged, this, [this]() { visualize(); });
    connect(m_ui->spacesBetweenBytesCheckbox, &QCheckBox::stateChanged, this, [this]() { visualize(); });
}

void VisualizerWidget::setProtocolSerializer(ez::protocol_serializer* ps)
{
    m_ps = ps;
}

void VisualizerWidget::visualize()
{
    using vp = ez::protocol_serializer::visualization_params;
    using dvp = ez::protocol_serializer::data_visualization_params;
    int prevPosition = m_ui->getVisualizationOutputEdit->verticalScrollBar()->value();
    m_ui->getVisualizationOutputEdit->clear();
    m_ui->getVisualizationOutputEdit->append(QString(m_ps->get_visualization(vp()
                                                                            .set_draw_header(m_ui->drawHeaderCheckbox->isChecked())
                                                                            .set_first_line_num(m_ui->firstLineNumSpinbox->value())
                                                                            .set_horizontal_bit_margin(m_ui->horizontalBitMarginSpinbox->value())
                                                                            .set_name_lines_count(m_ui->namesLinesCountSpinbo->value())
                                                                            .set_print_values(m_ui->printValuesCheckbox->isChecked())).c_str()));
    m_ui->getVisualizationOutputEdit->verticalScrollBar()->setValue(prevPosition);

    prevPosition = m_ui->getDataVisualizationOutputEdit->verticalScrollBar()->value();
    m_ui->getDataVisualizationOutputEdit->clear();
    m_ui->getDataVisualizationOutputEdit->append(QString(m_ps->get_data_visualization(dvp()
                                                                                    .set_first_line_num(m_ui->firstLineNum2Spinbox->value())
                                                                                    .set_bytes_per_line(m_ui->bytesPerLineSpinbox->value())
                                                                                    .set_base(static_cast<dvp::base>(m_ui->baseCombobox->currentIndex()))
                                                                                    .set_spaces_between_bytes(m_ui->spacesBetweenBytesCheckbox->isChecked())).c_str()));
    m_ui->getDataVisualizationOutputEdit->verticalScrollBar()->setValue(prevPosition);
}
