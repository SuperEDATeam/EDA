#pragma once
#include <wx/wx.h>
#include <vector>
#include <memory>

class CanvasPanel;

struct WireAnchor {
    size_t wireIdx;   // 哪条线
    size_t ptIdx;     // 该线第几个控制点（0 或 最后）
    bool   isInput;   // 元件侧是输入还是输出引脚
    size_t pinIdx;    // 元件侧引脚索引
    wxPoint oldPos;   // 原来的位置（用于撤销）
};


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
    CmdMoveElement(size_t idx,
        const wxPoint& oldPos,
        const std::vector<WireAnchor>& anchors)
        : m_index(idx), m_oldPos(oldPos), m_anchors(anchors) {
    }

    void undo(CanvasPanel* canvas) override;
    wxString GetName() const override { return "Move Selection"; }

private:
    size_t m_index;
    wxPoint m_oldPos;   // 移动前的坐标
    std::vector<WireAnchor> m_anchors;
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

class CmdAddBranchWire : public Command {
private:
    size_t m_parentWire;
    size_t m_parentCell;
    size_t m_branchWire;

public:
    CmdAddBranchWire(size_t parentWire, size_t parentCell, size_t branchWire)
        : m_parentWire(parentWire), m_parentCell(parentCell), m_branchWire(branchWire) {
    }

    void undo(CanvasPanel* canvas) override;

    wxString GetName() const override {
        return "Add Branch Wire";
    }
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

