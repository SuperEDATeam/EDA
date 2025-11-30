#pragma once
#include <wx/wx.h>
#include <vector>
#include <memory>

class CanvasPanel;

struct WireAnchor {
    size_t wireIdx;   // ������
    size_t ptIdx;     // ���ߵڼ������Ƶ㣨0 �� ���
    bool   isInput;   // Ԫ���������뻹���������
    size_t pinIdx;    // Ԫ������������
    wxPoint oldPos;   // ԭ����λ�ã����ڳ�����
};


/* �������� */
class Command
{
public:
    virtual ~Command() = default;
    virtual void undo(CanvasPanel* canvas) = 0;
    virtual wxString GetName() const = 0;
};

/* �����������Ԫ�� */
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

/* ��������ƶ�Ԫ�� */
class CmdMoveElement : public Command
{
public:
    // ��չ����������oldRotation��anchors
    CmdMoveElement(size_t idx, const wxPoint& oldPos, int oldRotation, const std::vector<WireAnchor>& anchors)
        : m_index(idx), m_oldPos(oldPos), m_oldRotation(oldRotation), m_anchors(anchors) {
    }

    void undo(CanvasPanel* canvas) override;
    wxString GetName() const override { return "Move Selection"; }

private:
    size_t m_index;
    wxPoint m_oldPos;   // �ƶ�ǰ������
    int m_oldRotation;
    std::vector<WireAnchor> m_anchors;
};

/* ����������ӵ��� */
class CmdAddWire : public Command
{
    size_t m_wireIdx;   // �� m_wires �е��±�
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


/* ����ջ */
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
