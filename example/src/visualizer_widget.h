#ifndef VISUALIZER_WIDGET_H
#define VISUALIZER_WIDGET_H

#include "ui_visualizer_widget.h"
#include "../../include/ez_protocol_serializer.h"
#include <QWidget>

class VisualizerWidget : public QWidget
{
    Q_OBJECT
public:
    VisualizerWidget(QWidget* parent = nullptr);
    void setProtocolSerializer(ez::protocol_serializer* ps);

public slots:
    void visualize();

private:
    Ui::VisualizerWidget* m_ui;
    ez::protocol_serializer* m_ps;
};

#endif //VISUALIZER_WIDGET_H