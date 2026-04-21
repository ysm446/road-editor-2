#pragma once

#include <array>
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
    float divideLength   = 1.0f;
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
    void applyBankAngleChanges();
    void applyLaneSectionChanges();
    void applySocketChanges();
    void removeVerticalCurve();
    void removeBankAngle();
    void removeLaneSection();
    void addSocket();
    void removeSocket();

private:
    void setRoadEnabled(bool on);
    void setPointEnabled(bool on);
    void setVerticalCurveEnabled(bool on);
    void setBankAngleEnabled(bool on);
    void setLaneSectionEnabled(bool on);
    void setSocketEnabled(bool on);
    void populate(const Road& road);
    void populatePoint(const Selection& sel);
    void populateVerticalCurve(const Road& road, int verticalCurveIdx);
    void populateBankAngle(const Road& road, int bankAngleIdx);
    void populateLaneSection(const Road& road, int laneSectionIdx);
    void populateSocket(const Intersection& ix, int socketIdx);
    std::array<QCheckBox*, 5> roadLaneChecks() const;
    std::array<QDoubleSpinBox*, 5> roadLaneSpins() const;
    std::array<QCheckBox*, 5> laneSectionChecks() const;
    std::array<QDoubleSpinBox*, 5> laneSectionSpins() const;
    void setLaneControlsBlocked(const std::array<QCheckBox*, 5>& checks,
                                const std::array<QDoubleSpinBox*, 5>& spins,
                                bool blocked);
    void setLaneControlsEnabled(const std::array<QCheckBox*, 5>& checks,
                                const std::array<QDoubleSpinBox*, 5>& spins,
                                bool enabled);
    void setLaneControlsValues(const std::array<QCheckBox*, 5>& checks,
                               const std::array<QDoubleSpinBox*, 5>& spins,
                               const std::array<bool, 5>& enabledValues,
                               const std::array<float, 5>& widthValues);

    const RoadNetwork* m_net     = nullptr;
    int                m_roadIdx = -1;
    int                m_pointIdx = -1;
    int                m_verticalCurveIdx = -1;
    int                m_bankAngleIdx = -1;
    int                m_laneSectionIdx = -1;
    int                m_intersectionIdx = -1;
    int                m_socketIdx = -1;

    QLabel*         m_nameLabel;
    QGroupBox*      m_pointGroup;
    QLabel*         m_pointSummaryLabel;
    QLabel*         m_pointRoadLabel;
    QLabel*         m_pointIndexLabel;
    QLabel*         m_pointXLabel;
    QLabel*         m_pointYLabel;
    QLabel*         m_pointZLabel;
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
    QDoubleSpinBox* m_divideLengthSpin;
    QCheckBox*      m_equalMidpointCheck;
    QGroupBox*      m_verticalCurveGroup;
    QDoubleSpinBox* m_verticalUCoordSpin;
    QDoubleSpinBox* m_verticalVclSpin;
    QDoubleSpinBox* m_verticalOffsetSpin;
    QPushButton*    m_removeVerticalCurveButton;
    QGroupBox*      m_bankAngleGroup;
    QDoubleSpinBox* m_bankUCoordSpin;
    QDoubleSpinBox* m_bankTargetSpeedSpin;
    QCheckBox*      m_bankUseAngleCheck;
    QDoubleSpinBox* m_bankAngleSpin;
    QPushButton*    m_removeBankAngleButton;
    QGroupBox*      m_laneSectionGroup;
    QDoubleSpinBox* m_laneSectionUSpin;
    QCheckBox*      m_laneSectionUseLaneLeft2Check;
    QDoubleSpinBox* m_laneSectionWidthLaneLeft2Spin;
    QCheckBox*      m_laneSectionUseLaneLeft1Check;
    QDoubleSpinBox* m_laneSectionWidthLaneLeft1Spin;
    QCheckBox*      m_laneSectionUseLaneCenterCheck;
    QDoubleSpinBox* m_laneSectionWidthLaneCenterSpin;
    QCheckBox*      m_laneSectionUseLaneRight1Check;
    QDoubleSpinBox* m_laneSectionWidthLaneRight1Spin;
    QCheckBox*      m_laneSectionUseLaneRight2Check;
    QDoubleSpinBox* m_laneSectionWidthLaneRight2Spin;
    QDoubleSpinBox* m_laneSectionOffsetCenterSpin;
    QPushButton*    m_removeLaneSectionButton;
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
    void selectedBankAngleModified(int roadIdx, int curveIdx, float u, float targetSpeed, bool useAngle, float angle);
    void removeSelectedBankAngleRequested(int roadIdx, int curveIdx);
    void selectedLaneSectionModified(
        int roadIdx, int curveIdx, float u,
        bool useLaneLeft2, float widthLaneLeft2,
        bool useLaneLeft1, float widthLaneLeft1,
        bool useLaneCenter, float widthLaneCenter,
        bool useLaneRight1, float widthLaneRight1,
        bool useLaneRight2, float widthLaneRight2,
        float offsetCenter);
    void removeSelectedLaneSectionRequested(int roadIdx, int curveIdx);
    void selectedSocketModified(const QString& name, float yaw, bool enabled);
    void addSocketRequested();
    void removeSocketRequested();
};

Q_DECLARE_METATYPE(RoadProperties)
