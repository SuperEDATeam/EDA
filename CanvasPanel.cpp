#include "CanvasPanel.h"
#include "MainFrame.h"
#include <wx/dcbuffer.h>   // 双缓冲
#include "CanvasElement.h" // 新增
#include "my_log.h"

wxBEGIN_EVENT_TABLE(CanvasPanel, wxPanel)
EVT_PAINT(CanvasPanel::OnPaint)
EVT_LEFT_DOWN(CanvasPanel::OnLeftDown)
EVT_LEFT_UP(CanvasPanel::OnLeftUp)    
EVT_MOTION(CanvasPanel::OnMouseMove)  
EVT_KEY_DOWN(CanvasPanel::OnKeyDown)
wxEND_EVENT_TABLE()

CanvasPanel::CanvasPanel(wxWindow* parent)
    : wxPanel(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize,
        wxFULL_REPAINT_ON_RESIZE | wxBORDER_NONE)
{
    SetBackgroundStyle(wxBG_STYLE_PAINT); // 双缓冲必须
    SetBackgroundStyle(wxBG_STYLE_PAINT);
    SetBackgroundColour(*wxYELLOW);
    MyLog("CanvasPanel: drop target set\n");
}

void CanvasPanel::AddElement(const CanvasElement& elem)
{
    m_elements.push_back(elem);
    Refresh();
    MyLog("CanvasPanel::AddElement: <%s> total=%zu\n",
        elem.GetName().ToUTF8().data(), m_elements.size());
}

void CanvasPanel::OnPaint(wxPaintEvent&)
{
    wxAutoBufferedPaintDC dc(this);
    //dc.SetBackground(*wxYELLOW);
    dc.Clear();

    // 1. 画网格（你原来就有）
    const int grid = 20;
    const wxColour c(240, 240, 240);
    dc.SetPen(wxPen(c, 1));
    wxSize sz = GetClientSize();
    for (int x = 0; x < sz.x; x += grid) {
        dc.DrawLine(x - 2, 0, x + 2, sz.y);   // 垂直贯穿
        dc.DrawLine(0, x - 2, sz.x, x + 2);   // 水平贯穿
    }

    // 2. 画所有元件（新增）
    for (const auto& elem : m_elements) {
        elem.Draw(dc);   // 调用 CanvasElement::Draw

    
    }

    for (size_t i = 0; i < m_elements.size(); ++i) {
        const auto& elem = m_elements[i];
        elem.Draw(dc);

        // 选中的元件绘制高亮边框
        if (i == (size_t)m_selectedIndex) {
            wxRect bounds = elem.GetBounds();
            dc.SetPen(wxPen(wxColour(255, 0, 0), 2, wxPENSTYLE_DOT)); // 红色虚线
            dc.SetBrush(*wxTRANSPARENT_BRUSH);
            dc.DrawRectangle(bounds); // 绘制边界框
        }
    }
}

void CanvasPanel::OnLeftDown(wxMouseEvent& evt) {
    wxPoint pt = evt.GetPosition();
    m_selectedIndex = HitTest(pt); // 先检测是否点击到元件

    if (m_selectedIndex != -1) {
        // 选中元件，准备拖动
        m_isDragging = true;
        m_dragStartPos = pt;
        m_elementStartPos = m_elements[m_selectedIndex].GetPos();
        SetFocus();
        Refresh();
    }
    else {
        // 点击空白区域：清除选中状态（核心修改）
        m_selectedIndex = -1; // 取消选中
        m_isDragging = false;

        // 原逻辑：放置新元件（如果有待放置的工具）
        MainFrame* mf = wxDynamicCast(GetParent(), MainFrame);
        if (mf && !mf->GetPendingTool().IsEmpty()) {
            PlaceElement(mf->GetPendingTool(), pt);
            mf->ClearPendingTool();
        }

        Refresh(); // 刷新以移除高亮边框
    }
}

void CanvasPanel::OnKeyDown(wxKeyEvent& evt) {
    if (evt.GetKeyCode() == WXK_ESCAPE) {
        MainFrame* mf = wxDynamicCast(GetParent(), MainFrame);
        if (mf) mf->ClearPendingTool();
    }
    else if (evt.GetKeyCode() == WXK_DELETE && m_selectedIndex != -1) {
        // 删除选中的元件
        m_elements.erase(m_elements.begin() + m_selectedIndex);
        m_selectedIndex = -1;
        Refresh();
    }
    else {
        evt.Skip();
    }
}

void CanvasPanel::PlaceElement(const wxString& name, const wxPoint& pos)
{
    extern std::vector<CanvasElement> g_elements;
    auto it = std::find_if(g_elements.begin(), g_elements.end(),
        [&](const CanvasElement& e) { return e.GetName() == name; });
    if (it == g_elements.end()) return;
    CanvasElement clone = *it;
    clone.SetPos(pos);
    AddElement(clone);          // 复用你已有的 AddElement/Refresh
}

// 计算元件的边界框（根据所有形状的顶点）
wxRect CanvasElement::GetBounds() const {
    if (m_shapes.empty()) return wxRect(m_pos, wxSize(1, 1));

    int minX = INT_MAX, minY = INT_MAX;
    int maxX = INT_MIN, maxY = INT_MIN;

    // 遍历所有形状的点计算边界
    auto updateBounds = [&](const Point& p) {
        int x = m_pos.x + p.x;
        int y = m_pos.y + p.y;
        minX = std::min(minX, x);
        minY = std::min(minY, y);
        maxX = std::max(maxX, x);
        maxY = std::max(maxY, y);
        };

    for (const auto& shape : m_shapes) {
        std::visit([&](auto&& arg) {
            using T = std::decay_t<decltype(arg)>;
            if constexpr (std::is_same_v<T, PolyShape>) {
                for (const auto& p : arg.pts) updateBounds(p);
            }
            else if constexpr (std::is_same_v<T, Line>) {
                updateBounds(arg.start);
                updateBounds(arg.end);
            }
            else if constexpr (std::is_same_v<T, Circle>) {
                updateBounds({ arg.center.x - arg.radius, arg.center.y - arg.radius });
                updateBounds({ arg.center.x + arg.radius, arg.center.y + arg.radius });
            }
            else if constexpr (std::is_same_v<T, Text>) {
                updateBounds(arg.pos);
            }
            }, shape);
    }

    return wxRect(minX, minY, maxX - minX, maxY - minY);
}

// 在CanvasPanel中实现HitTest
int CanvasPanel::HitTest(const wxPoint& pt) {
    for (size_t i = 0; i < m_elements.size(); ++i) {
        if (m_elements[i].GetBounds().Contains(pt)) {
            return i; // 返回选中的元件索引
        }
    }
    return -1; // 未选中任何元件
}

// 鼠标移动：处理拖动
void CanvasPanel::OnMouseMove(wxMouseEvent& evt) {
    if (m_isDragging && m_selectedIndex != -1) {
        wxPoint delta = evt.GetPosition() - m_dragStartPos;
        wxPoint newPos = m_elementStartPos + delta;
        m_elements[m_selectedIndex].SetPos(newPos);
        Refresh(); // 重绘以更新位置
    }
}

// 鼠标释放：结束拖动
void CanvasPanel::OnLeftUp(wxMouseEvent& evt) {
    m_isDragging = false;
}