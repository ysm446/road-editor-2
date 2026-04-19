#pragma once

#include <algorithm>
#include <QMetaType>
#include <deque>
#include <utility>
#include <vector>
#include "../model/RoadNetwork.h"

enum class ToolMode { Select, Edit, VerticalCurve, BankAngle };
enum class EditSubTool { None, CreateRoad, CreateIntersection };

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
    int verticalCurveIdx = -1;
    int bankAngleIdx    = -1;
    int intersectionIdx = -1;
    int socketIdx       = -1;
    std::vector<SelectedPoint> points;

    bool valid() const           { return !points.empty(); }
    bool hasVerticalCurve() const { return roadIdx >= 0 && verticalCurveIdx >= 0; }
    bool hasBankAngle() const    { return roadIdx >= 0 && bankAngleIdx >= 0; }
    bool hasIntersection() const { return intersectionIdx >= 0; }
    bool hasSocket() const       { return intersectionIdx >= 0 && socketIdx >= 0; }
    bool containsPoint(int selRoadIdx, int selPointIdx) const {
        return std::find(points.begin(), points.end(),
                         SelectedPoint{selRoadIdx, selPointIdx}) != points.end();
    }

    void setSinglePoint(int selRoadIdx, int selPointIdx) {
        points = {{selRoadIdx, selPointIdx}};
        roadIdx = selRoadIdx;
        pointIdx = selPointIdx;
        verticalCurveIdx = -1;
        bankAngleIdx = -1;
        intersectionIdx = -1;
        socketIdx = -1;
    }

    void setVerticalCurve(int selRoadIdx, int selVerticalCurveIdx) {
        points.clear();
        roadIdx = selRoadIdx;
        pointIdx = -1;
        verticalCurveIdx = selVerticalCurveIdx;
        bankAngleIdx = -1;
        intersectionIdx = -1;
        socketIdx = -1;
    }

    void setBankAngle(int selRoadIdx, int selBankAngleIdx) {
        points.clear();
        roadIdx = selRoadIdx;
        pointIdx = -1;
        verticalCurveIdx = -1;
        bankAngleIdx = selBankAngleIdx;
        intersectionIdx = -1;
        socketIdx = -1;
    }

    void setPoints(std::vector<SelectedPoint> newPoints) {
        points = std::move(newPoints);
        verticalCurveIdx = -1;
        bankAngleIdx = -1;
        intersectionIdx = -1;
        socketIdx = -1;
        if (!points.empty()) {
            roadIdx = points.front().roadIdx;
            pointIdx = points.front().pointIdx;
        } else {
            roadIdx = -1;
            pointIdx = -1;
        }
    }

    void setIntersection(int selIntersectionIdx, int selSocketIdx = -1) {
        points.clear();
        roadIdx = -1;
        pointIdx = -1;
        verticalCurveIdx = -1;
        bankAngleIdx = -1;
        intersectionIdx = selIntersectionIdx;
        socketIdx = selSocketIdx;
    }

    void clear() {
        roadIdx = pointIdx = verticalCurveIdx = bankAngleIdx = intersectionIdx = socketIdx = -1;
        points.clear();
    }
};

class EditorState {
public:
    Selection sel;
    ToolMode  mode = ToolMode::Edit;
    EditSubTool editSubTool = EditSubTool::None;

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
Q_DECLARE_METATYPE(EditSubTool)
Q_DECLARE_METATYPE(Selection)
