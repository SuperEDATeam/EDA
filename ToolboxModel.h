#pragma once
#include <wx/vector.h>
#include <wx/string.h>

/* һ������ + ����Ԫ�� */
struct ToolCategory {
    wxString        name;
    wxVector<wxString> items;
};

/* �������������� + ���� main �б� */
wxVector<ToolCategory> GetToolboxCategories();
wxVector<wxString> GetMainElements();   // main �µ� 7 ������