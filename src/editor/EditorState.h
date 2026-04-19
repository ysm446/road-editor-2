#pragma once

#include <algorithm>
#include <QMetaType>
#include <deque>
#include <utility>
#include <vector>
#include "../model/RoadNetwork.h"

enum class ToolMode { Select, Edit };

struct SelectedPoint {
    int roadIdx  = -1;
    int pointIdx = -1;

    bool operator==(const SelectedPoint& other) const {
        return roadIdx == other.roadIdx && pointIdx == other.pointIdx;
    }
};

struct Selection {
    int roadIdx         = -1;
    int pointIdx        = -1;
    int intersectionIdx = -1;
    std::vector<SelectedPoint> points;

    bool valid() const           { return !points.empty(); }
    bool hasIntersection() const { return intersectionIdx >= 0; }
    bool containsPoint(int selRoadIdx, int selPointIdx) const {
        return std::find(points.begin(), points.end(),
                         SelectedPoint{selRoadIdx, selPointIdx}) != points.end();
    }

    void setSinglePoint(int selRoadIdx, int selPointIdx) {
        points = {{selRoadIdx, selPointIdx}};
        roadIdx = selRoadIdx;
        pointIdx = selPointIdx;
        intersectionIdx = -1;
    }

    void setPoints(std::vector<SelectedPoint> newPoints) {
        points = std::move(newPoints);
        intersectionIdx = -1;
        if (!points.empty()) {
            roadIdx = points.front().roadIdx;
            pointIdx = points.front().pointIdx;
        } else {
            roadIdx = -1;
            pointIdx = -1;
        }
    }

    void clear() {
        roadIdx = pointIdx = intersectionIdx = -1;
        points.clear();
    }
};

class EditorState {
public:
    Selection sel;
    ToolMode  mode = ToolMode::Edit;

    void pushUndo(const RoadNetwork& net);
    bool undo(RoadNetwork& net);
    bool redo(RoadNetwork& net);

private:
    struct Snapshot { RoadNetwork net; Selection sel; };
    std::deque<Snapshot> m_undoStack;
    std::deque<Snapshot> m_redoStack;
    static constexpr int MAX_UNDO = 50;
};

Q_DECLARE_METATYPE(ToolMode)
