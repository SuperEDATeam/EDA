#pragma once
#include <wx/wx.h>
#include <vector>
#include <json/json.h>   

// ǰ������������ѭ������
class CanvasElement;

// ȫ�ֺ������� JSON �� ����Ԫ���б�
std::vector<CanvasElement> LoadCanvasElements(const wxString& jsonPath);