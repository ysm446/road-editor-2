#include "EditorState.h"

void EditorState::pushUndo(const RoadNetwork& net) {
    m_undoStack.push_back({ net, sel });
    if ((int)m_undoStack.size() > MAX_UNDO)
        m_undoStack.pop_front();
    m_redoStack.clear();
}

bool EditorState::undo(RoadNetwork& net) {
    if (m_undoStack.empty()) return false;
    m_redoStack.push_back({ net, sel });
    auto& snap = m_undoStack.back();
    net = snap.net;
    sel = snap.sel;
    m_undoStack.pop_back();
    return true;
}

bool EditorState::redo(RoadNetwork& net) {
    if (m_redoStack.empty()) return false;
    m_undoStack.push_back({ net, sel });
    auto& snap = m_redoStack.back();
    net = snap.net;
    sel = snap.sel;
    m_redoStack.pop_back();
    return true;
}
