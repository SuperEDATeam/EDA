#include "ToolboxModel.h"
#include <wx/filename.h>
#include <wx/stdpaths.h>
#include <fstream>
#include <json/json.h>

static wxString GetJsonPath()
{
    return wxFileName(wxStandardPaths::Get().GetExecutablePath())
        .GetPath() + wxFileName::GetPathSeparator() + "tools.json";
}

static bool ParseJson(const wxString& path,
    wxVector<ToolCategory>& catsOut,
    wxVector<wxString>& /*mainOut*/)
{
    std::ifstream f(path.ToStdString(), std::ios::binary);
    if (!f) return false;

    Json::Value root;
    Json::CharReaderBuilder builder;
    std::string errs;
    if (!Json::parseFromStream(builder, f, &root, &errs)) return false;

    for (const auto& node : root) {
        ToolCategory cat;
        cat.name = wxString::FromUTF8(node["name"].asString());
        for (const auto& item : node["items"])
            cat.items.push_back(wxString::FromUTF8(item.asString()));
        catsOut.push_back(cat);
    }
    return true;
}

wxVector<ToolCategory> GetToolboxCategories()
{
    wxVector<ToolCategory> v;
    wxVector<wxString> tmp;  
    ParseJson(GetJsonPath(), v, tmp);
    return v;
}

wxVector<wxString> GetMainElements()
{
    return {}; 
}