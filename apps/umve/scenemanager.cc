#include "scenemanager.h"

SceneManager::SceneManager (void)
{
}

SceneManager::~SceneManager (void)
{
}

SceneManager&
SceneManager::get (void)
{
    static SceneManager instance;
    return instance;
}
