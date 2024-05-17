#include "creator_widget.h"
#include <QLabel>

CreatorWidget::CreatorWidget(QWidget* parent /* = nullptr */)
    : QWidget(parent)
    , m_addButton(new QPushButton("Add field"))
    , m_removeButton(new QPushButton("Remove field"))
    , m_submitButton(new QPushButton("Submit changes"))
    , m_layout(new QVBoxLayout())
{
    connect(m_addButton, &QPushButton::clicked, this, [this]()
    {
        m_layout->insertWidget(m_layout->count() - 1, new CreatorFieldWidget(m_layout->count()));
    });
    connect(m_removeButton, &QPushButton::clicked, this, [this]()
    {
        if (m_layout->count() <= 1)
            return;
        m_layout->takeAt(m_layout->count() - 2)->widget()->deleteLater();
    });
    connect(m_submitButton, &QPushButton::clicked, this, [this]()
    {
        if (m_ps == nullptr)
            return;
        m_ps->removeAllFields();
        m_ps->clearWorkingBuffer();

        bool errorOccured = false;
        for (int i = 0; i < m_layout->count() - 1; ++i)
        {
            CreatorFieldWidget* fieldItem = static_cast<CreatorFieldWidget*>(m_layout->itemAt(i)->widget());
            if (fieldItem == nullptr)
                continue;

            const bool added = m_ps->appendField({fieldItem->getName().toStdString(), fieldItem->getBitCount(), fieldItem->getAssocType()});
            if (!added)
            {
                errorOccured = true;
                break;
            }
        }

        if (errorOccured)
        {
            m_ps->removeAllFields();
            m_ps->clearWorkingBuffer();
        }

        emit protocolSerializerChanged();
    });

    QHBoxLayout* buttonsLayout = new QHBoxLayout();
    buttonsLayout->addWidget(m_addButton);
    buttonsLayout->addWidget(m_removeButton);
    buttonsLayout->addSpacerItem(new QSpacerItem(0,0));
    buttonsLayout->addWidget(m_submitButton);
    m_layout->addLayout(buttonsLayout);

    setLayout(m_layout);
}

CreatorFieldWidget::CreatorFieldWidget(int index, QWidget* parent /* = nullptr */)
    : QWidget(parent)
    , m_index(index)
    , m_nameEdit(new QLineEdit(QString("field_%1").arg(m_index)))
    , m_bitCountEdit(new QLineEdit("8"))
    , m_assocTypeCombo(new QComboBox())
{
    m_nameEdit->setPlaceholderText("Field name...");
    m_bitCountEdit->setPlaceholderText("Field bit count...");
    m_assocTypeCombo->addItem("SIGNED_INTEGER", static_cast<int>(ez::protocol_serializer::ASSOCIATED_TYPE::SIGNED_INTEGER));
    m_assocTypeCombo->addItem("UNSIGNED_INTEGER", static_cast<int>(ez::protocol_serializer::ASSOCIATED_TYPE::UNSIGNED_INTEGER));
    m_assocTypeCombo->addItem("FLOATING_POINT", static_cast<int>(ez::protocol_serializer::ASSOCIATED_TYPE::FLOATING_POINT));

    QHBoxLayout* layout = new QHBoxLayout(this);
    layout->addWidget(new QLabel("{"));
    layout->addWidget(m_nameEdit, 1);
    layout->addWidget(new QLabel(","));
    layout->addWidget(m_bitCountEdit, 1);
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
    return m_bitCountEdit->text().toUInt();
}

ez::protocol_serializer::ASSOCIATED_TYPE CreatorFieldWidget::getAssocType() const
{
    return static_cast<ez::protocol_serializer::ASSOCIATED_TYPE>(m_assocTypeCombo->currentData().toInt());
}

void CreatorWidget::setProtocolSerializer(ez::protocol_serializer* ps)
{
    m_ps = ps;
}