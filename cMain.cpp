#include <wx/wx.h>
#include "MainFrame.h"

class MyApp : public wxApp
{
public:
    bool OnInit() override
    {
        (new MainFrame)->Show();
        return true;
    }
};
wxIMPLEMENT_APP(MyApp);