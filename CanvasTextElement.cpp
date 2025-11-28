#include "CanvasTextElement.h"
#include "CanvasPanel.h"

CanvasTextElement::CanvasTextElement(CanvasPanel* parent, const wxString& text, const wxPoint& pos)
    : m_parent(parent), m_text(text), m_position(pos), m_size(100, 30),
    m_editing(false), m_hiddenTextCtrl(nullptr) {
    UpdateSize();
}

CanvasTextElement::~CanvasTextElement() {
    DetachHiddenTextCtrl();
}

void CanvasTextElement::AttachHiddenTextCtrl(wxTextCtrl* hiddenCtrl) {
    m_hiddenTextCtrl = hiddenCtrl;
    if (m_hiddenTextCtrl && m_editing) {
        SyncToHiddenCtrl();
        UpdateHiddenTextCtrlPosition();
    }
}

void CanvasTextElement::DetachHiddenTextCtrl() {
    if (m_hiddenTextCtrl && m_editing) {
        SyncFromHiddenCtrl();
    }
    m_hiddenTextCtrl = nullptr;
}

void CanvasTextElement::UpdateHiddenTextCtrlPosition() {
    if (m_hiddenTextCtrl && m_editing) {
        // 将隐藏TextCtrl移动到CanvasTextElement的位置
        wxPoint screenPos = m_parent->CanvasToScreen(m_position);
        wxSize screenSize = wxSize(
            static_cast<int>(m_size.x * m_parent->GetScale()),
            static_cast<int>(m_size.y * m_parent->GetScale())
        );

        m_hiddenTextCtrl->SetPosition(screenPos);
        m_hiddenTextCtrl->SetSize(screenSize);
    }
}

void CanvasTextElement::StartEditing() {
    m_editing = true;

    if (m_hiddenTextCtrl) {
        SyncToHiddenCtrl();
        UpdateHiddenTextCtrlPosition();

        // 设置隐藏TextCtrl的样式
        m_hiddenTextCtrl->SetFont(GetModernFont());
        m_hiddenTextCtrl->SetForegroundColour(wxColour(60, 60, 60));
        m_hiddenTextCtrl->SetFocus();
        m_hiddenTextCtrl->SelectAll();
    }
}

void CanvasTextElement::StopEditing() {
    if (m_editing && m_hiddenTextCtrl) {
        SyncFromHiddenCtrl();
    }
    m_editing = false;
}

void CanvasTextElement::SyncToHiddenCtrl() {
    if (m_hiddenTextCtrl) {
        m_hiddenTextCtrl->SetValue(m_text);
    }
}

void CanvasTextElement::SyncFromHiddenCtrl() {
    if (m_hiddenTextCtrl) {
        m_text = m_hiddenTextCtrl->GetValue();
        UpdateSize();
    }
}

// 绘制方法保持不变
void CanvasTextElement::Draw(wxDC& dc) {
    if (m_editing) {
        DrawEditingState(dc);
    }
    else {
        DrawNormalState(dc);
    }
}

void CanvasTextElement::DrawNormalState(wxDC& dc) {
    dc.SetBrush(*wxTRANSPARENT_BRUSH);
    dc.SetPen(wxPen(wxColour(*wxLIGHT_GREY), 3));
    dc.DrawRectangle(wxRect(m_position - wxPoint(1, 1), m_size + wxSize(2, 2)));
    //DrawRoundedRect(dc, wxRect(m_position, m_size+wxSize(2,2)), 4);
    DrawTextContent(dc);
}

void CanvasTextElement::DrawEditingState(wxDC& dc) {
    dc.SetBrush(*wxTRANSPARENT_BRUSH);
    dc.SetPen(wxPen(wxColour(*wxBLACK), 3));
    dc.DrawRectangle(wxRect(m_position - wxPoint(1, 1), m_size + wxSize(2, 2)));
    //DrawRoundedRect(dc, wxRect(m_position, m_size + wxSize(2, 2)), 4);
    DrawTextContent(dc);

    // 绘制光标（模拟）
    if (m_editing) {
        wxSize textSize = dc.GetTextExtent(m_text);
        int cursorX = m_position.x + 8 + textSize.x;
        int cursorY1 = m_position.y + 5;
        int cursorY2 = m_position.y + m_size.y - 5;
        dc.SetPen(wxPen(*wxBLACK, 1));
        dc.DrawLine(cursorX, cursorY1, cursorX, cursorY2);
    }
}

void CanvasTextElement::DrawTextContent(wxDC& dc) {
    dc.SetFont(GetModernFont());
    dc.SetTextForeground(wxColour(60, 60, 60));

    wxSize textSize = dc.GetTextExtent(m_text);
    int textY = m_position.y + (m_size.y - textSize.y) / 2;
    dc.DrawText(m_text, m_position.x + 8, textY);
}

void CanvasTextElement::DrawRoundedRect(wxDC& dc, const wxRect& rect, int radius) {
    dc.DrawRoundedRectangle(rect, radius);
}

wxFont CanvasTextElement::GetModernFont() {
    return wxFont(11, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL,
        wxFONTWEIGHT_NORMAL, false, "Segoe UI");
}

void CanvasTextElement::UpdateSize() {
    wxClientDC dc(wxTheApp->GetTopWindow());
    dc.SetFont(GetModernFont());
    wxSize textSize = dc.GetTextExtent(m_text);
    m_size.x = textSize.x + 20;
    m_size.y = wxMax(30, textSize.y + 10);

    // 更新隐藏TextCtrl的位置
    UpdateHiddenTextCtrlPosition();
}

bool CanvasTextElement::Contains(const wxPoint& point) const {
    return wxRect(m_position, m_size).Contains(point);
}

void CanvasTextElement::SetText(const wxString& text) {
    m_text = text;
    UpdateSize();
    if (m_hiddenTextCtrl && m_editing) {
        SyncToHiddenCtrl();
    }
}

wxString CanvasTextElement::GetText() const {
    return m_text;
}

void CanvasTextElement::SetPosition(const wxPoint& pos) {
    m_position = pos;
    UpdateHiddenTextCtrlPosition();
}

void CanvasTextElement::OnTextChanged(const wxString& newText) {
    m_text = newText;
    UpdateSize();

    // 如果附加了隐藏TextCtrl，同步文本
    if (m_hiddenTextCtrl && m_editing) {
        m_hiddenTextCtrl->SetValue(newText);
    }
}

void CanvasTextElement::OnTextEnter() {
    StopEditing();
}

void CanvasTextElement::OnTextKillFocus() {
    StopEditing();
}

wxRect CanvasTextElement::GetBounds() const{
    return wxRect(m_position, m_size);
}

wxPoint CanvasTextElement::GetPosition() const {
	return m_position;
}