#pragma once
#include <wx/wx.h>
#include <vector>
#include <wx/graphics.h>
#include "CanvasElement.h"
#include "Wire.h"
#include "UndoStack.h"
#include "HandyToolKit.h"
#include "ToolStateMachine.h"
#include "CanvasEventHandler.h"
#include "SimulationEngine.h" // 新增

class HandyToolKit;
class CanvasEventHandler;
class MainFrame;
class CanvasTextElement;
class ToolStateMachine;

struct HoverInfo {
    wxPoint pos;
    int pinIndex = -1;
    bool isInputPin = false;
    wxPoint pinWorldPos;
    int cellIndex = -1;
    int wireIndex = -1;
    wxPoint cellWorldPos;
    int elementIndex = -1;
    wxString elementName;
    bool IsOverPin() const { return pinIndex != -1; }
    bool IsOverCell() const { return cellIndex != -1; }
    bool IsOverElement() const { return elementIndex != -1; }
    HoverInfo() : pinIndex(-1), isInputPin(false), cellIndex(-1), wireIndex(-1), elementIndex(-1) {}
};

class CanvasPanel : public wxPanel
{
    std::vector<WireAnchor> m_undoAnchors;
    wxPoint m_dragStartElemPos;
    size_t m_wireCountBeforeOperation = 0;
public:
    CanvasPanel(MainFrame* parent);
    void AddElement(const CanvasElement& elem);
    void PlaceElement(const wxString& name, const wxPoint& pos);

    UndoStack m_undoStack;
    MainFrame* m_mainFrame;

    float GetScale() const { return m_scale; }
    void SetScale(float scale);
    wxPoint ScreenToCanvas(const wxPoint& screenPos) const;
    wxPoint CanvasToScreen(const wxPoint& canvasPos) const;

    wxPoint m_offset;
    bool m_isPanning;
    wxPoint m_panStartPos;

    bool IsClickOnEmptyArea(const wxPoint& canvasPos);

    void OnLeftDown(wxMouseEvent& evt);
    void OnLeftUp(wxMouseEvent& evt);
    void OnMouseMove(wxMouseEvent& evt);
    void OnKeyDown(wxKeyEvent& evt);
    void OnMouseWheel(wxMouseEvent& evt);
    void OnRightDown(wxMouseEvent& evt);
    void OnRightUp(wxMouseEvent& evt);
    void OnCursorTimer(wxTimerEvent& event);

    const std::vector<CanvasElement>& GetElements() const { return m_elements; }
    void ClearAll();

    void AddWire(const Wire& wire); // 由 cpp 实现，方便在添加后重建仿真

    void CollectUndoAnchor(size_t elemIdx, std::vector<WireAnchor>& out);
    const std::vector<Wire>& GetWires() const { return m_wires; }
    void DeleteSelectedElement();

    void OnLeftDoubleClick(wxMouseEvent& evt);

    wxTimer m_clickTimer;
    wxPoint m_clickPos;
    int m_clickElementIndex = -1;
    void OnClickTimer(wxTimerEvent& evt);

public:
    std::vector<CanvasTextElement> m_textElements;
    wxTimer m_cursorTimer;
    int inWhichTextBox(const wxPoint& canvasPos);
    void CreateTextElement(const wxPoint& position);
    void SetFocusToTextElement(int index);

    wxTextCtrl* m_hiddenTextCtrl;
    int m_currentEditingTextIndex;
    void SetupHiddenTextCtrl();
    void AttachHiddenTextCtrlToElement(int textIndex);
    void DetachHiddenTextCtrl();

    std::vector<CanvasElement> m_elements;
    int  m_selectedIndex = -1;
    bool m_isDragging = false;
    wxPoint m_dragStartPos;
    wxPoint m_elementStartPos;

    void ClearElementWires(size_t elemIndex);

    std::vector<Wire> m_wires;
    Wire m_tempWire;
    enum class WireMode { Idle, DragNew, DragMove, DragBranch };
    WireMode m_wireMode = WireMode::Idle;
    ControlPoint m_startCP;
    wxPoint m_curSnap;

    float m_scale = 1.0f;

    void OnPaint(wxPaintEvent& evt);
    int  HitTest(const wxPoint& pt);

    wxPoint Snap(const wxPoint& raw, bool* snapped);

    std::vector<WireAnchor> m_movingWires;
    std::vector<WireAnchor> m_movingbranchWires;
    std::vector<WireAnchor> m_branchWires;

    int  HitHoverPin(const wxPoint& raw, bool* isInput, wxPoint* worldPos);
    int HitHoverCell(const wxPoint& raw, int* wireIdx, int* cellIdx, wxPoint* cellPos);

public:
    void ClearSelection();
    void SetSelectedIndex(int index);
    int HitTestPublic(const wxPoint& pt);
    bool IsClickOnEmptyAreaPublic(const wxPoint& canvasPos);

    HandyToolKit* m_HandyToolKit;
    ToolStateMachine* m_toolStateMachine;
    CanvasEventHandler* m_CanvasEventHandler;

    void SetStatus(wxString status);
    void SetCurrentTool(ToolType tool);
    void SetCurrentComponent(const wxString& componentName);

    HoverInfo m_hoverInfo;
    void UpdateHoverInfo(const wxPoint& canvasPos);

    std::vector<WireWireAnchor> m_wireWireAnchors;

    // ==== 仿真引擎支持 ====
public:
    bool m_simulationEnabled = true;
    void InitializeSimulationEngine();
    void SyncSimulationToCanvas();
    void SyncCanvasToSimulation();

private:
    SimulationEngine m_simEngine;

private:
    bool m_hasFocus;
    void OnFocus(wxFocusEvent& event);
    void OnKillFocus(wxFocusEvent& event);
    void EnsureFocus();

public:
    bool m_isUsingHiddenCtrl;
    void StartTextEditing(int index);
    wxTextCtrl* m_sharedTextCtrl;

private:
    std::vector<WireBranch> m_allBranches;
    struct BranchDragState {
        size_t parentWire;
        size_t parentCell;
        wxPoint branchStartPos;
    } m_branchDragState;

public:
    bool CanCreateBranchFromWire(size_t wireIdx, size_t cellIdx) const;
    size_t CreateBranchFromWire(size_t wireIdx, size_t cellIdx, const wxPoint& startPos);
    void UpdateWireBranches(size_t wireIdx);
    void DeleteWireWithBranches(size_t wireIdx);
    void StartBranchFromWire(size_t wireIdx, size_t cellIdx, const wxPoint& startPos);
    void CompleteBranchConnection();
    void EstablishBranchConnection(size_t parentWire, size_t parentCell, size_t branchWire);
    void RemoveBranchConnection(size_t parentWire, size_t branchWire);

    wxDECLARE_EVENT_TABLE();
};