#ifndef TOOLBARS_H
#define TOOLBARS_H

#include <wx/wx.h>
#include <wx/artprov.h>
#include <wx/toolbar.h>	
#include <vector>
#include <string>
#include <wx/event.h>
#include <map>
#include <functional>

class MainFrame;  // 前向声明

class ToolBars {
public:
	explicit ToolBars(MainFrame* owner);
	~ToolBars();

	// 工具栏工具的 ID
	std::vector<int>  toolBar1_ids; // 工具栏 1 的 ID
	std::vector<int>  toolBar2_ids;  // 工具栏 2 的 ID
	std::vector<int>  toolBar3_ids;  // 工具栏 3 的 ID

	// 工具栏工具的图标路径
	std::vector<std::string> toolBar1_toolPaths; // 工具栏 1 的图标路径
	std::vector<std::string> toolBar2_toolPaths; // 工具栏 2 的图标路径
	std::vector<std::string> toolBar3_toolPaths; // 工具栏 3 的图标路径

	// 工具栏工具的标签
	std::vector<std::string> toolBar1_labels; // 工具栏 1 的标签
	std::vector<std::string> toolBar2_labels; // 工具栏 2 的标签
	std::vector<std::string> toolBar3_labels; // 工具栏 3 的标签

	//ID对方法MAP
	std::map<int, std::function<void(int)>> toolIdToFunctionMap;

	MainFrame* m_owner; // 指向主窗口的指针

	wxToolBar* toolBar1; // 工具栏 1
	wxToolBar* toolBar2; // 工具栏 2
	wxToolBar* toolBar3; // 工具栏 3

	/* 三个工具栏创建函数 */
	wxToolBar* CreateToolBar1();
	wxToolBar* CreateToolBar2();
	wxToolBar* CreateToolBar3();

	/* 工具栏样式实现的基本函数 */
	void ArrangeIds(void);
	void OnToolClicked(wxCommandEvent& event);
	void HideTool(wxToolBar* toolbar, int toolId);
	void ShowTool(wxToolBar* toolbar, int toolId, const wxString& label, const wxBitmap& bitmap);
	void OneChoose(int toolId);
	void ChoosePageOne_toolBar1(int toolId);
	void ChoosePageTwo_toolBar1(int toolId);
	void ChoosePageOne_toolBar3(int toolId);
	void ChoosePageTwo_toolBar3(int toolId);

	/* 工具栏功能实现基本函数 */



	DECLARE_EVENT_TABLE()
};

#endif
