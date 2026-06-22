#include <Runtime/Engine.h>
#include <Core/Services.h>
#include <string>

using namespace Nova;

int main()
{
    Engine runtime;

    // Initialize runtime
    runtime.Init();
    
    // Setup scenes
    std::string defaultScene = "project/scenes/SampleScene.nova";
    SceneManager* sceneManager = Services::Fetch<SceneManager>();
    sceneManager->ChangeScene(defaultScene);

    // Update runtime
    runtime.Update();

    // Exit application
    runtime.Close();

    return 0;
}