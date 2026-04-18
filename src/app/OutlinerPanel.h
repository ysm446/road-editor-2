#pragma once

#include <QWidget>
#include "../model/RoadNetwork.h"

class QTreeWidget;
class QTreeWidgetItem;

class OutlinerPanel : public QWidget {
    Q_OBJECT
public:
    explicit OutlinerPanel(QWidget* parent = nullptr);

public slots:
    void refresh(const RoadNetwork* net);
    void onSelectionChanged(int roadIdx);

signals:
    void roadSelected(int roadIdx);

private slots:
    void onItemClicked(QTreeWidgetItem* item, int column);

private:
    QTreeWidget* m_tree;
    QTreeWidgetItem* m_roadsRoot        = nullptr;
    QTreeWidgetItem* m_intersectionsRoot = nullptr;
};
