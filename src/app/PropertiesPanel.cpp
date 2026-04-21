#include "PropertiesPanel.h"
#include <QCheckBox>
#include <QFormLayout>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QDoubleSpinBox>
#include <QGroupBox>
#include <QLineEdit>
#include <QPushButton>
#include <array>

static QDoubleSpinBox* makeWidthSpin(QWidget* parent) {
    auto* s = new QDoubleSpinBox(parent);
    s->setRange(0.0, 20.0);
    s->setValue(3.5);
    s->setSuffix(" m");
    s->setDecimals(2);
    s->setSingleStep(0.25);
    return s;
}

// One lane row: [checkbox label] [width spinbox]
static void addLaneRow(QFormLayout* form, const QString& label,
                       QCheckBox*& checkOut, QDoubleSpinBox*& spinOut,
                       bool defaultChecked, double defaultWidth, QWidget* parent)
{
    checkOut = new QCheckBox(label, parent);
    checkOut->setChecked(defaultChecked);
    spinOut  = makeWidthSpin(parent);
    spinOut->setValue(defaultWidth);

    auto* row = new QHBoxLayout;
    row->setContentsMargins(0,0,0,0);
    row->addWidget(checkOut);
    row->addStretch();
    row->addWidget(spinOut);

    form->addRow(row);
}

PropertiesPanel::PropertiesPanel(QWidget* parent)
    : QWidget(parent)
{
    setMinimumWidth(320);
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(6, 6, 6, 6);
    root->setSpacing(4);

    m_nameLabel = new QLabel("—", this);
    m_nameLabel->setStyleSheet("font-weight: bold; color: #aaa;");
    root->addWidget(m_nameLabel);

    m_pointGroup = new QGroupBox("Point", this);
    auto* pointForm = new QFormLayout(m_pointGroup);
    pointForm->setLabelAlignment(Qt::AlignRight);
    m_pointSummaryLabel = new QLabel("—", this);
    m_pointRoadLabel = new QLabel("—", this);
    m_pointIndexLabel = new QLabel("—", this);
    m_pointXLabel = new QLabel("—", this);
    m_pointYLabel = new QLabel("—", this);
    m_pointZLabel = new QLabel("—", this);
    pointForm->addRow("Selection:", m_pointSummaryLabel);
    pointForm->addRow("Road:", m_pointRoadLabel);
    pointForm->addRow("Point:", m_pointIndexLabel);
    pointForm->addRow("X:", m_pointXLabel);
    pointForm->addRow("Y:", m_pointYLabel);
    pointForm->addRow("Z:", m_pointZLabel);
    root->addWidget(m_pointGroup);

    // --- Speed ---
    m_speedGroup  = new QGroupBox("Speed", this);
    auto* speedForm = new QFormLayout(m_speedGroup);
    speedForm->setLabelAlignment(Qt::AlignRight);
    m_speedSpin = new QDoubleSpinBox(this);
    m_speedSpin->setRange(0, 200);
    m_speedSpin->setSuffix(" km/h");
    m_speedSpin->setDecimals(1);
    speedForm->addRow("Target Speed:", m_speedSpin);
    root->addWidget(m_speedGroup);

    // --- Lanes ---
    m_laneGroup  = new QGroupBox("Lanes (left=negative, right=positive)", this);
    auto* laneForm = new QFormLayout(m_laneGroup);
    laneForm->setLabelAlignment(Qt::AlignRight);

    addLaneRow(laneForm, "Left 2  (lane -2)", m_useLaneLeft2Check,  m_widthLaneLeft2Spin,  false, 3.5, this);
    addLaneRow(laneForm, "Left 1  (lane -1)", m_useLaneLeft1Check,  m_widthLaneLeft1Spin,  true,  4.0, this);
    addLaneRow(laneForm, "Center  (lane  0)", m_useLaneCenterCheck, m_widthLaneCenterSpin, false, 0.0, this);
    addLaneRow(laneForm, "Right 1 (lane +1)", m_useLaneRight1Check, m_widthLaneRight1Spin, true,  4.0, this);
    addLaneRow(laneForm, "Right 2 (lane +2)", m_useLaneRight2Check, m_widthLaneRight2Spin, false, 3.5, this);
    root->addWidget(m_laneGroup);

    // --- Mesh / curve settings ---
    m_meshGroup  = new QGroupBox("Mesh", this);
    auto* meshForm = new QFormLayout(m_meshGroup);
    meshForm->setLabelAlignment(Qt::AlignRight);

    m_segmentLengthSpin = new QDoubleSpinBox(this);
    m_segmentLengthSpin->setRange(0.1, 50.0);
    m_segmentLengthSpin->setValue(1.0);
    m_segmentLengthSpin->setSuffix(" m");
    m_segmentLengthSpin->setDecimals(2);
    m_segmentLengthSpin->setSingleStep(0.5);
    m_segmentLengthSpin->setToolTip("Mesh tessellation interval along the road direction");
    meshForm->addRow("Segment Length:", m_segmentLengthSpin);

    m_equalMidpointCheck = new QCheckBox("Equal midpoint (t=0.5)", this);
    m_equalMidpointCheck->setToolTip(
        "Checked: bend pivot at exact edge midpoint\n"
        "Unchecked: pivot weighted by adjacent edge lengths");
    meshForm->addRow("Midpoint split:", m_equalMidpointCheck);
    root->addWidget(m_meshGroup);

    m_verticalCurveGroup = new QGroupBox("Vertical Curve", this);
    auto* verticalForm = new QFormLayout(m_verticalCurveGroup);
    verticalForm->setLabelAlignment(Qt::AlignRight);

    m_verticalUCoordSpin = new QDoubleSpinBox(this);
    m_verticalUCoordSpin->setRange(0.0, 1.0);
    m_verticalUCoordSpin->setDecimals(4);
    m_verticalUCoordSpin->setSingleStep(0.01);
    verticalForm->addRow("U:", m_verticalUCoordSpin);

    m_verticalVclSpin = new QDoubleSpinBox(this);
    m_verticalVclSpin->setRange(0.0, 10000.0);
    m_verticalVclSpin->setDecimals(2);
    m_verticalVclSpin->setSuffix(" m");
    verticalForm->addRow("VCL:", m_verticalVclSpin);

    m_verticalOffsetSpin = new QDoubleSpinBox(this);
    m_verticalOffsetSpin->setRange(-1000.0, 1000.0);
    m_verticalOffsetSpin->setDecimals(2);
    m_verticalOffsetSpin->setSuffix(" m");
    verticalForm->addRow("Offset:", m_verticalOffsetSpin);

    m_removeVerticalCurveButton = new QPushButton("Remove Point", this);
    verticalForm->addRow(m_removeVerticalCurveButton);
    root->addWidget(m_verticalCurveGroup);

    m_bankAngleGroup = new QGroupBox("Bank Angle", this);
    auto* bankForm = new QFormLayout(m_bankAngleGroup);
    bankForm->setLabelAlignment(Qt::AlignRight);

    m_bankUCoordSpin = new QDoubleSpinBox(this);
    m_bankUCoordSpin->setRange(0.0, 1.0);
    m_bankUCoordSpin->setDecimals(4);
    m_bankUCoordSpin->setSingleStep(0.01);
    bankForm->addRow("U:", m_bankUCoordSpin);

    m_bankTargetSpeedSpin = new QDoubleSpinBox(this);
    m_bankTargetSpeedSpin->setRange(0.0, 200.0);
    m_bankTargetSpeedSpin->setDecimals(1);
    m_bankTargetSpeedSpin->setSuffix(" km/h");
    bankForm->addRow("Target Speed:", m_bankTargetSpeedSpin);

    m_bankUseAngleCheck = new QCheckBox("Use explicit angle", this);
    bankForm->addRow("Mode:", m_bankUseAngleCheck);

    m_bankAngleSpin = new QDoubleSpinBox(this);
    m_bankAngleSpin->setRange(-90.0, 90.0);
    m_bankAngleSpin->setDecimals(1);
    m_bankAngleSpin->setSuffix(" deg");
    bankForm->addRow("Angle:", m_bankAngleSpin);

    m_removeBankAngleButton = new QPushButton("Remove Point", this);
    bankForm->addRow(m_removeBankAngleButton);
    root->addWidget(m_bankAngleGroup);

    m_laneSectionGroup = new QGroupBox("Lane Section", this);
    auto* laneSectionForm = new QFormLayout(m_laneSectionGroup);
    laneSectionForm->setLabelAlignment(Qt::AlignRight);

    m_laneSectionUSpin = new QDoubleSpinBox(this);
    m_laneSectionUSpin->setRange(0.0, 1.0);
    m_laneSectionUSpin->setDecimals(4);
    m_laneSectionUSpin->setSingleStep(0.01);
    laneSectionForm->addRow("U:", m_laneSectionUSpin);

    addLaneRow(laneSectionForm, "Left 2  (lane -2)",
               m_laneSectionUseLaneLeft2Check, m_laneSectionWidthLaneLeft2Spin,
               false, 3.0, this);
    addLaneRow(laneSectionForm, "Left 1  (lane -1)",
               m_laneSectionUseLaneLeft1Check, m_laneSectionWidthLaneLeft1Spin,
               true, 3.0, this);
    addLaneRow(laneSectionForm, "Center  (lane  0)",
               m_laneSectionUseLaneCenterCheck, m_laneSectionWidthLaneCenterSpin,
               true, 0.0, this);
    addLaneRow(laneSectionForm, "Right 1 (lane +1)",
               m_laneSectionUseLaneRight1Check, m_laneSectionWidthLaneRight1Spin,
               true, 3.0, this);
    addLaneRow(laneSectionForm, "Right 2 (lane +2)",
               m_laneSectionUseLaneRight2Check, m_laneSectionWidthLaneRight2Spin,
               false, 3.0, this);

    m_laneSectionOffsetCenterSpin = new QDoubleSpinBox(this);
    m_laneSectionOffsetCenterSpin->setRange(-100.0, 100.0);
    m_laneSectionOffsetCenterSpin->setDecimals(2);
    m_laneSectionOffsetCenterSpin->setSuffix(" m");
    laneSectionForm->addRow("Center Offset:", m_laneSectionOffsetCenterSpin);

    m_removeLaneSectionButton = new QPushButton("Remove Point", this);
    laneSectionForm->addRow(m_removeLaneSectionButton);
    root->addWidget(m_laneSectionGroup);

    m_socketGroup = new QGroupBox("Socket", this);
    auto* socketForm = new QFormLayout(m_socketGroup);
    socketForm->setLabelAlignment(Qt::AlignRight);

    m_socketNameEdit = new QLineEdit(this);
    socketForm->addRow("Name:", m_socketNameEdit);

    m_socketYawSpin = new QDoubleSpinBox(this);
    m_socketYawSpin->setRange(-360.0, 360.0);
    m_socketYawSpin->setDecimals(1);
    m_socketYawSpin->setSingleStep(5.0);
    m_socketYawSpin->setSuffix(" deg");
    socketForm->addRow("Yaw:", m_socketYawSpin);

    m_socketEnabledCheck = new QCheckBox("Enabled", this);
    socketForm->addRow("State:", m_socketEnabledCheck);

    auto* socketButtons = new QHBoxLayout;
    socketButtons->setContentsMargins(0, 0, 0, 0);
    m_addSocketButton = new QPushButton("Add Socket", this);
    m_removeSocketButton = new QPushButton("Remove Socket", this);
    socketButtons->addWidget(m_addSocketButton);
    socketButtons->addWidget(m_removeSocketButton);
    socketForm->addRow(socketButtons);
    root->addWidget(m_socketGroup);

    const auto roadChecks = roadLaneChecks();
    const auto roadSpins = roadLaneSpins();
    const auto laneSectionCheckControls = laneSectionChecks();
    const auto laneSectionSpinControls = laneSectionSpins();

    connect(m_speedSpin, &QDoubleSpinBox::valueChanged,
            this, QOverload<>::of(&PropertiesPanel::applyChanges));
    for (size_t i = 0; i < roadChecks.size(); ++i) {
        connect(roadChecks[i], &QCheckBox::toggled,
                this, QOverload<>::of(&PropertiesPanel::applyChanges));
        connect(roadSpins[i], &QDoubleSpinBox::valueChanged,
                this, QOverload<>::of(&PropertiesPanel::applyChanges));
    }
    connect(m_segmentLengthSpin, &QDoubleSpinBox::valueChanged,
            this, QOverload<>::of(&PropertiesPanel::applyChanges));
    connect(m_equalMidpointCheck, &QCheckBox::toggled,
            this, QOverload<>::of(&PropertiesPanel::applyChanges));
    connect(m_verticalUCoordSpin, &QDoubleSpinBox::valueChanged,
            this, QOverload<>::of(&PropertiesPanel::applyVerticalCurveChanges));
    connect(m_verticalVclSpin, &QDoubleSpinBox::valueChanged,
            this, QOverload<>::of(&PropertiesPanel::applyVerticalCurveChanges));
    connect(m_verticalOffsetSpin, &QDoubleSpinBox::valueChanged,
            this, QOverload<>::of(&PropertiesPanel::applyVerticalCurveChanges));
    connect(m_removeVerticalCurveButton, &QPushButton::clicked,
            this, &PropertiesPanel::removeVerticalCurve);
    connect(m_bankUCoordSpin, &QDoubleSpinBox::valueChanged,
            this, QOverload<>::of(&PropertiesPanel::applyBankAngleChanges));
    connect(m_bankTargetSpeedSpin, &QDoubleSpinBox::valueChanged,
            this, QOverload<>::of(&PropertiesPanel::applyBankAngleChanges));
    connect(m_bankUseAngleCheck, &QCheckBox::toggled,
            this, QOverload<>::of(&PropertiesPanel::applyBankAngleChanges));
    connect(m_bankAngleSpin, &QDoubleSpinBox::valueChanged,
            this, QOverload<>::of(&PropertiesPanel::applyBankAngleChanges));
    connect(m_removeBankAngleButton, &QPushButton::clicked,
            this, &PropertiesPanel::removeBankAngle);
    connect(m_laneSectionUSpin, &QDoubleSpinBox::valueChanged,
            this, QOverload<>::of(&PropertiesPanel::applyLaneSectionChanges));
    for (size_t i = 0; i < laneSectionCheckControls.size(); ++i) {
        connect(laneSectionCheckControls[i], &QCheckBox::toggled,
                this, QOverload<>::of(&PropertiesPanel::applyLaneSectionChanges));
        connect(laneSectionSpinControls[i], &QDoubleSpinBox::valueChanged,
                this, QOverload<>::of(&PropertiesPanel::applyLaneSectionChanges));
    }
    connect(m_laneSectionOffsetCenterSpin, &QDoubleSpinBox::valueChanged,
            this, QOverload<>::of(&PropertiesPanel::applyLaneSectionChanges));
    connect(m_removeLaneSectionButton, &QPushButton::clicked,
            this, &PropertiesPanel::removeLaneSection);
    connect(m_socketNameEdit, &QLineEdit::editingFinished,
            this, &PropertiesPanel::applySocketChanges);
    connect(m_socketYawSpin, &QDoubleSpinBox::valueChanged,
            this, QOverload<>::of(&PropertiesPanel::applySocketChanges));
    connect(m_socketEnabledCheck, &QCheckBox::toggled,
            this, QOverload<>::of(&PropertiesPanel::applySocketChanges));
    connect(m_addSocketButton, &QPushButton::clicked,
            this, &PropertiesPanel::addSocket);
    connect(m_removeSocketButton, &QPushButton::clicked,
            this, &PropertiesPanel::removeSocket);

    root->addStretch();

    setRoadEnabled(false);
    setPointEnabled(false);
    setVerticalCurveEnabled(false);
    setBankAngleEnabled(false);
    setLaneSectionEnabled(false);
    setSocketEnabled(false);
}

QSize PropertiesPanel::sizeHint() const {
    return {320, QWidget::sizeHint().height()};
}

void PropertiesPanel::setNetwork(const RoadNetwork* net) {
    m_net = net;
}

std::array<QCheckBox*, 5> PropertiesPanel::roadLaneChecks() const {
    return {m_useLaneLeft2Check, m_useLaneLeft1Check, m_useLaneCenterCheck,
            m_useLaneRight1Check, m_useLaneRight2Check};
}

std::array<QDoubleSpinBox*, 5> PropertiesPanel::roadLaneSpins() const {
    return {m_widthLaneLeft2Spin, m_widthLaneLeft1Spin, m_widthLaneCenterSpin,
            m_widthLaneRight1Spin, m_widthLaneRight2Spin};
}

std::array<QCheckBox*, 5> PropertiesPanel::laneSectionChecks() const {
    return {m_laneSectionUseLaneLeft2Check, m_laneSectionUseLaneLeft1Check, m_laneSectionUseLaneCenterCheck,
            m_laneSectionUseLaneRight1Check, m_laneSectionUseLaneRight2Check};
}

std::array<QDoubleSpinBox*, 5> PropertiesPanel::laneSectionSpins() const {
    return {m_laneSectionWidthLaneLeft2Spin, m_laneSectionWidthLaneLeft1Spin, m_laneSectionWidthLaneCenterSpin,
            m_laneSectionWidthLaneRight1Spin, m_laneSectionWidthLaneRight2Spin};
}

void PropertiesPanel::setLaneControlsBlocked(const std::array<QCheckBox*, 5>& checks,
                                             const std::array<QDoubleSpinBox*, 5>& spins,
                                             bool blocked) {
    for (size_t i = 0; i < checks.size(); ++i) {
        checks[i]->blockSignals(blocked);
        spins[i]->blockSignals(blocked);
    }
}

void PropertiesPanel::setLaneControlsEnabled(const std::array<QCheckBox*, 5>& checks,
                                             const std::array<QDoubleSpinBox*, 5>& spins,
                                             bool enabled) {
    for (size_t i = 0; i < checks.size(); ++i) {
        checks[i]->setEnabled(enabled);
        spins[i]->setEnabled(enabled);
    }
}

void PropertiesPanel::setLaneControlsValues(const std::array<QCheckBox*, 5>& checks,
                                            const std::array<QDoubleSpinBox*, 5>& spins,
                                            const std::array<bool, 5>& enabledValues,
                                            const std::array<float, 5>& widthValues) {
    for (size_t i = 0; i < checks.size(); ++i) {
        checks[i]->setChecked(enabledValues[i]);
        spins[i]->setValue(widthValues[i]);
    }
}

void PropertiesPanel::onSelectionChanged(int roadIdx) {
    m_roadIdx = roadIdx;
    if (!m_net || roadIdx < 0 || roadIdx >= (int)m_net->roads.size()) {
        if (m_socketIdx < 0 && m_verticalCurveIdx < 0 && m_bankAngleIdx < 0) {
            m_nameLabel->setText("—");
            setRoadEnabled(false);
            setPointEnabled(false);
        }
        return;
    }
    populate(m_net->roads[roadIdx]);
    setRoadEnabled(true);
}

void PropertiesPanel::onSelectionStateChanged(const Selection& sel) {
    m_roadIdx = sel.roadIdx;
    m_pointIdx = sel.pointIdx;
    m_verticalCurveIdx = sel.verticalCurveIdx;
    m_bankAngleIdx = sel.bankAngleIdx;
    m_laneSectionIdx = sel.laneSectionIdx;
    m_intersectionIdx = sel.intersectionIdx;
    m_socketIdx = sel.socketIdx;

    if (m_net && sel.hasVerticalCurve() &&
        sel.roadIdx >= 0 && sel.roadIdx < (int)m_net->roads.size()) {
        populateVerticalCurve(m_net->roads[sel.roadIdx], sel.verticalCurveIdx);
        setPointEnabled(false);
        setVerticalCurveEnabled(true);
        setBankAngleEnabled(false);
        setLaneSectionEnabled(false);
        setSocketEnabled(false);
        setRoadEnabled(false);
        return;
    }

    setVerticalCurveEnabled(false);

    if (m_net && sel.hasBankAngle() &&
        sel.roadIdx >= 0 && sel.roadIdx < (int)m_net->roads.size()) {
        populateBankAngle(m_net->roads[sel.roadIdx], sel.bankAngleIdx);
        setVerticalCurveEnabled(false);
        setPointEnabled(false);
        setBankAngleEnabled(true);
        setLaneSectionEnabled(false);
        setSocketEnabled(false);
        setRoadEnabled(false);
        return;
    }

    setBankAngleEnabled(false);

    if (m_net && sel.hasLaneSection() &&
        sel.roadIdx >= 0 && sel.roadIdx < (int)m_net->roads.size()) {
        populateLaneSection(m_net->roads[sel.roadIdx], sel.laneSectionIdx);
        setVerticalCurveEnabled(false);
        setBankAngleEnabled(false);
        setPointEnabled(false);
        setLaneSectionEnabled(true);
        setSocketEnabled(false);
        setRoadEnabled(false);
        return;
    }

    setLaneSectionEnabled(false);

    if (m_net && sel.hasSocket() &&
        sel.intersectionIdx >= 0 && sel.intersectionIdx < (int)m_net->intersections.size()) {
        populateSocket(m_net->intersections[sel.intersectionIdx], sel.socketIdx);
        setPointEnabled(false);
        setSocketEnabled(true);
        setRoadEnabled(false);
        return;
    }

    setSocketEnabled(false);
    if (m_net && sel.valid()) {
        populatePoint(sel);
        setPointEnabled(true);
        setRoadEnabled(false);
        return;
    }

    setPointEnabled(false);
    if (m_net && m_roadIdx >= 0 && m_roadIdx < (int)m_net->roads.size()) {
        populate(m_net->roads[m_roadIdx]);
        setRoadEnabled(true);
    } else {
        m_nameLabel->setText("—");
        setRoadEnabled(false);
    }
}

void PropertiesPanel::onNetworkChanged() {
    if (m_net && m_roadIdx >= 0 && m_roadIdx < (int)m_net->roads.size())
        populate(m_net->roads[m_roadIdx]);
    if (m_net && m_pointIdx >= 0 &&
        m_roadIdx >= 0 && m_roadIdx < (int)m_net->roads.size() &&
        m_pointIdx < (int)m_net->roads[m_roadIdx].points.size()) {
        Selection sel;
        sel.setSinglePoint(m_roadIdx, m_pointIdx);
        populatePoint(sel);
    }
    if (m_net && m_verticalCurveIdx >= 0 &&
        m_roadIdx >= 0 && m_roadIdx < (int)m_net->roads.size())
        populateVerticalCurve(m_net->roads[m_roadIdx], m_verticalCurveIdx);
    if (m_net && m_bankAngleIdx >= 0 &&
        m_roadIdx >= 0 && m_roadIdx < (int)m_net->roads.size())
        populateBankAngle(m_net->roads[m_roadIdx], m_bankAngleIdx);
    if (m_net && m_laneSectionIdx >= 0 &&
        m_roadIdx >= 0 && m_roadIdx < (int)m_net->roads.size())
        populateLaneSection(m_net->roads[m_roadIdx], m_laneSectionIdx);
    if (m_net && m_socketIdx >= 0 &&
        m_intersectionIdx >= 0 && m_intersectionIdx < (int)m_net->intersections.size())
        populateSocket(m_net->intersections[m_intersectionIdx], m_socketIdx);
}

void PropertiesPanel::populate(const Road& road) {
    QString name = road.name.empty()
        ? QString::fromStdString(road.id)
        : QString::fromStdString(road.name);
    m_nameLabel->setText(name);

    m_speedSpin->blockSignals(true);
    setLaneControlsBlocked(roadLaneChecks(), roadLaneSpins(), true);
    m_segmentLengthSpin->blockSignals(true);
    m_equalMidpointCheck->blockSignals(true);

    m_speedSpin->setValue(road.defaultTargetSpeed);
    setLaneControlsValues(
        roadLaneChecks(), roadLaneSpins(),
        {road.useLaneLeft2, road.useLaneLeft1, road.useLaneCenter, road.useLaneRight1, road.useLaneRight2},
        {road.defaultWidthLaneLeft2, road.defaultWidthLaneLeft1, road.defaultWidthLaneCenter,
         road.defaultWidthLaneRight1, road.defaultWidthLaneRight2});

    m_segmentLengthSpin->setValue(road.segmentLength);
    m_equalMidpointCheck->setChecked(road.equalMidpoint);

    m_speedSpin->blockSignals(false);
    setLaneControlsBlocked(roadLaneChecks(), roadLaneSpins(), false);
    m_segmentLengthSpin->blockSignals(false);
    m_equalMidpointCheck->blockSignals(false);
}

void PropertiesPanel::populatePoint(const Selection& sel) {
    if (!m_net || sel.points.empty()) {
        m_nameLabel->setText("—");
        m_pointSummaryLabel->setText("—");
        m_pointRoadLabel->setText("—");
        m_pointIndexLabel->setText("—");
        m_pointXLabel->setText("—");
        m_pointYLabel->setText("—");
        m_pointZLabel->setText("—");
        return;
    }

    if (sel.points.size() > 1) {
        m_nameLabel->setText(QString("Points / %1 selected").arg((int)sel.points.size()));
        m_pointSummaryLabel->setText(QString("%1 points").arg((int)sel.points.size()));
        m_pointRoadLabel->setText("—");
        m_pointIndexLabel->setText("—");
        m_pointXLabel->setText("—");
        m_pointYLabel->setText("—");
        m_pointZLabel->setText("—");
        return;
    }

    const auto& selectedPoint = sel.points.front();
    if (selectedPoint.roadIdx < 0 || selectedPoint.roadIdx >= (int)m_net->roads.size())
        return;

    const auto& road = m_net->roads[selectedPoint.roadIdx];
    if (selectedPoint.pointIdx < 0 || selectedPoint.pointIdx >= (int)road.points.size())
        return;

    const QString roadName = road.name.empty()
        ? QString::fromStdString(road.id)
        : QString::fromStdString(road.name);
    const auto& point = road.points[selectedPoint.pointIdx];

    m_nameLabel->setText(roadName + QString(" / Point %1").arg(selectedPoint.pointIdx));
    m_pointSummaryLabel->setText("Single point");
    m_pointRoadLabel->setText(roadName);
    m_pointIndexLabel->setText(QString::number(selectedPoint.pointIdx));
    m_pointXLabel->setText(QString::number(point.pos.x, 'f', 3));
    m_pointYLabel->setText(QString::number(point.pos.y, 'f', 3));
    m_pointZLabel->setText(QString::number(point.pos.z, 'f', 3));
}

void PropertiesPanel::populateSocket(const Intersection& ix, int socketIdx) {
    if (socketIdx < 0 || socketIdx >= (int)ix.sockets.size()) return;

    const auto& socket = ix.sockets[socketIdx];
    QString ixName = ix.name.empty()
        ? QString::fromStdString(ix.id)
        : QString::fromStdString(ix.name);
    QString socketName = socket.name.empty()
        ? QString::fromStdString(socket.id)
        : QString::fromStdString(socket.name);
    m_nameLabel->setText(ixName + " / " + socketName);

    m_socketNameEdit->blockSignals(true);
    m_socketYawSpin->blockSignals(true);
    m_socketEnabledCheck->blockSignals(true);

    m_socketNameEdit->setText(QString::fromStdString(socket.name));
    m_socketYawSpin->setValue(socket.yaw * 180.0 / 3.14159265358979323846);
    m_socketEnabledCheck->setChecked(socket.enabled);

    m_socketNameEdit->blockSignals(false);
    m_socketYawSpin->blockSignals(false);
    m_socketEnabledCheck->blockSignals(false);
}

void PropertiesPanel::populateVerticalCurve(const Road& road, int verticalCurveIdx) {
    if (verticalCurveIdx < 0 || verticalCurveIdx >= (int)road.verticalCurve.size()) return;

    const auto& point = road.verticalCurve[verticalCurveIdx];
    QString roadName = road.name.empty()
        ? QString::fromStdString(road.id)
        : QString::fromStdString(road.name);
    m_nameLabel->setText(roadName + QString(" / Vertical %1").arg(verticalCurveIdx));

    m_verticalUCoordSpin->blockSignals(true);
    m_verticalVclSpin->blockSignals(true);
    m_verticalOffsetSpin->blockSignals(true);

    m_verticalUCoordSpin->setValue(point.u);
    m_verticalVclSpin->setValue(point.vcl);
    m_verticalOffsetSpin->setValue(point.offset);

    m_verticalUCoordSpin->blockSignals(false);
    m_verticalVclSpin->blockSignals(false);
    m_verticalOffsetSpin->blockSignals(false);
}

void PropertiesPanel::populateBankAngle(const Road& road, int bankAngleIdx) {
    if (bankAngleIdx < 0 || bankAngleIdx >= (int)road.bankAngle.size()) return;

    const auto& point = road.bankAngle[bankAngleIdx];
    QString roadName = road.name.empty()
        ? QString::fromStdString(road.id)
        : QString::fromStdString(road.name);
    m_nameLabel->setText(roadName + QString(" / Bank %1").arg(bankAngleIdx));

    m_bankUCoordSpin->blockSignals(true);
    m_bankTargetSpeedSpin->blockSignals(true);
    m_bankUseAngleCheck->blockSignals(true);
    m_bankAngleSpin->blockSignals(true);

    m_bankUCoordSpin->setValue(point.u);
    m_bankTargetSpeedSpin->setValue(point.targetSpeed);
    m_bankUseAngleCheck->setChecked(point.useAngle != 0);
    m_bankAngleSpin->setValue(point.angle);
    m_bankAngleSpin->setEnabled(point.useAngle != 0);

    m_bankUCoordSpin->blockSignals(false);
    m_bankTargetSpeedSpin->blockSignals(false);
    m_bankUseAngleCheck->blockSignals(false);
    m_bankAngleSpin->blockSignals(false);
}

void PropertiesPanel::populateLaneSection(const Road& road, int laneSectionIdx) {
    if (laneSectionIdx < 0 || laneSectionIdx >= (int)road.laneSections.size()) return;

    const auto& point = road.laneSections[laneSectionIdx];
    QString roadName = road.name.empty()
        ? QString::fromStdString(road.id)
        : QString::fromStdString(road.name);
    m_nameLabel->setText(roadName + QString(" / Lane %1").arg(laneSectionIdx));

    m_laneSectionUSpin->blockSignals(true);
    setLaneControlsBlocked(laneSectionChecks(), laneSectionSpins(), true);
    m_laneSectionOffsetCenterSpin->blockSignals(true);

    m_laneSectionUSpin->setValue(point.u);
    setLaneControlsValues(
        laneSectionChecks(), laneSectionSpins(),
        {point.useLaneLeft2, point.useLaneLeft1, point.useLaneCenter, point.useLaneRight1, point.useLaneRight2},
        {point.widthLaneLeft2, point.widthLaneLeft1, point.widthLaneCenter,
         point.widthLaneRight1, point.widthLaneRight2});
    m_laneSectionOffsetCenterSpin->setValue(point.offsetCenter);

    m_laneSectionUSpin->blockSignals(false);
    setLaneControlsBlocked(laneSectionChecks(), laneSectionSpins(), false);
    m_laneSectionOffsetCenterSpin->blockSignals(false);
}

void PropertiesPanel::setRoadEnabled(bool on) {
    m_speedGroup->setVisible(on);
    m_laneGroup->setVisible(on);
    m_meshGroup->setVisible(on);
    m_speedSpin->setEnabled(on);
    setLaneControlsEnabled(roadLaneChecks(), roadLaneSpins(), on);
    m_segmentLengthSpin->setEnabled(on);
    m_equalMidpointCheck->setEnabled(on);
}

void PropertiesPanel::setPointEnabled(bool on) {
    m_pointGroup->setVisible(on);
}

void PropertiesPanel::setSocketEnabled(bool on) {
    m_socketGroup->setVisible(on);
    m_socketNameEdit->setEnabled(on);
    m_socketYawSpin->setEnabled(on);
    m_socketEnabledCheck->setEnabled(on);
    m_addSocketButton->setEnabled(on);
    m_removeSocketButton->setEnabled(on);
}

void PropertiesPanel::setVerticalCurveEnabled(bool on) {
    m_verticalCurveGroup->setVisible(on);
    m_verticalUCoordSpin->setEnabled(on);
    m_verticalVclSpin->setEnabled(on);
    m_verticalOffsetSpin->setEnabled(on);
    m_removeVerticalCurveButton->setEnabled(on);
}

void PropertiesPanel::setBankAngleEnabled(bool on) {
    m_bankAngleGroup->setVisible(on);
    m_bankUCoordSpin->setEnabled(on);
    m_bankTargetSpeedSpin->setEnabled(on);
    m_bankUseAngleCheck->setEnabled(on);
    m_bankAngleSpin->setEnabled(on && m_bankUseAngleCheck->isChecked());
    m_removeBankAngleButton->setEnabled(on);
}

void PropertiesPanel::setLaneSectionEnabled(bool on) {
    m_laneSectionGroup->setVisible(on);
    m_laneSectionUSpin->setEnabled(on);
    setLaneControlsEnabled(laneSectionChecks(), laneSectionSpins(), on);
    m_laneSectionOffsetCenterSpin->setEnabled(on);
    m_removeLaneSectionButton->setEnabled(on);
}

void PropertiesPanel::applyChanges() {
    if (!m_net || m_roadIdx < 0 || m_roadIdx >= (int)m_net->roads.size()) return;

    RoadProperties p;
    p.speed          = static_cast<float>(m_speedSpin->value());
    p.useLaneLeft2   = m_useLaneLeft2Check->isChecked();
    p.widthLaneLeft2 = static_cast<float>(m_widthLaneLeft2Spin->value());
    p.useLaneLeft1   = m_useLaneLeft1Check->isChecked();
    p.widthLaneLeft1 = static_cast<float>(m_widthLaneLeft1Spin->value());
    p.useLaneCenter  = m_useLaneCenterCheck->isChecked();
    p.widthLaneCenter = static_cast<float>(m_widthLaneCenterSpin->value());
    p.useLaneRight1  = m_useLaneRight1Check->isChecked();
    p.widthLaneRight1 = static_cast<float>(m_widthLaneRight1Spin->value());
    p.useLaneRight2  = m_useLaneRight2Check->isChecked();
    p.widthLaneRight2 = static_cast<float>(m_widthLaneRight2Spin->value());
    p.segmentLength  = static_cast<float>(m_segmentLengthSpin->value());
    p.equalMidpoint  = m_equalMidpointCheck->isChecked();

    emit roadModified(m_roadIdx, p);
}

void PropertiesPanel::applySocketChanges() {
    if (!m_net || m_intersectionIdx < 0 || m_socketIdx < 0) return;
    emit selectedSocketModified(
        m_socketNameEdit->text(),
        static_cast<float>(m_socketYawSpin->value() * 3.14159265358979323846 / 180.0),
        m_socketEnabledCheck->isChecked());
}

void PropertiesPanel::applyVerticalCurveChanges() {
    if (!m_net || m_roadIdx < 0 || m_verticalCurveIdx < 0) return;
    emit selectedVerticalCurveModified(
        m_roadIdx,
        m_verticalCurveIdx,
        static_cast<float>(m_verticalUCoordSpin->value()),
        static_cast<float>(m_verticalVclSpin->value()),
        static_cast<float>(m_verticalOffsetSpin->value()));
}

void PropertiesPanel::applyBankAngleChanges() {
    if (!m_net || m_roadIdx < 0 || m_bankAngleIdx < 0) return;
    m_bankAngleSpin->setEnabled(m_bankUseAngleCheck->isChecked());
    emit selectedBankAngleModified(
        m_roadIdx,
        m_bankAngleIdx,
        static_cast<float>(m_bankUCoordSpin->value()),
        static_cast<float>(m_bankTargetSpeedSpin->value()),
        m_bankUseAngleCheck->isChecked(),
        static_cast<float>(m_bankAngleSpin->value()));
}

void PropertiesPanel::applyLaneSectionChanges() {
    if (!m_net || m_roadIdx < 0 || m_laneSectionIdx < 0) return;
    emit selectedLaneSectionModified(
        m_roadIdx,
        m_laneSectionIdx,
        static_cast<float>(m_laneSectionUSpin->value()),
        m_laneSectionUseLaneLeft2Check->isChecked(),
        static_cast<float>(m_laneSectionWidthLaneLeft2Spin->value()),
        m_laneSectionUseLaneLeft1Check->isChecked(),
        static_cast<float>(m_laneSectionWidthLaneLeft1Spin->value()),
        m_laneSectionUseLaneCenterCheck->isChecked(),
        static_cast<float>(m_laneSectionWidthLaneCenterSpin->value()),
        m_laneSectionUseLaneRight1Check->isChecked(),
        static_cast<float>(m_laneSectionWidthLaneRight1Spin->value()),
        m_laneSectionUseLaneRight2Check->isChecked(),
        static_cast<float>(m_laneSectionWidthLaneRight2Spin->value()),
        static_cast<float>(m_laneSectionOffsetCenterSpin->value()));
}

void PropertiesPanel::removeVerticalCurve() {
    if (!m_net || m_roadIdx < 0 || m_verticalCurveIdx < 0) return;
    emit removeSelectedVerticalCurveRequested(m_roadIdx, m_verticalCurveIdx);
}

void PropertiesPanel::removeBankAngle() {
    if (!m_net || m_roadIdx < 0 || m_bankAngleIdx < 0) return;
    emit removeSelectedBankAngleRequested(m_roadIdx, m_bankAngleIdx);
}

void PropertiesPanel::removeLaneSection() {
    if (!m_net || m_roadIdx < 0 || m_laneSectionIdx < 0) return;
    emit removeSelectedLaneSectionRequested(m_roadIdx, m_laneSectionIdx);
}

void PropertiesPanel::addSocket() {
    emit addSocketRequested();
}

void PropertiesPanel::removeSocket() {
    emit removeSocketRequested();
}
