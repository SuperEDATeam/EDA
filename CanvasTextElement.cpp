#include "CanvasTextElement.h"

CanvasTextElement::CanvasTextElement(const wxString& text, const wxPoint& pos)
    : m_text(text), m_position(pos), m_size(100, 30),
    m_editing(false), m_cursorPos(0), m_cursorVisible(true),
    m_lastBlinkTime(wxGetLocalTimeMillis()) {
}

void CanvasTextElement::Draw(wxDC& dc) {
    // 现代简约样式：细边框，圆角，轻微阴影
    if (m_editing) {
        DrawEditingState(dc);
    }
    else {
        DrawNormalState(dc);
    }
}

void CanvasTextElement::DrawNormalState(wxDC& dc) {
    // 简约背景（轻微半透明）
    dc.SetBrush(wxBrush(wxColour(255, 255, 255, 230))); // 半透明白色
    dc.SetPen(wxPen(wxColour(200, 200, 200), 1)); // 浅灰色边框

    // 绘制圆角矩形背景
    DrawRoundedRect(dc, wxRect(m_position, m_size), 4);

    // 绘制文本
    DrawTextContent(dc);
}

void CanvasTextElement::DrawEditingState(wxDC& dc) {
    // 编辑状态：更明显的边框
    dc.SetBrush(wxBrush(wxColour(255, 255, 255, 240)));
    dc.SetPen(wxPen(wxColour(100, 150, 255), 2)); // 蓝色边框表示激活

    DrawRoundedRect(dc, wxRect(m_position, m_size), 4);
    DrawTextContent(dc);
    DrawCursor(dc);
}

void CanvasTextElement::DrawTextContent(wxDC& dc) {
    dc.SetFont(GetModernFont());
    dc.SetTextForeground(wxColour(60, 60, 60)); // 深灰色文本

    // 文本位置（居中垂直，左侧留白）
    wxSize textSize = dc.GetTextExtent(m_text);
    int textY = m_position.y + (m_size.y - textSize.y) / 2;

    dc.DrawText(m_text, m_position.x + 8, textY);
}

void CanvasTextElement::DrawCursor(wxDC& dc) {
    if (!m_cursorVisible) return;

    // 计算光标位置
    wxString prefix = m_text.Left(m_cursorPos);
    wxSize prefixSize = dc.GetTextExtent(prefix);

    int cursorX = m_position.x + 8 + prefixSize.GetWidth();
    int cursorY = m_position.y + 6;
    int cursorHeight = m_size.y - 12;

    // 现代风格光标：细线
    dc.SetPen(wxPen(wxColour(50, 120, 255), 2));
    dc.DrawLine(cursorX, cursorY, cursorX, cursorY + cursorHeight);
}

void CanvasTextElement::DrawRoundedRect(wxDC& dc, const wxRect& rect, int radius) {
    // 简化的圆角矩形绘制
    dc.DrawRoundedRectangle(rect, radius);
}

wxFont CanvasTextElement::GetModernFont() {
    return wxFont(11, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL,
        wxFONTWEIGHT_NORMAL, false, "Segoe UI");
}

// 输入处理
void CanvasTextElement::InsertChar(wxChar ch) {
    wxString str;
    str << ch;
    InsertText(str);
}

void CanvasTextElement::InsertText(const wxString& text) {
    // 在光标位置插入整个字符串
    m_text.insert(m_cursorPos, text);
    m_cursorPos += text.length();
    UpdateSize();
}

void CanvasTextElement::DeleteBackward() {
    if (m_cursorPos > 0) {
        m_text.Remove(m_cursorPos - 1, 1);
        m_cursorPos--;
        UpdateSize();
    }
}

void CanvasTextElement::DeleteForward() {
    if (m_cursorPos < (int)m_text.length()) {
        m_text.Remove(m_cursorPos, 1);
        UpdateSize();
    }
}

void CanvasTextElement::MoveCursorLeft() {
    if (m_cursorPos > 0) m_cursorPos--;
}

void CanvasTextElement::MoveCursorRight() {
    if (m_cursorPos < (int)m_text.length()) m_cursorPos++;
}

void CanvasTextElement::UpdateSize() {
    // 基于文本内容自动调整大小
    wxClientDC dc(wxTheApp->GetTopWindow());
    dc.SetFont(GetModernFont());
    wxSize textSize = dc.GetTextExtent(m_text);
    m_size.x = textSize.x + 20; // 左右留白
    m_size.y = wxMax(30, textSize.y + 10); // 最小高度
}

void CanvasTextElement::UpdateCursorBlink() {
    wxLongLong currentTime = wxGetLocalTimeMillis();
    if (currentTime - m_lastBlinkTime > 500) { // 500ms 闪烁间隔
        m_cursorVisible = !m_cursorVisible;
        m_lastBlinkTime = currentTime;
    }
}

// 命中测试
bool CanvasTextElement::Contains(const wxPoint& point) const {
    return wxRect(m_position, m_size).Contains(point);
}

// Getter/Setter
void CanvasTextElement::SetEditing(bool editing) {
    m_editing = editing;
    if (editing) m_cursorVisible = true;
}

void CanvasTextElement::SetPosition(const wxPoint& pos) {
    m_position = pos;
}

wxString CanvasTextElement::GetText() const {
    return m_text;
}

wxPoint CanvasTextElement::GetPosition() const {
    return m_position;
}

wxSize CanvasTextElement::GetSize() const {
    return m_size;
}
