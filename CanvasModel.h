#pragma once
#include <wx/wx.h>
#include <vector>
#include "3rd/json/json.h"

// ǰ������������ѭ������
class CanvasElement;

// ȫ�ֺ������� JSON �� ����Ԫ���б�
std::vector<CanvasElement> LoadCanvasElements(const wxString& jsonPath);