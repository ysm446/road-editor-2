#include "OutlinerPanel.h"
#include <QVBoxLayout>
#include <QTreeWidget>
#include <QHeaderView>

OutlinerPanel::OutlinerPanel(QWidget* parent)
    : QWidget(parent)
{
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(4, 4, 4, 4);
    layout->setSpacing(0);

    m_tree = new QTreeWidget(this);
    m_tree->setHeaderLabel("Scene");
    m_tree->setIndentation(14);
    m_tree->header()->setStretchLastSection(true);
    layout->addWidget(m_tree);

    connect(m_tree, &QTreeWidget::itemClicked,
            this,   &OutlinerPanel::onItemClicked);
}

void OutlinerPanel::refresh(const RoadNetwork* net) {
    m_tree->clear();
    m_roadsRoot        = nullptr;
    m_intersectionsRoot = nullptr;
    if (!net) return;

    m_roadsRoot = new QTreeWidgetItem(m_tree, QStringList("Roads"));
    m_roadsRoot->setData(0, Qt::UserRole, -2); // sentinel
    for (int i = 0; i < (int)net->roads.size(); ++i) {
        const auto& r = net->roads[i];
        QString label = r.name.empty()
            ? QString::fromStdString(r.id)
            : QString::fromStdString(r.name);
        auto* item = new QTreeWidgetItem(m_roadsRoot, QStringList(label));
        item->setData(0, Qt::UserRole, i);
    }
    m_roadsRoot->setExpanded(true);

    m_intersectionsRoot = new QTreeWidgetItem(m_tree, QStringList("Intersections"));
    m_intersectionsRoot->setData(0, Qt::UserRole, -2);
    for (const auto& n : net->intersections) {
        QString label = n.name.empty()
            ? QString::fromStdString(n.id)
            : QString::fromStdString(n.name);
        auto* item = new QTreeWidgetItem(m_intersectionsRoot, QStringList(label));
        item->setData(0, Qt::UserRole, -1); // intersections: no road index
        (void)item;
    }
    m_intersectionsRoot->setExpanded(true);
}

void OutlinerPanel::onSelectionChanged(int roadIdx) {
    if (!m_roadsRoot) return;
    // Highlight matching road item
    for (int i = 0; i < m_roadsRoot->childCount(); ++i) {
        auto* item = m_roadsRoot->child(i);
        bool sel = (item->data(0, Qt::UserRole).toInt() == roadIdx);
        item->setSelected(sel);
        if (sel) m_tree->scrollToItem(item);
    }
}

void OutlinerPanel::onItemClicked(QTreeWidgetItem* item, int /*column*/) {
    int idx = item->data(0, Qt::UserRole).toInt();
    if (idx >= 0)
        emit roadSelected(idx);
}
