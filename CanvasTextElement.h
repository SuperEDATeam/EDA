#pragma once
#ifndef CANVASTEXTELEMENT_H
#define CANVASTEXTELEMENT_H

#include <wx/wx.h>

class CanvasPanel; // 前向声明

class CanvasTextElement {
public:
    CanvasTextElement(CanvasPanel* parent, const wxString& text = "", const wxPoint& pos = wxPoint(0, 0));
    ~CanvasTextElement();

    // 绘制方法
    void Draw(wxDC& dc);
    bool Contains(const wxPoint& point) const;

    // 文本操作 - 代理到隐藏的TextCtrl
    void StartEditing();
    void StopEditing();
    void SyncToHiddenCtrl();  // 将CanvasTextElement的文本同步到隐藏TextCtrl
    void SyncFromHiddenCtrl(); // 从隐藏TextCtrl同步文本到CanvasTextElement

    // Getter/Setter
    void SetText(const wxString& text);
    wxString GetText() const;
    void SetPosition(const wxPoint& pos);
    wxPoint GetPosition() const;
    wxSize GetSize() const;
    bool IsEditing() const { return m_editing; }

    // 事件处理方法 - 新增
    void OnTextChanged(const wxString& newText);
    void OnTextEnter();
    void OnTextKillFocus();

    // 隐藏TextCtrl管理
    void AttachHiddenTextCtrl(wxTextCtrl* hiddenCtrl);
    void DetachHiddenTextCtrl();
    void UpdateHiddenTextCtrlPosition();

    wxFont GetModernFont();

private:
    void DrawNormalState(wxDC& dc);
    void DrawEditingState(wxDC& dc);
    void DrawTextContent(wxDC& dc);
    void DrawRoundedRect(wxDC& dc, const wxRect& rect, int radius);
    void UpdateSize();

private:
    CanvasPanel* m_parent;
    wxString m_text;
    wxPoint m_position;
    wxSize m_size;
    bool m_editing;

    wxTextCtrl* m_hiddenTextCtrl; // 指向共享的隐藏TextCtrl
};

#endif // CANVASTEXTELEMENT_H