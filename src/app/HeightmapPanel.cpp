#include "HeightmapPanel.h"
#include <QDir>
#include <QDoubleSpinBox>
#include <QFileInfo>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
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

    m_widthSpin = new QDoubleSpinBox(this);
    m_widthSpin->setRange(1.0, 100000.0);
    m_widthSpin->setDecimals(2);
    m_widthSpin->setSingleStep(10.0);
    m_widthSpin->setSuffix(" m");
    form->addRow("Width:", m_widthSpin);

    m_depthSpin = new QDoubleSpinBox(this);
    m_depthSpin->setRange(1.0, 100000.0);
    m_depthSpin->setDecimals(2);
    m_depthSpin->setSingleStep(10.0);
    m_depthSpin->setSuffix(" m");
    form->addRow("Depth:", m_depthSpin);

    m_heightSpin = new QDoubleSpinBox(this);
    m_heightSpin->setRange(0.1, 100000.0);
    m_heightSpin->setDecimals(2);
    m_heightSpin->setSingleStep(10.0);
    m_heightSpin->setSuffix(" m");
    form->addRow("Height:", m_heightSpin);

    m_offsetXSpin = new QDoubleSpinBox(this);
    m_offsetXSpin->setRange(-100000.0, 100000.0);
    m_offsetXSpin->setDecimals(2);
    m_offsetXSpin->setSingleStep(10.0);
    m_offsetXSpin->setSuffix(" m");
    form->addRow("Offset X:", m_offsetXSpin);

    m_offsetYSpin = new QDoubleSpinBox(this);
    m_offsetYSpin->setRange(-100000.0, 100000.0);
    m_offsetYSpin->setDecimals(2);
    m_offsetYSpin->setSingleStep(10.0);
    m_offsetYSpin->setSuffix(" m");
    form->addRow("Offset Y:", m_offsetYSpin);

    m_offsetZSpin = new QDoubleSpinBox(this);
    m_offsetZSpin->setRange(-100000.0, 100000.0);
    m_offsetZSpin->setDecimals(2);
    m_offsetZSpin->setSingleStep(10.0);
    m_offsetZSpin->setSuffix(" m");
    form->addRow("Offset Z:", m_offsetZSpin);

    m_meshCellsXSpin = new QSpinBox(this);
    m_meshCellsXSpin->setRange(0, 8192);
    m_meshCellsXSpin->setSpecialValueText("Auto");
    form->addRow("Detail X:", m_meshCellsXSpin);

    m_meshCellsZSpin = new QSpinBox(this);
    m_meshCellsZSpin->setRange(0, 8192);
    m_meshCellsZSpin->setSpecialValueText("Auto");
    form->addRow("Detail Z:", m_meshCellsZSpin);

    root->addWidget(settingsGroup);
    root->addStretch(1);

    connect(m_importButton, &QPushButton::clicked, this, &HeightmapPanel::requestImport);
    connect(m_clearButton, &QPushButton::clicked, this, &HeightmapPanel::requestClear);
    connect(m_importTextureButton, &QPushButton::clicked, this, &HeightmapPanel::requestImportTexture);
    connect(m_clearTextureButton, &QPushButton::clicked, this, &HeightmapPanel::requestClearTexture);
    connect(m_widthSpin, &QDoubleSpinBox::valueChanged, this, QOverload<>::of(&HeightmapPanel::emitSettingsChanged));
    connect(m_depthSpin, &QDoubleSpinBox::valueChanged, this, QOverload<>::of(&HeightmapPanel::emitSettingsChanged));
    connect(m_heightSpin, &QDoubleSpinBox::valueChanged, this, QOverload<>::of(&HeightmapPanel::emitSettingsChanged));
    connect(m_offsetXSpin, &QDoubleSpinBox::valueChanged, this, QOverload<>::of(&HeightmapPanel::emitSettingsChanged));
    connect(m_offsetYSpin, &QDoubleSpinBox::valueChanged, this, QOverload<>::of(&HeightmapPanel::emitSettingsChanged));
    connect(m_offsetZSpin, &QDoubleSpinBox::valueChanged, this, QOverload<>::of(&HeightmapPanel::emitSettingsChanged));
    connect(m_meshCellsXSpin, &QSpinBox::valueChanged, this, QOverload<>::of(&HeightmapPanel::emitSettingsChanged));
    connect(m_meshCellsZSpin, &QSpinBox::valueChanged, this, QOverload<>::of(&HeightmapPanel::emitSettingsChanged));

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
        const QSignalBlocker blockWidth(m_widthSpin);
        const QSignalBlocker blockDepth(m_depthSpin);
        const QSignalBlocker blockHeight(m_heightSpin);
        const QSignalBlocker blockOffsetX(m_offsetXSpin);
        const QSignalBlocker blockOffsetY(m_offsetYSpin);
        const QSignalBlocker blockOffsetZ(m_offsetZSpin);
        const QSignalBlocker blockCellsX(m_meshCellsXSpin);
        const QSignalBlocker blockCellsZ(m_meshCellsZSpin);

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
            m_widthSpin->setValue(terrain.width);
            m_depthSpin->setValue(terrain.depth);
            m_heightSpin->setValue(terrain.height);
            m_offsetXSpin->setValue(terrain.offset.x);
            m_offsetYSpin->setValue(terrain.offset.y);
            m_offsetZSpin->setValue(terrain.offset.z);
            m_meshCellsXSpin->setValue(terrain.meshCellsX);
            m_meshCellsZSpin->setValue(terrain.meshCellsZ);
        } else {
            m_pathLabel->setText("No heightmap loaded");
            m_pathLabel->setToolTip({});
            m_texturePathLabel->setText("No texture loaded");
            m_texturePathLabel->setToolTip({});
            m_widthSpin->setValue(1024.0);
            m_depthSpin->setValue(1024.0);
            m_heightSpin->setValue(128.0);
            m_offsetXSpin->setValue(0.0);
            m_offsetYSpin->setValue(0.0);
            m_offsetZSpin->setValue(0.0);
            m_meshCellsXSpin->setValue(0);
            m_meshCellsZSpin->setValue(0);
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
    settings.width = static_cast<float>(m_widthSpin->value());
    settings.depth = static_cast<float>(m_depthSpin->value());
    settings.height = static_cast<float>(m_heightSpin->value());
    settings.offset.x = static_cast<float>(m_offsetXSpin->value());
    settings.offset.y = static_cast<float>(m_offsetYSpin->value());
    settings.offset.z = static_cast<float>(m_offsetZSpin->value());
    settings.meshCellsX = m_meshCellsXSpin->value();
    settings.meshCellsZ = m_meshCellsZSpin->value();
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
    m_widthSpin->setEnabled(hasTerrain);
    m_depthSpin->setEnabled(hasTerrain);
    m_heightSpin->setEnabled(hasTerrain);
    m_offsetXSpin->setEnabled(hasTerrain);
    m_offsetYSpin->setEnabled(hasTerrain);
    m_offsetZSpin->setEnabled(hasTerrain);
    m_meshCellsXSpin->setEnabled(hasTerrain);
    m_meshCellsZSpin->setEnabled(hasTerrain);
}
