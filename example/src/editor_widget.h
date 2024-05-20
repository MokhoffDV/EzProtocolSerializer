#ifndef EDITOR_WIDGET_H
#define EDITOR_WIDGET_H

#include "../../include/ez_protocol_serializer.h"
#include <QWidget>
#include <QMap>
#include <QVector>
#include <QVBoxLayout>

class EditorFieldWidget;
class EditorWidget : public QWidget
{
    Q_OBJECT
public:
    EditorWidget(QWidget* parent = nullptr);
    void setProtocolSerializer(ez::protocol_serializer* ps);
    void regenerate();
    void read(const QString& parameterName);

signals:
    void fieldValueChanged();

private:
    ez::protocol_serializer* m_ps;
    QMap<QString, EditorFieldWidget*> m_parentFieldWidgets;
    QVBoxLayout* m_mainLayout;
};

class QLineEdit;
class QPushButton;
class EditorFieldWidget : public QWidget
{
    Q_OBJECT
public:
    EditorFieldWidget(ez::protocol_serializer* ps,
                      const ez::protocol_serializer::field_metadata& fieldMetadata,
                      const unsigned int startBitInd,
                      const unsigned int lastBitInd,
                      bool isMain,
                      QWidget* parent = nullptr);
signals:
    void fieldValueChanged(const QString& fieldName) const;

private:
    friend class EditorWidget;
    void addChildField(EditorFieldWidget* childFieldWidget);
    void syncFieldWithBuffer();
    void switchBit(const QString& fieldName, const unsigned int bitInd);
    QString getFieldValueAsText();
    void syncBitButtonsToBuffer(unsigned char* buffer);
    void processValueText(const QString& valueText) const;

    ez::protocol_serializer* m_ps;
    ez::protocol_serializer::field_metadata m_fieldMetadata;
    unsigned int m_firstBit;
    unsigned int m_lastBit;
    bool m_isMain;

    QVector<EditorFieldWidget*> m_children;
    QLineEdit* m_valueEdit;
    QVector<QPushButton*> m_bitButtons;
};

#endif // EDITOR_WIDGET_H
