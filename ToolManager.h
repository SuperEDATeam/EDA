#pragma once
#include <wx/wx.h>
#include "ToolBars.h"
#include "CanvasPanel.h"

class ToolManager {
private:
    ToolType m_currentTool;
    ToolBars* m_toolBars;
    CanvasPanel* m_canvas;
    MainFrame* m_mainFrame;
    wxString m_currentComponent;
    bool m_eventHandled; // 标记事件是否已处理

public:
    ToolManager(MainFrame* mainFrame, ToolBars* toolBars, CanvasPanel* canvas);

    void SetCurrentTool(ToolType tool);
    ToolType GetCurrentTool() const { return m_currentTool; }
    void SetCurrentComponent(const wxString& componentName);

    bool IsEventHandled() const { return m_eventHandled; }
    void ResetEventHandled() { m_eventHandled = false; }

    // 工具行为方法
    void HandleDefaultTool(const wxPoint& canvasPos);
    void HandleSelectTool(const wxPoint& canvasPos);
    void HandleTextTool(const wxPoint& canvasPos);
    void HandleComponentTool(const wxPoint& canvasPos);

    // 画布事件转发
    void OnCanvasLeftDown(const wxPoint& canvasPos);
    void OnCanvasLeftUp(const wxPoint& canvasPos);
    void OnCanvasMouseMove(const wxPoint& canvasPos);
};