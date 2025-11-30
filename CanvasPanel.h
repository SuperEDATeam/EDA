#pragma once
#include <wx/wx.h>
#include <vector>
#include <wx/graphics.h>
#include "CanvasElement.h"
#include "Wire.h"          // �� ��������������
#include "UndoStack.h"
#include "HandyToolKit.h"
#include "ToolStateMachine.h"
#include "CanvasEventHandler.h"
#include <chrono>

class HandyToolKit;
class CanvasEventHandler;
class MainFrame;
class CanvasTextElement;
class ToolStateMachine;

struct HoverInfo {
    wxPoint pos;
    wxPoint snappedPos; // ���ȼ������ţ����߿��Ƶ�

    // -----------------
    // 1. ���� (Pin) ��ͣ��Ϣ
    // -----------------
    int pinIndex = -1;         // ��ͣ���ŵ�������-1 ��ʾû����ͣ���κ������ϡ�
    bool isInputPin = false;   // ��ͣ�����Ƿ�Ϊ�������š�
    wxPoint pinWorldPos;       // ��ͣ��������������ϵ�е�λ�á�

    // -----------------
    // 2. ���߿��Ƶ� (Cell/Wire Control Point) ��ͣ��Ϣ
    // -----------------
    int cellIndex = -1;        // ��ͣ���߿��Ƶ��������-1 ��ʾû����ͣ�ڿ��Ƶ��ϡ�
    int wireIndex = -1;        // ���Ƶ��������ߵ�������
    wxPoint cellWorldPos;      // ���Ƶ�����������ϵ�е�λ�á�

    // -----------------
    // 3. Ԫ�� (Element) ��ͣ��Ϣ (����״̬������)
    // -----------------
    int elementIndex = -1;     // ��ͣԪ����������-1 ��ʾû����ͣ���κ�Ԫ���ϡ�
    wxString elementName;      // ��ͣԪ�������ơ�

    // �����������ж��Ƿ���ͣ��ĳ�����������
    bool IsOverPin() const { return pinIndex != -1; }
    bool IsOverCell() const { return cellIndex != -1; }
    bool IsOverElement() const { return elementIndex != -1; }

    // ���캯��
    HoverInfo() : pinIndex(-1), isInputPin(false), cellIndex(-1), wireIndex(-1), elementIndex(-1) {}
};

class CanvasPanel : public wxPanel
{
    std::vector<WireAnchor> m_undoAnchors;
    wxPoint m_dragStartElemPos;        // �϶�ǰԪ�����Ͻ�
    size_t m_wireCountBeforeOperation = 0;  // ����ǰ�ĵ�������

    // ����/�϶����
    std::chrono::steady_clock::time_point m_leftDownTime;
    wxPoint m_leftDownPos;
    bool m_maybeClick = false;
    int m_leftDownElementIndex = -1;
    static const int CLICK_MAX_MS = 250;      // �������ʱ��
    static const int DRAG_THRESHOLD_PX = 6;   // �϶���ֵ����
public:
    CanvasPanel(MainFrame* parent, size_t size_x, size_t size_y);
    void AddElement(const CanvasElement& elem);
    void PlaceElement(const wxString& name, const wxPoint& pos);

    UndoStack m_undoStack;

    MainFrame* m_mainFrame;

    // ������ط���
    float GetScale() const { return m_scale; }  // ��ȡ��ǰ���ű���
    void SetScale(float scale);                 // �������ű�����ˢ��

    // ����ת������Ļ���� <-> �������꣬�������ţ�
    wxPoint ScreenToCanvas(const wxPoint& screenPos) const;
    wxPoint CanvasToScreen(const wxPoint& canvasPos) const;

    wxPoint m_offset;          // ����ƫ������ƽ�����꣩

    // �������ж��Ƿ����ڿհ�������Ԫ�أ�
    bool IsClickOnEmptyArea(const wxPoint& canvasPos);

    // �¼��������������м���Ϊ�����أ�
    void OnLeftDown(wxMouseEvent& evt);
    void OnLeftUp(wxMouseEvent& evt);
    void OnMouseMove(wxMouseEvent& evt);
    void OnKeyDown(wxKeyEvent& evt);
    void OnMouseWheel(wxMouseEvent& evt);
    void OnRightDown(wxMouseEvent& evt);
    void OnRightUp(wxMouseEvent& evt);
    void OnCursorTimer(wxTimerEvent& event);

    const std::vector<CanvasElement>& GetElements() const { return m_elements; }
    void ClearAll() {
        m_elements.clear();
        m_wires.clear();
        m_selectedIndex = -1;
        Refresh();
    }

    void AddWire(const Wire& wire) {
        m_wires.push_back(wire);
        Refresh();
    }

    void CollectUndoAnchor(size_t elemIdx, std::vector<WireAnchor>& out);

    // ��¶�������������ⲿ����/����ʹ��
    const std::vector<Wire>& GetWires() const { return m_wires; }


    void DeleteSelectedElement();


    // �ı�Ԫ�����
    std::vector<CanvasTextElement> m_textElements;
    int inWhichTextBox(const wxPoint& canvasPos);
    void CreateTextElement(const wxPoint& position);
    void SetFocusToTextElement(int index);

    wxTextCtrl* m_hiddenTextCtrl;
    int m_currentEditingTextIndex;

    void SetupHiddenTextCtrl();
    void AttachHiddenTextCtrlToElement(int textIndex);
    void DetachHiddenTextCtrl();

    /* ---------- ԭ��Ԫ����� ---------- */
    std::vector<CanvasElement> m_elements;
    int  m_selectedIndex = -1;
    bool m_isDragging = false;
    wxPoint m_dragStartPos;
    wxPoint m_elementStartPos;


    void ClearElementWires(size_t elemIndex);


    std::vector<Wire> m_wires;   // �Ѷ��͵�����
    Wire m_tempWire;             // �����϶�/Ԥ��������


    float m_scale = 1.0f;
    wxPoint m_size;


    void OnPaint(wxPaintEvent& evt);
    int  HitTest(const wxPoint& pt);


    // ���������� + ����
    wxPoint Snap(const wxPoint& raw, bool* snapped);


    int HitHoverPin(const wxPoint& raw, bool* isInput, wxPoint* worldPos);

    int HitHoverCell(const wxPoint& raw, int* wireIdx, int* cellIdx, wxPoint* cellPos);

    int GetSelectedIndex() const { return m_selectedIndex; }
public:
    void ClearSelection();
    void SetSelectedIndex(int index);
    int HitTestPublic(const wxPoint& pt);
    bool IsClickOnEmptyAreaPublic(const wxPoint& canvasPos);

    // ��ݹ�����
    HandyToolKit* m_HandyToolKit;

    // ����״̬��
    ToolStateMachine* m_toolStateMachine;

    // ���߹�����
    CanvasEventHandler* m_CanvasEventHandler;

    // ����״̬������
    void SetStatus(wxString status);

    // ����Ĺ���״̬�ӿ�
    void SetCurrentTool(ToolType tool);
    void SetCurrentComponent(const wxString& componentName);

    // ��ͣ��Ϣ
    HoverInfo m_hoverInfo;

    // ��ͣ��Ϣ���
    void UpdateHoverInfo(const wxPoint& canvasPos);
private:
    // ���ӽ���״̬
    bool m_hasFocus;

    // ���ӽ����¼�����
    void OnFocus(wxFocusEvent& event);
    void OnKillFocus(wxFocusEvent& event);
    void EnsureFocus();


public:
    // ���ص��ı��ؼ��������뷨֧��
    bool m_isUsingHiddenCtrl;

    void StartTextEditing(int index);

    wxTextCtrl* m_sharedTextCtrl;


public:
    int HitTestText(wxPoint canvasPos);
    wxRect m_selectRect;

private:
    wxScrollBar* m_hScroll;    // ˮƽ������
    wxScrollBar* m_vScroll;    // ��ֱ������

public:
    void LayoutScrollbars();
    void OnScroll(wxScrollEvent& event);
    std::pair<wxPoint, wxPoint> ValidSetOffRange();
    std::pair<float, float> ValidScaleRange();
    void SetoffSet(wxPoint offset);

    // �߼����� �� �豸����
    wxPoint LogicToDevice(const wxPoint& logicPoint) const;
    // �豸���� �� �߼�����  
    wxPoint DeviceToLogic(const wxPoint& devicePoint) const;

private:
    CanvasElement m_previewElement = CanvasElement("AND GATE", wxPoint(0, 0)); // Ԥ��Ԫ��ָ��
public:
    void SetPreviewElement(const wxString& name, wxPoint pos);









    wxDECLARE_EVENT_TABLE();
};