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

static QDoubleSpinBox* makeWidthSpin(QWidget* parent) {
    auto* s = new QDoubleSpinBox(parent);
    s->setRange(0.1, 20.0);
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

    connect(m_speedSpin, &QDoubleSpinBox::valueChanged,
            this, QOverload<>::of(&PropertiesPanel::applyChanges));

    connect(m_useLaneLeft2Check, &QCheckBox::toggled,
            this, QOverload<>::of(&PropertiesPanel::applyChanges));
    connect(m_widthLaneLeft2Spin, &QDoubleSpinBox::valueChanged,
            this, QOverload<>::of(&PropertiesPanel::applyChanges));
    connect(m_useLaneLeft1Check, &QCheckBox::toggled,
            this, QOverload<>::of(&PropertiesPanel::applyChanges));
    connect(m_widthLaneLeft1Spin, &QDoubleSpinBox::valueChanged,
            this, QOverload<>::of(&PropertiesPanel::applyChanges));
    connect(m_useLaneCenterCheck, &QCheckBox::toggled,
            this, QOverload<>::of(&PropertiesPanel::applyChanges));
    connect(m_widthLaneCenterSpin, &QDoubleSpinBox::valueChanged,
            this, QOverload<>::of(&PropertiesPanel::applyChanges));
    connect(m_useLaneRight1Check, &QCheckBox::toggled,
            this, QOverload<>::of(&PropertiesPanel::applyChanges));
    connect(m_widthLaneRight1Spin, &QDoubleSpinBox::valueChanged,
            this, QOverload<>::of(&PropertiesPanel::applyChanges));
    connect(m_useLaneRight2Check, &QCheckBox::toggled,
            this, QOverload<>::of(&PropertiesPanel::applyChanges));
    connect(m_widthLaneRight2Spin, &QDoubleSpinBox::valueChanged,
            this, QOverload<>::of(&PropertiesPanel::applyChanges));
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
    connect(m_laneSectionUseLaneLeft2Check, &QCheckBox::toggled,
            this, QOverload<>::of(&PropertiesPanel::applyLaneSectionChanges));
    connect(m_laneSectionWidthLaneLeft2Spin, &QDoubleSpinBox::valueChanged,
            this, QOverload<>::of(&PropertiesPanel::applyLaneSectionChanges));
    connect(m_laneSectionUseLaneLeft1Check, &QCheckBox::toggled,
            this, QOverload<>::of(&PropertiesPanel::applyLaneSectionChanges));
    connect(m_laneSectionWidthLaneLeft1Spin, &QDoubleSpinBox::valueChanged,
            this, QOverload<>::of(&PropertiesPanel::applyLaneSectionChanges));
    connect(m_laneSectionUseLaneCenterCheck, &QCheckBox::toggled,
            this, QOverload<>::of(&PropertiesPanel::applyLaneSectionChanges));
    connect(m_laneSectionWidthLaneCenterSpin, &QDoubleSpinBox::valueChanged,
            this, QOverload<>::of(&PropertiesPanel::applyLaneSectionChanges));
    connect(m_laneSectionUseLaneRight1Check, &QCheckBox::toggled,
            this, QOverload<>::of(&PropertiesPanel::applyLaneSectionChanges));
    connect(m_laneSectionWidthLaneRight1Spin, &QDoubleSpinBox::valueChanged,
            this, QOverload<>::of(&PropertiesPanel::applyLaneSectionChanges));
    connect(m_laneSectionUseLaneRight2Check, &QCheckBox::toggled,
            this, QOverload<>::of(&PropertiesPanel::applyLaneSectionChanges));
    connect(m_laneSectionWidthLaneRight2Spin, &QDoubleSpinBox::valueChanged,
            this, QOverload<>::of(&PropertiesPanel::applyLaneSectionChanges));
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

void PropertiesPanel::onSelectionChanged(int roadIdx) {
    m_roadIdx = roadIdx;
    if (!m_net || roadIdx < 0 || roadIdx >= (int)m_net->roads.size()) {
        if (m_socketIdx < 0 && m_verticalCurveIdx < 0 && m_bankAngleIdx < 0) {
            m_nameLabel->setText("—");
            setRoadEnabled(false);
        }
        return;
    }
    populate(m_net->roads[roadIdx]);
    setRoadEnabled(true);
}

void PropertiesPanel::onSelectionStateChanged(const Selection& sel) {
    m_roadIdx = sel.roadIdx;
    m_verticalCurveIdx = sel.verticalCurveIdx;
    m_bankAngleIdx = sel.bankAngleIdx;
    m_laneSectionIdx = sel.laneSectionIdx;
    m_intersectionIdx = sel.intersectionIdx;
    m_socketIdx = sel.socketIdx;

    if (m_net && sel.hasVerticalCurve() &&
        sel.roadIdx >= 0 && sel.roadIdx < (int)m_net->roads.size()) {
        populateVerticalCurve(m_net->roads[sel.roadIdx], sel.verticalCurveIdx);
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
        setLaneSectionEnabled(true);
        setSocketEnabled(false);
        setRoadEnabled(false);
        return;
    }

    setLaneSectionEnabled(false);

    if (m_net && sel.hasSocket() &&
        sel.intersectionIdx >= 0 && sel.intersectionIdx < (int)m_net->intersections.size()) {
        populateSocket(m_net->intersections[sel.intersectionIdx], sel.socketIdx);
        setSocketEnabled(true);
        setRoadEnabled(false);
        return;
    }

    setSocketEnabled(false);
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

    auto blockAll = [&](bool b) {
        m_speedSpin->blockSignals(b);
        m_useLaneLeft2Check->blockSignals(b);  m_widthLaneLeft2Spin->blockSignals(b);
        m_useLaneLeft1Check->blockSignals(b);  m_widthLaneLeft1Spin->blockSignals(b);
        m_useLaneCenterCheck->blockSignals(b); m_widthLaneCenterSpin->blockSignals(b);
        m_useLaneRight1Check->blockSignals(b); m_widthLaneRight1Spin->blockSignals(b);
        m_useLaneRight2Check->blockSignals(b); m_widthLaneRight2Spin->blockSignals(b);
        m_segmentLengthSpin->blockSignals(b);
        m_equalMidpointCheck->blockSignals(b);
    };
    blockAll(true);

    m_speedSpin->setValue(road.defaultTargetSpeed);

    m_useLaneLeft2Check->setChecked(road.useLaneLeft2);
    m_widthLaneLeft2Spin->setValue(road.defaultWidthLaneLeft2);
    m_useLaneLeft1Check->setChecked(road.useLaneLeft1);
    m_widthLaneLeft1Spin->setValue(road.defaultWidthLaneLeft1);
    m_useLaneCenterCheck->setChecked(road.useLaneCenter);
    m_widthLaneCenterSpin->setValue(road.defaultWidthLaneCenter);
    m_useLaneRight1Check->setChecked(road.useLaneRight1);
    m_widthLaneRight1Spin->setValue(road.defaultWidthLaneRight1);
    m_useLaneRight2Check->setChecked(road.useLaneRight2);
    m_widthLaneRight2Spin->setValue(road.defaultWidthLaneRight2);

    m_segmentLengthSpin->setValue(road.segmentLength);
    m_equalMidpointCheck->setChecked(road.equalMidpoint);

    blockAll(false);
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
    m_laneSectionUseLaneLeft2Check->blockSignals(true);
    m_laneSectionWidthLaneLeft2Spin->blockSignals(true);
    m_laneSectionUseLaneLeft1Check->blockSignals(true);
    m_laneSectionWidthLaneLeft1Spin->blockSignals(true);
    m_laneSectionUseLaneCenterCheck->blockSignals(true);
    m_laneSectionWidthLaneCenterSpin->blockSignals(true);
    m_laneSectionUseLaneRight1Check->blockSignals(true);
    m_laneSectionWidthLaneRight1Spin->blockSignals(true);
    m_laneSectionUseLaneRight2Check->blockSignals(true);
    m_laneSectionWidthLaneRight2Spin->blockSignals(true);
    m_laneSectionOffsetCenterSpin->blockSignals(true);

    m_laneSectionUSpin->setValue(point.u);
    m_laneSectionUseLaneLeft2Check->setChecked(point.useLaneLeft2);
    m_laneSectionWidthLaneLeft2Spin->setValue(point.widthLaneLeft2);
    m_laneSectionUseLaneLeft1Check->setChecked(point.useLaneLeft1);
    m_laneSectionWidthLaneLeft1Spin->setValue(point.widthLaneLeft1);
    m_laneSectionUseLaneCenterCheck->setChecked(point.useLaneCenter);
    m_laneSectionWidthLaneCenterSpin->setValue(point.widthLaneCenter);
    m_laneSectionUseLaneRight1Check->setChecked(point.useLaneRight1);
    m_laneSectionWidthLaneRight1Spin->setValue(point.widthLaneRight1);
    m_laneSectionUseLaneRight2Check->setChecked(point.useLaneRight2);
    m_laneSectionWidthLaneRight2Spin->setValue(point.widthLaneRight2);
    m_laneSectionOffsetCenterSpin->setValue(point.offsetCenter);

    m_laneSectionUSpin->blockSignals(false);
    m_laneSectionUseLaneLeft2Check->blockSignals(false);
    m_laneSectionWidthLaneLeft2Spin->blockSignals(false);
    m_laneSectionUseLaneLeft1Check->blockSignals(false);
    m_laneSectionWidthLaneLeft1Spin->blockSignals(false);
    m_laneSectionUseLaneCenterCheck->blockSignals(false);
    m_laneSectionWidthLaneCenterSpin->blockSignals(false);
    m_laneSectionUseLaneRight1Check->blockSignals(false);
    m_laneSectionWidthLaneRight1Spin->blockSignals(false);
    m_laneSectionUseLaneRight2Check->blockSignals(false);
    m_laneSectionWidthLaneRight2Spin->blockSignals(false);
    m_laneSectionOffsetCenterSpin->blockSignals(false);
}

void PropertiesPanel::setRoadEnabled(bool on) {
    m_speedGroup->setVisible(on);
    m_laneGroup->setVisible(on);
    m_meshGroup->setVisible(on);
    m_speedSpin->setEnabled(on);
    m_useLaneLeft2Check->setEnabled(on);  m_widthLaneLeft2Spin->setEnabled(on);
    m_useLaneLeft1Check->setEnabled(on);  m_widthLaneLeft1Spin->setEnabled(on);
    m_useLaneCenterCheck->setEnabled(on); m_widthLaneCenterSpin->setEnabled(on);
    m_useLaneRight1Check->setEnabled(on); m_widthLaneRight1Spin->setEnabled(on);
    m_useLaneRight2Check->setEnabled(on); m_widthLaneRight2Spin->setEnabled(on);
    m_segmentLengthSpin->setEnabled(on);
    m_equalMidpointCheck->setEnabled(on);
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
    m_laneSectionUseLaneLeft2Check->setEnabled(on);
    m_laneSectionWidthLaneLeft2Spin->setEnabled(on);
    m_laneSectionUseLaneLeft1Check->setEnabled(on);
    m_laneSectionWidthLaneLeft1Spin->setEnabled(on);
    m_laneSectionUseLaneCenterCheck->setEnabled(on);
    m_laneSectionWidthLaneCenterSpin->setEnabled(on);
    m_laneSectionUseLaneRight1Check->setEnabled(on);
    m_laneSectionWidthLaneRight1Spin->setEnabled(on);
    m_laneSectionUseLaneRight2Check->setEnabled(on);
    m_laneSectionWidthLaneRight2Spin->setEnabled(on);
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
