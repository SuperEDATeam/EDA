#include <wx/msgdlg.h>

#include "CanvasEventHandler.h"
#include "CanvasTextElement.h"
#include "HandyToolKit.h"
#include "ToolBars.h"

CanvasEventHandler::CanvasEventHandler(CanvasPanel* canvas, ToolStateMachine* toolstate)
    : m_canvas(canvas), m_toolStateMachine(toolstate), m_isTemporaryAction(false), m_eventHandled(false),
    m_editingWireIndex(-1), m_editingPointIndex(-1), m_draggingElementIndex(-1), m_editingTextIndex(-1){
}

void CanvasEventHandler::SetCurrentTool(ToolType tool) {
	TextToolState textState = m_toolStateMachine->GetTextState();
    if (textState != TextToolState::IDLE) {
        m_canvas->FinishTextEditing();
    }
    m_compntIdx.clear();
    m_textElemIdx.clear();
    m_wireIdx.clear();
    m_canvas->ClearSelection();

    m_toolStateMachine->SetCurrentTool(tool);

    // 设置适当的光标
    if (m_canvas) {
        switch (tool) {
        case ToolType::COMPONENT_TOOL: {
            m_canvas->SetCursor(wxCursor(wxCURSOR_CROSS));
            break;
            }
        case ToolType::TEXT_TOOL: {
            m_canvas->SetCursor(wxCursor(wxCURSOR_IBEAM));
            break;
            }
        case ToolType::WIRE_TOOL: {
            wxImage image("res\\icons\\wiring.png", wxBITMAP_TYPE_PNG);
            image.GetOptionInt(wxIMAGE_OPTION_CUR_HOTSPOT_X);
            image.GetOptionInt(wxIMAGE_OPTION_CUR_HOTSPOT_Y);
            wxCursor cursor(image);
            m_canvas->SetCursor(cursor);
            break;
        }
        case ToolType::DRAG_TOOL: {
            m_canvas->SetCursor(wxCursor(wxCURSOR_HAND));
            break;
            }
        default: {
            m_canvas->SetCursor(wxCursor(wxCURSOR_ARROW));
            break;
            }
        }
    }

    // 状态栏反馈
    if (true) {
        wxString toolName;
        switch (tool) {
        case ToolType::SELECT_TOOL:
            toolName = "选中工具";
            break;
        case ToolType::TEXT_TOOL:
            toolName = "文本工具";
            break;
        case ToolType::COMPONENT_TOOL:
            toolName = "元件工具";
            break;
        case ToolType::WIRE_TOOL:
            toolName = "导线工具";
            break;
        case ToolType::DRAG_TOOL:
            toolName = "拖动工具";
            break;
        case ToolType::DRAWING_TOOL:
            toolName = "绘图工具";
			break;
        }
        m_canvas->SetStatus(wxString::Format("当前工具: %s", toolName));
    }
}

void CanvasEventHandler::OnCanvasLeftDown(wxMouseEvent& evt) {
    // 清除文本编辑焦点
    if (m_editingTextIndex != -1) {
        m_canvas->FinishTextEditing();
    }

    wxPoint canvasPos = m_hoverInfo.pos;
    ToolType currentTool = m_toolStateMachine->GetCurrentTool();
    
    // 按优先级高低处理
    // 选择工具
    if (currentTool == ToolType::SELECT_TOOL){
        HandleSelectTool(evt);
        m_eventHandled = true;
        return;
    }

    // 元件工具
    if (currentTool == ToolType::COMPONENT_TOOL){
        HandleComponentTool();
        m_eventHandled = true;
        return;
    }

    // 导线工具，绘制导线中
    if (currentTool == ToolType::WIRE_TOOL && m_toolStateMachine->GetWireState() == WireToolState::WIRE_DRAWING) {
        PlaceWirePoint();
        m_eventHandled = true;
        return;
    }

    // 其他工具下，点击导线Cell，转换到导线工具，拉出分支导线
    if (m_hoverInfo.IsOverCell()) {
        m_previousTool = currentTool;
        m_isTemporaryAction = true;// 定义为临时切换，使用完毕后恢复
        SetCurrentTool(ToolType::WIRE_TOOL);
        StartWireDrawingDown(m_hoverInfo.snappedPos, CPType::Branch);
        m_eventHandled = true;
        return;
    }


    // 其他工具下，点击元件Pin，转换到导线工具，拉出导线
    if (m_hoverInfo.IsOverPin() || m_hoverInfo.IsOverCell()){
        m_isTemporaryAction = true; // 定义为临时切换，使用完毕后恢复
        SetCurrentTool(ToolType::WIRE_TOOL);
        CPType type = CPType::Free;
        if (m_hoverInfo.IsOverPin()) type = CPType::Pin;
        else if (m_hoverInfo.IsOverCell()) type = CPType::Branch;
        StartWireDrawingDown(m_hoverInfo.snappedPos, CPType::Pin);
        m_eventHandled = true;
        return;
    }

    // 其他工具下，点击文本框，转换到文本工具，进行文本编辑
    if (m_hoverInfo.textIndex != -1) {
        m_previousTool = currentTool;
        m_isTemporaryAction = true; // 定义为临时切换，使用完毕后恢复
        SetCurrentTool(ToolType::TEXT_TOOL);
        StartTextEditing(m_hoverInfo.textIndex);
        return;
    }

	// 在临时文本模式下，点击空白区域退回上个工具
    if (m_isTemporaryAction && currentTool == ToolType::TEXT_TOOL) {
        SetCurrentTool(m_previousTool);
        currentTool = m_previousTool;
        m_isTemporaryAction = false;
        m_eventHandled = true;
	}


    // 按工具类型处理其他情况
    switch (currentTool) {
    case ToolType::TEXT_TOOL: {
        HandleTextTool();
        m_eventHandled = true;
        break;
    }
    case ToolType::WIRE_TOOL: {
        CPType startType = CPType::Free;
        if (m_hoverInfo.IsOverPin()) startType = CPType::Pin;
        else if (m_hoverInfo.IsOverCell()) startType = CPType::Branch;
        StartWireDrawingDown(m_hoverInfo.snappedPos, startType);
        m_eventHandled = true;
        break;
    }
    case ToolType::DRAG_TOOL: {
        if (m_hoverInfo.IsEmptyArea()) {
            StartPanning(canvasPos);
            m_eventHandled = true;
        }
        else if (m_hoverInfo.IsOverElement()) {
            m_canvas->ElementStatusChange(m_hoverInfo.elementIndex);
            m_eventHandled = true;
        }
        break;
    }
    }
}


void CanvasEventHandler::StartWireDrawingDown(const wxPoint& startPos, CPType startType) {
    // 初始化临时导线
    m_isWireDraingCancel = false;
    m_tempWire.Clear();
    m_tempWire.AddPoint({startPos , startType});

    switch (startType) {
    case CPType::Pin:
        m_canvas->SetStatus("绘制导线: 从引脚开始，点击放置折点");
        break;
    case CPType::Free:
        m_canvas->SetStatus("绘制导线: 从自由点开始，点击放置折点");
        break;
    case CPType::Branch:
        m_canvas->SetStatus("绘制导线: 从分支点开始，点击放置折点");
        break;
    }
}

void  CanvasEventHandler::StartWireDrawingUp() {
    m_toolStateMachine->SetWireState(WireToolState::WIRE_DRAWING);
    lengthOfTempWire = m_tempWire.Size();
}

void CanvasEventHandler::UpdateWireDrawing(wxMouseEvent& evt) {
	if (m_toolStateMachine->GetWireState() != WireToolState::WIRE_DRAWING) return;
    ControlPoint pt;
    if (m_tempWire.Size() > lengthOfTempWire) pt = m_tempWire.pts[lengthOfTempWire];
    else pt = m_tempWire.pts.back();
    m_tempWire.pts.resize(lengthOfTempWire);
    CPType type = CPType::Bend;
    if(m_hoverInfo.IsOverPin()) type = CPType::Pin;
    if (m_hoverInfo.IsOverCell()) type = CPType::Branch;
    wxPoint pos = m_hoverInfo.snappedPos;
    tempWirePlus = 1;
    if (evt.ShiftDown()) {
        m_tempWire.AddPoint({ pos, type });
    }
    else {
        if (pos.x == m_tempWire.pts.back().pos.x) {
            m_tempWire.AddPoint({ pos, type });
        }
        else if (pos.y == m_tempWire.pts.back().pos.y) {
            m_tempWire.AddPoint({ pos, type });
        }
        else {
            tempWirePlus = 2;
            ControlPoint v = { wxPoint(m_tempWire.pts.back().pos.x, pos.y), CPType::Free };
            ControlPoint h = { wxPoint(pos.x, m_tempWire.pts.back().pos.y), CPType::Free };
            if (pt.pos.x == m_tempWire.pts.back().pos.x) {
                m_tempWire.AddPoint(v);
                m_tempWire.AddPoint({ pos, type });
            }
            else if (pt.pos.y == m_tempWire.pts.back().pos.y) {
                m_tempWire.AddPoint(h);
                m_tempWire.AddPoint({ pos, type });
            }
            else {
                m_tempWire.AddPoint(h);
                m_tempWire.AddPoint({ pos, type });
            }
        }
    }
    
    
    m_canvas->UpdatePreviewWire(m_tempWire);
    m_canvas->SetStatus(wxString::Format("正在绘制导线: 鼠标移动，点击放置折点"));
}

void CanvasEventHandler::PlaceWirePoint() {
    lengthOfTempWire += tempWirePlus;
    if (m_tempWire.pts.back().type == CPType::Pin || m_tempWire.pts.back().type == CPType::Branch) {
        FinishWireDrawing();
        return;
    }
}

void CanvasEventHandler::FinishWireDrawing() {
    if (m_toolStateMachine->GetWireState() != WireToolState::WIRE_DRAWING) return;
    m_tempWire.pts.resize(lengthOfTempWire);
    if (m_tempWire.pts[lengthOfTempWire - 2].pos == m_tempWire.pts[lengthOfTempWire - 1].pos) {
        m_tempWire.pts.pop_back();
    }
    m_tempWire.pts.back().type = CPType::Free;
    m_canvas->AddWire(m_tempWire);
    CancelWireDrawing();
}

void CanvasEventHandler::CancelWireDrawing() {
    m_tempWire.Clear();
    lengthOfTempWire = 0;
    m_isWireDraingCancel = true;
    m_toolStateMachine->SetWireState(WireToolState::IDLE);
	m_canvas->ClearPreviewWire();
}



void CanvasEventHandler::HandleComponentTool() {
    if (!m_currentComponent.IsEmpty()) {
        m_canvas->AddElement(m_currentComponent, m_hoverInfo.snappedPos);
        m_canvas->SetStatus(wxString::Format("已放置: %s， 吸附到 (%d, %d)", m_currentComponent, m_hoverInfo.snappedPos.x, m_hoverInfo.snappedPos.y));
        SetCurrentTool(ToolType::SELECT_TOOL);
        m_currentComponent.Clear(); // 清空当前元件
    }
    else {
        wxMessageBox("请先从元件库选择一个元件");
    }
}

void CanvasEventHandler::OnCanvasLeftUp(wxMouseEvent& evt) {
    wxPoint canvasPos = m_canvas->ScreenToCanvas(evt.GetPosition());
	ToolType currentTool = m_toolStateMachine->GetCurrentTool();
    
    // 单击选中
    if (m_toolStateMachine->GetSelectState() == SelectToolState::CLICK_SELECT) {
        FinishClickSelect();
        m_eventHandled = true;
        return;
    }
    // 选中拖动
    else if (m_toolStateMachine->GetSelectState() == SelectToolState::DRAG_SELECT) {
        m_canvas->UndoStackPush(std::make_unique<CmdMoveSelected>(m_textElemIdx, m_compntIdx, m_wireIdx, m_textElemPos, m_compntPos, m_wirePos, m_movingWires));
        m_canvas->SetStatus(wxString::Format("选择工具：结束拖动"));
        m_toolStateMachine->SetSelectState(SelectToolState::IDLE);
        m_eventHandled = true;
        return;
    }
    // 矩形框选
    else if (m_toolStateMachine->GetSelectState() == SelectToolState::RECTANGLE_SELECT) {
        FinishRectangleSelect();
		m_eventHandled = true;
    }
    // 画布拖动
    else if (m_toolStateMachine->GetDragState() == DragToolState::CANVAS_DRAGGING) {
        FinishPanning();
        m_eventHandled = true;
    }
    
    // 导线绘制前的准备
    else if (m_toolStateMachine->GetWireState() == WireToolState::IDLE && currentTool == ToolType::WIRE_TOOL) {
        if (m_isWireDraingCancel) m_eventHandled = true;
        else {
            StartWireDrawingUp();
            m_eventHandled = true;
        }
    }
    
    // 导线绘制中
    else if (m_toolStateMachine->GetWireState() == WireToolState::WIRE_DRAWING && currentTool == ToolType::WIRE_TOOL) {
        m_eventHandled = true;
    }
    else {
        // 其他工具不处理
	}
}

void CanvasEventHandler::OnCanvasLeftDoubleClick(wxMouseEvent& evt) {
    FinishWireDrawing();
    m_eventHandled = true;
}

void CanvasEventHandler::OnCanvasKeyDown(wxKeyEvent& evt) {
    // Ctrl + z 撤销
    if (evt.ControlDown() && evt.GetKeyCode() == 'z') {
        m_eventHandled = true;
        return;
    }

    // Ctrl + s 保存
    if (evt.ControlDown() && evt.GetKeyCode() == 's') {
        m_eventHandled = true;
        return;
    }

    // Space 开始/终止仿真
    //if () {
    //    m_eventHandled = true;
    //    return;
    //}

	// Ctrl + c 复制画布元素
    if (evt.ControlDown() && evt.GetKeyCode() == 'c') {
        m_eventHandled = true;
        return;
	}

	// Ctrl + v 粘贴画布元素
    if (evt.ControlDown() && evt.GetKeyCode() == 'v') {
        m_eventHandled = true;
        return;
	}

	// Ctrl + x 剪切画布元素
    if (evt.ControlDown() && evt.GetKeyCode() == 'x') {
        m_eventHandled = true;
        return;
    }

    ToolType currentTool = m_toolStateMachine->GetCurrentTool();

    switch (currentTool) {
    case ToolType::DRAG_TOOL: {

    }
    case ToolType::SELECT_TOOL: {

    }
    case ToolType::TEXT_TOOL: {

    }
    case ToolType::WIRE_TOOL: {
        if (m_toolStateMachine->GetWireState() == WireToolState::WIRE_DRAWING) {
            if (evt.GetKeyCode() == WXK_ESCAPE) {
                CancelWireDrawing();
                m_eventHandled = true;
            }
            if (evt.GetKeyCode() == WXK_RETURN) {
                FinishWireDrawing();
                m_eventHandled = true;
            }
        }
    }

    default: {
		return;
    }
    }
}

void CanvasEventHandler::OnCanvasMouseWheel(wxMouseEvent& evt) {
    if (evt.ControlDown()) {
        // 处理缩放
        wxPoint mouseScreenPos = evt.GetPosition();
        wxPoint mouseCanvasPos = m_canvas->ScreenToCanvas(mouseScreenPos);

        float oldScale = m_canvas->GetScale();
        float newScale;

        if (evt.GetWheelRotation() > 0) {
            newScale = oldScale * 1.1f;  // 放大
        }
        else {
            newScale = oldScale / 1.1f;  // 缩小
        }

        m_canvas->SetScale(newScale);

        // 调整偏移量，使鼠标指向的画布位置保持不变
        wxPoint newMouseScreenPos = m_canvas->CanvasToScreen(mouseCanvasPos);
        wxPoint offset = m_canvas->GetoffSet();
        m_canvas->SetoffSet(offset + mouseScreenPos - newMouseScreenPos);

		m_canvas->SetStatus(wxString::Format("缩放画布: %.2f%%", newScale * 100.0f));

        evt.Skip(false);  // 已处理
    }
    else if(evt.ShiftDown() || evt.GetWheelAxis() == wxMOUSE_WHEEL_HORIZONTAL){
        wxPoint offset = m_canvas->GetoffSet();

        wxPoint delta = wxPoint(40, 0);
        if (evt.GetWheelRotation() > 0) {
            m_canvas->SetoffSet(offset - delta);;  // 右移
        }
        else {
            m_canvas->SetoffSet(offset + delta);;  // 左移
        }

    

        m_canvas->Refresh();
        //m_canvas->SetStatus(wxString::Format("平移画布: 偏移(%d, %d)", realDelta.x, 0));
            //m_canvas->SetStatus(wxString::Format("平移画布: 偏移(%d, %d), (%d, %d)", m_panStartPos.x, m_panStartPos.y, currentPos.x, currentPos.y));
    }
    else {

        wxPoint delta = wxPoint(0, 40);
        wxPoint offset = m_canvas->GetoffSet();
        if (evt.GetWheelRotation() > 0) {
            m_canvas->SetoffSet(offset + delta);;  // 上移
        }
        else {
            m_canvas->SetoffSet(offset - delta);;  // 下移
        }

        m_canvas->Refresh();

    }
}

void CanvasEventHandler::HandleSelectTool(wxMouseEvent& evt) {
    preIn = false;
    wxPoint canvasPos = m_hoverInfo.pos;
    wxString status = "选择工具: ";

    int elemIdx = m_hoverInfo.elementIndex;
    int textIdx = m_hoverInfo.textIndex;
    int wireIdx = m_hoverInfo.wireIndex;

    auto AddIfNew = [&](int idx, std::vector<int>& idxList) {
        if (idx != -1) {
            auto it = std::find(idxList.begin(), idxList.end(), idx);
            if (it == idxList.end()) {
                idxList.push_back(idx);
            }
            else return true;
        }
        else return false;
    };
 
    preIn = AddIfNew(elemIdx, m_compntIdx) || preIn;
    preIn = AddIfNew(wireIdx, m_wireIdx) || preIn;
    preIn = AddIfNew(textIdx, m_textElemIdx) || preIn;

    if (!evt.ShiftDown()) {
        m_compntIdx.clear();
        m_textElemIdx.clear();
        m_wireIdx.clear();

        AddIfNew(elemIdx, m_compntIdx);
        AddIfNew(wireIdx, m_wireIdx);
        AddIfNew(textIdx, m_textElemIdx);
    }
    m_canvas->UpdateSelection(m_compntIdx, m_textElemIdx, m_wireIdx);


    // 状态栏更新
    if (m_hoverInfo.IsEmptyArea()) {
        StartRectangleSelect(canvasPos);

        if (evt.ShiftDown()) status = status + wxString::Format("拖动框选，选中未选择元素，取消选中已选择元素，");
        else status = status + wxString::Format("拖动框选，选中框内选择元素，取消选中框外元素，");

    }
    else {
        m_toolStateMachine->SetSelectState(SelectToolState::CLICK_SELECT);
        StartSelectedDragging(canvasPos);

        if (evt.ShiftDown()) status = status + wxString::Format("单击多选，选中未选择元素，取消选中已选择元素；拖动平移被选中元素");
        else status = status + wxString::Format("单击单选，选中被单击的元素，取消选中其他元素；拖动平移被选中元素");
    }
    m_canvas->SetStatus(status);
}

void CanvasEventHandler::HandleTextTool() {
    // 检查是否点击了现有文本元素
    if (m_hoverInfo.IsOverText()) StartTextEditing(m_hoverInfo.textIndex);
    else {
        m_canvas->CreateTextElement(m_hoverInfo.pos);
        m_canvas->SetStatus(wxString::Format("放置文本框：(%d, %d)", m_hoverInfo.pos.x, m_hoverInfo.pos.y));
    }
}

// 添加设置当前元件的方法
void CanvasEventHandler::SetCurrentComponent(const wxString& componentName) {
    m_currentComponent = componentName;
    SetCurrentTool(ToolType::COMPONENT_TOOL);

    if (true) {
        m_canvas->SetStatus(wxString::Format("准备放置: %s - 在画布上点击放置", componentName));
        // 设置十字光标
        m_canvas->SetCursor(wxCursor(wxCURSOR_CROSS));
    }
}


// 导线编辑方法
void CanvasEventHandler::StartWireEditing(int wireIndex, int pointIndex, const wxPoint& startPos) {
    if (wireIndex < 0 || wireIndex >= (int)m_canvas->GetWires().size()) return;
    ToolType currentTool = m_toolStateMachine->GetCurrentTool();

	currentTool = ToolType::WIRE_TOOL;
    //m_isEditingWire = true;
	m_toolStateMachine->SetWireState(WireToolState::WIRE_EDITING);
    m_editingWireIndex = wireIndex;
    m_editingPointIndex = pointIndex;
    m_editStartPos = startPos;

    if (true) {
        m_canvas->SetStatus("编辑导线: 拖动控制点调整路径");
    }
}

void CanvasEventHandler::UpdateWireEditing(const wxPoint& currentPos) {
    if (m_toolStateMachine->GetWireState() != WireToolState::WIRE_EDITING) return;

    //Wire& wire = m_canvas->m_wires[m_editingWireIndex];

    //// 更新控制点位置
    //if (m_editingPointIndex >= 0 && m_editingPointIndex < (int)wire.pts.size()) {
    //    wire.pts[m_editingPointIndex].pos = currentPos;

    //    // 重新生成导线路径（如果是端点）
    //    if (m_editingPointIndex == 0 || m_editingPointIndex == (int)wire.pts.size() - 1) {
    //        wire.pts = Wire::Route(wire.pts.front(), wire.pts.back());
    //    }

    //    // 重新生成控制点
    //    wire.GenerateCells();
    //}

    m_canvas->Refresh();
}

void CanvasEventHandler::FinishWireEditing() {
    //m_isEditingWire = false;
	m_toolStateMachine->SetWireState(WireToolState::IDLE);
    m_editingWireIndex = -1;
    m_editingPointIndex = -1;

    if (m_isTemporaryAction) {
        SetCurrentTool(m_previousTool);
        m_isTemporaryAction = false;
    }

    if (true) {
        m_canvas->SetStatus("导线编辑完成");
    }
}

void CanvasEventHandler::CancelWireEditing() {
    //m_isEditingWire = false;
	m_toolStateMachine->SetWireState(WireToolState::IDLE);
    m_editingWireIndex = -1;
    m_editingPointIndex = -1;

    if (true) {
        m_canvas->SetStatus("导线编辑取消");
    }
}

void CanvasEventHandler::OnCanvasMouseMove(wxMouseEvent& evt) {
    wxPoint canvasPos = m_hoverInfo.pos;

    // 导线绘制
    if (m_toolStateMachine->GetWireState() == WireToolState::WIRE_DRAWING) {
        if (SnapPosChanged()) {
            UpdateWireDrawing(evt);
        }
        m_eventHandled = true;
    }

	// 选中拖动的第一个函数，执行一次后转到第二个函数
    else if (m_toolStateMachine->GetSelectState() == SelectToolState::CLICK_SELECT) {
        UpdateSelectedDragging();
        m_toolStateMachine->SetSelectState(SelectToolState::DRAG_SELECT);
        m_eventHandled = true;
	}
    // 选中拖动的第二个函数
    else if (m_toolStateMachine->GetSelectState() == SelectToolState::DRAG_SELECT) {
        UpdateSelectedDragging();
        m_eventHandled = true;
    }

    // 矩形框选
    else if (m_toolStateMachine->GetSelectState() == SelectToolState::RECTANGLE_SELECT) {
        UpdateRectangleSelect(evt);
        m_eventHandled = true;
    }
    
    // 画布拖动 
    else if (m_toolStateMachine->GetDragState() == DragToolState::CANVAS_DRAGGING) {
        UpdatePanning();
        m_eventHandled = true;
    }

    // 元件放置预览
    else if (m_toolStateMachine->GetComponentState() == ComponentToolState::COMPONENT_PREVIEW) {
        wxPoint snappedPos = m_hoverInfo.snappedPos;
		m_canvas->SetPreviewElement(m_currentComponent, snappedPos);
        m_canvas->SetStatus(wxString::Format("放置%s: (%d, %d)", m_currentComponent, snappedPos.x, snappedPos.y));
        m_eventHandled = true;
	}
	// 其他情况下打印当前工具状态
    else {
        wxString toolInfo;
        switch (m_toolStateMachine->GetCurrentTool()) {
            case ToolType::DRAG_TOOL:{
                toolInfo = wxString::Format("工具: 拖拽工具，长按空白处拖动画布，单击Pin元件可改变状态");
				break;
            }
            case ToolType::SELECT_TOOL: {
                toolInfo = wxString::Format("工具: 选中工具");
                break;
            }
            case ToolType::TEXT_TOOL: {
                toolInfo = wxString::Format("工具: 文本工具");
                break;
            }
            case ToolType::COMPONENT_TOOL: {
                toolInfo = wxString::Format("工具: 元件工具 %s", m_currentComponent);
                break;
            }
            case ToolType::WIRE_TOOL: {
                toolInfo = wxString::Format("工具: 导线工具");
                break;
            }
            case ToolType::DRAWING_TOOL: {
                toolInfo = wxString::Format("工具: 绘图工具");
                break;
            }
        }
        m_canvas->SetStatus(toolInfo);
    }
}

void CanvasEventHandler::StartPanning(const wxPoint& startPos) {
	m_toolStateMachine->SetDragState(DragToolState::CANVAS_DRAGGING);
    m_panStartPos = startPos;
    m_panStartOffSet = m_canvas->GetoffSet();
}

void CanvasEventHandler::UpdatePanning() {
    if (m_toolStateMachine->GetDragState() != DragToolState::CANVAS_DRAGGING) return;
    wxPoint delta = m_hoverInfo.pos - m_panStartPos;
    m_canvas->SetoffSet(m_panStartOffSet + delta);

    m_canvas->SetStatus(wxString::Format("平移画布: 偏移(%d, %d)", delta.x, delta.y));
}

void CanvasEventHandler::FinishPanning() {
    m_toolStateMachine->SetDragState(DragToolState::IDLE);
    if (m_isTemporaryAction) {
        SetCurrentTool(m_previousTool);
        m_isTemporaryAction = false;
    }
}

// 修改StartTextEditing方法
void CanvasEventHandler::StartTextEditing(int index) {
    // 结束之前的编辑
    if (m_editingTextIndex != -1) {
        m_canvas->FinishTextEditing();
    }

    m_toolStateMachine->SetTextState(TextToolState::CANVAS_TEXT_EDITING);

    // 开始新的编辑
    m_editingTextIndex = index;

    if (m_canvas) {
        m_canvas->AttachHiddenTextCtrlToElement(index);
    }


     m_canvas->SetStatus("文本编辑: 在文本框中输入内容，按回车完成");

}

void CanvasEventHandler::StartSelectedDragging(const wxPoint& startPos) {
    m_selectedDragPos = startPos;
    m_compntPos.clear();
    m_textElemPos.clear();
    m_wirePos.clear();

    std::vector<wxPoint> points;
    m_movingWires.clear();

    const std::vector<CanvasElement> elements = m_canvas->GetElements();
	const std::vector<CanvasTextElement> textElements = m_canvas->GetTextElements();
    const std::vector<Wire> wires = m_canvas->GetWires();

    for (int i = 0; i < m_compntIdx.size(); i++) {
        StartElementDragging(i);
        m_compntPos.push_back(elements[m_compntIdx[i]].GetPos());
    }
    for (int i = 0; i < m_textElemIdx.size(); i++) {
        m_textElemPos.push_back(textElements[m_textElemIdx[i]].GetPosition());
    }
    for (int i = 0; i < m_wireIdx.size(); i++) {
        points.clear();
        for (int j = 0; j < wires[m_wireIdx[i]].Size(); j++) {
            points.push_back(wires[m_wireIdx[i]].pts[j].pos);
        }
        m_wirePos.push_back(points);
    }
}

void CanvasEventHandler::StartElementDragging(int i) {
    if (m_compntIdx[i] < 0 || m_compntIdx[i] >= (int)m_canvas->GetElements().size()) return;

    // 收集该元件所有引脚对应的导线端点
    const auto& elem = m_canvas->GetElements()[m_compntIdx[i]];

    std::vector<WireAnchor> tmp;
    auto collect = [&](const auto& pins, bool isIn) {
        for (size_t p = 0; p < pins.size(); ++p) {
            wxPoint pinWorld = elem.GetPos() + wxPoint(pins[p].pos.x, pins[p].pos.y);
            for (size_t w = 0; w < m_canvas->GetWires().size(); ++w) {
                const auto& wire = m_canvas->GetWires()[w];
                if (!wire.pts.empty() && wire.pts.front().type == CPType::Pin &&
                    wire.pts.front().pos == pinWorld)
                    tmp.push_back({w, 0, isIn, p});

                if (wire.pts.size() > 1 &&
                    wire.pts.back().type == CPType::Pin &&
                    wire.pts.back().pos == pinWorld)
                    tmp.push_back({ w, wire.pts.size() - 1, isIn, p });
            }
        }
    };
    collect(elem.GetInputPins(), true);
    collect(elem.GetOutputPins(), false);
	m_movingWires.push_back(tmp);
}

void CanvasEventHandler::UpdateSelectedDragging() {
    int grid = m_canvas->GetGrid();

    wxPoint raw = m_hoverInfo.pos - m_selectedDragPos;
    wxPoint delta((raw.x + grid / 2) / grid * grid, (raw.y + grid / 2) / grid * grid);

    for (int i = 0; i < m_compntIdx.size(); i++) {
        m_canvas->ElementSetPos(m_compntIdx[i], m_compntPos[i] + delta);

        for (const auto& aw : m_movingWires[i]) {
            if (aw.wireIdx >= m_canvas->GetWires().size()) continue;
            auto it = std::find(m_wireIdx.begin(), m_wireIdx.end(), aw.wireIdx);
            const Wire& wire = m_canvas->GetWires()[aw.wireIdx];

            // 计算新引脚世界坐标
            const auto& elem = m_canvas->GetElements()[m_compntIdx[i]];
            const auto& pins = aw.isInput ? elem.GetInputPins() : elem.GetOutputPins();
            if (aw.pinIdx >= pins.size()) continue;
            wxPoint pinOffset = wxPoint(pins[aw.pinIdx].pos.x, pins[aw.pinIdx].pos.y);
            wxPoint newPinPos = elem.GetPos() + pinOffset;

            Wire tmp_wire = wire;
            // 更新导线端点
            if (aw.ptIdx == 0) {
                tmp_wire.pts.front().pos = newPinPos;
                tmp_wire.pts[1].pos.y = newPinPos.y;
            }  
            else{
                tmp_wire.pts.back().pos = newPinPos;
                tmp_wire.pts[tmp_wire.pts.size() - 2].pos.y = newPinPos.y;
            }
            m_canvas->UpdateWire(tmp_wire, aw.wireIdx);
        }
    }
    for (int i = 0; i < m_textElemIdx.size(); i++) {
        m_canvas->TextSetPos(m_textElemIdx[i], m_textElemPos[i] + delta);

    }
    for (int i = 0; i < m_wireIdx.size(); i++) {
        for (int j = 0; j < m_canvas->GetWires()[m_wireIdx[i]].Size(); j++) {
            wxPoint rawPos = m_wirePos[i][j];
            m_canvas->WirePtsSetPos(m_wireIdx[i], j, m_wirePos[i][j] + delta);

        }
    }
    m_canvas->SetStatus(wxString::Format("选择工具：动已选中元素(%d, %d)", delta.x, delta.y));
}


void CanvasEventHandler::UpdateHoverInfo(HoverInfo hf) {
    m_snapPosChanged = m_hoverInfo.snappedPos != hf.snappedPos ? true : false;

	m_hoverInfo.pos = hf.pos;
    m_hoverInfo.snappedPos = hf.snappedPos;
	m_hoverInfo.pinIndex = hf.pinIndex;
	m_hoverInfo.isInputPin = hf.isInputPin;
	m_hoverInfo.pinWorldPos = hf.pinWorldPos;

	m_hoverInfo.cellIndex = hf.cellIndex;
	m_hoverInfo.wireIndex = hf.wireIndex;
	m_hoverInfo.cellWorldPos = hf.cellWorldPos;
	
	m_hoverInfo.elementIndex = hf.elementIndex;
	m_hoverInfo.elementName = hf.elementName;

	m_hoverInfo.textIndex = hf.textIndex;
}



void CanvasEventHandler::StartRectangleSelect(const wxPoint& startPos){
    m_toolStateMachine->SetSelectState(SelectToolState::RECTANGLE_SELECT);
    m_selectStartPos = startPos;
    m_textRecElemIdx.clear();
    m_compntRecIdx.clear();
    m_wireRecIdx.clear();
}

void CanvasEventHandler::UpdateRectangleSelect(wxMouseEvent& evt) {
    wxPoint canvasPos = m_hoverInfo.pos;
    wxRect selectionRect(
        wxPoint(std::min(m_selectStartPos.x, canvasPos.x),
            std::min(m_selectStartPos.y, canvasPos.y)),
        wxSize(std::abs(canvasPos.x - m_selectStartPos.x),
            std::abs(canvasPos.y - m_selectStartPos.y))
    );
    m_canvas->SetSelectionRect(selectionRect);

    auto searchSelectedElements0 = [&](std::vector<int>& indexList, std::vector<int>& recList, const auto& elements) {
        for (size_t i = 0; i < elements.size(); ++i) {
            const auto& elem = elements[i];
            wxRect elemRect = elem.GetBounds();
            if (m_canvas->GetSelectionRect().Contains(elemRect)) {
                auto it0 = std::find(recList.begin(), recList.end(), i);
                if (it0 == recList.end()) {
                    recList.push_back(i);
                    auto it = std::find(indexList.begin(), indexList.end(), i);
                    if (it == indexList.end()) indexList.push_back(i);
                    else indexList.erase(it);
                }
            }
            else {
                auto it0 = std::find(recList.begin(), recList.end(), i);
                if (it0 != recList.end()) {
                    recList.erase(it0);
                    auto it = std::find(indexList.begin(), indexList.end(), i);
                    if (it != indexList.end()) indexList.erase(it);
                    else indexList.push_back(i);
                }
            }
        }
        };

    auto searchSelectedElements1 = [&](std::vector<int>& indexList, const auto& elements) {
        for (size_t i = 0; i < elements.size(); ++i) {
            const auto& elem = elements[i];
            wxRect elemRect = elem.GetBounds();
            if (m_canvas->GetSelectionRect().Contains(elemRect)) {
                auto it = std::find(indexList.begin(), indexList.end(), i);
                if (it == indexList.end()) indexList.push_back(i);
                else indexList.erase(it);
            }
        }
        };

    if (! evt.ShiftDown()) {
        m_compntIdx.clear();
        m_textElemIdx.clear();
        m_wireIdx.clear();
        searchSelectedElements1(m_compntIdx, m_canvas->GetElements());
        searchSelectedElements1(m_textElemIdx, m_canvas->GetTextElements());
        searchSelectedElements1(m_wireIdx, m_canvas->GetWires());
    }
    else {
        searchSelectedElements0(m_compntIdx, m_compntRecIdx, m_canvas->GetElements());
        searchSelectedElements0(m_textElemIdx, m_textRecElemIdx, m_canvas->GetTextElements());
        searchSelectedElements0(m_wireIdx, m_wireRecIdx, m_canvas->GetWires());
    }
    m_canvas->UpdateSelection(m_compntIdx, m_textElemIdx, m_wireIdx);
}

void CanvasEventHandler::FinishRectangleSelect() {
    m_toolStateMachine->SetSelectState(SelectToolState::IDLE);
    m_canvas->ClearSelectionRect();
}

void CanvasEventHandler::FinishClickSelect() {
    if (evt.ShiftDown()) {
        m_canvas->SetStatus(wxString::Format("选择工具：点击继续选择"));
        if (preIn) {
            auto AddOrRemove = [](std::vector<int>& vec, int idx) {
                if (idx != -1) {
                    auto it = std::find(vec.begin(), vec.end(), idx);
                    if (it == vec.end()) vec.push_back(idx);
                    else vec.erase(it);
                }
                };

            AddOrRemove(m_compntIdx, m_hoverInfo.elementIndex);
            AddOrRemove(m_wireIdx, m_hoverInfo.wireIndex);
            AddOrRemove(m_textElemIdx, m_hoverInfo.textIndex);
            m_canvas->UpdateSelection(m_compntIdx, m_textElemIdx, m_wireIdx);
        }
    }
    else {
        m_canvas->SetStatus(wxString::Format("选择工具：结束选择"));
        m_compntIdx.clear();
        m_textElemIdx.clear();
        m_wireIdx.clear();
        m_canvas->Refresh();
        if (!preIn) {
            auto AddOrRemove = [](std::vector<int>& vec, int idx) {
                if (idx != -1) {
                    auto it = std::find(vec.begin(), vec.end(), idx);
                    if (it == vec.end()) vec.push_back(idx);
                    else vec.erase(it);
                }
                };

            AddOrRemove(m_compntIdx, m_hoverInfo.elementIndex);
            AddOrRemove(m_wireIdx, m_hoverInfo.wireIndex);
            AddOrRemove(m_textElemIdx, m_hoverInfo.textIndex);
            m_canvas->UpdateSelection(m_compntIdx, m_textElemIdx, m_wireIdx);
        }
    }
    m_toolStateMachine->SetSelectState(SelectToolState::IDLE);
}