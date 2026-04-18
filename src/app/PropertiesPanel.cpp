#include "PropertiesPanel.h"
#include <QCheckBox>
#include <QFormLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QDoubleSpinBox>
#include <QPushButton>
#include <QGroupBox>

PropertiesPanel::PropertiesPanel(QWidget* parent)
    : QWidget(parent)
{
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(6, 6, 6, 6);
    root->setSpacing(4);

    m_nameLabel = new QLabel("—", this);
    m_nameLabel->setStyleSheet("font-weight: bold; color: #aaa;");
    root->addWidget(m_nameLabel);

    auto* grp = new QGroupBox("Road Parameters", this);
    auto* form = new QFormLayout(grp);
    form->setLabelAlignment(Qt::AlignRight);

    m_speedSpin = new QDoubleSpinBox(this);
    m_speedSpin->setRange(0, 200); m_speedSpin->setSuffix(" km/h"); m_speedSpin->setDecimals(1);
    form->addRow("Target Speed:", m_speedSpin);

    m_leftWidthSpin = new QDoubleSpinBox(this);
    m_leftWidthSpin->setRange(0, 50); m_leftWidthSpin->setSuffix(" m"); m_leftWidthSpin->setDecimals(2);
    form->addRow("Left Width:", m_leftWidthSpin);

    m_rightWidthSpin = new QDoubleSpinBox(this);
    m_rightWidthSpin->setRange(0, 50); m_rightWidthSpin->setSuffix(" m"); m_rightWidthSpin->setDecimals(2);
    form->addRow("Right Width:", m_rightWidthSpin);

    m_segmentLengthSpin = new QDoubleSpinBox(this);
    m_segmentLengthSpin->setRange(0.1, 50.0);
    m_segmentLengthSpin->setValue(1.0);
    m_segmentLengthSpin->setSuffix(" m");
    m_segmentLengthSpin->setDecimals(2);
    m_segmentLengthSpin->setSingleStep(0.5);
    m_segmentLengthSpin->setToolTip("Mesh tessellation interval along the road direction");
    form->addRow("Segment Length:", m_segmentLengthSpin);

    m_equalMidpointCheck = new QCheckBox("Equal midpoint (t=0.5)", this);
    m_equalMidpointCheck->setToolTip(
        "Checked: bend pivot is at the exact midpoint of each edge\n"
        "Unchecked: pivot is weighted by adjacent edge lengths (proportional)");
    form->addRow("Midpoint split:", m_equalMidpointCheck);

    root->addWidget(grp);

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

    m_speedSpin->blockSignals(true);
    m_leftWidthSpin->blockSignals(true);
    m_rightWidthSpin->blockSignals(true);
    m_segmentLengthSpin->blockSignals(true);
    m_equalMidpointCheck->blockSignals(true);

    m_speedSpin->setValue(road.defaultTargetSpeed);
    m_leftWidthSpin->setValue(road.defaultWidthLaneLeft1);
    m_rightWidthSpin->setValue(road.defaultWidthLaneRight1);
    m_segmentLengthSpin->setValue(static_cast<double>(road.segmentLength));
    m_equalMidpointCheck->setChecked(road.equalMidpoint);

    m_speedSpin->blockSignals(false);
    m_leftWidthSpin->blockSignals(false);
    m_rightWidthSpin->blockSignals(false);
    m_segmentLengthSpin->blockSignals(false);
    m_equalMidpointCheck->blockSignals(false);
}

void PropertiesPanel::setEnabled(bool on) {
    m_speedSpin->setEnabled(on);
    m_leftWidthSpin->setEnabled(on);
    m_rightWidthSpin->setEnabled(on);
    m_segmentLengthSpin->setEnabled(on);
    m_equalMidpointCheck->setEnabled(on);
}

void PropertiesPanel::applyChanges() {
    if (!m_net || m_roadIdx < 0 || m_roadIdx >= (int)m_net->roads.size()) return;
    emit roadModified(m_roadIdx,
                      static_cast<float>(m_speedSpin->value()),
                      static_cast<float>(m_leftWidthSpin->value()),
                      static_cast<float>(m_rightWidthSpin->value()),
                      static_cast<float>(m_segmentLengthSpin->value()),
                      m_equalMidpointCheck->isChecked());
}
