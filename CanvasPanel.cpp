#include <wx/graphics.h> 
#include <wx/dcbuffer.h>
#include <wx/dcgraph.h>  

#include "CanvasPanel.h"
#include "my_log.h"

#include "MainFrame.h"
#include "ToolStateMachine.h"
#include "HandyToolKit.h"
#include "CanvasEventHandler.h"

wxBEGIN_EVENT_TABLE(CanvasPanel, wxPanel)
EVT_PAINT(CanvasPanel::OnPaint)
EVT_LEFT_DOWN(CanvasPanel::OnLeftDown)
EVT_LEFT_UP(CanvasPanel::OnLeftUp)
EVT_LEFT_DCLICK(CanvasPanel::OnLeftDoubleClick)
EVT_RIGHT_DOWN(CanvasPanel::OnRightDown)
EVT_RIGHT_UP(CanvasPanel::OnRightUp)
EVT_MOTION(CanvasPanel::OnMouseMove)
EVT_KEY_DOWN(CanvasPanel::OnKeyDown)
EVT_MOUSEWHEEL(CanvasPanel::OnMouseWheel)
EVT_SET_FOCUS(CanvasPanel::OnFocus)
EVT_KILL_FOCUS(CanvasPanel::OnKillFocus)
EVT_SCROLL(CanvasPanel::OnScroll)
wxEND_EVENT_TABLE()

CanvasPanel::CanvasPanel(MainFrame* parent, size_t size_x, size_t size_y)
    : wxPanel(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize,
        wxFULL_REPAINT_ON_RESIZE | wxBORDER_NONE),
    m_mainFrame(parent),
    m_size{wxSize(size_x,size_y)},
    m_offset(0, 0), m_scale(1.0f), m_grid(20),
    m_hoverInfo{}, m_hasFocus(false),
    m_hiddenTextCtrl(nullptr),
    m_isUsingHiddenCtrl(false), m_currentEditingTextIndex(-1) {
    SetupHiddenTextCtrl();

    //滚动条
    m_vScroll = new wxScrollBar(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSB_VERTICAL);
    m_hScroll = new wxScrollBar(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSB_HORIZONTAL);
    LayoutScrollbars();

    // 工具状态机
    m_toolStateMachine = new ToolStateMachine(this);

    // 工具管理器
    m_CanvasEventHandler = new CanvasEventHandler(this, m_toolStateMachine);

    // 快捷工具栏
    m_HandyToolKit = new HandyToolKit(this, m_CanvasEventHandler);

    m_CanvasEventHandler->SetCurrentTool(ToolType::SELECT_TOOL);

    SetBackgroundStyle(wxBG_STYLE_PAINT);
    SetBackgroundColour(*wxWHITE);

    SetFocus();
    MyLog("CanvasPanel: constructed\n");
}

void CanvasPanel::OnLeftDown(wxMouseEvent& evt){
    EnsureFocus();
    m_HandyToolKit->Hide();

    m_CanvasEventHandler->ResetEventHandled();
    // 交给工具管理器处理
    if (m_CanvasEventHandler) {
        m_CanvasEventHandler->OnCanvasLeftDown(evt);
        if (m_CanvasEventHandler->IsEventHandled()) {
            return;
        }
    }

    evt.Skip();
}

void CanvasPanel::OnMouseMove(wxMouseEvent& evt) {
    UpdateHoverInfo(evt.GetPosition());

    m_CanvasEventHandler->ResetEventHandled();
    if (m_CanvasEventHandler) {
        m_CanvasEventHandler->OnCanvasMouseMove(evt);
        if (m_CanvasEventHandler->IsEventHandled()) {
            return; 
        }
    }
    evt.Skip();
}

void CanvasPanel::OnLeftUp(wxMouseEvent& evt){

    m_CanvasEventHandler->ResetEventHandled();
    if (m_CanvasEventHandler) {
        m_CanvasEventHandler->OnCanvasLeftUp(evt);
        if (m_CanvasEventHandler->IsEventHandled()) {
            return; 
        }
    }
    evt.Skip();
}

void CanvasPanel::OnLeftDoubleClick(wxMouseEvent& evt) {

    m_CanvasEventHandler->ResetEventHandled();
    if (m_CanvasEventHandler) {
        m_CanvasEventHandler->OnCanvasLeftDoubleClick(evt);
        if (m_CanvasEventHandler->IsEventHandled()) {
            return;
        }
    }
    evt.Skip();
}

void CanvasPanel::OnKeyDown(wxKeyEvent& evt) {

    m_CanvasEventHandler->ResetEventHandled();
    if (m_CanvasEventHandler) {
        m_CanvasEventHandler->OnCanvasKeyDown(evt);
        if (m_CanvasEventHandler->IsEventHandled()) {
            return; 
        }
    }
    evt.Skip();
}

void CanvasPanel::OnMouseWheel(wxMouseEvent& evt) {
    m_CanvasEventHandler->ResetEventHandled();
    if (m_CanvasEventHandler){
        m_CanvasEventHandler->OnCanvasMouseWheel(evt);
        if (m_CanvasEventHandler->IsEventHandled()) {
            return; 
        }
    }
    evt.Skip();
}

void CanvasPanel::SetScale(float scale) {
	float min_scale, max_scale;
	std::tie(min_scale, max_scale) = ValidScaleRange();
    if (scale < min_scale) scale = min_scale;
    if (scale > max_scale) scale = max_scale;
    m_scale = scale;
    Refresh(); 
}

wxPoint CanvasPanel::ScreenToCanvas(const wxPoint& screenPos) const{
    return wxPoint(
        static_cast<int>((screenPos.x - m_offset.x) / m_scale),
        static_cast<int>((screenPos.y - m_offset.y) / m_scale)
    );
}

wxPoint CanvasPanel::CanvasToScreen(const wxPoint& canvasPos) const
{
    return wxPoint(
        static_cast<int>(canvasPos.x * m_scale + m_offset.x),
        static_cast<int>(canvasPos.y * m_scale + m_offset.y)
    );
}

void CanvasPanel::AddElement(const wxString& name, const wxPoint& pos){
    extern std::vector<CanvasElement> g_elements;
    auto it = std::find_if(g_elements.begin(), g_elements.end(),
        [&](const CanvasElement& e) { return e.GetName() == name; });
    if (it == g_elements.end()) return;
    CanvasElement clone = *it;

    wxPoint standardpos = pos;
    const auto& outputPins = clone.GetOutputPins();
    const auto& inputPins = clone.GetInputPins();

    if (!outputPins.empty()) {
        // 优先使用输出引脚
        Pin standardPin = outputPins[0];
        standardpos = pos - wxPoint(standardPin.pos.x + m_grid, standardPin.pos.y - m_grid);
    }
    else if (!inputPins.empty()) {
        // 如果没有输出引脚，使用输入引脚
        Pin standardPin = inputPins[0];
        standardpos = pos - wxPoint(standardPin.pos.x + m_grid, standardPin.pos.y - m_grid);
    }
    // 如果都没有引脚，直接使用原位置
    clone.SetPos(standardpos);

    m_elements.push_back(clone);
    Refresh();
    UndoStackPush(std::make_unique<CmdAddElement>(clone.GetName(), m_elements.size() - 1));
}

void CanvasPanel::AddElementWithIns(CanvasElement element) {
    m_elements.push_back(element);
    Refresh();

}

void CanvasPanel::AddElementWithoutRecord(const wxString& name, const wxPoint& pos) {
    extern std::vector<CanvasElement> g_elements;
    auto it = std::find_if(g_elements.begin(), g_elements.end(),
        [&](const CanvasElement& e) { return e.GetName() == name; });
    if (it == g_elements.end()) return;
    CanvasElement clone = *it;

    wxPoint standardpos = pos;
    const auto& outputPins = clone.GetOutputPins();
    const auto& inputPins = clone.GetInputPins();

    if (!outputPins.empty()) {
        // 优先使用输出引脚
        Pin standardPin = outputPins[0];
        standardpos = pos - wxPoint(standardPin.pos.x + m_grid, standardPin.pos.y - m_grid);
    }
    else if (!inputPins.empty()) {
        // 如果没有输出引脚，使用输入引脚
        Pin standardPin = inputPins[0];
        standardpos = pos - wxPoint(standardPin.pos.x + m_grid, standardPin.pos.y - m_grid);
    }
    // 如果都没有引脚，直接使用原位置
    clone.SetPos(standardpos);

    m_elements.push_back(clone);
    Refresh();
}


void CanvasPanel::ReclaimElement(CanvasElement element, int index) {
    m_elements.insert(m_elements.begin() + index, element);
    Refresh();
}

void CanvasPanel::OnPaint(wxPaintEvent&) {
    LayoutScrollbars();
    wxAutoBufferedPaintDC dc(this);
    dc.Clear();
    
    wxGCDC* gcdc = nullptr;
    wxGraphicsContext* gc = nullptr;

    if (wxGraphicsRenderer::GetDefaultRenderer()) {
        gcdc = new wxGCDC(dc);
        gc = gcdc->GetGraphicsContext();
    }

    if (gc) {
        // 启用高质量抗锯齿
        gc->SetAntialiasMode(wxANTIALIAS_DEFAULT);

        // 应用缩放和偏移（逻辑坐标 -> 设备坐标）
        gc->Scale(m_scale, m_scale);
        gc->Translate(m_offset.x / m_scale, m_offset.y / m_scale);

        // 高DPI适配：获取设备缩放因子
        double dpiScale = GetContentScaleFactor();
        gc->Scale(dpiScale, dpiScale);

        // 绘制网格（逻辑坐标，线宽随缩放自适应）
        const wxColour gridColor(240, 240, 240);
        //const wxColour gridColor(50, 60, 80);
        gc->SetPen(wxPen(gridColor, 1.0 / m_scale)); // 笔宽在逻辑坐标下调整

        wxSize sz = wxSize(m_size.x + 1, m_size.y + 1);
        int maxX = static_cast<int>(sz.x);
        int maxY = static_cast<int>(sz.y);

        for (int x = 0; x < maxX; x += m_grid) {
            gc->StrokeLine(x, 0, x, maxY);
        }
        for (int y = 0; y < maxY; y += m_grid) {
            gc->StrokeLine(0, y, maxX, y);
        }

        // 绘制导线（矢量线段）
        gc->SetPen(wxPen(*wxBLACK, 1.5 / m_scale)); // 导线宽度自适应
        for (const auto& w : m_wires) w.Draw(*gcdc);
        if (m_toolStateMachine->GetWireState() == WireToolState::WIRE_DRAWING) m_previewWire.Draw(*gcdc);

        // 绘制预览元素
        if (m_toolStateMachine->GetComponentState() == ComponentToolState::COMPONENT_PREVIEW) {
            m_previewElement.Draw(*gcdc);
        }

        // 绘制元素（使用矢量绘制）
        for (size_t i = 0; i < m_elements.size(); ++i) {
            m_elements[i].Draw(*gcdc); // 确保元素内部使用gc绘制
        }

        // 悬停引脚高亮（绿色空心圆）
        if (m_hoverInfo.IsOverPin()) {
            gc->SetBrush(*wxTRANSPARENT_BRUSH);
            gc->SetPen(wxPen(wxColour(0, 255, 0), 1.0 / m_scale));
            gc->DrawEllipse(m_hoverInfo.snappedPos.x - 3, m_hoverInfo.snappedPos.y - 3, 6, 6);
        }

        if (m_hoverInfo.IsOverCell()) {
            gc->SetBrush(*wxTRANSPARENT_BRUSH);
            gc->SetPen(wxPen(wxColour(0, 255, 0), 1.0 / m_scale));
            gc->DrawEllipse(m_hoverInfo.cellPos.x - 3, m_hoverInfo.cellPos.y - 3, 6, 6);

            gc->SetPen(wxPen(wxColour(*wxBLACK), 1.0 / m_scale));
            gc->SetBrush(wxColour(*wxBLACK));
            Wire w = GetWires()[m_hoverInfo.wireIndex];
            wxPoint sectionStart = w.pts[m_hoverInfo.wireSectionIndex].pos;
            wxPoint sectionEnd = w.pts[m_hoverInfo.wireSectionIndex + 1].pos;
            wxPoint sectionMid = (sectionStart + sectionEnd) / 2;
            gc->DrawRectangle(sectionMid.x - 3, sectionMid.y - 3, 6, 6);
        }

        

        //if (m_hoverInfo.IsOverMidCell()) {
        //    gc->SetPen(wxPen(wxColour(*wxBLACK), 1.0 / m_scale));
        //    gc->SetBrush(wxColour(*wxBLACK));
        //    gc->DrawEllipse(GetWires()[m_hoverInfo.midWireIndex].midCells[m_hoverInfo.midCellIndex].pos.x-6, GetWires()[m_hoverInfo.midWireIndex].midCells[m_hoverInfo.midCellIndex].pos.y - 6, 12, 12);
        //}

        // 绘制文本元素 - 修改为使用 unique_ptr
        for (auto& textElem : m_textElements) {
            textElem.Draw(*gcdc);
        }

        // 绘制选中边框
        if (m_toolStateMachine->GetCurrentTool() == ToolType::SELECT_TOOL) {
            for (size_t i = 0; i < m_selElemIdx.size(); i++) {
                wxRect b = m_elements[m_selElemIdx[i]].GetBounds();
                gc->SetPen(wxPen(wxColor(44, 145, 224), 2.0));
                gc->SetBrush(*wxTRANSPARENT_BRUSH);
                gc->DrawRectangle(b.x - 2, b.y - 3, b.width + 5, b.height + 5);

                gc->SetPen(wxPen(wxColor(44, 145, 224, 32), 2.0));
                gc->SetBrush(*wxTRANSPARENT_BRUSH);
                gc->DrawRectangle(b.x - 4, b.y - 5, b.width + 9, b.height + 9);

                gc->SetPen(wxPen(wxColor(44, 145, 224, 32), 4.0));
                gc->SetBrush(*wxTRANSPARENT_BRUSH);
                gc->DrawRectangle(b.x - 6, b.y - 7, b.width + 13, b.height + 13);
            }
            for (size_t i = 0; i < m_selTxtIdx.size(); i++) {
                wxRect b = m_textElements[m_selTxtIdx[i]].GetBounds();
                gc->SetPen(wxPen(wxColor(44, 145, 224), 2.0));
                gc->SetBrush(*wxTRANSPARENT_BRUSH);
                gc->DrawRectangle(b.x - 3, b.y - 3, b.width + 5, b.height + 5);

                gc->SetPen(wxPen(wxColor(44, 145, 224, 32), 2.0));
                gc->SetBrush(*wxTRANSPARENT_BRUSH);
                gc->DrawRectangle(b.x - 5, b.y - 5, b.width + 9, b.height + 9);

                gc->SetPen(wxPen(wxColor(44, 145, 224, 32), 4.0));
                gc->SetBrush(*wxTRANSPARENT_BRUSH);
                gc->DrawRectangle(b.x - 7, b.y - 7, b.width + 13, b.height + 13);
            }
            for (size_t i = 0; i < m_selWireIdx.size(); i++) {
                m_wires[m_selWireIdx[i]].DrawColor(*gcdc);

            }
        }

        // 绘制选择边框
        if (m_toolStateMachine->GetSelectState() == SelectToolState::RECTANGLE_SELECT) {
            wxRect selRect = m_selectRect;
            gc->SetPen(wxPen(wxColor(44, 145, 224), 2 / m_scale));
            gc->SetBrush(wxColor(44, 145, 224, 32));
            gc->DrawRectangle(selRect.x, selRect.y, selRect.width, selRect.height);
        }

        if (m_toolStateMachine->GetEraserState() == EraserToolState::RECTANGLE_ERASER) {
            wxRect eraRect = m_eraserRect;
            gc->SetPen(wxPen(wxColor(128, 128, 128), 2 / m_scale));
            gc->SetBrush(wxColor(128, 128, 128, 32));
            gc->DrawRectangle(eraRect.x, eraRect.y, eraRect.width, eraRect.height);
        }
        delete gcdc; // 释放资源
    }
    else {
        // 如果无法获取 GraphicsContext，回退到原始绘制方法
        // 应用缩放和偏移
        dc.SetUserScale(m_scale, m_scale);
        dc.SetDeviceOrigin(m_offset.x, m_offset.y);  // 设置设备原点偏移

        // 1. 绘制网格（网格大小随缩放自适应）
        const wxColour c(240, 240, 240);
        dc.SetPen(wxPen(c, 1));
        // 计算可见区域的网格范围（基于缩放后的画布大小）
        wxSize sz = GetClientSize();
        int maxX = static_cast<int>(sz.x);  // 转换为画布坐标
        int maxY = static_cast<int>(sz.y);
        for (int x = 0; x < maxX; x += m_grid)
            dc.DrawLine(x, 0, x, maxY);
        for (int y = 0; y < maxY; y += m_grid)
            dc.DrawLine(0, y, maxX, y);

        // 绘制预览元素
        if (m_toolStateMachine->GetComponentState() == ComponentToolState::COMPONENT_PREVIEW) {
            m_previewElement.Draw(dc);
        }

        // 3. 绘制导线（导线坐标基于画布，缩放由DC处理）
        for (const auto& w : m_wires) w.Draw(dc);
        if (m_toolStateMachine->GetWireState() == WireToolState::WIRE_DRAWING) m_CanvasEventHandler->m_tempWire.Draw(dc);

        // 4. 悬停引脚：绿色空心圆
        if (m_hoverInfo.pinIndex != -1) {
            dc.SetBrush(*wxTRANSPARENT_BRUSH);              // 不填充 → 空心
            dc.SetPen(wxPen(wxColour(0, 255, 0), 1));       // 绿色边框，线宽 2
            dc.DrawCircle(m_hoverInfo.pinPos, 3);                // 半径 3 像素
        }

        if (m_hoverInfo.cellIndex != -1) {
            /*MyLog("DRAW GREEN CELL: wire=%zu cell=%zu  pos=(%d,%d)\n",
                m_hoverCellWire, m_hoverCellIdx,
                m_hoverCellPos.x, m_hoverCellPos.y);*/
            dc.SetBrush(*wxTRANSPARENT_BRUSH);
            dc.SetPen(wxPen(wxColour(0, 255, 0), 1));
            dc.DrawCircle(m_hoverInfo.cellPos, 3);
        }
        // 5. 绘制文本元素 - 修改为使用 unique_ptr
        for (auto& textElem : m_textElements) {
            textElem.Draw(dc);
        }
    }
}

int CanvasPanel::HitElementTest(const wxPoint& canvasPos)
{
    for (size_t i = 0; i < m_elements.size(); ++i) {
        // 元素的边界是画布坐标，直接比较
        if (m_elements[i].GetBounds().Contains(canvasPos)) {
            return i;
        }
    }
    return -1;
}

int CanvasPanel::HitHoverPin(const wxPoint& canvasPos, bool* isInput, wxPoint* worldPos){
    for (size_t i = 0; i < m_elements.size(); ++i) {
        const auto& elem = m_elements[i];
        // 输入引脚尖端（突出 1 px）
        for (size_t p = 0; p < elem.GetInputPins().size(); ++p) {
            wxPoint tip = elem.GetPos() + wxPoint(elem.GetInputPins()[p].pos.x - 1,
                elem.GetInputPins()[p].pos.y);
            if (abs(canvasPos.x - tip.x) <= 4 && abs(canvasPos.y - tip.y) <= 4) {
                *isInput = true;
                *worldPos = tip;
                return p;
            }
        }
        // 输出引脚尖端（突出 1 px）
        for (size_t p = 0; p < elem.GetOutputPins().size(); ++p) {
            wxPoint tip = elem.GetPos() + wxPoint(elem.GetOutputPins()[p].pos.x + 1,
                elem.GetOutputPins()[p].pos.y);
            if (abs(canvasPos.x - tip.x) <= 4 && abs(canvasPos.y - tip.y) <= 4) {
                *isInput = false;
                *worldPos = tip;
                return p;
            }
        }
    }
    return -1;
}

int CanvasPanel::HitWire(const wxPoint& canvasPos) {
    // 首先将点击位置对齐到最近的网格点
    wxPoint snappedPos = Snap(canvasPos);
    for (size_t w = 0; w < m_wires.size(); ++w) {
        const auto& wire = m_wires[w];
        for (size_t c = 0; c < wire.cells.size(); ++c) {
            const wxPoint& cell = wire.cells[c].pos;

            // 检查是否在网格点上且与对齐后的点击位置匹配
            if (cell.x == snappedPos.x && cell.y == snappedPos.y) {
                // 二次验证：确保在点击半径内
                if (abs(canvasPos.x - cell.x) <= HIT_RADIUS &&
                    abs(canvasPos.y - cell.y) <= HIT_RADIUS) {
                    return w;
                }
            }
        }
    }
    return -1;
}

int CanvasPanel::HitHoverCell(const wxPoint& canvasPos) {
    int wireIdx = HitWire(canvasPos);
    if (wireIdx == -1) return -1;

    wxPoint snappedPos = Snap(canvasPos);

    // 用于暂存命中的非Mid节点（作为备选）
    int fallbackIndex = -1;

    for (size_t c = 0; c < m_wires[wireIdx].cells.size(); ++c) {
        const auto& cellData = m_wires[wireIdx].cells[c];
        const wxPoint& cell = cellData.pos;
        bool isHit = false;

        // --- 命中检测逻辑 (保持原样) ---
        if (cell.x == snappedPos.x && cell.y == snappedPos.y) {
            if (abs(canvasPos.x - cell.x) <= HIT_RADIUS &&
                abs(canvasPos.y - cell.y) <= HIT_RADIUS) {
                isHit = true;
            }
        }
        else {
            // 原逻辑：只有Mid类型才允许在没Snap住的情况下进行半径检测
            if (cellData.type == CellType::Mid) {
                if (abs(canvasPos.x - cell.x) <= HIT_RADIUS &&
                    abs(canvasPos.y - cell.y) <= HIT_RADIUS) {
                    isHit = true;
                }
            }
        }

        // --- 优先级处理逻辑 ---
        if (isHit) {
            if (cellData.type == CellType::Mid) {
                return c; // 优先级最高：一旦发现 Mid 命中，立即返回！
            }

            // 如果不是 Mid，但命中了，先记下来。
            // 只有当 fallbackIndex 还没被赋值时才赋值(保留第一个命中的)，
            // 或者你可以根据距离更新最近的一个。
            if (fallbackIndex == -1) {
                fallbackIndex = c;
            }
            // 关键：不要在这里 return，继续找后面有没有 Mid
        }
    }

    // 如果循环跑完都没找到 Mid，但找到了其他点，就返回那个点
    return fallbackIndex;
}

int CanvasPanel::HitWireSection(const wxPoint& canvasPos) {
    return GetWires()[HitWire(canvasPos)].cells[HitHoverCell(canvasPos)].pre_pts_idx;
}

void CanvasPanel::DeleteSelected() {


    Refresh();
}

void CanvasPanel::OnRightDown(wxMouseEvent& evt) {
    m_HandyToolKit->SetPosition(evt.GetPosition() + wxPoint(240, 124));
    m_HandyToolKit->Show();
    m_HandyToolKit->SetFocus();
}

void CanvasPanel::OnRightUp(wxMouseEvent& evt) {
    m_HandyToolKit->Hide();
}

void CanvasPanel::SetStatus(wxString status) {
    m_mainFrame->SetStatusText(status, 0);

}

void CanvasPanel::SetCurrentTool(ToolType tool) {
	m_toolStateMachine->SetCurrentTool(tool);
}

void CanvasPanel::SetCurrentComponent(const wxString& componentName) {
    m_toolStateMachine->SetCurrentTool(ToolType::COMPONENT_TOOL);
	m_toolStateMachine->SetComponentState(ComponentToolState::COMPONENT_PREVIEW);
	m_CanvasEventHandler->SetCurrentComponent(componentName);
}

void CanvasPanel::UpdateHoverInfo(const wxPoint& screenPos) {
    m_hoverInfo.screenPos = screenPos;
    m_hoverInfo.canvasPos = ScreenToCanvas(screenPos);
    m_hoverInfo.snappedPos = Snap(m_hoverInfo.canvasPos);

    // 悬停引脚信息检测
    bool isInput = false;
    wxPoint pinWorldPos;
	int pinIdx = HitHoverPin(m_hoverInfo.canvasPos, &isInput, &pinWorldPos);

	// 导线控制点信息检测
    int wireIdx = HitWire(m_hoverInfo.canvasPos);
    int wireSectionIdx = -1;
    int cellIdx = HitHoverCell(m_hoverInfo.canvasPos);
    bool isCellMid = false;
	wxPoint cellWorldPos;

    if (wireIdx != -1 && cellIdx != -1) {
        Cell cell = m_wires[wireIdx].cells[cellIdx];
        isCellMid = cell.type == CellType::Mid ? true : false;
        cellWorldPos = cell.pos;
        wireSectionIdx = cell.pre_pts_idx;
    }

	// 悬停元件信息检测
    int elementIndex = HitElementTest(m_hoverInfo.canvasPos);

    // 悬停文本检测
	int textIndex = HitTestText(m_hoverInfo.canvasPos);

    // 更新信息
    m_hoverInfo.pinIndex = pinIdx;
	m_hoverInfo.isInputPin = isInput;
	m_hoverInfo.pinPos = pinWorldPos;

    m_hoverInfo.wireIndex = wireIdx;
    m_hoverInfo.wireSectionIndex = wireSectionIdx;
    m_hoverInfo.cellIndex = cellIdx;
    m_hoverInfo.isCellMiddle = isCellMid;
	m_hoverInfo.cellPos = cellWorldPos;

    m_hoverInfo.elementIndex = elementIndex;
    if (elementIndex != -1) m_hoverInfo.elementName = m_elements[elementIndex].GetName();
    else m_hoverInfo.elementName = "";

	m_hoverInfo.textIndex = textIndex;
    Refresh(); // 触发重绘以显示悬停效果
    wxString hover = "";
    if (m_hoverInfo.IsOverPin()) {
        hover = (wxString::Format("悬停于: %sPin[%d]",
            m_hoverInfo.isInputPin ? "Input" : "Output", m_hoverInfo.pinIndex));
    }
    else if (m_hoverInfo.IsOverCell()) {
        if (m_hoverInfo.isCellMiddle) {
            hover = (wxString::Format("悬停于: Wire[%d] Section[%d] 控制点",
                m_hoverInfo.wireIndex, m_hoverInfo.wireSectionIndex));
        }
        else {
            hover = (wxString::Format("悬停于: Wire[%d] Cell[%d]",
                m_hoverInfo.wireIndex, m_hoverInfo.cellIndex));
        }
    }
    else if (m_hoverInfo.IsOverElement()) {
        hover = (wxString::Format("悬停于: Component[%s]",
            m_hoverInfo.elementName));
    }
    else if (m_hoverInfo.IsOverText()) {
        hover = (wxString::Format("悬停于: TextBox[%d]",
            m_hoverInfo.textIndex));
    }
    else {
        hover = wxString::Format("悬停于: 无悬停对象");
    }

    wxString cursor = wxString::Format("指针位置: (%d, %d)", m_hoverInfo.canvasPos.x, m_hoverInfo.canvasPos.y);

    wxString zoom = wxString::Format("缩放：%d%%", int(m_scale * 100));

    m_mainFrame->SetStatusText(cursor, 1);
    m_mainFrame->SetStatusText(hover, 2);
    m_mainFrame->SetStatusText(zoom, 3);
    m_CanvasEventHandler->UpdateHoverInfo(m_hoverInfo);
}

void CanvasPanel::OnFocus(wxFocusEvent& event) {
    m_hasFocus = true;
    Refresh();
    event.Skip();
}

void CanvasPanel::OnKillFocus(wxFocusEvent& event) {
    m_hasFocus = false;
    Refresh();
    event.Skip();
}

void CanvasPanel::EnsureFocus() {
    if (!m_hasFocus) {
        SetFocus();
    }
}

void CanvasPanel::SetupHiddenTextCtrl() {
    if (!m_hiddenTextCtrl) {
        m_hiddenTextCtrl = new wxTextCtrl(this, wxID_ANY, "",
            wxPoint(-1000, -1000), wxSize(1, 1),
            wxTE_PROCESS_ENTER | wxTE_RICH2);
        m_hiddenTextCtrl->Hide();

        m_hiddenTextCtrl->Bind(wxEVT_TEXT_ENTER, &CanvasPanel::OnHiddenTextCtrlEnter, this);
        m_hiddenTextCtrl->Bind(wxEVT_KILL_FOCUS, &CanvasPanel::OnHiddenTextCtrlKillFocus, this);
    }
}

void CanvasPanel::AttachHiddenTextCtrlToElement(int textIndex) {
    if (textIndex >= 0 && textIndex < (int)m_textElements.size()) {
        // 分离当前编辑的元素
        if (m_currentEditingTextIndex != -1 && m_currentEditingTextIndex < (int)m_textElements.size()) {
            m_textElements[m_currentEditingTextIndex].DetachHiddenTextCtrl();
            m_textElements[m_currentEditingTextIndex].StopEditing();
        }

        // 附加到新元素
        m_currentEditingTextIndex = textIndex;
        m_textElements[textIndex].AttachHiddenTextCtrl(m_hiddenTextCtrl);
        m_textElements[textIndex].StartEditing();

        // 显示隐藏TextCtrl（在正确位置）
        m_hiddenTextCtrl->Show();
        Refresh();
    }
}

void CanvasPanel::DetachHiddenTextCtrl() {
    if (m_currentEditingTextIndex != -1 && m_currentEditingTextIndex < (int)m_textElements.size()) {
        m_textElements[m_currentEditingTextIndex].DetachHiddenTextCtrl();
        m_textElements[m_currentEditingTextIndex].StopEditing();
        m_currentEditingTextIndex = -1;
    }

    m_hiddenTextCtrl->Hide();
    m_hiddenTextCtrl->SetPosition(wxPoint(-1000, -1000));
    Refresh();
}

void CanvasPanel::CreateTextElement(const wxPoint& position, wxString text) {
    m_textElements.push_back(std::move(CanvasTextElement(this, text, position)));
    AttachHiddenTextCtrlToElement(static_cast<int>(m_textElements.size() - 1));

    Refresh();
    UndoStackPush(std::make_unique<CmdAddText>( m_textElements.size() - 1));
}

void CanvasPanel::AddTextWithIns(CanvasTextElement text) {
    m_textElements.push_back(std::move(text));
    Refresh();
}

void CanvasPanel::ReclaimText(CanvasTextElement text, int index) {
    m_textElements.insert(m_textElements.begin() + index, std::move(text));
    Refresh();
}

void CanvasPanel::CreateTextElementWithoutRecord(const wxPoint& position, wxString text) {
    m_textElements.push_back(std::move(CanvasTextElement(this, text, position)));
    //AttachHiddenTextCtrlToElement(static_cast<int>(m_textElements.size() - 1));

    Refresh();
}

void CanvasPanel::StartTextEditing(int index) {
    AttachHiddenTextCtrlToElement(index);
}

int CanvasPanel::HitTestText(wxPoint canvasPos) {
    for (int i = 0; i < m_textElements.size(); i++) {
        if (m_textElements[i].Contains(canvasPos)) return i;
    }
    return -1;
}

void CanvasPanel::OnScroll(wxScrollEvent& event) {
    if (event.GetOrientation() == wxHORIZONTAL) {
        m_offset.x = -event.GetPosition() * m_scale;
    }
    else {
        m_offset.y = -event.GetPosition() * m_scale;
    }
    Refresh();
}

void CanvasPanel::LayoutScrollbars(){
    wxSize clientSize = GetClientSize();
    int scrollbarSize = wxSystemSettings::GetMetric(wxSYS_VSCROLL_X);
    int width = 24;

    // 水平滚动条：底部，宽度要减去垂直滚动条的宽度
    m_hScroll->SetSize(clientSize.x - width, width);
    m_hScroll->SetPosition(wxPoint(0, clientSize.y - width));
    m_hScroll->SetThumbSize((clientSize.x - width - 1) / m_scale);
    m_hScroll->SetRange(m_size.x);
    m_hScroll->SetThumbPosition(-m_offset.x / m_scale);

    // 垂直滚动条：右侧，高度要减去水平滚动条的高度   
    m_vScroll->SetSize(width, clientSize.y);
    m_vScroll->SetPosition(wxPoint(clientSize.x - width, 0));
    m_vScroll->SetThumbSize((clientSize.y - width - 1) / m_scale);
    m_vScroll->SetRange(m_size.y);
    m_vScroll->SetThumbPosition(-m_offset.y / m_scale);

    Refresh();
}

void CanvasPanel::SetoffSet(wxPoint offset) {
	wxPoint min_offset, max_offset;
	std::tie(min_offset, max_offset) = ValidSetOffRange();
	if (offset.x < min_offset.x) offset.x = min_offset.x;
	if (offset.x > max_offset.x) offset.x = max_offset.x;
	if (offset.y < min_offset.y) offset.y = min_offset.y;
	if (offset.y > max_offset.y) offset.y = max_offset.y;
	m_offset = offset;
    Refresh();
}

wxPoint CanvasPanel::LogicToDevice(const wxPoint& logicPoint) const
{
    return wxPoint(
        static_cast<int>(logicPoint.x * m_scale + m_offset.x),
        static_cast<int>(logicPoint.y * m_scale + m_offset.y)
    );
}

wxPoint CanvasPanel::DeviceToLogic(const wxPoint& devicePoint) const
{
    return wxPoint(
        static_cast<int>((devicePoint.x - m_offset.x) / m_scale),
        static_cast<int>((devicePoint.y - m_offset.y) / m_scale)
    );
}

std::pair<wxPoint, wxPoint> CanvasPanel::ValidSetOffRange() {
    // (0, 0)的逻辑坐标对应的设备坐标为 m_offset，因此m_offset的最大值为(0,0)
    wxPoint maxOffset(0, 0);
    // 计算逻辑坐标 (m_size.x, m_size.y) 对应的设备坐标的最大值为 (GetClientSize().x- m_hScroll->GetSize().y - 1, GetClientSize().y- m_hScroll->GetSize().y - 1)，此时的 m_offset 即为最小值
    wxPoint minOffset(
        GetClientSize().x - static_cast<int>(m_size.x * m_scale) - m_hScroll->GetSize().y - 1,
        GetClientSize().y - static_cast<int>(m_size.y * m_scale) - m_hScroll->GetSize().y - 1
    );

    return std::make_pair(minOffset, maxOffset);
}

std::pair<float, float> CanvasPanel::ValidScaleRange() {
    // 对于最右侧最下侧的逻辑坐标 (m_size.x, m_size.y) 对应的设备坐标恰好为 (GetClientSize().x- m_hScroll->GetSize().y - 1, GetClientSize().y- m_hScroll->GetSize().y - 1) 时，计算出最小缩放比例
    float minScaleX = static_cast<float>((GetClientSize().x - m_hScroll->GetSize().y - 1) - m_offset.x) / static_cast<float>(m_size.x);
    float minScaleY = static_cast<float>((GetClientSize().y - m_hScroll->GetSize().y - 1) - m_offset.y) / static_cast<float>(m_size.y);
    float minScale = std::max(minScaleX, minScaleY);
    float maxScale = 5.0f; // 最大缩放比例
    return std::make_pair(minScale, maxScale);
}

void CanvasPanel::SetPreviewElement(const wxString& name, wxPoint pos) {
    extern std::vector<CanvasElement> g_elements;
    auto it = std::find_if(g_elements.begin(), g_elements.end(),
        [&](const CanvasElement& e) { return e.GetName() == name; });
    if (it == g_elements.end()) return;
    CanvasElement clone = *it;
    wxPoint standardpos = pos;
    const auto& outputPins = clone.GetOutputPins();
    const auto& inputPins = clone.GetInputPins();

    if (!outputPins.empty()) {
        // 优先使用输出引脚
        Pin standardPin = outputPins[0];
        standardpos = pos - wxPoint(standardPin.pos.x + m_grid, standardPin.pos.y - m_grid);
    }
    else if (!inputPins.empty()) {
        // 如果没有输出引脚，使用输入引脚
        Pin standardPin = inputPins[0];
        standardpos = pos - wxPoint(standardPin.pos.x + m_grid, standardPin.pos.y - m_grid);
    }
    clone.SetPos(standardpos);
    m_previewElement = clone;
    Refresh();
}

void CanvasPanel::UndoStackPush(std::unique_ptr<Command> command) {
	m_undoStack.Push(std::move(command));
	MainFrame* mainFrame = wxDynamicCast(GetParent(), MainFrame);
    if (mainFrame) {
        mainFrame->OnUndoStackChanged();
	}
}

void CanvasPanel::WireSetWholeOffSet(int index, const wxPoint& offset) {
    for (auto& cp : m_wires[index].pts) {
        cp.pos += offset;
	}
    m_wires[index].GenerateCells();
}

void CanvasPanel::WirePtsSetPos(int wireIndex, int controlPointIndex, const wxPoint& pos) {
	m_wires[wireIndex].pts[controlPointIndex].pos = pos;
    m_wires[wireIndex].GenerateCells();
    Refresh();
}

void CanvasPanel::ElementStatusChange(int index) {
    CanvasElement& elem = m_elements[index];
    wxString elementId = elem.GetId();

    if (elementId == "Pin_Input" || elementId.StartsWith("Pin"))
    {
        // 切换 Pin 状态
        if (elementId == "Pin_Input") {
            elem.ToggleState();  // 0↔1
        }
        else {
            int currentState = elem.GetOutputState();
            int newState = (currentState >= 2) ? 0 : currentState + 1; // 0→1→X→0
            elem.SetOutputState(newState);
        }

        Refresh();
    }
}

void CanvasPanel::ClearAll() {
	m_elements.clear();
	m_textElements.clear();
    m_wires.clear();
    Refresh();
}

void CanvasPanel::UpdateSelection(std::vector<int> m_elemIdx, std::vector<int> m_textIdx, std::vector<int> m_wireIdx ) {
    m_selTxtIdx = m_textIdx;
    m_selElemIdx = m_elemIdx;
    m_selWireIdx = m_wireIdx;
    Refresh();
}

void CanvasPanel::AddWire(const Wire& wire) {
    m_wires.push_back(wire); 
    m_wires.back().GenerateCells(); 
    Refresh(); 
    UndoStackPush(std::make_unique<CmdAddWire>(m_wires.size() - 1));
};

void CanvasPanel::AddWireWithoutRecord(const Wire& wire) {
    m_wires.push_back(wire);
    m_wires.back().GenerateCells();
    Refresh();
};

void CanvasPanel::ReclaimWire(Wire wire, int index) {
    m_wires.insert(m_wires.begin() + index, std::move(wire));
    m_wires[index].GenerateCells();
    Refresh();
};

void CanvasPanel::ElementSetPos(int index, const wxPoint& pos) {
    wxPoint oldPos = m_elements[index].GetPos();
    m_elements[index].SetPos(pos); 
    Refresh(); 
};