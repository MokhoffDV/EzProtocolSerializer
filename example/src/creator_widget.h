#ifndef CREATOR_WIDGET_H
#define CREATOR_WIDGET_H

#include "../../include/ez_protocol_serializer.h"

#include <QLineEdit>
#include <QComboBox>
#include <QPushButton>
#include <QHBoxLayout>
#include <QVBoxLayout>

class FieldCreatorWidgetItem : public QWidget
{
    Q_OBJECT
public:
    FieldCreatorWidgetItem(int index, QWidget* parent = nullptr);
    QString getName() const;
    unsigned int getBitCount() const;
    ez::protocol_serializer::ASSOCIATED_TYPE getAssocType() const;
private:
    int m_index;
    QLineEdit* m_nameEdit;
    QLineEdit* m_bitCountEdit;
    QComboBox* m_assocTypeCombo;
};

class FieldCreatorWidget : public QWidget
{
    Q_OBJECT
public:
    FieldCreatorWidget(QWidget* parent = nullptr);
    void setProtocolSerializer(ez::protocol_serializer* ps);

signals:
    void protocolSerializerChanged();

private:
    ez::protocol_serializer* m_ps = nullptr;
    QVector<FieldCreatorWidgetItem*> m_fieldItems;

    QPushButton* m_addButton;
    QPushButton* m_removeButton;
    QPushButton* m_submitButton;
    QVBoxLayout* m_layout;
};

#endif // CREATOR_WIDGET_H