#include <wx/wx.h>
#include <wx/artprov.h>
#include <wx/toolbar.h>	
#include "ToolBars.h"
#include "MainFrame.h"
#include <wx/event.h>

ToolBars::ToolBars(MainFrame* owner)
	: m_owner(owner) {
	wxInitAllImageHandlers();
	//����ID
	ArrangeIds(); // ���� ID
	toolBar1 = CreateToolBar1(); // ���������� 1
	toolBar2 = CreateToolBar2(); // ���������� 2
	toolBar3 = CreateToolBar3(); // ���������� 3
}


// ���乤�������ߵ� ID ��·��
void ToolBars::ArrangeIds() {
	// ��ʼ�������� 1 �� ID ��·��
	toolBar1_ids.resize(17); // ������ 1 �� 17 ������
	for (int i = 0; i < 17; ++i) {
		toolBar1_ids[i] = wxNewId();
	}
	toolBar1_toolPaths = {
		"res\\drag.png",       "res\\choose.png",    "res\\text.png",
		"res\\east_pin.png",   "res\\west_pin.png",  "res\\NOT_Gate.png",
		"res\\AND_Gate.png",   "res\\OR_Gate.png",   "res\\choose.png",
		"res\\text.png",       "res\\line.png",      "res\\curve.png",
		"res\\polyline.png",   "res\\retangle.png",  "res\\rounded_retangle.png",
		"res\\oval.png",       "res\\polygon.png"
	};

	toolBar1_labels = {
		"Drag", "Choose", "Text", "East Pin", "West Pin", "NOT Gate", "AND Gate", "OR Gate",
		"Choose", "Text", "Line", "Curve", "Polyline", "Retangle", "Rounded Retangle", "Oval", "Polygon"
	};
	//ʵ��ID�Է���MAP
	toolIdToFunctionMap[toolBar1_ids[0]] = [this](int id) { OneChoose(id); };
	toolIdToFunctionMap[toolBar1_ids[1]] = [this](int id) { OneChoose(id); };
	toolIdToFunctionMap[toolBar1_ids[2]] = [this](int id) { OneChoose(id); };
	toolIdToFunctionMap[toolBar1_ids[3]] = [this](int id) { OneChoose(id); };
	toolIdToFunctionMap[toolBar1_ids[4]] = [this](int id) { OneChoose(id); };
	toolIdToFunctionMap[toolBar1_ids[5]] = [this](int id) { OneChoose(id); };
	toolIdToFunctionMap[toolBar1_ids[6]] = [this](int id) { OneChoose(id); };
	toolIdToFunctionMap[toolBar1_ids[7]] = [this](int id) { OneChoose(id); };
	toolIdToFunctionMap[toolBar1_ids[8]] = [this](int id) { OneChoose(id); };
	toolIdToFunctionMap[toolBar1_ids[9]] = [this](int id) { OneChoose(id); };
	toolIdToFunctionMap[toolBar1_ids[10]] = [this](int id) { OneChoose(id); };
	toolIdToFunctionMap[toolBar1_ids[11]] = [this](int id) { OneChoose(id); };
	toolIdToFunctionMap[toolBar1_ids[12]] = [this](int id) { OneChoose(id); };
	toolIdToFunctionMap[toolBar1_ids[13]] = [this](int id) { OneChoose(id); };
	toolIdToFunctionMap[toolBar1_ids[14]] = [this](int id) { OneChoose(id); };
	toolIdToFunctionMap[toolBar1_ids[15]] = [this](int id) { OneChoose(id); };
	toolIdToFunctionMap[toolBar1_ids[16]] = [this](int id) { OneChoose(id); };

	// ��ʼ�������� 2 �� ID ��·��
	toolBar2_ids.resize(4); // ������ 2 �� 4 ������
	for (int i = 0; i < 4; ++i) {
		toolBar2_ids[i] = wxNewId();
	}
	toolBar2_toolPaths = {
		"res\\tools.png", "res\\branch.png", "res\\map.png", "res\\draw.png"
	};
	toolBar2_labels = {
		"Tools", "Branch", "Map", "Draw"
	};
	//ʵ��ID�Է���MAP
	toolIdToFunctionMap[toolBar2_ids[0]] = [this](int id) { ChoosePageOne_toolBar3(id); };
	toolIdToFunctionMap[toolBar2_ids[1]] = [this](int id) { ChoosePageTwo_toolBar3(id); };
	toolIdToFunctionMap[toolBar2_ids[2]] = [this](int id) { ChoosePageOne_toolBar1(id); };
	toolIdToFunctionMap[toolBar2_ids[3]] = [this](int id) { ChoosePageTwo_toolBar1(id); };

	// ��ʼ�������� 3 �� ID ��·��
	toolBar3_ids.resize(8); // ������ 3 �� 8 ������
	for (int i = 0; i < 8; ++i) {
		toolBar3_ids[i] = wxNewId();
	}
	toolBar3_toolPaths = {
		"res\\plus.png", "res\\up.png", "res\\down.png", "res\\wrong.png",
		"res\\start.png", "res\\3.png", "res\\2.png", "res\\1.png"
	};
	toolBar3_labels = {
		"Plus", "Up", "Down", "Wrong", "1", "2", "3", "4"
	};
	toolIdToFunctionMap[toolBar3_ids[0]] = [this](int id) { OneChoose(id); };
	toolIdToFunctionMap[toolBar3_ids[1]] = [this](int id) { OneChoose(id); };
	toolIdToFunctionMap[toolBar3_ids[2]] = [this](int id) { OneChoose(id); };
	toolIdToFunctionMap[toolBar3_ids[3]] = [this](int id) { OneChoose(id); };
	toolIdToFunctionMap[toolBar3_ids[4]] = [this](int id) { OneChoose(id); };
	toolIdToFunctionMap[toolBar3_ids[5]] = [this](int id) { OneChoose(id); };
	toolIdToFunctionMap[toolBar3_ids[6]] = [this](int id) { OneChoose(id); };
	toolIdToFunctionMap[toolBar3_ids[7]] = [this](int id) { OneChoose(id); };
}



// ʵ����ʽ
wxToolBar* ToolBars::CreateToolBar1() {
	wxToolBar* toolbar = new wxToolBar(m_owner, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTB_FLAT);
	toolbar->SetToolBitmapSize(wxSize(24, 24)); // ����ͼ���С
	//��ӹ����� 1 �Ĺ���
	for (size_t i = 0; i < toolBar1_ids.size(); ++i) {
		wxBitmap bitmap(toolBar1_toolPaths[i], wxBITMAP_TYPE_PNG);
		//ShowTool(toolbar, toolBar1_ids[i], toolBar1_labels[i], bitmap);
		toolbar->AddCheckTool(toolBar1_ids[i], toolBar1_labels[i], bitmap);
		toolbar->Bind(wxEVT_TOOL, &ToolBars::OnToolClicked, this, toolBar1_ids[i]);
	}

	return toolbar;
}

// ���������� 2
wxToolBar* ToolBars::CreateToolBar2() {
	wxToolBar* toolbar = new wxToolBar(m_owner, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTB_FLAT);
	toolbar->SetToolBitmapSize(wxSize(24, 24)); // ����ͼ���С
	wxInitAllImageHandlers();
	// ��ӹ����� 2 �Ĺ���
	for (size_t i = 0; i < toolBar2_ids.size(); ++i) {
		wxBitmap bitmap(toolBar2_toolPaths[i], wxBITMAP_TYPE_PNG);
		//ShowTool(toolbar, toolBar2_ids[i], toolBar2_labels[i], wxBitmap(toolBar2_toolPaths[i], wxBITMAP_TYPE_PNG));
		toolbar->AddCheckTool(toolBar2_ids[i], toolBar2_labels[i], bitmap);
		toolbar->Bind(wxEVT_TOOL, &ToolBars::OnToolClicked, this, toolBar2_ids[i]);
	}

	return toolbar;
}

// ���������� 3
wxToolBar* ToolBars::CreateToolBar3() {
	wxToolBar* toolbar = new wxToolBar(m_owner, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTB_FLAT);
	toolbar->SetToolBitmapSize(wxSize(24, 24)); // ����ͼ���С

	// ��ӹ����� 3 �Ĺ���
	wxInitAllImageHandlers();
	for (size_t i = 0; i < toolBar3_ids.size(); ++i) {
		wxBitmap bitmap(toolBar3_toolPaths[i], wxBITMAP_TYPE_PNG);
		//ShowTool(toolbar, toolBar3_ids[i], toolBar3_labels[i], bitmap);
		toolbar->AddCheckTool(toolBar3_ids[i], toolBar3_labels[i], bitmap);
		toolbar->Bind(wxEVT_TOOL, &ToolBars::OnToolClicked, this, toolBar3_ids[i]);
	}

	return toolbar;
}

ToolBars::~ToolBars() {
	// ����������ʵ��
	// ���û����Ҫ�������Դ�����Ա���Ϊ��
}

void ToolBars::OnToolClicked(wxCommandEvent& event) {
	int toolId = event.GetId(); // ��ȡ������Ĺ��ߵ� ID

	// ���Ҷ�Ӧ�Ĺ��� ID �������䷽��
	auto it = toolIdToFunctionMap.find(toolId);
	if (it != toolIdToFunctionMap.end()) {
		it->second(toolId); // ���ö�Ӧ�ķ���
	}

	// �����߼�...
}

void ToolBars::OneChoose(int toolId) {
	// �жϹ��� ID �����ĸ�������
	for (size_t i = 0; i < toolBar1_ids.size(); ++i) {
		if (toolBar1_ids[i] == toolId) {
			// ������ 1 �Ĺ��߱����
			// ���������� 1 �����й��ߣ�ȡ���������ߵ�ѡ��
			for (size_t i = 0; i < toolBar1_ids.size(); ++i) {
				if (toolBar1_ids[i] != toolId) {
					toolBar1->ToggleTool(toolBar1_ids[i], false); // ȡ���������ߵ�ѡ��
				}
			}
			break;
		}
	}
	for (size_t i = 0; i < toolBar2_ids.size(); ++i) {
		if (toolBar2_ids[i] == toolId) {
			// ������ 2 �Ĺ��߱����
			// ���������� 2 �����й��ߣ�ȡ���������ߵ�ѡ��
			for (size_t i = 0; i < toolBar2_ids.size(); ++i) {
				if (toolBar2_ids[i] != toolId) {
					toolBar2->ToggleTool(toolBar2_ids[i], false); // ȡ���������ߵ�ѡ��
				}
			}
			break;
		}
	}

	for (size_t i = 0; i < toolBar3_ids.size(); ++i) {
		if (toolBar3_ids[i] == toolId) {
			// ������ 3 �Ĺ��߱����
			// ���������� 3 �����й��ߣ�ȡ���������ߵ�ѡ��
			for (size_t i = 0; i < toolBar3_ids.size(); ++i) {
				if (toolBar3_ids[i] != toolId) {
					toolBar3->ToggleTool(toolBar3_ids[i], false); // ȡ���������ߵ�ѡ��
				}
			}
			break;
		}
	}
}

void ToolBars::ChoosePageOne_toolBar1(int toolId) {
	//������3�ĵ�һҳ��toolBar1_ids[0] �� toolBar3_ids[7]
	for (size_t i = 0; i < toolBar1_ids.size(); ++i) {
		// ����ǵ�һҳ�Ĺ��ߣ�������Ϊ����״̬
		HideTool(toolBar1, toolBar1_ids[i]);
		if (i <= 7) {
			ShowTool(toolBar1, toolBar1_ids[i], toolBar1_labels[i], wxBitmap(toolBar1_toolPaths[i], wxBITMAP_TYPE_PNG));
		}

		//HideTool(toolBar1, toolBar1_ids[0]);
		//HideTool(toolBar1, toolBar1_ids[1]);
		//HideTool(toolBar1, toolBar1_ids[2]);
		//HideTool(toolBar1, toolBar1_ids[3]);
	}
	OneChoose(toolBar2_ids[0]); // Ĭ��ѡ���һ������
}

void ToolBars::ChoosePageTwo_toolBar1(int toolId) {
	// ������3�ĵڶ�ҳ��toolBar3_ids[7] �� toolBar3_ids[15]
	for (size_t i = 0; i < toolBar1_ids.size(); ++i) {
		// ����ǵڶ�ҳ�Ĺ��ߣ�������Ϊ����״̬
		HideTool(toolBar1, toolBar1_ids[i]);
		if (i >= 8) {
			ShowTool(toolBar1, toolBar1_ids[i], toolBar1_labels[i], wxBitmap(toolBar1_toolPaths[i], wxBITMAP_TYPE_PNG));
		}
	}
	OneChoose(toolBar2_ids[1]);
}


void ToolBars::ChoosePageOne_toolBar3(int toolId) {
	// ������3�ĵ�һҳ��toolBar3_ids[0] �� toolBar3_ids[3]
	for (size_t i = 0; i < toolBar3_ids.size(); ++i) {
		// ����ǵ�һҳ�Ĺ��ߣ�������Ϊ����״̬
		HideTool(toolBar3, toolBar3_ids[i]);
		if (i <= 3) {
			ShowTool(toolBar3, toolBar3_ids[i], toolBar3_labels[i], wxBitmap(toolBar3_toolPaths[i], wxBITMAP_TYPE_PNG));
		}
	}
	OneChoose(toolBar2_ids[2]);
}

void ToolBars::ChoosePageTwo_toolBar3(int toolId) {
	// ������3�ĵڶ�ҳ��toolBar3_ids[4] �� toolBar3_ids[7]
	for (size_t i = 0; i < toolBar3_ids.size(); ++i) {
		// ����ǵڶ�ҳ�Ĺ��ߣ�������Ϊ����״̬
		HideTool(toolBar3, toolBar3_ids[i]);
		if (i >= 4) {
			ShowTool(toolBar3, toolBar3_ids[i], toolBar3_labels[i], wxBitmap(toolBar3_toolPaths[i], wxBITMAP_TYPE_PNG));
		}
	}
	OneChoose(toolBar2_ids[3]);
}


void ToolBars::HideTool(wxToolBar* toolbar, int toolId) {
	// �Ƴ�����
	toolbar->DeleteTool(toolId);
	toolbar->Realize(); // ���²��ֹ�����
}

void ToolBars::ShowTool(wxToolBar* toolbar, int toolId, const wxString& label, const wxBitmap& bitmap) {
	// ��ӹ���
	toolbar->AddCheckTool(toolId, label, bitmap);
	toolbar->Bind(wxEVT_TOOL, &ToolBars::OnToolClicked, this, toolId);
	toolbar->Realize(); // ���²��ֹ�����
}
