#pragma once
#ifndef CANVASTEXTELEMENT_H
#define CANVASTEXTELEMENT_H

#include <wx/wx.h>

class CanvasTextElement {
public:
    CanvasTextElement(const wxString& text = "", const wxPoint& pos = wxPoint(0, 0));

    void Draw(wxDC& dc);
    void InsertChar(wxChar ch);
    void InsertText(const wxString& text);
    void DeleteBackward();
    void DeleteForward();
    void MoveCursorLeft();
    void MoveCursorRight();
    void UpdateSize();
    void UpdateCursorBlink();
    bool Contains(const wxPoint& point) const;
    void SetFocus();

    // Getter/Setter
    void SetEditing(bool editing);
    void SetPosition(const wxPoint& pos);
    wxString GetText() const;
    wxPoint GetPosition() const;
    wxSize GetSize() const;

private:
    void DrawNormalState(wxDC& dc);
    void DrawEditingState(wxDC& dc);
    void DrawTextContent(wxDC& dc);
    void DrawCursor(wxDC& dc);
    void DrawRoundedRect(wxDC& dc, const wxRect& rect, int radius);
    wxFont GetModernFont();

private:
    wxString m_text;
    wxPoint m_position;
    wxSize m_size;
    bool m_editing;
    int m_cursorPos;
    bool m_cursorVisible;
    bool m_isFoucus;
    wxLongLong m_lastBlinkTime;
};

#endif // CANVASTEXTELEMENT_H