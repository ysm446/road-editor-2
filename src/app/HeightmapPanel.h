#pragma once

#include <QWidget>
#include "../model/RoadNetwork.h"

class QLabel;
class QPushButton;
class QDoubleSpinBox;
class QSpinBox;

class HeightmapPanel : public QWidget {
    Q_OBJECT
public:
    explicit HeightmapPanel(QWidget* parent = nullptr);
    QSize sizeHint() const override;

public slots:
    void setNetwork(const RoadNetwork* net);

private slots:
    void emitSettingsChanged();
    void requestImport();
    void requestClear();
    void requestImportTexture();
    void requestClearTexture();

signals:
    void importRequested();
    void clearRequested();
    void importTextureRequested();
    void clearTextureRequested();
    void settingsChanged(const TerrainSettings& settings);

private:
    void updateUiEnabledState(bool hasTerrain);

    const RoadNetwork* m_net = nullptr;
    bool m_updatingUi = false;

    QLabel*         m_pathLabel = nullptr;
    QLabel*         m_texturePathLabel = nullptr;
    QPushButton*    m_importButton = nullptr;
    QPushButton*    m_clearButton = nullptr;
    QPushButton*    m_importTextureButton = nullptr;
    QPushButton*    m_clearTextureButton = nullptr;
    QDoubleSpinBox* m_sizeSpin = nullptr;
    QDoubleSpinBox* m_heightSpin = nullptr;
    QDoubleSpinBox* m_offsetXSpin = nullptr;
    QDoubleSpinBox* m_offsetYSpin = nullptr;
    QDoubleSpinBox* m_offsetZSpin = nullptr;
    QSpinBox*       m_meshCellsXSpin = nullptr;
    QSpinBox*       m_meshCellsZSpin = nullptr;
};
