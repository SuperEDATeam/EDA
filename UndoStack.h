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

/* 具体命令：添加导线 */
class CmdAddWire : public Command
{
    size_t m_wireIdx;   // �� m_wires �е��±�
public:
    CmdAddWire(size_t idx) : m_wireIdx(idx) {}
    void undo(CanvasPanel* canvas) override;
    wxString GetName() const override { return "Add Wire"; }
};



/* 具体命令：添加文本框 */
class CmdAddText : public Command
{
    size_t m_textIdx;   // 在 m_wires 中的下标
public:
    CmdAddText(size_t idx) : m_textIdx(idx) {}
    void undo(CanvasPanel* canvas) override;
    wxString GetName() const override { return "Add TextBox"; }
};

class CmdMoveSelected : public Command
{
    std::vector<int> m_textElemIdx;
    std::vector<int> m_compntIdx;
    std::vector<int> m_wireIdx;

    std::vector<wxPoint> m_textElemPos;
    std::vector<wxPoint> m_compntPos;
    std::vector<std::vector<wxPoint>> m_wirePos;

    std::vector<std::vector<WireAnchor>> m_movingWires;
public:
    CmdMoveSelected(std::vector<int> textElemIdx, std::vector<int> compntIdx, std::vector<int> wireIdx, std::vector<wxPoint> textElemPos, std::vector<wxPoint> compntPos, std::vector<std::vector<wxPoint>> wirePos, std::vector<std::vector<WireAnchor>> movingWires) {
        m_textElemIdx = textElemIdx;
        m_compntIdx = compntIdx;
        m_wireIdx = wireIdx;
        m_textElemPos = textElemPos;
        m_compntPos = compntPos;
        m_wirePos = wirePos;
        m_movingWires = movingWires;
    }
    void undo(CanvasPanel* canvas) override;
    wxString GetName() const override { return "Move Selected"; }
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