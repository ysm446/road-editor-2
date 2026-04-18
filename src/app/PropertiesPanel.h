#pragma once

#include <QWidget>
#include "../model/RoadNetwork.h"

class QLineEdit;
class QDoubleSpinBox;
class QSpinBox;
class QLabel;

class PropertiesPanel : public QWidget {
    Q_OBJECT
public:
    explicit PropertiesPanel(QWidget* parent = nullptr);

public slots:
    void onSelectionChanged(int roadIdx);
    void onNetworkChanged();
    void setNetwork(const RoadNetwork* net);

private slots:
    void applyChanges();

private:
    void setEnabled(bool on);
    void populate(const Road& road);

    const RoadNetwork* m_net    = nullptr;
    int                m_roadIdx = -1;

    QLabel*        m_nameLabel;
    QDoubleSpinBox* m_speedSpin;
    QDoubleSpinBox* m_leftWidthSpin;
    QDoubleSpinBox* m_rightWidthSpin;

signals:
    void roadModified(int roadIdx);
};
