#include "PropertiesPanel.h"
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

    m_speedSpin->setValue(road.defaultTargetSpeed);
    m_leftWidthSpin->setValue(road.defaultWidthLaneLeft1);
    m_rightWidthSpin->setValue(road.defaultWidthLaneRight1);

    m_speedSpin->blockSignals(false);
    m_leftWidthSpin->blockSignals(false);
    m_rightWidthSpin->blockSignals(false);
}

void PropertiesPanel::setEnabled(bool on) {
    m_speedSpin->setEnabled(on);
    m_leftWidthSpin->setEnabled(on);
    m_rightWidthSpin->setEnabled(on);
}

void PropertiesPanel::applyChanges() {
    if (!m_net || m_roadIdx < 0 || m_roadIdx >= (int)m_net->roads.size()) return;
    emit roadModified(m_roadIdx);
}
