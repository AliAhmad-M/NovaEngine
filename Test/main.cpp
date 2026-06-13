#include <Runtime/EngineRuntime.h>
#include <Test/Scenes/DynamicsTest.h>
#include <Test/Scenes/GraphicsTest.h>
#include <string>

int main()
{
    EngineRuntime runtime;

    // Initialize runtime
    runtime.Init();
    EngineContext* context = runtime.GetContext();
    
    // Setup scenes
    std::string defaultScene = "Graphics";
    SceneManager* sceneManager = context->Get<SceneManager>();
    sceneManager->CreateScene<DynamicsTest>("Physics", context);
    sceneManager->CreateScene<GraphicsTest>("Graphics", context);
    sceneManager->ChangeScene(defaultScene);

    // Update runtime
    runtime.Update();

    // Exit application
    runtime.Close();

    return 0;
}