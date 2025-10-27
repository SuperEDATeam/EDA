#include "ToolManager.h"
#include "MainFrame.h"

ToolManager::ToolManager(MainFrame* mainFrame, ToolBars* toolBars, CanvasPanel* canvas)
    : m_mainFrame(mainFrame), m_toolBars(toolBars), m_canvas(canvas),
    m_currentTool(ToolType::DEFAULT_TOOL) {
}

void ToolManager::OnCanvasLeftDown(const wxPoint& canvasPos) {
    m_eventHandled = false;
    switch (m_currentTool) {
    case ToolType::DEFAULT_TOOL:
        HandleDefaultTool(canvasPos);
        break;
    case ToolType::SELECT_TOOL:
        HandleSelectTool(canvasPos);
        break;
    case ToolType::TEXT_TOOL:
        HandleTextTool(canvasPos);
        m_eventHandled = true; // 文本工具处理了事件
        break;
    case ToolType::COMPONENT_TOOL:
        HandleComponentTool(canvasPos);
        m_eventHandled = true; // 元件工具处理了事件
        break;
    }
}

void ToolManager::HandleDefaultTool(const wxPoint& canvasPos) {
    // 默认工具的行为：允许平移、选择等现有功能
    // 这里不需要额外处理，因为画布已经有默认行为

    // 可以添加一些调试信息或状态栏反馈
    if (m_mainFrame) {
        m_mainFrame->SetStatusText(wxString::Format("默认工具: 点击位置 (%d, %d)",
            canvasPos.x, canvasPos.y));
    }

    // 注意：默认工具不应该阻止事件继续传播
    // 这样画布原有的导线、选择等功能还能正常工作
}

void ToolManager::HandleComponentTool(const wxPoint& canvasPos) {
    if (!m_currentComponent.IsEmpty() && m_canvas) {
        // 放置元件
        m_canvas->PlaceElement(m_currentComponent, canvasPos);

        // 重要：放置元件后自动切换回默认工具
        SetCurrentTool(ToolType::DEFAULT_TOOL);
        m_currentComponent.Clear(); // 清空当前元件

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


void ToolManager::SetCurrentTool(ToolType tool) {
    m_currentTool = tool;

    // 设置适当的光标
    if (m_canvas) {
        switch (tool) {
        case ToolType::COMPONENT_TOOL:
            m_canvas->SetCursor(wxCursor(wxCURSOR_CROSS));
            break;
        case ToolType::TEXT_TOOL:
            m_canvas->SetCursor(wxCursor(wxCURSOR_IBEAM));
            break;
        default:
            m_canvas->SetCursor(wxCursor(wxCURSOR_ARROW));
            break;
        }
    }

    // 状态栏反馈
    if (m_mainFrame) {
        wxString toolName;
        switch (tool) {
        case ToolType::DEFAULT_TOOL: toolName = "默认工具"; break;
        case ToolType::SELECT_TOOL: toolName = "选中工具"; break;
        case ToolType::TEXT_TOOL: toolName = "文本工具"; break;
        case ToolType::COMPONENT_TOOL: toolName = "元件工具"; break;
        }
        m_mainFrame->SetStatusText(wxString::Format("当前工具: %s", toolName));
    }
}

void ToolManager::OnCanvasMouseMove(const wxPoint& canvasPos) {
    m_eventHandled = false;

    // 元件工具模式下，可以在这里预览元件位置（如果需要）
    if (m_currentTool == ToolType::COMPONENT_TOOL && !m_currentComponent.IsEmpty()) {
        // 可以在这里实现元件放置预览
        if (m_mainFrame) {
            m_mainFrame->SetStatusText(wxString::Format("准备放置 %s 在 (%d, %d)",
                m_currentComponent, canvasPos.x, canvasPos.y));
        }
    }
}