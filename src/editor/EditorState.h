#pragma once

#include <deque>
#include "../model/RoadNetwork.h"

struct Selection {
    int roadIdx  = -1;
    int pointIdx = -1;
    bool valid() const { return roadIdx >= 0 && pointIdx >= 0; }
    void clear()       { roadIdx = pointIdx = -1; }
};

class EditorState {
public:
    Selection sel;

    void pushUndo(const RoadNetwork& net);
    bool undo(RoadNetwork& net);
    bool redo(RoadNetwork& net);

private:
    struct Snapshot { RoadNetwork net; Selection sel; };
    std::deque<Snapshot> m_undoStack;
    std::deque<Snapshot> m_redoStack;
    static constexpr int MAX_UNDO = 50;
};
