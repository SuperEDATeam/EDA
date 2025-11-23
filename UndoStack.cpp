#include "UndoStack.h"
#include "CanvasPanel.h"
#include "UndoNotifier.h"

/* ---------------- CmdAddElement ---------------- */
void CmdAddElement::undo(CanvasPanel* canvas)
{
    if (m_index < canvas->m_elements.size())
    {
        canvas->m_elements.erase(canvas->m_elements.begin() + m_index);
        canvas->m_selectedIndex = -1;
        canvas->Refresh();
    }
}

wxString CmdAddElement::GetName() const
{
    return wxString("Add ") + m_name;
}

/* ---------------- UndoStack ---------------- */
void UndoStack::Push(std::unique_ptr<Command> cmd)
{
    m_stack.emplace_back(std::move(cmd));
    if (m_stack.size() > m_limit)
        m_stack.erase(m_stack.begin());
    // 发出通知
    UndoNotifier::Notify(GetUndoName(), CanUndo());
}

void UndoStack::Undo(CanvasPanel* canvas)
{
    if (!CanUndo()) return;
    auto& top = m_stack.back();
    top->undo(canvas);
    m_stack.pop_back();
}

wxString UndoStack::GetUndoName() const
{
    return CanUndo() ? m_stack.back()->GetName() : wxString("Can't Undo");
}

void CmdMoveElement::undo(CanvasPanel* canvas)
{
    if (m_index >= canvas->m_elements.size()) return;

    /* 1. 恢复元件位置 */
    canvas->m_elements[m_index].SetPos(m_oldPos);

    /* 2. 恢复导线端点 */
    for (const auto& a : m_anchors) {
        if (a.wireIdx >= canvas->m_wires.size()) continue;
        auto& wire = canvas->m_wires[a.wireIdx];
        if (a.ptIdx < wire.pts.size())
            wire.pts[a.ptIdx].pos = a.oldPos;

        // 重新布线 & 小格
        wire.pts = Wire::RouteOrtho(wire.pts.front(), wire.pts.back(),
            PinDirection::Right, PinDirection::Left);
        wire.GenerateCells();
    }
    canvas->Refresh();
}

void CmdAddWire::undo(CanvasPanel* canvas)
{
    if (m_wireIdx < canvas->m_wires.size())
    {
        canvas->m_wires.erase(canvas->m_wires.begin() + m_wireIdx);
        canvas->Refresh();
    }
}