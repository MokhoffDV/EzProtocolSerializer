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
        m_fieldsLayout->insertWidget(m_fieldsLayout->count() - 1, new CreatorFieldWidget(m_fieldsLayout->count()));
    });
    connect(m_removeButton, &QPushButton::clicked, this, [this]()
    {
        if (m_fieldsLayout->count() <= 1)
            return;
        m_fieldsLayout->takeAt(m_fieldsLayout->count() - 2)->widget()->deleteLater();
    });
    connect(m_submitButton, &QPushButton::clicked, this, [this]()
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
        m_ps->clear_working_buffer();

        bool errorOccured = false;
        for (int i = 0; i < m_fieldsLayout->count() - 1; ++i) {
            CreatorFieldWidget* fieldItem = static_cast<CreatorFieldWidget*>(m_fieldsLayout->itemAt(i)->widget());
            if (fieldItem == nullptr)
                continue;

            const bool added = m_ps->append_field({fieldItem->getName().toStdString(), fieldItem->getBitCount(), fieldItem->getAssocType()});
            if (!added) {
                errorOccured = true;
                break;
            }
        }

        if (errorOccured) {
            m_ps->clear_protocol();
            m_ps->clear_working_buffer();
        }

        // Check new length, copy old values if possible
        const unsigned int newWorkingBufferLength = m_ps->get_internal_buffer_length();
        if (prevWorkingBufferLength > 0 && newWorkingBufferLength > 0)
            memcpy(m_ps->get_working_buffer(), prevWorkingBufferCopy.get(), std::min(prevWorkingBufferLength, newWorkingBufferLength));

        emit protocolSerializerChanged();
    });

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

CreatorFieldWidget::CreatorFieldWidget(int index, QWidget* parent /* = nullptr */)
    : QWidget(parent)
    , m_index(index)
    , m_nameEdit(new QLineEdit(QString("field_%1").arg(m_index)))
    , m_bitCountSpinbox(new QSpinBox())
    , m_assocTypeCombo(new QComboBox())
{
    m_nameEdit->setPlaceholderText("Field name...");
    m_bitCountSpinbox->setRange(1, 32768);
    m_bitCountSpinbox->setValue(8);
    m_assocTypeCombo->addItem("signed_integer", static_cast<int>(ez::protocol_serializer::associated_type::signed_integer));
    m_assocTypeCombo->addItem("unsigned_integer", static_cast<int>(ez::protocol_serializer::associated_type::unsigned_integer));
    m_assocTypeCombo->addItem("floating_point", static_cast<int>(ez::protocol_serializer::associated_type::floating_point));
    m_assocTypeCombo->setCurrentIndex(0);
    m_assocTypeCombo->setProperty("prev_index", m_assocTypeCombo->currentIndex());

    const auto& dropAssociatedType = [this](int indexToDropTo)
    {
        QMessageBox::warning(m_assocTypeCombo, "Not applicable", "Associated floating point type is only available for fields with length of 32 (float) or 64(double) bits!", QMessageBox::Ok);
        m_assocTypeCombo->blockSignals(true);
        m_assocTypeCombo->setCurrentIndex(indexToDropTo);
        m_assocTypeCombo->setProperty("prev_index", m_assocTypeCombo->currentIndex());
        m_assocTypeCombo->blockSignals(false);
    };

    connect(m_assocTypeCombo, &QComboBox::currentIndexChanged, this, [this, dropAssociatedType](int index)
    {
        using assoc_type = ez::protocol_serializer::associated_type;
        if (static_cast<assoc_type>(index) == assoc_type::floating_point && m_bitCountSpinbox->value() != 32 && m_bitCountSpinbox->value() != 64) {
            dropAssociatedType(m_assocTypeCombo->property("prev_index").toInt());
            return;
        }

        m_assocTypeCombo->setProperty("prev_index", m_assocTypeCombo->currentIndex());
    });

    connect(m_bitCountSpinbox, &QSpinBox::valueChanged, this, [this, dropAssociatedType](int value)
    {
        using assoc_type = ez::protocol_serializer::associated_type;
        if (static_cast<assoc_type>(m_assocTypeCombo->currentIndex()) == assoc_type::floating_point && value != 32 && value != 64)
            dropAssociatedType(0);
    });

    QHBoxLayout* layout = new QHBoxLayout(this);
    layout->addWidget(new QLabel("{"));
    layout->addWidget(m_nameEdit, 1);
    layout->addWidget(new QLabel(","));
    layout->addWidget(m_bitCountSpinbox, 1);
    layout->addWidget(new QLabel(","));
    layout->addWidget(m_assocTypeCombo, 1);
    layout->addWidget(new QLabel("},"));
    setLayout(layout);
}

QString CreatorFieldWidget::getName() const
{
    return m_nameEdit->text();
}

unsigned int CreatorFieldWidget::getBitCount() const
{
    return m_bitCountSpinbox->value();
}

ez::protocol_serializer::associated_type CreatorFieldWidget::getAssocType() const
{
    return static_cast<ez::protocol_serializer::associated_type>(m_assocTypeCombo->currentData().toInt());
}

void CreatorWidget::setProtocolSerializer(ez::protocol_serializer* ps)
{
    m_ps = ps;
}
