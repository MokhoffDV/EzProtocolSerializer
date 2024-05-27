#include "creator_widget.h"
#include <QLabel>
#include <QMessageBox>

CreatorWidget::CreatorWidget(QWidget* parent /* = nullptr */)
    : QWidget(parent)
    , m_addButton(new QPushButton("Add field"))
    , m_removeButton(new QPushButton("Remove field"))
    , m_submitButton(new QPushButton("Submit changes"))
    , m_isLittleEndianCheckbox(new QCheckBox("isLittleEndian"))
    , m_fieldsLayout(new QVBoxLayout())
    , m_mainLayout(new QVBoxLayout())
{
    connect(m_addButton, &QPushButton::clicked, this, [this]()
    {
        appendField(QString("field_%1").arg(m_fieldsLayout->count()), 8, ez::protocol_serializer::visualization_type::signed_integer);
    });

    connect(m_removeButton, &QPushButton::clicked, this, [this]()
    {
        if (m_fieldsLayout->count() <= 1)
            return;
        m_fieldsLayout->takeAt(m_fieldsLayout->count() - 2)->widget()->deleteLater();
    });

    connect(m_submitButton, &QPushButton::clicked, this, &CreatorWidget::submit);

    connect(m_isLittleEndianCheckbox, &QCheckBox::stateChanged, this, [this](bool checked)
    {
        m_ps->set_is_little_endian(checked);
        emit protocolSerializerChanged();
    });

    QHBoxLayout* buttonsLayout = new QHBoxLayout();
    buttonsLayout->addWidget(m_addButton);
    buttonsLayout->addWidget(m_removeButton);
    buttonsLayout->addSpacerItem(new QSpacerItem(0, 0));
    buttonsLayout->addWidget(m_submitButton);
    m_fieldsLayout->addLayout(buttonsLayout);

    m_mainLayout->addWidget(new QLabel("ez::protocol_serializer ps({"));
    m_mainLayout->addLayout(m_fieldsLayout);
    m_mainLayout->addWidget(m_isLittleEndianCheckbox);
    m_mainLayout->addWidget(new QLabel("});"));
    m_mainLayout->setContentsMargins(0, 0, 0, 0);
    setLayout(m_mainLayout);
}

void CreatorWidget::submit()
{
    if (m_ps == nullptr)
        return;

    // Remember previous working buffer state to restore if needed
    const unsigned int prevWorkingBufferLength = m_ps->get_internal_buffer_length();
    std::unique_ptr<unsigned char[]> prevWorkingBufferCopy = nullptr;
    if (prevWorkingBufferLength > 0) {
        prevWorkingBufferCopy.reset(new unsigned char[prevWorkingBufferLength]);
        memcpy_s(prevWorkingBufferCopy.get(), prevWorkingBufferLength, m_ps->get_working_buffer(), prevWorkingBufferLength);
    }
    m_ps->clear_protocol();

    bool errorOccured = false;
    for (int i = 0; i < m_fieldsLayout->count() - 1; ++i) {
        CreatorFieldWidget* fieldItem = static_cast<CreatorFieldWidget*>(m_fieldsLayout->itemAt(i)->widget());
        if (fieldItem == nullptr)
            continue;

        const ez::protocol_serializer::result_code resultCode = m_ps->append_field({fieldItem->getName().toStdString(), fieldItem->getBitCount(), fieldItem->getVisualizationType()});
        if (resultCode != ez::protocol_serializer::result_code::ok) {
            errorOccured = true;
            break;
        }
    }

    if (errorOccured)
        m_ps->clear_protocol();

    // Check new length, copy old values if possible
    const unsigned int newWorkingBufferLength = m_ps->get_internal_buffer_length();
    if (prevWorkingBufferLength > 0 && newWorkingBufferLength > 0)
        memcpy(m_ps->get_working_buffer(), prevWorkingBufferCopy.get(), std::min(prevWorkingBufferLength, newWorkingBufferLength));

    emit protocolSerializerChanged();
}

CreatorFieldWidget::CreatorFieldWidget(const QString& name,
                                       const unsigned int bitCount,
                                       const ez::protocol_serializer::visualization_type visType,
                                       QWidget* parent /* = nullptr */)
    : QWidget(parent)
    , m_nameEdit(new QLineEdit(name))
    , m_bitCountSpinbox(new QSpinBox())
    , m_visTypeCombo(new QComboBox())
{
    m_nameEdit->setPlaceholderText("Field name...");
    m_nameEdit->setToolTip("Field name");
    m_bitCountSpinbox->setRange(1, 32768);
    m_bitCountSpinbox->setValue(bitCount);
    m_bitCountSpinbox->setToolTip("Bit count");
    m_visTypeCombo->addItem("signed_integer", static_cast<int>(ez::protocol_serializer::visualization_type::signed_integer));
    m_visTypeCombo->addItem("unsigned_integer", static_cast<int>(ez::protocol_serializer::visualization_type::unsigned_integer));
    m_visTypeCombo->addItem("floating_point", static_cast<int>(ez::protocol_serializer::visualization_type::floating_point));
    m_visTypeCombo->setCurrentIndex(static_cast<int>(visType));
    m_visTypeCombo->setProperty("prev_index", m_visTypeCombo->currentIndex());
    m_visTypeCombo->setToolTip("Visualization type");

    const auto& dropVisType = [this](int indexToDropTo)
    {
        QMessageBox::warning(m_visTypeCombo, "Not applicable", "'floating point' visualization type is only available for fields with length of 32 (float) or 64(double) bits!", QMessageBox::Ok);
        m_visTypeCombo->blockSignals(true);
        m_visTypeCombo->setCurrentIndex(indexToDropTo);
        m_visTypeCombo->setProperty("prev_index", m_visTypeCombo->currentIndex());
        m_visTypeCombo->blockSignals(false);
    };

    connect(m_visTypeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this, dropVisType](int index)
    {
        using vis_type = ez::protocol_serializer::visualization_type;
        if (static_cast<vis_type>(index) == vis_type::floating_point && m_bitCountSpinbox->value() != 32 && m_bitCountSpinbox->value() != 64) {
            dropVisType(m_visTypeCombo->property("prev_index").toInt());
            return;
        }

        m_visTypeCombo->setProperty("prev_index", m_visTypeCombo->currentIndex());
    });

    connect(m_bitCountSpinbox, QOverload<int>::of(&QSpinBox::valueChanged), this, [this, dropVisType](int value)
    {
        using vis_type = ez::protocol_serializer::visualization_type;
        if (static_cast<vis_type>(m_visTypeCombo->currentIndex()) == vis_type::floating_point && value != 32 && value != 64)
            dropVisType(0);
    });

    QHBoxLayout* layout = new QHBoxLayout(this);
    layout->addWidget(new QLabel("{"));
    layout->addWidget(m_nameEdit, 1);
    layout->addWidget(new QLabel(","));
    layout->addWidget(m_bitCountSpinbox, 1);
    layout->addWidget(new QLabel(","));
    layout->addWidget(m_visTypeCombo, 1);
    layout->addWidget(new QLabel("},"));
    setLayout(layout);
}

QString CreatorFieldWidget::getName() const
{
    return m_nameEdit->text();
}

void CreatorFieldWidget::setName(const QString& name)
{
    m_nameEdit->setText(name);
}

unsigned int CreatorFieldWidget::getBitCount() const
{
    return m_bitCountSpinbox->value();
}

void CreatorFieldWidget::setBitCount(const unsigned int count)
{
    m_bitCountSpinbox->setValue(count);
}

ez::protocol_serializer::visualization_type CreatorFieldWidget::getVisualizationType() const
{
    return static_cast<ez::protocol_serializer::visualization_type>(m_visTypeCombo->currentData().toInt());
}

void CreatorFieldWidget::setVisualizationType(const ez::protocol_serializer::visualization_type visType)
{
    m_visTypeCombo->setCurrentIndex(static_cast<int>(visType));
}

void CreatorWidget::setProtocolSerializer(ez::protocol_serializer* ps)
{
    m_ps = ps;
}

void CreatorWidget::appendField(const QString& name, const unsigned int bitCount, const ez::protocol_serializer::visualization_type visType)
{
    m_fieldsLayout->insertWidget(m_fieldsLayout->count() - 1, new CreatorFieldWidget(name, bitCount, visType));
}
