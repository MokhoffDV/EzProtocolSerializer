#ifndef CREATOR_WIDGET_H
#define CREATOR_WIDGET_H

#include "../../include/ez_protocol_serializer.h"

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
    CreatorFieldWidget(int index, QWidget* parent = nullptr);
    QString getName() const;
    unsigned int getBitCount() const;
    ez::protocol_serializer::visualization_type getVisualizationType() const;
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