#pragma once
#include <wx/wx.h>
#include <vector>

enum class CPType { Pin, Bend, Free };

struct ControlPoint {
    wxPoint  pos;
    CPType   type = CPType::Free;
    // �����������ţ�����չ��Ԫ��ָ��/pin����
};

class Wire {
public:
    std::vector<ControlPoint> pts;

    Wire() = default;
    explicit Wire(std::vector<ControlPoint> v) : pts(std::move(v)) {}

    // ���Ľӿ�
    void Draw(wxDC& dc) const;                          // ����
    void AddPoint(const ControlPoint& cp) { pts.push_back(cp); }
    void Clear() { pts.clear(); }
    bool Empty() const { return pts.empty(); }
    size_t Size() const { return pts.size(); }

    // ����·�ɣ������յ� �����۵�
    static std::vector<ControlPoint> RouteOrtho(const wxPoint& a, const wxPoint& b);
};