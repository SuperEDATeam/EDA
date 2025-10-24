#pragma once
#include <wx/vector.h>
#include <wx/string.h>

/* 一级分类 + 二级元件 */
struct ToolCategory {
    wxString        name;
    wxVector<wxString> items;
};

/* 返回整个树数据 + 特殊 main 列表 */
wxVector<ToolCategory> GetToolboxCategories();
wxVector<wxString> GetMainElements();   // main 下的 7 个工具