//
//  SimpleScene.h
//  BVHOQ
//
//  Created by dmitryk on 08.10.13.
//  Copyright (c) 2013 Dmitry Kozlov. All rights reserved.
//

#ifndef FIRERAYS_H
#define FIRERAYS_H

#include "CommonTypes.h"
#include "SceneBase.h"
#include "RenderBase.h"
#include "CameraBase.h"
#include "TextureBase.h"

std::unique_ptr<RenderBase> CreateRender();

#endif