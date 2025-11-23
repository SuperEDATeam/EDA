#pragma once
#include <wx/wx.h>
#include <vector>
#include <memory>

class CanvasPanel;

/* 抽象命令 */
class Command
{
public:
    virtual ~Command() = default;
    virtual void undo(CanvasPanel* canvas) = 0;
    virtual wxString GetName() const = 0;
};

/* 具体命令：放置元件 */
class CmdAddElement : public Command
{
    wxString m_name;
    size_t   m_index;
public:
    CmdAddElement(const wxString& name, size_t idx)
        : m_name(name), m_index(idx) {
    }
    void undo(CanvasPanel* canvas) override;
    wxString GetName() const override;
};

/* 具体命令：移动元件 */
class CmdMoveElement : public Command
{
public:
    struct Anchor {
        size_t wireIdx;
        size_t ptIdx;   // 0 前端  1 后端
        wxPoint oldPos;
    };
    CmdMoveElement(size_t idx,
        const wxPoint& oldPos,
        const std::vector<Anchor>& anchors)
        : m_index(idx), m_oldPos(oldPos), m_anchors(anchors) {
    }

    void undo(CanvasPanel* canvas) override;
    wxString GetName() const override { return "Move Selection"; }

private:
    size_t m_index;
    wxPoint m_oldPos;   // 移动前的坐标
    std::vector<Anchor> m_anchors;
};

/* 具体命令：添加导线 */
class CmdAddWire : public Command
{
    size_t m_wireIdx;   // 在 m_wires 中的下标
public:
    CmdAddWire(size_t idx) : m_wireIdx(idx) {}
    void undo(CanvasPanel* canvas) override;
    wxString GetName() const override { return "Add Wire"; }
};

/* 撤销栈 */
class UndoStack
{
    std::vector<std::unique_ptr<Command>> m_stack;
    size_t m_limit = 100;
public:
    void Push(std::unique_ptr<Command> cmd);
    bool CanUndo() const { return !m_stack.empty(); }
    void Undo(CanvasPanel* canvas);
    wxString GetUndoName() const;
    void Clear() { m_stack.clear(); }
};