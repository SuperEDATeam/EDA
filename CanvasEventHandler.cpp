#include "CanvasEventHandler.h"
#include "MainFrame.h"
#include "Wire.h"
#include <wx/image.h>
#include "HandyToolKit.h"

CanvasEventHandler::CanvasEventHandler(CanvasPanel* canvas, ToolStateMachine* toolstate)
    : m_canvas(canvas), m_toolStateMachine(toolstate), m_isTemporaryAction(false), m_eventHandled(false),
    m_editingWireIndex(-1), m_editingPointIndex(-1), m_draggingElementIndex(-1) {
}

void CanvasEventHandler::SetCurrentTool(ToolType tool) {
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

void CanvasEventHandler::OnCanvasLeftDown(const wxPoint& canvasPos) {
    m_eventHandled = false;
    ToolType currentTool = m_toolStateMachine->GetCurrentTool();
    wxLogDebug("CanvasEventHandler::OnCanvasLeftDown at (%d, %d) with tool %d",
		canvasPos.x, canvasPos.y, static_cast<int>(currentTool));
    // 1. 最高优先级：检查是否点击了导线控制点
    int cellWire, cellIdx;
    wxPoint cellPos;
    int newCell = m_canvas->HitHoverCell(canvasPos, &cellWire, &cellIdx, &cellPos);
    if (newCell != -1) {
		m_previousTool = currentTool;
		m_isTemporaryAction = true;
		SetCurrentTool(ToolType::WIRE_TOOL);
        StartWireEditing(cellWire, cellIdx, canvasPos);
        m_eventHandled = true;
        return;
    }

     //2. 检查是否点击了引脚（开始绘制导线）
    bool snapped = false;
    wxPoint snappedPos = m_canvas->Snap(canvasPos, &snapped);
    if (snapped) {
        m_previousTool = currentTool;
        m_isTemporaryAction = true;
		SetCurrentTool(ToolType::WIRE_TOOL);
        StartWireDrawing(snappedPos, true);
        m_eventHandled = true;
        return;
    }

    // 3. 检查是否点击了元件
    int elementIndex = m_canvas->HitTestPublic(canvasPos);
    if (elementIndex != -1) {
        // 在选择工具下点击元件：选择但不拖动
        if (currentTool == ToolType::SELECT_TOOL) {
            m_canvas->SetSelectedIndex(elementIndex);
            m_eventHandled = true;
            return;
        }
        else {
            m_previousTool = currentTool;
            m_isTemporaryAction = true;
			SetCurrentTool(ToolType::DRAG_TOOL);
            StartElementDragging(elementIndex, canvasPos);
            m_eventHandled = true;
			return;
        }
    }

    // 4. 按工具类型处理其他情况
    switch (currentTool) {
    case ToolType::SELECT_TOOL: {
        // 选择工具下点击空白区域：清除选择
        if (m_canvas->IsClickOnEmptyAreaPublic(canvasPos)) {
            m_canvas->ClearSelection();
            m_eventHandled = true;
        }
        break;
    }
    case ToolType::TEXT_TOOL: {
        HandleTextTool(canvasPos);
        m_eventHandled = true;
        break;
    }
    case ToolType::COMPONENT_TOOL: {
        HandleComponentTool(canvasPos);
        m_eventHandled = true;
        break;
    }
    case ToolType::WIRE_TOOL:{
        bool snapped = false;
        wxPoint snappedPos = m_canvas->Snap(canvasPos, &snapped);
		StartWireDrawing(snappedPos, snapped);
        m_eventHandled = true;
        break;
        }
    case ToolType::DRAG_TOOL: {
        HandleDragTool(m_canvas->CanvasToScreen(canvasPos));
        wxLogDebug("拖拽工具常用");
        m_eventHandled = true;
        break;
        }
    }
}

void CanvasEventHandler::HandleWireTool(const wxPoint& canvasPos) {
    // 专门的导线工具：可以从任意点开始绘制导线
    bool snapped = false;
    wxPoint snappedPos = m_canvas->Snap(canvasPos, &snapped);
	ToolType currentTool = m_toolStateMachine->GetCurrentTool();

    //if (!m_isDrawingWire) {
    if (m_toolStateMachine->GetWireState() != WireToolState::WIRE_DRAWING) {
        // 开始绘制导线（可以从任意点开始，不一定是引脚）
        StartWireDrawing(snappedPos, snapped);
    }
    else {
        // 完成导线绘制
        FinishWireDrawing(snappedPos);
		SetCurrentTool(ToolType::WIRE_TOOL);

        // 导线工具模式下，绘制完成后可以立即开始新的导线
        // 这提供了连续绘制的体验
        if (currentTool == ToolType::WIRE_TOOL) {
            // 可以选择是否自动开始新的导线
            // 如果希望连续绘制，取消下面这行的注释
            // StartWireDrawing(snappedPos, snapped);
        }
    }
}

void CanvasEventHandler::HandleDragTool(const wxPoint& canvasPos) {
    // 专门的平移工具
    StartPanning(canvasPos);
    m_eventHandled = true;
}

void CanvasEventHandler::StartWireDrawing(const wxPoint& startPos, bool fromPin) {
    // 如果已经在进行其他操作，先取消
    //if (m_isDraggingElement) {
    if (m_toolStateMachine->GetComponentState() == ComponentToolState::COMPONENT_PREVIEW) {
        FinishElementDragging();
    }
    //if (m_isPanning) {
    if (m_toolStateMachine->GetDragState() == DragToolState::CANVAS_DRAGGING) {
        FinishPanning(startPos);
    }
    //m_isDrawingWire = true;
	m_toolStateMachine->SetWireState(WireToolState::WIRE_DRAWING);
    m_wireStartPos = startPos;
    m_startCP = { startPos, fromPin ? CPType::Pin : CPType::Free };

    // 初始化临时导线
    m_canvas->m_tempWire.Clear();
    m_canvas->m_tempWire.AddPoint(m_startCP);
    m_canvas->m_wireMode = CanvasPanel::WireMode::DragNew;

    if (true) {
        if (fromPin) {
            m_canvas->SetStatus("绘制导线: 从引脚开始，点击终点完成绘制 (ESC取消)");
        }
        else {
            m_canvas->SetStatus("绘制导线: 从自由点开始，点击终点完成绘制 (ESC取消)");
        }
    }
}

void CanvasEventHandler::UpdateWireDrawing(const wxPoint& currentPos) {
    //if (!m_isDrawingWire) return;
	if (m_toolStateMachine->GetWireState() != WireToolState::WIRE_DRAWING) return;

    bool snapped = false;
    wxPoint snappedPos = m_canvas->Snap(currentPos, &snapped);
	ControlPoint endCP = { snappedPos, snapped ? CPType::Pin : CPType::Free };

    // 更新导线预览
    m_canvas->m_tempWire.pts = Wire::RouteOrtho(m_startCP, endCP, PinDirection::Right, PinDirection::Left);
    m_canvas->Refresh();

    if (true) {
        if (snapped) {
            m_canvas->SetStatus(wxString::Format("绘制导线: 终点吸附到引脚 (%d,%d)",
                snappedPos.x, snappedPos.y));
        }
        else {
            m_canvas->SetStatus(wxString::Format("绘制导线: 终点为自由端点 (%d,%d)",
                snappedPos.x, snappedPos.y));
        }
    }
}

void CanvasEventHandler::FinishPanning(const wxPoint& currentPos) {
    //m_isPanning = false;
	m_toolStateMachine->SetDragState(DragToolState::IDLE);
    m_panStartPos = currentPos;
    m_fakeStartPos = currentPos;
    if (m_isTemporaryAction) {
        SetCurrentTool(m_previousTool);
        m_isTemporaryAction = false;
	}
}

void CanvasEventHandler::HandleComponentTool(const wxPoint& canvasPos) {
    if (!m_currentComponent.IsEmpty() && m_canvas) {
        // 放置元件
		bool snapped = false;
		wxPoint snappedPos = m_canvas->Snap(canvasPos, &snapped);
        m_canvas->PlaceElement(m_currentComponent, snappedPos);

        if (true) {
            m_canvas->SetStatus(wxString::Format("已放置: %s， 吸附到 (%d, %d)", m_currentComponent, snappedPos.x, snappedPos.y));
        }
    }
    else {
        if (true) {
            m_canvas->SetStatus("请先从元件库选择一个元件");
        }
    }
}

void CanvasEventHandler::OnCanvasLeftUp(const wxPoint& canvasPos) {
    //if (m_isEditingWire) {
	ToolType currentTool = m_toolStateMachine->GetCurrentTool();

    if (m_toolStateMachine->GetDragState() == DragToolState::COMPONENT_DRAGGING) {
        FinishElementDragging();
        m_eventHandled = true;
    }
    //else if (m_isPanning) {
    else if (m_toolStateMachine->GetDragState() == DragToolState::CANVAS_DRAGGING) {
        //FinishPanning(canvasPos);
        FinishPanning(m_canvas->CanvasToScreen(canvasPos));
        m_eventHandled = true;
    }
    //else if (m_isDrawingWire && currentTool == ToolType::WIRE_TOOL) {
    else if (m_toolStateMachine->GetWireState() == WireToolState::WIRE_DRAWING && currentTool == ToolType::WIRE_TOOL) {
        // 导线工具模式下，点击完成导线
        FinishWireDrawing(canvasPos);
        m_eventHandled = true;
    }
    else {
        // 其他工具不处理
	}
}

void CanvasEventHandler::OnCanvasKeyDown(wxKeyEvent& evt) {
    if (evt.GetKeyCode() == WXK_ESCAPE) {
        if (m_toolStateMachine->GetWireState() == WireToolState::WIRE_DRAWING) {
            CancelWireDrawing();
        }
        else if (m_toolStateMachine->GetComponentState() == ComponentToolState::COMPONENT_PREVIEW) {
            // 取消元件拖动，恢复原位
            if (m_draggingElementIndex != -1) {
                m_canvas->m_elements[m_draggingElementIndex].SetPos(m_elementStartCanvasPos);
                m_canvas->Refresh();
            }
            FinishElementDragging();
        }
        else {
            SetCurrentTool(ToolType::DRAG_TOOL);
        }
        evt.Skip(false); // 已处理
    }
    else {
        evt.Skip(); // 其他按键继续传递
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
            newScale = oldScale * 1.2f;  // 放大
        }
        else {
            newScale = oldScale / 1.2f;  // 缩小
        }

        m_canvas->SetScale(newScale);

        // 调整偏移量，使鼠标指向的画布位置保持不变
        wxPoint newMouseScreenPos = m_canvas->CanvasToScreen(mouseCanvasPos);
        m_canvas->m_offset += mouseScreenPos - newMouseScreenPos;

		m_canvas->SetStatus(wxString::Format("缩放画布: %.2f%%", newScale * 100.0f));

        evt.Skip(false);  // 已处理
    }
    else {
        evt.Skip();  // 非Ctrl滚轮继续传递
    }
}

void CanvasEventHandler::HandleSelectTool(const wxPoint& canvasPos) {
    if (m_canvas->IsClickOnEmptyAreaPublic(canvasPos)) {
        m_canvas->ClearSelection();
        if (true) {
            m_canvas->SetStatus("选中工具: 清除选择");
        }
    }
    else {
        int selectedIndex = m_canvas->HitTestPublic(canvasPos);
        if (selectedIndex != -1) {
            m_canvas->SetSelectedIndex(selectedIndex);
            if (true) {
                m_canvas->SetStatus(wxString::Format("选中工具: 选择元件 %d", selectedIndex));
            }
        }
    }
}

void CanvasEventHandler::HandleTextTool(const wxPoint& canvasPos) {
    // 文本工具行为：在点击位置创建文本元素
    // 注意：这里暂时注释掉文本输入对话框，等工具稳定后再实现
    /*
    wxString text = wxGetTextFromUser("请输入文本:", "添加文本", "", true);
    if (!text.IsEmpty()) {
        if (true) {
            m_canvas->SetStatus(wxString::Format("文本工具: 添加文本 '%s'", text.ToUTF8().data()));
        }
    }
    */

    if (true) {
        m_canvas->SetStatus("文本工具: 点击添加文本");
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

// 改进的导线绘制完成方法
void CanvasEventHandler::FinishWireDrawing(const wxPoint& endPos) {
    if (m_toolStateMachine->GetWireState() != WireToolState::WIRE_DRAWING) return;

    bool snappedEnd = false;
    wxPoint endSnapPos = m_canvas->Snap(endPos, &snappedEnd);

    // 创建终点控制点
    ControlPoint end{ endSnapPos, snappedEnd ? CPType::Pin : CPType::Free };

    // 使用固定的起点创建完整导线
    Wire completedWire;
    completedWire.AddPoint(m_startCP);
    completedWire.AddPoint(end);

    // 使用路由算法生成最终路径
    completedWire.pts = Wire::RouteOrtho(m_startCP, end, PinDirection::Right, PinDirection::Left);

    // 添加到导线列表
    m_canvas->m_wires.emplace_back(completedWire);
    Wire& newWire = m_canvas->m_wires.back();
    newWire.GenerateCells();

    // 记录连接关系（如果连接到引脚）
    auto recordConnection = [&](const wxPoint& pinPos, size_t ptIdx) {
        for (size_t i = 0; i < m_canvas->m_elements.size(); ++i) {
            const auto& elem = m_canvas->m_elements[i];
            auto test = [&](const auto& pins, bool isIn) {
                for (size_t p = 0; p < pins.size(); ++p) {
                    wxPoint w = elem.GetPos() + wxPoint(pins[p].pos.x, pins[p].pos.y);
                    if (w == pinPos) {
                        m_canvas->m_movingWires.push_back({ m_canvas->m_wires.size() - 1, ptIdx, isIn, p });
                        return true;
                    }
                }
                return false;
                };
            if (test(elem.GetInputPins(), true)) return;
            test(elem.GetOutputPins(), false);
        }
        };

    // 记录起点连接
    if (newWire.pts.front().type == CPType::Pin)
        recordConnection(newWire.pts.front().pos, 0);

    // 记录终点连接
    if (newWire.pts.back().type == CPType::Pin)
        recordConnection(newWire.pts.back().pos, newWire.pts.size() - 1);

    // 重置状态
    //m_isDrawingWire = false;
	m_toolStateMachine->SetWireState(WireToolState::IDLE);
    m_canvas->m_wireMode = CanvasPanel::WireMode::Idle;
    m_canvas->m_tempWire.Clear();
    if (m_isTemporaryAction) {
        SetCurrentTool(m_previousTool);
        m_isTemporaryAction = false;
	}

    if (true) {
        if (snappedEnd) {
            m_canvas->SetStatus("导线绘制完成: 连接到引脚");
        }
        else {
            m_canvas->SetStatus(wxString::Format("导线绘制完成: 自由端点(%d, %d)", endSnapPos.x, endSnapPos.y));
        }
    }


    m_canvas->Refresh();
}

void CanvasEventHandler::CancelWireDrawing() {
    //m_isDrawingWire = false;
	m_toolStateMachine->SetWireState(WireToolState::IDLE);
    m_canvas->m_wireMode = CanvasPanel::WireMode::Idle;
    m_canvas->m_tempWire.Clear();
    m_canvas->Refresh();

    if (true) {
        m_canvas->SetStatus("导线绘制取消");
    }
}
// 导线编辑方法
void CanvasEventHandler::StartWireEditing(int wireIndex, int pointIndex, const wxPoint& startPos) {
    if (wireIndex < 0 || wireIndex >= (int)m_canvas->m_wires.size()) return;
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

    Wire& wire = m_canvas->m_wires[m_editingWireIndex];

    // 更新控制点位置
    if (m_editingPointIndex >= 0 && m_editingPointIndex < (int)wire.pts.size()) {
        wire.pts[m_editingPointIndex].pos = currentPos;

        // 重新生成导线路径（如果是端点）
        if (m_editingPointIndex == 0 || m_editingPointIndex == (int)wire.pts.size() - 1) {
            wire.pts = Wire::RouteOrtho(wire.pts.front(), wire.pts.back(), PinDirection::Right, PinDirection::Left);
        }

        // 重新生成控制点
        wire.GenerateCells();
    }

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

// 元件拖动方法
void CanvasEventHandler::StartElementDragging(int elementIndex, const wxPoint& startPos) {
    if (elementIndex < 0 || elementIndex >= (int)m_canvas->m_elements.size()) return;

    //m_isDraggingElement = true;
	m_toolStateMachine->SetDragState(DragToolState::COMPONENT_DRAGGING);
    m_draggingElementIndex = elementIndex;
    m_elementDragStartPos = startPos;
    m_elementStartCanvasPos = m_canvas->m_elements[elementIndex].GetPos();

    // 收集该元件所有引脚对应的导线端点
    m_canvas->m_movingWires.clear();
    const auto& elem = m_canvas->m_elements[elementIndex];
    auto collect = [&](const auto& pins, bool isIn) {
        for (size_t p = 0; p < pins.size(); ++p) {
            wxPoint pinWorld = elem.GetPos() + wxPoint(pins[p].pos.x, pins[p].pos.y);
            for (size_t w = 0; w < m_canvas->m_wires.size(); ++w) {
                const auto& wire = m_canvas->m_wires[w];
                if (!wire.pts.empty() && wire.pts.front().type == CPType::Pin &&
                    wire.pts.front().pos == pinWorld)
                    m_canvas->m_movingWires.push_back({ w, 0, isIn, p });

                if (wire.pts.size() > 1 &&
                    wire.pts.back().type == CPType::Pin &&
                    wire.pts.back().pos == pinWorld)
                    m_canvas->m_movingWires.push_back({ w, wire.pts.size() - 1, isIn, p });
            }
        }
        };
    collect(elem.GetInputPins(), true);
    collect(elem.GetOutputPins(), false);

    if (true) {
        m_canvas->SetStatus("拖动元件: 移动鼠标调整位置 (ESC取消)");
    }
}

void CanvasEventHandler::UpdateElementDragging(const wxPoint& currentPos) {
    if ((m_toolStateMachine->GetDragState() != DragToolState::COMPONENT_DRAGGING) || m_draggingElementIndex == -1) return;

    // 计算偏移量
    wxPoint raw = currentPos - m_elementDragStartPos;
    const int grid = 20;
    wxPoint delta((raw.x + grid / 2) / grid * grid, (raw.y + grid / 2) / grid * grid);
    
    wxPoint newPos = m_elementStartCanvasPos + delta;

    // 构建调试信息字符串
    wxString debugInfo = wxString::Format("拖动元件: (%d,%d)    关联导线[",
        newPos.x, newPos.y);

    // 更新元件位置
    m_canvas->m_elements[m_draggingElementIndex].SetPos(newPos);

    // 更新所有相关导线端点
    bool firstWire = true;
    for (const auto& aw : m_canvas->m_movingWires) {
        if (aw.wireIdx >= m_canvas->m_wires.size()) continue;

        Wire& wire = m_canvas->m_wires[aw.wireIdx];

        // 计算新引脚世界坐标
        const auto& elem = m_canvas->m_elements[m_draggingElementIndex];
        const auto& pins = aw.isInput ? elem.GetInputPins() : elem.GetOutputPins();
        if (aw.pinIdx >= pins.size()) continue;

        wxPoint pinOffset = wxPoint(pins[aw.pinIdx].pos.x, pins[aw.pinIdx].pos.y);
        wxPoint newPinPos = elem.GetPos() + pinOffset;

        // 添加详细导线信息到调试信息 - 分开构建避免格式化问题
        if (!firstWire) {
            debugInfo += ", ";
        }

        // 分开构建字符串，避免复杂的格式化
        wxString wireType = aw.isInput ? wxString("输入") : wxString("输出");
        wxString wireInfo = wxString::Format("导线%d-%s引脚%d(%d,%d)",
            (int)aw.wireIdx,
            wireType,
            (int)aw.pinIdx,
            newPinPos.x, newPinPos.y);
        debugInfo += wireInfo;
        firstWire = false;

        // 更新导线端点
        if (aw.ptIdx == 0)
            wire.pts.front().pos = newPinPos;
        else
            wire.pts.back().pos = newPinPos;

        // 重新生成导线路径
        wire.pts = Wire::RouteOrtho(wire.pts.front(), wire.pts.back(), PinDirection::Right, PinDirection::Left);
        wire.GenerateCells();
    }

	debugInfo += "]";
    // 更新状态栏显示调试信息
    if (true) {
        m_canvas->SetStatus(debugInfo);
    }

    m_canvas->Refresh();
}

void CanvasEventHandler::FinishElementDragging() {
    //m_isDraggingElement = false;
	m_toolStateMachine->SetDragState(DragToolState::IDLE);
    m_draggingElementIndex = -1;
    m_canvas->m_movingWires.clear();

    if (m_isTemporaryAction) {
        SetCurrentTool(m_previousTool);
        m_isTemporaryAction = false;
	}
}

void CanvasEventHandler::OnCanvasMouseMove(const wxPoint& canvasPos) {
    m_eventHandled = false;

    // 按优先级处理各种操作
    if (m_toolStateMachine->GetWireState() == WireToolState::WIRE_DRAWING) {
        UpdateWireDrawing(canvasPos);
        m_eventHandled = true;
    }
    else if (m_toolStateMachine->GetDragState() == DragToolState::COMPONENT_DRAGGING) {
        UpdateElementDragging(canvasPos);
        m_eventHandled = true;
    }
    else if (m_toolStateMachine->GetDragState() == DragToolState::CANVAS_DRAGGING) {
        //UpdatePanning(canvasPos);
        UpdatePanning(m_canvas->CanvasToScreen(canvasPos));
        m_eventHandled = true;
    }
    else {
        wxString toolInfo;
        switch (m_toolStateMachine->GetCurrentTool()) {
            case ToolType::DRAG_TOOL:{
                toolInfo = wxString::Format("工具: 拖拽工具");
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
    // 如果已经在进行其他操作，先取消
    if (m_toolStateMachine->GetWireState() == WireToolState::WIRE_DRAWING) {
        CancelWireDrawing();
    }
    if (m_toolStateMachine->GetComponentState() == ComponentToolState::COMPONENT_PREVIEW) {
        FinishElementDragging();
    }
    if (m_toolStateMachine->GetWireState() == WireToolState::WIRE_DRAWING) {
        CancelWireEditing();
    }

    //m_isPanning = true;
	m_toolStateMachine->SetDragState(DragToolState::CANVAS_DRAGGING);
    m_panStartPos = startPos;
	m_fakeStartPos = startPos;
}

void CanvasEventHandler::UpdatePanning(const wxPoint& currentPos) {
    if (m_toolStateMachine->GetDragState() != DragToolState::CANVAS_DRAGGING) return;

    wxPoint delta = currentPos - m_fakeStartPos;
    m_canvas->m_offset += delta;
    m_fakeStartPos = currentPos;

	wxPoint realDelta = currentPos - m_panStartPos;
    
    m_canvas->Refresh();

    if (true) {
        m_canvas->SetStatus(wxString::Format("平移画布: 偏移(%d, %d)", realDelta.x, realDelta.y));
        //m_canvas->SetStatus(wxString::Format("平移画布: 偏移(%d, %d), (%d, %d)", m_panStartPos.x, m_panStartPos.y, currentPos.x, currentPos.y));
    }
}

void CanvasEventHandler::OnCanvasRightDown(const wxPoint& canvasPos){
    wxPoint toolBarPos = canvasPos + wxPoint(240, 124);
	m_canvas->m_HandyToolKit->SetPosition(toolBarPos);
	m_canvas->m_HandyToolKit->Show();
	m_canvas->m_HandyToolKit->SetFocus();
}

void CanvasEventHandler::OnCanvasRightUp(const wxPoint& canvasPos) {
    m_canvas->m_HandyToolKit->Hide();
}