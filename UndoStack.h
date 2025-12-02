#pragma once
#include <wx/wx.h>
#include <vector>
#include <memory>

#include "Wire.h"
#include "CanvasElement.h"
#include "CanvasTextElement.h"

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

struct Element_Re {
    CanvasElement elem;
    int idx;
};

struct Wire_Re {
    Wire wire;
    int idx;
};

struct Text_Re {
    CanvasTextElement txt;
    int idx;
};

// 删除选中的元素
class CmdDeleteSelected : public Command{

    std::vector<Element_Re> Elements;
    std::vector<Wire_Re> Wires;
    std::vector<Text_Re> Texts;

public:
    CmdDeleteSelected(std::vector<CanvasElement> elements, std::vector<int> elementIdx, std::vector<Wire> wires, std::vector<int> wireIdx, std::vector<CanvasTextElement> txtBoxes, std::vector<int> txtBoxIdx) {
        for (int i = 0; i < elements.size(); i++) {
            Element_Re e;
            e.elem = elements[i];
            e.idx = elementIdx[i];
            Elements.push_back(e);
        }
        if (elements.size() > 0) {
            std::sort(Elements.begin(), Elements.end(),
                [](const auto& a, const auto& b) {
                    return a.idx < b.idx;
                });
        }


        for (int i = 0; i < wires.size(); i++) {
            Wire_Re w;
            w.wire = wires[i];
            w.idx = wireIdx[i];
            Wires.push_back(w);
        }

        if (wires.size() > 0) {
            std::sort(Wires.begin(), Wires.end(),
                [](const auto& a, const auto& b) {
                    return a.idx < b.idx;
                });
        }

        for (int i = 0; i < txtBoxes.size(); i++) {
            Texts.push_back({ txtBoxes[i], txtBoxIdx[i] });
        }
        if (txtBoxes.size() > 0) {
            std::sort(Texts.begin(), Texts.end(),
                [](const auto& a, const auto& b) {
                    return a.idx < b.idx;
                });
        }
    }
    void undo(CanvasPanel* canvas) override;
    wxString GetName() const override { return "Delete Selected"; }
};

//class CmdCopySelected : public Command {
//    std::vector<int> m_textElemIdx;
//    std::vector<int> m_compntIdx;
//    std::vector<int> m_wireIdx;
//
//public:
//    CmdCopySelected(std::vector<int> m_textElemIdx, std::vector<int> m_compntIdx;
//    std::vector<int> m_wireIdx;) {
//        m_textElemIdx = elements;
//        m_wires = wires;
//        m_txtBoxes = txtBoxes;
//    }
//    void undo(CanvasPanel* canvas) override;
//    wxString GetName() const override { return "Copy Selected"; }
//};


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