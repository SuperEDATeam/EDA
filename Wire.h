#pragma once
#include <wx/wx.h>
#include <vector>

enum class CPType { Pin, Bend, Free };

struct ControlPoint {
    wxPoint  pos;
    CPType   type = CPType::Free;
    // �����������ţ�����չ��Ԫ��ָ��/pin����
};

/* ����-�������Ӽ�¼ */
struct WireWireAnchor {
    size_t srcWire;   // �µ�������
    size_t srcPt;     // 0 �� pts.size()-1
    size_t dstWire;   // �����ӵĵ�������
    size_t dstCell;   // �����ӵ�С�������
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

    //����������·�ɣ�Manhattan Wiring�����洫ͳ������·��
    static std::vector<ControlPoint> RouteOrtho(const wxPoint& a, const wxPoint& b);

    std::vector<wxPoint> cells;          // ÿ 2 px С������
    void GenerateCells();                // һ�����з�
private:

};