#pragma once
#include <functional>
#include <wx/string.h>

class UndoNotifier {
public:
    using Callback = std::function<void(const wxString& name, bool canUndo)>;

    static void Subscribe(Callback cb);
    static void Notify(const wxString& name, bool canUndo);

private:
    static Callback s_callback;
};