#include <Editor/Editor.h>

int main()
{
    Editor editor;
    if (!editor.Init())
        return -1;

    editor.Run();
    editor.Shutdown();
    return 0;
}