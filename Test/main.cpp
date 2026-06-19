#include <Runtime/EngineRuntime.h>
#include <Core/Services.h>
#include <Test/Scenes/DynamicsTest.h>
#include <Test/Scenes/GraphicsTest.h>
#include <string>

int main()
{
    EngineRuntime runtime;

    // Initialize runtime
    runtime.Init();
    
    // Setup scenes
    std::string defaultScene = "Physics";
    SceneManager* sceneManager = Services::Fetch<SceneManager>();
    sceneManager->CreateScene<DynamicsTest>("Physics");
    sceneManager->CreateScene<GraphicsTest>("Graphics");
    sceneManager->ChangeScene(defaultScene);

    // Update runtime
    runtime.Update();

    // Exit application
    runtime.Close();

    return 0;
}