//#include <wx/wx.h>
//#include "MainFrame.h"
//
//class MyApp : public wxApp
//{
//public:
//    bool OnInit() override
//    {
//        (new MainFrame)->Show();
//        return true;
//    }
//};
//wxIMPLEMENT_APP(MyApp);

#include <wx/wx.h>
#include "MainFrame.h"
#include <wx/log.h>
#include <fstream>
#include "my_log.h"

class MyApp : public wxApp
{
public:
    bool OnInit() override
    {

        MyLog("===== APP STARTED =====\n");
        (new MainFrame)->Show();
        return true;
    }
};
wxIMPLEMENT_APP(MyApp);