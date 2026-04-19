#pragma once

#include <QMetaType>
#include <QWidget>
#include "../model/RoadNetwork.h"
#include "../editor/EditorState.h"

class QCheckBox;
class QDoubleSpinBox;
class QLabel;
class QGroupBox;
class QLineEdit;
class QPushButton;

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
    QSize sizeHint() const override;

public slots:
    void onSelectionChanged(int roadIdx);
    void onSelectionStateChanged(const Selection& sel);
    void onNetworkChanged();
    void setNetwork(const RoadNetwork* net);

private slots:
    void applyChanges();
    void applyVerticalCurveChanges();
    void applySocketChanges();
    void removeVerticalCurve();
    void addSocket();
    void removeSocket();

private:
    void setRoadEnabled(bool on);
    void setVerticalCurveEnabled(bool on);
    void setSocketEnabled(bool on);
    void populate(const Road& road);
    void populateVerticalCurve(const Road& road, int verticalCurveIdx);
    void populateSocket(const Intersection& ix, int socketIdx);

    const RoadNetwork* m_net     = nullptr;
    int                m_roadIdx = -1;
    int                m_verticalCurveIdx = -1;
    int                m_intersectionIdx = -1;
    int                m_socketIdx = -1;

    QLabel*         m_nameLabel;
    QGroupBox*      m_speedGroup;
    QGroupBox*      m_laneGroup;
    QGroupBox*      m_meshGroup;
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
    QGroupBox*      m_verticalCurveGroup;
    QDoubleSpinBox* m_verticalUCoordSpin;
    QDoubleSpinBox* m_verticalVclSpin;
    QDoubleSpinBox* m_verticalOffsetSpin;
    QPushButton*    m_removeVerticalCurveButton;
    QGroupBox*      m_socketGroup;
    QLineEdit*      m_socketNameEdit;
    QDoubleSpinBox* m_socketYawSpin;
    QCheckBox*      m_socketEnabledCheck;
    QPushButton*    m_addSocketButton;
    QPushButton*    m_removeSocketButton;

signals:
    void roadModified(int roadIdx, RoadProperties props);
    void selectedVerticalCurveModified(int roadIdx, int curveIdx, float u, float vcl, float offset);
    void removeSelectedVerticalCurveRequested(int roadIdx, int curveIdx);
    void selectedSocketModified(const QString& name, float yaw, bool enabled);
    void addSocketRequested();
    void removeSocketRequested();
};

Q_DECLARE_METATYPE(RoadProperties)
