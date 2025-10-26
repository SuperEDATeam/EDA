#include <wx/wx.h>
#include <wx/artprov.h>
#include <wx/toolbar.h>	
#include "ToolBars.h"
#include "MainFrame.h"
#include <wx/event.h>

ToolBars::ToolBars(MainFrame* owner)
	: m_owner(owner) {
	wxInitAllImageHandlers();
	//分配ID
	ArrangeIds(); // 分配 ID
	toolBar1 = CreateToolBar1(); // 创建工具栏 1
	toolBar2 = CreateToolBar2(); // 创建工具栏 2
	toolBar3 = CreateToolBar3(); // 创建工具栏 3
}


// 分配工具栏工具的 ID 和路径
void ToolBars::ArrangeIds() {
	// 初始化工具栏 1 的 ID 和路径
	toolBar1_ids.resize(17); // 工具栏 1 有 17 个工具
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
	//实现ID对方法MAP
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

	// 初始化工具栏 2 的 ID 和路径
	toolBar2_ids.resize(4); // 工具栏 2 有 4 个工具
	for (int i = 0; i < 4; ++i) {
		toolBar2_ids[i] = wxNewId();
	}
	toolBar2_toolPaths = {
		"res\\tools.png", "res\\branch.png", "res\\map.png", "res\\draw.png"
	};
	toolBar2_labels = {
		"Tools", "Branch", "Map", "Draw"
	};
	//实现ID对方法MAP
	toolIdToFunctionMap[toolBar2_ids[0]] = [this](int id) { ChoosePageOne_toolBar3(id); };
	toolIdToFunctionMap[toolBar2_ids[1]] = [this](int id) { ChoosePageTwo_toolBar3(id); };
	toolIdToFunctionMap[toolBar2_ids[2]] = [this](int id) { ChoosePageOne_toolBar1(id); };
	toolIdToFunctionMap[toolBar2_ids[3]] = [this](int id) { ChoosePageTwo_toolBar1(id); };

	// 初始化工具栏 3 的 ID 和路径
	toolBar3_ids.resize(8); // 工具栏 3 有 8 个工具
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



// 实现样式
wxToolBar* ToolBars::CreateToolBar1() {
	wxToolBar* toolbar = new wxToolBar(m_owner, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTB_FLAT);
	toolbar->SetToolBitmapSize(wxSize(24, 24)); // 设置图标大小
	//添加工具栏 1 的工具
	for (size_t i = 0; i < toolBar1_ids.size(); ++i) {
		wxBitmap bitmap(toolBar1_toolPaths[i], wxBITMAP_TYPE_PNG);
		//ShowTool(toolbar, toolBar1_ids[i], toolBar1_labels[i], bitmap);
		toolbar->AddCheckTool(toolBar1_ids[i], toolBar1_labels[i], bitmap);
		toolbar->Bind(wxEVT_TOOL, &ToolBars::OnToolClicked, this, toolBar1_ids[i]);
	}

	return toolbar;
}

// 创建工具栏 2
wxToolBar* ToolBars::CreateToolBar2() {
	wxToolBar* toolbar = new wxToolBar(m_owner, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTB_FLAT);
	toolbar->SetToolBitmapSize(wxSize(24, 24)); // 设置图标大小
	wxInitAllImageHandlers();
	// 添加工具栏 2 的工具
	for (size_t i = 0; i < toolBar2_ids.size(); ++i) {
		wxBitmap bitmap(toolBar2_toolPaths[i], wxBITMAP_TYPE_PNG);
		//ShowTool(toolbar, toolBar2_ids[i], toolBar2_labels[i], wxBitmap(toolBar2_toolPaths[i], wxBITMAP_TYPE_PNG));
		toolbar->AddCheckTool(toolBar2_ids[i], toolBar2_labels[i], bitmap);
		toolbar->Bind(wxEVT_TOOL, &ToolBars::OnToolClicked, this, toolBar2_ids[i]);
	}

	return toolbar;
}

// 创建工具栏 3
wxToolBar* ToolBars::CreateToolBar3() {
	wxToolBar* toolbar = new wxToolBar(m_owner, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTB_FLAT);
	toolbar->SetToolBitmapSize(wxSize(24, 24)); // 设置图标大小

	// 添加工具栏 3 的工具
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
	// 析构函数的实现
	// 如果没有需要清理的资源，可以保持为空
}

void ToolBars::OnToolClicked(wxCommandEvent& event) {
	int toolId = event.GetId(); // 获取被点击的工具的 ID

	// 查找对应的工具 ID 并调用其方法
	auto it = toolIdToFunctionMap.find(toolId);
	if (it != toolIdToFunctionMap.end()) {
		it->second(toolId); // 调用对应的方法
	}

	// 其他逻辑...
}

void ToolBars::OneChoose(int toolId) {
	// 判断工具 ID 属于哪个工具栏
	for (size_t i = 0; i < toolBar1_ids.size(); ++i) {
		if (toolBar1_ids[i] == toolId) {
			// 工具栏 1 的工具被点击
			// 遍历工具栏 1 的所有工具，取消其他工具的选择
			for (size_t i = 0; i < toolBar1_ids.size(); ++i) {
				if (toolBar1_ids[i] != toolId) {
					toolBar1->ToggleTool(toolBar1_ids[i], false); // 取消其他工具的选择
				}
			}
			break;
		}
	}
	for (size_t i = 0; i < toolBar2_ids.size(); ++i) {
		if (toolBar2_ids[i] == toolId) {
			// 工具栏 2 的工具被点击
			// 遍历工具栏 2 的所有工具，取消其他工具的选择
			for (size_t i = 0; i < toolBar2_ids.size(); ++i) {
				if (toolBar2_ids[i] != toolId) {
					toolBar2->ToggleTool(toolBar2_ids[i], false); // 取消其他工具的选择
				}
			}
			break;
		}
	}

	for (size_t i = 0; i < toolBar3_ids.size(); ++i) {
		if (toolBar3_ids[i] == toolId) {
			// 工具栏 3 的工具被点击
			// 遍历工具栏 3 的所有工具，取消其他工具的选择
			for (size_t i = 0; i < toolBar3_ids.size(); ++i) {
				if (toolBar3_ids[i] != toolId) {
					toolBar3->ToggleTool(toolBar3_ids[i], false); // 取消其他工具的选择
				}
			}
			break;
		}
	}
}

void ToolBars::ChoosePageOne_toolBar1(int toolId) {
	//工具栏3的第一页：toolBar1_ids[0] 到 toolBar3_ids[7]
	for (size_t i = 0; i < toolBar1_ids.size(); ++i) {
		// 如果是第一页的工具，则设置为启用状态
		HideTool(toolBar1, toolBar1_ids[i]);
		if (i <= 7) {
			ShowTool(toolBar1, toolBar1_ids[i], toolBar1_labels[i], wxBitmap(toolBar1_toolPaths[i], wxBITMAP_TYPE_PNG));
		}

		//HideTool(toolBar1, toolBar1_ids[0]);
		//HideTool(toolBar1, toolBar1_ids[1]);
		//HideTool(toolBar1, toolBar1_ids[2]);
		//HideTool(toolBar1, toolBar1_ids[3]);
	}
	OneChoose(toolBar2_ids[0]); // 默认选择第一个工具
}

void ToolBars::ChoosePageTwo_toolBar1(int toolId) {
	// 工具栏3的第二页：toolBar3_ids[7] 到 toolBar3_ids[15]
	for (size_t i = 0; i < toolBar1_ids.size(); ++i) {
		// 如果是第二页的工具，则设置为启用状态
		HideTool(toolBar1, toolBar1_ids[i]);
		if (i >= 8) {
			ShowTool(toolBar1, toolBar1_ids[i], toolBar1_labels[i], wxBitmap(toolBar1_toolPaths[i], wxBITMAP_TYPE_PNG));
		}
	}
	OneChoose(toolBar2_ids[1]);
}


void ToolBars::ChoosePageOne_toolBar3(int toolId) {
	// 工具栏3的第一页：toolBar3_ids[0] 到 toolBar3_ids[3]
	for (size_t i = 0; i < toolBar3_ids.size(); ++i) {
		// 如果是第一页的工具，则设置为启用状态
		HideTool(toolBar3, toolBar3_ids[i]);
		if (i <= 3) {
			ShowTool(toolBar3, toolBar3_ids[i], toolBar3_labels[i], wxBitmap(toolBar3_toolPaths[i], wxBITMAP_TYPE_PNG));
		}
	}
	OneChoose(toolBar2_ids[2]);
}

void ToolBars::ChoosePageTwo_toolBar3(int toolId) {
	// 工具栏3的第二页：toolBar3_ids[4] 到 toolBar3_ids[7]
	for (size_t i = 0; i < toolBar3_ids.size(); ++i) {
		// 如果是第二页的工具，则设置为启用状态
		HideTool(toolBar3, toolBar3_ids[i]);
		if (i >= 4) {
			ShowTool(toolBar3, toolBar3_ids[i], toolBar3_labels[i], wxBitmap(toolBar3_toolPaths[i], wxBITMAP_TYPE_PNG));
		}
	}
	OneChoose(toolBar2_ids[3]);
}


void ToolBars::HideTool(wxToolBar* toolbar, int toolId) {
	// 移除工具
	toolbar->DeleteTool(toolId);
	toolbar->Realize(); // 重新布局工具栏
}

void ToolBars::ShowTool(wxToolBar* toolbar, int toolId, const wxString& label, const wxBitmap& bitmap) {
	// 添加工具
	toolbar->AddCheckTool(toolId, label, bitmap);
	toolbar->Bind(wxEVT_TOOL, &ToolBars::OnToolClicked, this, toolId);
	toolbar->Realize(); // 重新布局工具栏
}
