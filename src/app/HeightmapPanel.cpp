#include "HeightmapPanel.h"
#include <QDir>
#include <QDoubleSpinBox>
#include <QFileInfo>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QCheckBox>
#include <QSignalBlocker>
#include <QSpinBox>
#include <QVBoxLayout>

HeightmapPanel::HeightmapPanel(QWidget* parent)
    : QWidget(parent) {
    setMinimumWidth(260);

    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(6, 6, 6, 6);
    root->setSpacing(6);

    auto* sourceGroup = new QGroupBox("Source", this);
    auto* sourceLayout = new QVBoxLayout(sourceGroup);
    sourceLayout->setContentsMargins(6, 6, 6, 6);
    sourceLayout->setSpacing(6);

    m_pathLabel = new QLabel("No heightmap loaded", this);
    m_pathLabel->setWordWrap(true);
    sourceLayout->addWidget(m_pathLabel);

    auto* buttonRow = new QHBoxLayout;
    buttonRow->setContentsMargins(0, 0, 0, 0);
    m_importButton = new QPushButton("Import Heightmap...", this);
    m_clearButton = new QPushButton("Clear", this);
    buttonRow->addWidget(m_importButton);
    buttonRow->addWidget(m_clearButton);
    sourceLayout->addLayout(buttonRow);

    m_texturePathLabel = new QLabel("No texture loaded", this);
    m_texturePathLabel->setWordWrap(true);
    sourceLayout->addWidget(m_texturePathLabel);

    auto* textureButtonRow = new QHBoxLayout;
    textureButtonRow->setContentsMargins(0, 0, 0, 0);
    m_importTextureButton = new QPushButton("Import Texture...", this);
    m_clearTextureButton = new QPushButton("Clear Texture", this);
    textureButtonRow->addWidget(m_importTextureButton);
    textureButtonRow->addWidget(m_clearTextureButton);
    sourceLayout->addLayout(textureButtonRow);
    root->addWidget(sourceGroup);

    auto* settingsGroup = new QGroupBox("Settings", this);
    auto* form = new QFormLayout(settingsGroup);
    form->setLabelAlignment(Qt::AlignRight);

    m_visibleCheck = new QCheckBox("Show Heightmap", this);
    form->addRow("Display:", m_visibleCheck);

    m_snapWhileMovingCheck = new QCheckBox("Snap While Moving", this);
    form->addRow("Edit:", m_snapWhileMovingCheck);

    m_sizeSpin = new QDoubleSpinBox(this);
    m_sizeSpin->setRange(1.0, 100000.0);
    m_sizeSpin->setDecimals(2);
    m_sizeSpin->setSingleStep(10.0);
    m_sizeSpin->setButtonSymbols(QAbstractSpinBox::NoButtons);
    m_sizeSpin->setSuffix(" m");
    form->addRow("Size:", m_sizeSpin);

    m_heightSpin = new QDoubleSpinBox(this);
    m_heightSpin->setRange(0.1, 100000.0);
    m_heightSpin->setDecimals(2);
    m_heightSpin->setSingleStep(10.0);
    m_heightSpin->setButtonSymbols(QAbstractSpinBox::NoButtons);
    m_heightSpin->setSuffix(" m");
    form->addRow("Height Scale:", m_heightSpin);

    m_offsetXSpin = new QDoubleSpinBox(this);
    m_offsetXSpin->setRange(-100000.0, 100000.0);
    m_offsetXSpin->setDecimals(2);
    m_offsetXSpin->setSingleStep(10.0);
    m_offsetXSpin->setButtonSymbols(QAbstractSpinBox::NoButtons);
    m_offsetXSpin->setPrefix("X ");
    m_offsetXSpin->setSuffix(" m");

    m_offsetYSpin = new QDoubleSpinBox(this);
    m_offsetYSpin->setRange(-100000.0, 100000.0);
    m_offsetYSpin->setDecimals(2);
    m_offsetYSpin->setSingleStep(10.0);
    m_offsetYSpin->setButtonSymbols(QAbstractSpinBox::NoButtons);
    m_offsetYSpin->setPrefix("Y ");
    m_offsetYSpin->setSuffix(" m");

    m_offsetZSpin = new QDoubleSpinBox(this);
    m_offsetZSpin->setRange(-100000.0, 100000.0);
    m_offsetZSpin->setDecimals(2);
    m_offsetZSpin->setSingleStep(10.0);
    m_offsetZSpin->setButtonSymbols(QAbstractSpinBox::NoButtons);
    m_offsetZSpin->setPrefix("Z ");
    m_offsetZSpin->setSuffix(" m");
    auto* offsetRow = new QHBoxLayout;
    offsetRow->setContentsMargins(0, 0, 0, 0);
    offsetRow->addWidget(m_offsetXSpin);
    offsetRow->addWidget(m_offsetYSpin);
    offsetRow->addWidget(m_offsetZSpin);
    form->addRow("Offset:", offsetRow);

    m_meshCellsXSpin = new QSpinBox(this);
    m_meshCellsXSpin->setRange(0, 8192);
    m_meshCellsXSpin->setButtonSymbols(QAbstractSpinBox::NoButtons);
    m_meshCellsXSpin->setPrefix("X ");
    m_meshCellsXSpin->setSpecialValueText("Auto");

    m_meshCellsZSpin = new QSpinBox(this);
    m_meshCellsZSpin->setRange(0, 8192);
    m_meshCellsZSpin->setButtonSymbols(QAbstractSpinBox::NoButtons);
    m_meshCellsZSpin->setPrefix("Z ");
    m_meshCellsZSpin->setSpecialValueText("Auto");
    auto* detailRow = new QHBoxLayout;
    detailRow->setContentsMargins(0, 0, 0, 0);
    detailRow->addWidget(m_meshCellsXSpin);
    detailRow->addWidget(m_meshCellsZSpin);
    form->addRow("Detail:", detailRow);

    root->addWidget(settingsGroup);
    root->addStretch(1);

    connect(m_importButton, &QPushButton::clicked, this, &HeightmapPanel::requestImport);
    connect(m_clearButton, &QPushButton::clicked, this, &HeightmapPanel::requestClear);
    connect(m_importTextureButton, &QPushButton::clicked, this, &HeightmapPanel::requestImportTexture);
    connect(m_clearTextureButton, &QPushButton::clicked, this, &HeightmapPanel::requestClearTexture);
    connect(m_sizeSpin, &QDoubleSpinBox::valueChanged, this, QOverload<>::of(&HeightmapPanel::emitSettingsChanged));
    connect(m_heightSpin, &QDoubleSpinBox::valueChanged, this, QOverload<>::of(&HeightmapPanel::emitSettingsChanged));
    connect(m_offsetXSpin, &QDoubleSpinBox::valueChanged, this, QOverload<>::of(&HeightmapPanel::emitSettingsChanged));
    connect(m_offsetYSpin, &QDoubleSpinBox::valueChanged, this, QOverload<>::of(&HeightmapPanel::emitSettingsChanged));
    connect(m_offsetZSpin, &QDoubleSpinBox::valueChanged, this, QOverload<>::of(&HeightmapPanel::emitSettingsChanged));
    connect(m_meshCellsXSpin, &QSpinBox::valueChanged, this, QOverload<>::of(&HeightmapPanel::emitSettingsChanged));
    connect(m_meshCellsZSpin, &QSpinBox::valueChanged, this, QOverload<>::of(&HeightmapPanel::emitSettingsChanged));
    connect(m_visibleCheck, &QCheckBox::toggled, this, QOverload<>::of(&HeightmapPanel::emitSettingsChanged));
    connect(m_snapWhileMovingCheck, &QCheckBox::toggled, this, &HeightmapPanel::snapWhileMovingChanged);

    updateUiEnabledState(false);
}

QSize HeightmapPanel::sizeHint() const {
    return {320, 360};
}

void HeightmapPanel::setNetwork(const RoadNetwork* net) {
    m_net = net;
    const bool hasTerrain = m_net && m_net->terrain.enabled && !m_net->terrain.path.empty();

    m_updatingUi = true;
    {
        const QSignalBlocker blockSize(m_sizeSpin);
        const QSignalBlocker blockHeight(m_heightSpin);
        const QSignalBlocker blockOffsetX(m_offsetXSpin);
        const QSignalBlocker blockOffsetY(m_offsetYSpin);
        const QSignalBlocker blockOffsetZ(m_offsetZSpin);
        const QSignalBlocker blockCellsX(m_meshCellsXSpin);
        const QSignalBlocker blockCellsZ(m_meshCellsZSpin);
        const QSignalBlocker blockVisible(m_visibleCheck);

        if (hasTerrain) {
            const auto& terrain = m_net->terrain;
            const QFileInfo info(QString::fromStdString(terrain.path));
            m_pathLabel->setText(info.fileName().isEmpty() ? QString::fromStdString(terrain.path)
                                                           : info.fileName());
            m_pathLabel->setToolTip(QDir::toNativeSeparators(QString::fromStdString(terrain.path)));
            const QFileInfo textureInfo(QString::fromStdString(terrain.texturePath));
            m_texturePathLabel->setText(
                terrain.texturePath.empty()
                    ? QStringLiteral("No texture loaded")
                    : (textureInfo.fileName().isEmpty() ? QString::fromStdString(terrain.texturePath)
                                                        : textureInfo.fileName()));
            m_texturePathLabel->setToolTip(
                terrain.texturePath.empty() ? QString()
                                            : QDir::toNativeSeparators(QString::fromStdString(terrain.texturePath)));
            m_sizeSpin->setValue(terrain.width);
            m_heightSpin->setValue(terrain.height);
            m_offsetXSpin->setValue(terrain.offset.x);
            m_offsetYSpin->setValue(terrain.offset.y);
            m_offsetZSpin->setValue(terrain.offset.z);
            m_meshCellsXSpin->setValue(terrain.meshCellsX);
            m_meshCellsZSpin->setValue(terrain.meshCellsZ);
            m_visibleCheck->setChecked(terrain.visible);
        } else {
            m_pathLabel->setText("No heightmap loaded");
            m_pathLabel->setToolTip({});
            m_texturePathLabel->setText("No texture loaded");
            m_texturePathLabel->setToolTip({});
            m_sizeSpin->setValue(1024.0);
            m_heightSpin->setValue(128.0);
            m_offsetXSpin->setValue(0.0);
            m_offsetYSpin->setValue(0.0);
            m_offsetZSpin->setValue(0.0);
            m_meshCellsXSpin->setValue(0);
            m_meshCellsZSpin->setValue(0);
            m_visibleCheck->setChecked(true);
        }
    }
    m_updatingUi = false;

    updateUiEnabledState(hasTerrain);
}

void HeightmapPanel::emitSettingsChanged() {
    if (m_updatingUi || !m_net)
        return;
    if (!m_net->terrain.enabled || m_net->terrain.path.empty())
        return;

    TerrainSettings settings = m_net->terrain;
    settings.width = static_cast<float>(m_sizeSpin->value());
    settings.depth = static_cast<float>(m_sizeSpin->value());
    settings.height = static_cast<float>(m_heightSpin->value());
    settings.offset.x = static_cast<float>(m_offsetXSpin->value());
    settings.offset.y = static_cast<float>(m_offsetYSpin->value());
    settings.offset.z = static_cast<float>(m_offsetZSpin->value());
    settings.meshCellsX = m_meshCellsXSpin->value();
    settings.meshCellsZ = m_meshCellsZSpin->value();
    settings.visible = m_visibleCheck->isChecked();
    emit settingsChanged(settings);
}

void HeightmapPanel::requestImport() {
    emit importRequested();
}

void HeightmapPanel::requestClear() {
    emit clearRequested();
}

void HeightmapPanel::requestImportTexture() {
    emit importTextureRequested();
}

void HeightmapPanel::requestClearTexture() {
    emit clearTextureRequested();
}

void HeightmapPanel::updateUiEnabledState(bool hasTerrain) {
    m_clearButton->setEnabled(hasTerrain);
    m_importTextureButton->setEnabled(hasTerrain);
    m_clearTextureButton->setEnabled(hasTerrain);
    m_sizeSpin->setEnabled(hasTerrain);
    m_heightSpin->setEnabled(hasTerrain);
    m_offsetXSpin->setEnabled(hasTerrain);
    m_offsetYSpin->setEnabled(hasTerrain);
    m_offsetZSpin->setEnabled(hasTerrain);
    m_meshCellsXSpin->setEnabled(hasTerrain);
    m_meshCellsZSpin->setEnabled(hasTerrain);
    m_visibleCheck->setEnabled(hasTerrain);
    m_snapWhileMovingCheck->setEnabled(hasTerrain);
}
