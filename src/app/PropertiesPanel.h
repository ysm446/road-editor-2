#pragma once

#include <QMetaType>
#include <QWidget>
#include "../model/RoadNetwork.h"

class QCheckBox;
class QDoubleSpinBox;
class QLabel;

struct RoadProperties {
    float speed          = 40.0f;
    bool  useLaneLeft2   = false;
    float widthLaneLeft2 = 3.5f;
    bool  useLaneLeft1   = true;
    float widthLaneLeft1 = 4.0f;
    bool  useLaneCenter  = false;
    float widthLaneCenter = 0.0f;
    bool  useLaneRight1  = true;
    float widthLaneRight1 = 4.0f;
    bool  useLaneRight2  = false;
    float widthLaneRight2 = 3.5f;
    float segmentLength  = 1.0f;
    bool  equalMidpoint  = false;
};

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

    const RoadNetwork* m_net     = nullptr;
    int                m_roadIdx = -1;

    QLabel*         m_nameLabel;
    QDoubleSpinBox* m_speedSpin;

    QCheckBox*      m_useLaneLeft2Check;
    QDoubleSpinBox* m_widthLaneLeft2Spin;
    QCheckBox*      m_useLaneLeft1Check;
    QDoubleSpinBox* m_widthLaneLeft1Spin;
    QCheckBox*      m_useLaneCenterCheck;
    QDoubleSpinBox* m_widthLaneCenterSpin;
    QCheckBox*      m_useLaneRight1Check;
    QDoubleSpinBox* m_widthLaneRight1Spin;
    QCheckBox*      m_useLaneRight2Check;
    QDoubleSpinBox* m_widthLaneRight2Spin;

    QDoubleSpinBox* m_segmentLengthSpin;
    QCheckBox*      m_equalMidpointCheck;

signals:
    void roadModified(int roadIdx, RoadProperties props);
};

Q_DECLARE_METATYPE(RoadProperties)
