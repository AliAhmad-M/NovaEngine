#include <Editor/EditorApp.h>

using namespace Nova::Editor;

int main()
{
    EditorApp editor;
    if (!editor.Init())
        return -1;

    editor.Run();
    editor.Shutdown();
    return 0;
}