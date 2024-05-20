#include "editor_widget.h"
#include "validators.h"

#include <QLabel>
#include <QLineEdit>
#include <QScrollBar>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QPushButton>
#include <QHeaderView>
#include <QTableWidget>
#include <QTableWidgetItem>

EditorWidget::EditorWidget(QWidget* parent)
    : QWidget(parent)
{
    m_mainLayout = new QVBoxLayout();
    m_mainLayout->setStretch(1, 1);
    m_mainLayout->setSpacing(0);
    m_mainLayout->setContentsMargins(0, 0, 0, 0);
    setLayout(m_mainLayout);
}

void EditorWidget::setProtocolSerializer(ez::protocol_serializer* ps)
{
    m_ps = ps;
}

void EditorWidget::regenerate()
{
    while(m_mainLayout->count())
        delete m_mainLayout->takeAt(m_mainLayout->count() - 1)->widget();

    m_parentFieldWidgets.clear();
    if (m_ps->get_fields_list().size() == 0)
        return;

    // Main table
    QTableWidget* fieldsTable = new QTableWidget();
    fieldsTable->setColumnCount(1);
    fieldsTable->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    fieldsTable->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    fieldsTable->setSelectionMode(QAbstractItemView::NoSelection);
    fieldsTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    fieldsTable->horizontalHeader()->hide();
    fieldsTable->setRowCount(m_ps->get_internal_buffer_length() / 2 + m_ps->get_internal_buffer_length() % 2);

    // Layout of every row in the table
    QVector<QHBoxLayout*> rowsLayout(fieldsTable->rowCount());
    for (int i = 0; i < rowsLayout.size(); ++i) {
        rowsLayout[i] = new QHBoxLayout();
        rowsLayout[i]->setContentsMargins(0, 0, 0, 0);
        rowsLayout[i]->setSpacing(0);
    }

    // Go through every bit of every field
    for (const std::string& fieldName : m_ps->get_fields_list()) {
        const ez::protocol_serializer::field_metadata& fieldMetadata = m_ps->get_field_metadata(fieldName);
        unsigned int currentWordInd = fieldMetadata.firstBitInd / 16;
        unsigned int currentBitInd = fieldMetadata.firstBitInd;
        unsigned int firstBitIndInsideCurrentWord = fieldMetadata.firstBitInd;
        EditorFieldWidget* mainFieldWidget = nullptr;
        while (true) {
            bool fieldWidgetEndsHere = false;
            bool fieldEndsHere = false;

            // It was the last bit of current field
            if (currentBitInd >= fieldMetadata.firstBitInd + fieldMetadata.bitCount - 1) {
                fieldWidgetEndsHere = true;
                fieldEndsHere = true;
            }

            // It was the last bit of current line
            if (currentWordInd != (currentBitInd + 1) / 16)
                fieldWidgetEndsHere = true;

            // Line or field finishes here. Insert widget (either main one with value edit or child widget)
            if (fieldWidgetEndsHere) {
                if (mainFieldWidget == nullptr) {
                    mainFieldWidget = new EditorFieldWidget(m_ps,
                                                            fieldMetadata,
                                                            firstBitIndInsideCurrentWord,
                                                            currentBitInd,
                                                            true);
                    rowsLayout[currentWordInd]->addWidget(mainFieldWidget);
                } else {
                    EditorFieldWidget* wgt = new EditorFieldWidget(m_ps,
                                                                   fieldMetadata,
                                                                   firstBitIndInsideCurrentWord,
                                                                   currentBitInd,
                                                                   false);
                    rowsLayout[currentWordInd]->addWidget(wgt);
                    mainFieldWidget->addChildField(wgt);
                }

                // Stretch this field widget respective to its bit length
                const int lastWidgetInRowIndex = rowsLayout[currentWordInd]->count() - 1;
                rowsLayout[currentWordInd]->setStretch(lastWidgetInRowIndex, currentBitInd - firstBitIndInsideCurrentWord + 1);

                // Listen to changes made to field value
                connect(static_cast<EditorFieldWidget*>(rowsLayout[currentWordInd]->itemAt(lastWidgetInRowIndex)->widget()), &EditorFieldWidget::fieldValueChanged, this, [this](const QString& fieldName)
                {
                    read(fieldName);
                    emit fieldValueChanged();
                });
            }

            // Field ended here. Exit loop through field bits to start next field
            if (fieldEndsHere)
                break;

            // Iterate
            currentBitInd++;
            if (currentWordInd != currentBitInd / 16)
                firstBitIndInsideCurrentWord = currentBitInd;

            currentWordInd = (currentBitInd) / 16;
        }
        m_parentFieldWidgets.insert(QString(fieldMetadata.name.c_str()), mainFieldWidget);
    }

    // Get summ of stretches of the last row to check if last row is complete
    // Add blank widget to take that last empty space
    QHBoxLayout* lastRowLayout = rowsLayout.last();
    int filledBits = 0;
    for (int i = 0; i < lastRowLayout->count(); ++i)
        filledBits += lastRowLayout->stretch(i);
    if (filledBits < 16)
        lastRowLayout->addWidget(new QWidget(), 16 - filledBits);

    // Insert all rows widgets in the table
    for (int i = 0; i < rowsLayout.size(); ++i) {
        QWidget* row = new QWidget();
        row->setLayout(rowsLayout[i]);
        fieldsTable->setCellWidget(i, 0, row);
    }

    // Fit column widths with table
    fieldsTable->resizeRowsToContents();

    // Create an empty table to use it as a header
    QTableWidget* tableHeader = new QTableWidget();
    tableHeader->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    tableHeader->setRowCount(fieldsTable->rowCount());
    tableHeader->setColumnCount(16);
    tableHeader->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    tableHeader->horizontalHeader()->setMinimumSectionSize(1);
    tableHeader->setFixedHeight(tableHeader->horizontalHeader()->height());
    QStringList lessSignificant, mostSignificant;
    for (int i = 7; i >= 0; i--) lessSignificant += QStringLiteral("%1").arg(i, 2, 10, QChar('0'));
    for (int i = 15; i >= 8; i--) mostSignificant += QStringLiteral("%1").arg(i, 2, 10, QChar('0'));
    if (!m_ps->get_is_little_endian())
        tableHeader->setHorizontalHeaderLabels(mostSignificant + lessSignificant);
    else
        tableHeader->setHorizontalHeaderLabels(lessSignificant + mostSignificant);

    // Final creation
    m_mainLayout->addWidget(tableHeader);
    m_mainLayout->addWidget(fieldsTable);
}

void EditorWidget::read(const QString& parameterName)
{
    if (!m_parentFieldWidgets.contains(parameterName))
        return;
    m_parentFieldWidgets[parameterName]->syncFieldWithBuffer();
}

EditorFieldWidget::EditorFieldWidget(ez::protocol_serializer* ps,
                                     const ez::protocol_serializer::field_metadata& fieldMetadata,
                                     const unsigned int startBitInd,
                                     const unsigned int lastBitInd,
                                     bool isMain,
                                     QWidget* parent)
    : QWidget(parent)
    , m_ps(ps)
    , m_fieldMetadata(fieldMetadata)
    , m_firstBit(startBitInd)
    , m_lastBit(lastBitInd)
    , m_isMain(isMain)
    , m_valueEdit(nullptr)
{
    QVBoxLayout* mainLayout = new QVBoxLayout();
    mainLayout->setSpacing(0);
    mainLayout->setContentsMargins(0, 0, 0, 0);

    // It is a main field (with line edit) 
    if (m_isMain) {
        m_valueEdit = new QLineEdit();
        m_valueEdit->setMinimumWidth(1);
        m_valueEdit->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);

        // Set respective validators to only allow values which fit this field
        if (m_fieldMetadata.associatedType == ez::protocol_serializer::associated_type::floating_point) {
            if (m_fieldMetadata.bitCount == 32) {
                QDoubleValidator* validator = new QDoubleValidator(std::numeric_limits<float>::min(), std::numeric_limits<float>::max(), 6, m_valueEdit);
                m_valueEdit->setValidator(validator);
            } else if (m_fieldMetadata.bitCount == 64) {
                QDoubleValidator* validator = new QDoubleValidator(std::numeric_limits<double>::min(), std::numeric_limits<double>::max(), 6, m_valueEdit);
                m_valueEdit->setValidator(validator);
            }
        } else if (m_fieldMetadata.associatedType == ez::protocol_serializer::associated_type::signed_integer) {
            int64_t max = 0;
            for (unsigned int i = 0; i < m_fieldMetadata.bitCount - 1; ++i) { max = (max << 1) + 1; }
            Int64Validator* validator = new Int64Validator(-max - 1, max, m_valueEdit);
            m_valueEdit->setValidator(validator);
        } else if (m_fieldMetadata.associatedType == ez::protocol_serializer::associated_type::unsigned_integer) {
            uint64_t max = 0;
            for (unsigned int i = 0; i < m_fieldMetadata.bitCount; ++i) { max = (max << 1) + 1; }
            UInt64Validator* validator = new UInt64Validator(max, m_valueEdit);
            m_valueEdit->setValidator(validator);
        }

        // Process new entered integer value as soon as text changes
        connect(m_valueEdit, &QLineEdit::textChanged, this, [this](const QString& text)
        {
            if (m_fieldMetadata.associatedType == ez::protocol_serializer::associated_type::floating_point)
                return;

            processValueText(text);
        }, Qt::DirectConnection);

        // Process new entered floating point value only after enter is pressed
        connect(m_valueEdit, &QLineEdit::returnPressed, this, [this]()
        {
            if (m_fieldMetadata.associatedType != ez::protocol_serializer::associated_type::floating_point)
                return;
            
            processValueText(m_valueEdit->text());
        }, Qt::DirectConnection);

        QLabel* name = new QLabel(fieldMetadata.name.c_str());
        QFont font = name->font();
        font.setFamily("monospace");
        font.setPointSize(8);
        name->setFont(font);
        name->setToolTip(fieldMetadata.name.c_str());
        name->setMinimumWidth(1);
        name->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);

        // Create field name on top, value edit at the bottom
        mainLayout->addWidget(name);
        mainLayout->addWidget(m_valueEdit);
    } else {
        // Otherwise create empty label on top and empty value edit on the bottom
        // just to match visual look of main field
        QLineEdit* dummyEdit = new QLineEdit();
        dummyEdit->setEnabled(false);
        dummyEdit->setMinimumWidth(1);
        dummyEdit->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);

        QLabel* dummyName = new QLabel();
        QFont font = dummyName->font();
        font.setFamily("monospace");
        font.setPointSize(8);
        dummyName->setFont(font);
        dummyName->setMinimumWidth(1);
        dummyName->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);

        mainLayout->addWidget(dummyName);
        mainLayout->addWidget(dummyEdit);
    }

    // Create bit switch buttons under value edit widget
    QHBoxLayout* bitsLayout = new QHBoxLayout();
    bitsLayout->setSpacing(0);
    bitsLayout->setContentsMargins(0, 0, 0, 0);
    for (uint32_t i = startBitInd; i <= lastBitInd; ++i) {
        QPushButton* bitButton = new QPushButton();
        bitButton->setMinimumWidth(1);
        bitButton->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
        bitButton->setProperty("field_name", m_fieldMetadata.name.c_str());
        bitButton->setProperty("bit_index", i);

        connect(bitButton, &QPushButton::clicked, this, [this]()
        {
            QPushButton* senderButton = static_cast<QPushButton*>(sender());
            switchBit(senderButton->property("field_name").toString(),
                      senderButton->property("bit_index").toInt());
        }, Qt::DirectConnection);

        m_bitButtons.append(bitButton);
        bitsLayout->addWidget(bitButton);
    }

    mainLayout->addLayout(bitsLayout);
    setLayout(mainLayout);

    syncFieldWithBuffer();
}

void EditorFieldWidget::processValueText(const QString& valueText) const
{
    QString textToCheck = valueText;
    if (textToCheck.isEmpty() || (textToCheck.length() == 1 && textToCheck.at(0) == '-'))
        return;

    int unused = 0;
    if (m_valueEdit->validator()->validate(textToCheck, unused) != QValidator::Acceptable)
        return;

    const QString fieldName = m_fieldMetadata.name.c_str();
    if (m_fieldMetadata.associatedType == ez::protocol_serializer::associated_type::floating_point) {
        if (m_fieldMetadata.bitCount == 32)
            m_ps->write(fieldName.toStdString(), textToCheck.toFloat());
        else if (m_fieldMetadata.bitCount == 64)
            m_ps->write(fieldName.toStdString(), textToCheck.toDouble());
    } else if (m_fieldMetadata.associatedType == ez::protocol_serializer::associated_type::signed_integer)
        m_ps->write(fieldName.toStdString(), textToCheck.toLongLong());
    else
        m_ps->write(fieldName.toStdString(), textToCheck.toULongLong());

    emit fieldValueChanged(fieldName);
}

void EditorFieldWidget::addChildField(EditorFieldWidget* childFieldWidget)
{
    m_children.append(childFieldWidget);
}

void EditorFieldWidget::syncFieldWithBuffer()
{
    if (m_isMain) {
        m_valueEdit->blockSignals(true);
        m_valueEdit->setText(getFieldValueAsText());
        m_valueEdit->blockSignals(false);
    }

    syncBitButtonsToBuffer(m_ps->get_working_buffer());
    // Tell child widgets to also sync their bit buttons with working buffer
    for (EditorFieldWidget* child : m_children)
        child->syncBitButtonsToBuffer(m_ps->get_working_buffer());
}

void EditorFieldWidget::switchBit(const QString& fieldName, const unsigned int bitInd)
{
    unsigned char* buffer = m_ps->get_internal_buffer().get();
    if (buffer == nullptr)
        return;

    unsigned int byteInd = bitInd / 8;
    unsigned int bitIndInsideByte = 7 - bitInd % 8;

    buffer[byteInd] ^= (1UL << bitIndInsideByte);
    emit fieldValueChanged(fieldName);
}

QString EditorFieldWidget::getFieldValueAsText()
{
    if (m_fieldMetadata.associatedType == ez::protocol_serializer::associated_type::floating_point && (m_fieldMetadata.bitCount == 32 || m_fieldMetadata.bitCount == 64)) {
        if (m_fieldMetadata.bitCount == 32)      return QString::number(m_ps->read<float>(m_fieldMetadata.name), 'f', 6);
        else if (m_fieldMetadata.bitCount == 64) return QString::number(m_ps->read<double>(m_fieldMetadata.name), 'f', 6);
    } else if (m_fieldMetadata.associatedType == ez::protocol_serializer::associated_type::signed_integer) {
        if (m_fieldMetadata.bitCount <= 8)       return QString::number(m_ps->read<int8_t>(m_fieldMetadata.name));
        else if (m_fieldMetadata.bitCount <= 16) return QString::number(m_ps->read<int16_t>(m_fieldMetadata.name));
        else if (m_fieldMetadata.bitCount <= 32) return QString::number(m_ps->read<int32_t>(m_fieldMetadata.name));
        else if (m_fieldMetadata.bitCount <= 64) return QString::number(m_ps->read<int64_t>(m_fieldMetadata.name));
    } else {
        if (m_fieldMetadata.bitCount <= 8)       return QString::number(m_ps->read<uint8_t>(m_fieldMetadata.name));
        else if (m_fieldMetadata.bitCount <= 16) return QString::number(m_ps->read<uint16_t>(m_fieldMetadata.name));
        else if (m_fieldMetadata.bitCount <= 32) return QString::number(m_ps->read<uint32_t>(m_fieldMetadata.name));
        else if (m_fieldMetadata.bitCount <= 64) return QString::number(m_ps->read<uint64_t>(m_fieldMetadata.name));
    }
    return "";
}

void EditorFieldWidget::syncBitButtonsToBuffer(unsigned char* buffer)
{
    unsigned int bitInd, byteInd, bitIndInsideByte;
    for (QPushButton* bitButton : m_bitButtons) {
        bitInd = bitButton->property("bit_index").toInt();
        byteInd = bitInd / 8;
        bitIndInsideByte = 7 - bitInd % 8;
        bitButton->setText(QString::number((buffer[byteInd] >> bitIndInsideByte) & 1));
    }
}
