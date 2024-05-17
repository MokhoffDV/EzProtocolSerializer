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
    int prevPosition = m_ui->getVisualizationOutputEdit->verticalScrollBar()->value();
    m_ui->getVisualizationOutputEdit->clear();
    m_ui->getVisualizationOutputEdit->append(QString(m_ps->getVisualization(m_ui->drawHeaderCheckbox->isChecked(),
                                                                            m_ui->firstLineNumSpinbox->value(),
                                                                            m_ui->horizontalBitMarginSpinbox->value(),
                                                                            m_ui->namesLinesCountSpinbo->value(),
                                                                            m_ui->printValuesCheckbox->isChecked()).c_str()));
    m_ui->getVisualizationOutputEdit->verticalScrollBar()->setValue(prevPosition);

    prevPosition = m_ui->getDataVisualizationOutputEdit->verticalScrollBar()->value();
    m_ui->getDataVisualizationOutputEdit->clear();
    m_ui->getDataVisualizationOutputEdit->append(QString(m_ps->getDataVisualization(m_ui->firstLineNum2Spinbox->value(),
                                                                                    m_ui->bytesPerLineSpinbox->value(),
                                                                                    static_cast<ez::protocol_serializer::BASE>(m_ui->baseCombobox->currentIndex()),
                                                                                    m_ui->spacesBetweenBytesCheckbox->isChecked()).c_str()));
    m_ui->getDataVisualizationOutputEdit->verticalScrollBar()->setValue(prevPosition);
}
