#include "UndoNotifier.h"

UndoNotifier::Callback UndoNotifier::s_callback;

void UndoNotifier::Subscribe(Callback cb) {
    s_callback = cb;
}

void UndoNotifier::Notify(const wxString& name, bool canUndo) {
    if (s_callback) s_callback(name, canUndo);
}