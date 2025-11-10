#include "ToolManager.h"
#include "MainFrame.h"
#include "Wire.h"
#include <wx/image.h>

ToolManager::ToolManager(MainFrame* mainFrame, ToolBars* toolBars, CanvasPanel* canvas)
    : m_mainFrame(mainFrame), m_toolBars(toolBars), m_canvas(canvas),
    m_currentTool(ToolType::DEFAULT_TOOL), m_eventHandled(false),
    m_isDrawingWire(false), m_isPanning(false),
    m_isEditingWire(false), m_editingWireIndex(-1), m_editingPointIndex(-1),
    m_isDraggingElement(false), m_draggingElementIndex(-1),
	m_previousTool(ToolType::DEFAULT_TOOL), m_tempTool(false) {
}

void ToolManager::SetCurrentTool(ToolType tool) {
    // 切换工具前清理当前操作状态
    //if (m_isDrawingWire) {
    //    CancelWireDrawing();
    //}
    //if (m_isEditingWire) {
    //    CancelWireEditing();
    //}
    //if (m_isDraggingElement) {
    //    FinishElementDragging();
    //}
    //if (m_isPanning) {
    //    FinishPanning(canvasPos);
    //}

	m_previousTool = m_currentTool;
    m_currentTool = tool;

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
    if (m_mainFrame) {
        wxString toolName;
        switch (tool) {
        case ToolType::DEFAULT_TOOL:
            toolName = "默认工具 (导线绘制/元件拖动/画布平移)";
            break;
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
        }
        m_mainFrame->SetStatusText(wxString::Format("当前工具: %s", toolName));
    }
}

void ToolManager::OnCanvasLeftDown(const wxPoint& canvasPos) {
    m_eventHandled = false;

    // 1. 最高优先级：检查是否点击了导线控制点
    int cellWire, cellIdx;
    wxPoint cellPos;
    int newCell = m_canvas->HitHoverCell(canvasPos, &cellWire, &cellIdx, &cellPos);
    if (newCell != -1) {
		m_tempTool = true;
		SetCurrentTool(ToolType::WIRE_TOOL);
        StartWireEditing(cellWire, cellIdx, canvasPos);
        m_eventHandled = true;
        return;
    }

     //2. 检查是否点击了引脚（开始绘制导线）
    bool snapped = false;
    wxPoint snappedPos = m_canvas->Snap(canvasPos, &snapped);
    if (snapped && m_currentTool == ToolType::DEFAULT_TOOL) {
        m_tempTool = true;
		SetCurrentTool(ToolType::WIRE_TOOL);
        StartWireDrawing(snappedPos, true);
        m_eventHandled = true;
        return;
    }

    // 3. 检查是否点击了元件
    int elementIndex = m_canvas->HitTestPublic(canvasPos);
    if (elementIndex != -1) {
        // 在选择工具下点击元件：选择但不拖动
        if (m_currentTool == ToolType::SELECT_TOOL) {
            m_canvas->SetSelectedIndex(elementIndex);
            m_eventHandled = true;
            return;
        }
        else {
            m_tempTool = true;
			SetCurrentTool(ToolType::DRAG_TOOL);
            StartElementDragging(elementIndex, canvasPos);
            m_eventHandled = true;
			return;
        }
    }

	// 4. 是否点击在默认工具下的空白区域
    if (m_currentTool == ToolType::DEFAULT_TOOL &&
        m_canvas->IsClickOnEmptyAreaPublic(canvasPos)) {
		m_tempTool = true;
		SetCurrentTool(ToolType::DRAG_TOOL);
        //StartPanning(canvasPos);
        StartPanning(m_canvas->CanvasToScreen(canvasPos));
        m_eventHandled = true;
        return;
	}


    // 4. 按工具类型处理其他情况
    switch (m_currentTool) {
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
        HandleDragTool(canvasPos);
        m_eventHandled = true;
        break;
        }
    }
}

void ToolManager::HandleDefaultTool(const wxPoint& canvasPos) {
    // 默认工具：包含导线绘制、平移等综合功能

    // 首先检查是否可以开始绘制导线（点击引脚）
    bool snapped = false;
    wxPoint snappedPos = m_canvas->Snap(canvasPos, &snapped);

    if (snapped) {
        // 开始绘制导线
        StartWireDrawing(snappedPos, true);
        m_eventHandled = true;
    }
    else {
        // 检查是否可以开始平移
        if (m_canvas->IsClickOnEmptyAreaPublic(canvasPos)) {
            StartPanning(canvasPos);
            m_eventHandled = true;
        }
        // 如果没有点击元件或引脚，不处理，让其他逻辑处理
    }
}

void ToolManager::HandleWireTool(const wxPoint& canvasPos) {
    // 专门的导线工具：可以从任意点开始绘制导线
    bool snapped = false;
    wxPoint snappedPos = m_canvas->Snap(canvasPos, &snapped);

    if (!m_isDrawingWire) {
        // 开始绘制导线（可以从任意点开始，不一定是引脚）
        StartWireDrawing(snappedPos, snapped);
    }
    else {
        // 完成导线绘制
        FinishWireDrawing(snappedPos);
		SetCurrentTool(ToolType::WIRE_TOOL);

        // 导线工具模式下，绘制完成后可以立即开始新的导线
        // 这提供了连续绘制的体验
        if (m_currentTool == ToolType::WIRE_TOOL) {
            // 可以选择是否自动开始新的导线
            // 如果希望连续绘制，取消下面这行的注释
            // StartWireDrawing(snappedPos, snapped);
        }
    }
}

void ToolManager::HandleDragTool(const wxPoint& canvasPos) {
    // 专门的平移工具
    StartPanning(canvasPos);
    m_eventHandled = true;
}

void ToolManager::StartWireDrawing(const wxPoint& startPos, bool fromPin) {
    // 如果已经在进行其他操作，先取消
    if (m_isDraggingElement) {
        FinishElementDragging();
    }
    if (m_isPanning) {
        FinishPanning(startPos);
    }
	m_currentTool = ToolType::WIRE_TOOL;
    m_isDrawingWire = true;
    m_wireStartPos = startPos;
    m_startCP = { startPos, fromPin ? CPType::Pin : CPType::Free };

    // 初始化临时导线
    m_canvas->m_tempWire.Clear();
    m_canvas->m_tempWire.AddPoint(m_startCP);
    m_canvas->m_wireMode = CanvasPanel::WireMode::DragNew;

    if (m_mainFrame) {
        if (fromPin) {
            m_mainFrame->SetStatusText("绘制导线: 从引脚开始，点击终点完成绘制 (ESC取消)");
        }
        else {
            m_mainFrame->SetStatusText("绘制导线: 从自由点开始，点击终点完成绘制 (ESC取消)");
        }
    }
}

void ToolManager::UpdateWireDrawing(const wxPoint& currentPos) {
    if (!m_isDrawingWire) return;

    bool snapped = false;
    wxPoint snappedPos = m_canvas->Snap(currentPos, &snapped);
	ControlPoint endCP = { snappedPos, snapped ? CPType::Pin : CPType::Free };

    // 更新导线预览
    m_canvas->m_tempWire.pts = Wire::RouteOrtho(m_startCP, endCP, PinDirection::Right, PinDirection::Left);
    m_canvas->Refresh();

    if (m_mainFrame) {
        if (snapped) {
            m_mainFrame->SetStatusText(wxString::Format("绘制导线: 终点吸附到引脚 (%d,%d)",
                snappedPos.x, snappedPos.y));
        }
        else {
            m_mainFrame->SetStatusText(wxString::Format("绘制导线: 终点为自由端点 (%d,%d)",
                snappedPos.x, snappedPos.y));
        }
    }
}

void ToolManager::FinishPanning(const wxPoint& currentPos) {
    m_isPanning = false;
    m_panStartPos = currentPos;
    m_fakeStartPos = currentPos;
    if (m_tempTool) {
        SetCurrentTool(m_previousTool);
        m_tempTool = false;
	}

    if (m_mainFrame) {
        m_mainFrame->SetStatusText("平移完成");
    }
}

void ToolManager::HandleComponentTool(const wxPoint& canvasPos) {
    if (!m_currentComponent.IsEmpty() && m_canvas) {
        // 放置元件
        m_canvas->PlaceElement(m_currentComponent, canvasPos);

        if (m_mainFrame) {
            m_mainFrame->SetStatusText(wxString::Format("已放置: %s", m_currentComponent));
        }
    }
    else {
        if (m_mainFrame) {
            m_mainFrame->SetStatusText("请先从元件库选择一个元件");
        }
    }
}

void ToolManager::OnCanvasLeftUp(const wxPoint& canvasPos) {
    if (m_isEditingWire) {
        FinishWireEditing();
        m_eventHandled = true;
    }
    else if (m_isDraggingElement) {
        FinishElementDragging();
        m_eventHandled = true;
    }
    else if (m_isPanning) {
        //FinishPanning(canvasPos);
        FinishPanning(m_canvas->CanvasToScreen(canvasPos));
        m_eventHandled = true;
    }
    else if (m_isDrawingWire && m_currentTool == ToolType::WIRE_TOOL) {
        // 导线工具模式下，点击完成导线
        FinishWireDrawing(canvasPos);
        m_eventHandled = true;
    }
    else {
        // 其他工具不处理
	}
}

void ToolManager::OnCanvasKeyDown(wxKeyEvent& evt) {
    if (evt.GetKeyCode() == WXK_ESCAPE) {
        if (m_isDrawingWire) {
            CancelWireDrawing();
        }
        else if (m_isEditingWire) {
            CancelWireEditing();
        }
        else if (m_isDraggingElement) {
            // 取消元件拖动，恢复原位
            if (m_draggingElementIndex != -1) {
                m_canvas->m_elements[m_draggingElementIndex].SetPos(m_elementStartCanvasPos);
                m_canvas->Refresh();
            }
            FinishElementDragging();
        }
        else {
            SetCurrentTool(ToolType::DEFAULT_TOOL);
        }
        evt.Skip(false); // 已处理
    }
    else {
        evt.Skip(); // 其他按键继续传递
    }
}

void ToolManager::OnCanvasMouseWheel(wxMouseEvent& evt) {
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

        evt.Skip(false);  // 已处理
    }
    else {
        evt.Skip();  // 非Ctrl滚轮继续传递
    }
}

void ToolManager::HandleSelectTool(const wxPoint& canvasPos) {
    if (m_canvas->IsClickOnEmptyAreaPublic(canvasPos)) {
        m_canvas->ClearSelection();
        if (m_mainFrame) {
            m_mainFrame->SetStatusText("选中工具: 清除选择");
        }
    }
    else {
        int selectedIndex = m_canvas->HitTestPublic(canvasPos);
        if (selectedIndex != -1) {
            m_canvas->SetSelectedIndex(selectedIndex);
            if (m_mainFrame) {
                m_mainFrame->SetStatusText(wxString::Format("选中工具: 选择元件 %d", selectedIndex));
            }
        }
    }
}

void ToolManager::HandleTextTool(const wxPoint& canvasPos) {
    // 文本工具行为：在点击位置创建文本元素
    // 注意：这里暂时注释掉文本输入对话框，等工具稳定后再实现
    /*
    wxString text = wxGetTextFromUser("请输入文本:", "添加文本", "", m_mainFrame);
    if (!text.IsEmpty()) {
        if (m_mainFrame) {
            m_mainFrame->SetStatusText(wxString::Format("文本工具: 添加文本 '%s'", text.ToUTF8().data()));
        }
    }
    */

    if (m_mainFrame) {
        m_mainFrame->SetStatusText("文本工具: 点击添加文本");
    }
}

// 添加设置当前元件的方法
void ToolManager::SetCurrentComponent(const wxString& componentName) {
    m_currentComponent = componentName;
    SetCurrentTool(ToolType::COMPONENT_TOOL);

    if (m_mainFrame) {
        m_mainFrame->SetStatusText(wxString::Format("准备放置: %s - 在画布上点击放置", componentName));
        // 设置十字光标
        m_canvas->SetCursor(wxCursor(wxCURSOR_CROSS));
    }
}

// 改进的导线绘制完成方法
void ToolManager::FinishWireDrawing(const wxPoint& endPos) {
    if (!m_isDrawingWire) return;

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
    m_isDrawingWire = false;
    m_canvas->m_wireMode = CanvasPanel::WireMode::Idle;
    m_canvas->m_tempWire.Clear();
    if (m_tempTool) {
        SetCurrentTool(m_previousTool);
        m_tempTool = false;
	}

    if (m_mainFrame) {
        if (snappedEnd) {
            m_mainFrame->SetStatusText("导线绘制完成: 连接到引脚");
        }
        else {
            m_mainFrame->SetStatusText("导线绘制完成: 自由端点");
        }
    }


    m_canvas->Refresh();
}

void ToolManager::CancelWireDrawing() {
    m_isDrawingWire = false;
    m_canvas->m_wireMode = CanvasPanel::WireMode::Idle;
    m_canvas->m_tempWire.Clear();
    m_canvas->Refresh();

    if (m_mainFrame) {
        m_mainFrame->SetStatusText("导线绘制取消");
    }
}
// 导线编辑方法
void ToolManager::StartWireEditing(int wireIndex, int pointIndex, const wxPoint& startPos) {
    if (wireIndex < 0 || wireIndex >= (int)m_canvas->m_wires.size()) return;

	m_currentTool = ToolType::WIRE_TOOL;
    m_isEditingWire = true;
    m_editingWireIndex = wireIndex;
    m_editingPointIndex = pointIndex;
    m_editStartPos = startPos;

    if (m_mainFrame) {
        m_mainFrame->SetStatusText("编辑导线: 拖动控制点调整路径");
    }
}

void ToolManager::UpdateWireEditing(const wxPoint& currentPos) {
    if (!m_isEditingWire) return;

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

void ToolManager::FinishWireEditing() {
    m_isEditingWire = false;
    m_editingWireIndex = -1;
    m_editingPointIndex = -1;

    if (m_tempTool) {
        SetCurrentTool(m_previousTool);
        m_tempTool = false;
    }

    if (m_mainFrame) {
        m_mainFrame->SetStatusText("导线编辑完成");
    }
}

void ToolManager::CancelWireEditing() {
    m_isEditingWire = false;
    m_editingWireIndex = -1;
    m_editingPointIndex = -1;

    if (m_mainFrame) {
        m_mainFrame->SetStatusText("导线编辑取消");
    }
}

// 元件拖动方法
void ToolManager::StartElementDragging(int elementIndex, const wxPoint& startPos) {
    // 如果已经在进行其他操作，先取消
    if (m_isDrawingWire) {
        CancelWireDrawing();
    }
    if (m_isPanning) {
        FinishPanning(startPos);
    }

    if (elementIndex < 0 || elementIndex >= (int)m_canvas->m_elements.size()) return;

    m_isDraggingElement = true;
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

    if (m_mainFrame) {
        m_mainFrame->SetStatusText("拖动元件: 移动鼠标调整位置 (ESC取消)");
    }
}

void ToolManager::UpdateElementDragging(const wxPoint& currentPos) {
    if (!m_isDraggingElement || m_draggingElementIndex == -1) return;

    // 计算偏移量
    wxPoint delta = currentPos - m_elementDragStartPos;
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
    if (m_mainFrame) {
        m_mainFrame->SetStatusText(debugInfo);
    }

    m_canvas->Refresh();
}

void ToolManager::FinishElementDragging() {
    m_isDraggingElement = false;
    m_draggingElementIndex = -1;
    m_canvas->m_movingWires.clear();

    if (m_tempTool) {
        SetCurrentTool(m_previousTool);
        m_tempTool = false;
	}

    if (m_mainFrame) {
        m_mainFrame->SetStatusText("元件放置完成");
    }
}

void ToolManager::OnCanvasMouseMove(const wxPoint& canvasPos) {
    m_eventHandled = false;

    // 按优先级处理各种操作
    if (m_isEditingWire) {
        UpdateWireEditing(canvasPos);
        m_eventHandled = true;
    }
    else if (m_isDrawingWire) {
        UpdateWireDrawing(canvasPos);
        m_eventHandled = true;
    }
    else if (m_isDraggingElement) {
        UpdateElementDragging(canvasPos);
        m_eventHandled = true;
    }
    else if (m_isPanning) {
        //UpdatePanning(canvasPos);
        UpdatePanning(m_canvas->CanvasToScreen(canvasPos));
        m_eventHandled = true;
    }
    else {
        // 悬停检测和反馈
        HandleHoverFeedback(canvasPos);
    }
}

void ToolManager::HandleHoverFeedback(const wxPoint& canvasPos) {
    // 引脚悬停检测
    bool isInput = false;
    wxPoint hoverWorld;
    int newHover = m_canvas->HitHoverPin(canvasPos, &isInput, &hoverWorld);
    if (newHover != m_canvas->m_hoverPinIdx || isInput != m_canvas->m_hoverIsInput) {
        m_canvas->m_hoverPinIdx = newHover;
        m_canvas->m_hoverIsInput = isInput;
        m_canvas->m_hoverPinPos = hoverWorld;
        m_canvas->Refresh();
    }

    // 导线控制点悬停检测
    int cellWire, cellIdx;
    wxPoint cellPos;
    int newCell = m_canvas->HitHoverCell(canvasPos, &cellWire, &cellIdx, &cellPos);
    if (newCell != m_canvas->m_hoverCellIdx || cellWire != m_canvas->m_hoverCellWire) {
        m_canvas->m_hoverCellWire = cellWire;
        m_canvas->m_hoverCellIdx = cellIdx;
        m_canvas->m_hoverCellPos = cellPos;
        m_canvas->Refresh();
    }

    // 状态栏反馈
    if (m_mainFrame) {
        wxString status;
        if (newHover != -1) {
            status = wxString::Format("悬停在%s引脚 - 点击开始绘制导线",
                isInput ? "输入" : "输出");
        }
        else if (newCell != -1) {
            status = "悬停在导线控制点 - 点击拖动调整";
        }
        else {
            int elementIndex = m_canvas->HitTestPublic(canvasPos);
            if (elementIndex != -1) {
                status = wxString::Format("悬停在元件: %s - 点击拖动移动",
                    m_canvas->m_elements[elementIndex].GetName());
            }
            else {
                status = wxString::Format("位置: (%d, %d)", canvasPos.x, canvasPos.y);
            }
        }
        m_mainFrame->SetStatusText(status);
    }
}

void ToolManager::StartPanning(const wxPoint& startPos) {
    // 如果已经在进行其他操作，先取消
    if (m_isDrawingWire) {
        CancelWireDrawing();
    }
    if (m_isDraggingElement) {
        FinishElementDragging();
    }
    if (m_isEditingWire) {
        CancelWireEditing();
    }

    m_isPanning = true;
    m_panStartPos = startPos;
	m_fakeStartPos = startPos;

    if (m_mainFrame) {
        m_mainFrame->SetStatusText("平移画布: 拖动鼠标移动画布 (ESC取消)");
    }
}

void ToolManager::UpdatePanning(const wxPoint& currentPos) {
    if (!m_isPanning) return;

    wxPoint delta = currentPos - m_fakeStartPos;
    m_canvas->m_offset += delta;
    m_fakeStartPos = currentPos;

	wxPoint realDelta = currentPos - m_panStartPos;
    
    m_canvas->Refresh();

    if (m_mainFrame) {
        m_mainFrame->SetStatusText(wxString::Format("平移画布: 偏移(%d, %d)", realDelta.x, realDelta.y));
        //m_mainFrame->SetStatusText(wxString::Format("平移画布: 偏移(%d, %d), (%d, %d)", m_panStartPos.x, m_panStartPos.y, currentPos.x, currentPos.y));
    }
}