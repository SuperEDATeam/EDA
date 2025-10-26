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

class MainFrame;  // ǰ������

class ToolBars {
public:
	explicit ToolBars(MainFrame* owner);
	~ToolBars();

	// ���������ߵ� ID
	std::vector<int>  toolBar1_ids; // ������ 1 �� ID
	std::vector<int>  toolBar2_ids;  // ������ 2 �� ID
	std::vector<int>  toolBar3_ids;  // ������ 3 �� ID

	// ���������ߵ�ͼ��·��
	std::vector<std::string> toolBar1_toolPaths; // ������ 1 ��ͼ��·��
	std::vector<std::string> toolBar2_toolPaths; // ������ 2 ��ͼ��·��
	std::vector<std::string> toolBar3_toolPaths; // ������ 3 ��ͼ��·��

	// ���������ߵı�ǩ
	std::vector<std::string> toolBar1_labels; // ������ 1 �ı�ǩ
	std::vector<std::string> toolBar2_labels; // ������ 2 �ı�ǩ
	std::vector<std::string> toolBar3_labels; // ������ 3 �ı�ǩ

	//ID�Է���MAP
	std::map<int, std::function<void(int)>> toolIdToFunctionMap;

	MainFrame* m_owner; // ָ�������ڵ�ָ��

	wxToolBar* toolBar1; // ������ 1
	wxToolBar* toolBar2; // ������ 2
	wxToolBar* toolBar3; // ������ 3

	/* ������������������ */
	wxToolBar* CreateToolBar1();
	wxToolBar* CreateToolBar2();
	wxToolBar* CreateToolBar3();

	/* ��������ʽʵ�ֵĻ������� */
	void ArrangeIds(void);
	void OnToolClicked(wxCommandEvent& event);
	void HideTool(wxToolBar* toolbar, int toolId);
	void ShowTool(wxToolBar* toolbar, int toolId, const wxString& label, const wxBitmap& bitmap);
	void OneChoose(int toolId);
	void ChoosePageOne_toolBar1(int toolId);
	void ChoosePageTwo_toolBar1(int toolId);
	void ChoosePageOne_toolBar3(int toolId);
	void ChoosePageTwo_toolBar3(int toolId);

	/* ����������ʵ�ֻ������� */



	DECLARE_EVENT_TABLE()
};

#endif
