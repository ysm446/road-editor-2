#include "PropertiesPanel.h"
#include <QCheckBox>
#include <QFormLayout>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QDoubleSpinBox>
#include <QPushButton>
#include <QGroupBox>

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
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(6, 6, 6, 6);
    root->setSpacing(4);

    m_nameLabel = new QLabel("—", this);
    m_nameLabel->setStyleSheet("font-weight: bold; color: #aaa;");
    root->addWidget(m_nameLabel);

    // --- Speed ---
    auto* speedGrp  = new QGroupBox("Speed", this);
    auto* speedForm = new QFormLayout(speedGrp);
    speedForm->setLabelAlignment(Qt::AlignRight);
    m_speedSpin = new QDoubleSpinBox(this);
    m_speedSpin->setRange(0, 200);
    m_speedSpin->setSuffix(" km/h");
    m_speedSpin->setDecimals(1);
    speedForm->addRow("Target Speed:", m_speedSpin);
    root->addWidget(speedGrp);

    // --- Lanes ---
    auto* laneGrp  = new QGroupBox("Lanes (left=negative, right=positive)", this);
    auto* laneForm = new QFormLayout(laneGrp);
    laneForm->setLabelAlignment(Qt::AlignRight);

    addLaneRow(laneForm, "Left 2  (lane -2)", m_useLaneLeft2Check,  m_widthLaneLeft2Spin,  false, 3.5, this);
    addLaneRow(laneForm, "Left 1  (lane -1)", m_useLaneLeft1Check,  m_widthLaneLeft1Spin,  true,  4.0, this);
    addLaneRow(laneForm, "Center  (lane  0)", m_useLaneCenterCheck, m_widthLaneCenterSpin, false, 0.0, this);
    addLaneRow(laneForm, "Right 1 (lane +1)", m_useLaneRight1Check, m_widthLaneRight1Spin, true,  4.0, this);
    addLaneRow(laneForm, "Right 2 (lane +2)", m_useLaneRight2Check, m_widthLaneRight2Spin, false, 3.5, this);
    root->addWidget(laneGrp);

    // --- Mesh / curve settings ---
    auto* meshGrp  = new QGroupBox("Mesh", this);
    auto* meshForm = new QFormLayout(meshGrp);
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
    root->addWidget(meshGrp);

    auto* applyBtn = new QPushButton("Apply", this);
    connect(applyBtn, &QPushButton::clicked, this, &PropertiesPanel::applyChanges);
    root->addWidget(applyBtn);

    root->addStretch();

    setEnabled(false);
}

void PropertiesPanel::setNetwork(const RoadNetwork* net) {
    m_net = net;
}

void PropertiesPanel::onSelectionChanged(int roadIdx) {
    m_roadIdx = roadIdx;
    if (!m_net || roadIdx < 0 || roadIdx >= (int)m_net->roads.size()) {
        m_nameLabel->setText("—");
        setEnabled(false);
        return;
    }
    populate(m_net->roads[roadIdx]);
    setEnabled(true);
}

void PropertiesPanel::onNetworkChanged() {
    if (m_net && m_roadIdx >= 0 && m_roadIdx < (int)m_net->roads.size())
        populate(m_net->roads[m_roadIdx]);
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

void PropertiesPanel::setEnabled(bool on) {
    m_speedSpin->setEnabled(on);
    m_useLaneLeft2Check->setEnabled(on);  m_widthLaneLeft2Spin->setEnabled(on);
    m_useLaneLeft1Check->setEnabled(on);  m_widthLaneLeft1Spin->setEnabled(on);
    m_useLaneCenterCheck->setEnabled(on); m_widthLaneCenterSpin->setEnabled(on);
    m_useLaneRight1Check->setEnabled(on); m_widthLaneRight1Spin->setEnabled(on);
    m_useLaneRight2Check->setEnabled(on); m_widthLaneRight2Spin->setEnabled(on);
    m_segmentLengthSpin->setEnabled(on);
    m_equalMidpointCheck->setEnabled(on);
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
