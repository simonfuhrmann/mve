/*
 * Copyright (C) 2015, Simon Fuhrmann
 * TU Darmstadt - Graphics, Capture and Massively Parallel Computing
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD 3-Clause license. See the LICENSE.txt file for details.
 */

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
