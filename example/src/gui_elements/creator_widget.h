#ifndef CREATOR_WIDGET_H
#define CREATOR_WIDGET_H

#include <ez_protocol_serializer.h>

#include <QLineEdit>
#include <QComboBox>
#include <QCheckBox>
#include <QSpinBox>
#include <QPushButton>
#include <QHBoxLayout>
#include <QVBoxLayout>

class CreatorFieldWidget : public QWidget
{
    Q_OBJECT
public:
    CreatorFieldWidget(const QString& name,
                       const unsigned int bitCount,
                       const ez::protocol_serializer::visualization_type visType,
                       QWidget* parent = nullptr);

    QString getName() const;
    void setName(const QString& name);

    unsigned int getBitCount() const;
    void setBitCount(const unsigned int count);

    ez::protocol_serializer::visualization_type getVisualizationType() const;
    void setVisualizationType(const ez::protocol_serializer::visualization_type visType);

private:
    int m_index;
    QLineEdit* m_nameEdit;
    QSpinBox* m_bitCountSpinbox;
    QComboBox* m_visTypeCombo;
};

class CreatorWidget : public QWidget
{
    Q_OBJECT
public:
    CreatorWidget(QWidget* parent = nullptr);

    void setProtocolSerializer(ez::protocol_serializer* ps);
    void appendField(const QString& name, const unsigned int bitCount, const ez::protocol_serializer::visualization_type visType);
    void submit();
signals:
    void protocolSerializerChanged();

private:
    ez::protocol_serializer* m_ps = nullptr;
    QVector<CreatorFieldWidget*> m_fieldItems;

    QPushButton* m_addButton;
    QPushButton* m_removeButton;
    QPushButton* m_submitButton;
    QCheckBox*   m_isLittleEndianCheckbox;
    QVBoxLayout* m_fieldsLayout;
    QVBoxLayout* m_mainLayout;
};

#endif // CREATOR_WIDGET_H