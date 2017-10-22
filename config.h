#pragma once

//
// 各種設定
//

// 補助プログラム
#include "gg.h"
using namespace gg;

// カメラに対するオブジェクトの中心位置
const GLfloat objectCenter[] = { 0.0f, 0.0f, -0.0f };

// カメラパラメータ
const GLfloat cameraFovy(1.0f);                         // 画角
const GLfloat cameraNear(0.1f);                         // 前方面までの距離
const GLfloat cameraFar(50.0f);                         // 後方面までの距離

// マウス操作の係数
const double motionFactor[] = { 1.0, 1.0, 0.05 };

// 光源
const GgSimpleLight lightData =
{
  { 0.2f, 0.2f, 0.2f, 1.0f },                           // 環境光成分
  { 1.0f, 1.0f, 1.0f, 1.0f },                           // 拡散反射光成分
  { 1.0f, 1.0f, 1.0f, 1.0f },                           // 鏡面光成分
  { 0.0f, 0.0f, 5.0f, 1.0f }                            // 位置
};

// 材質
const GgSimpleMaterial materialData =
{
  { 0.8f, 0.8f, 0.8f, 1.0f },                           // 環境光の反射係数
  { 0.8f, 0.8f, 0.8f, 1.0f },                           // 拡散反射係数
  { 0.2f, 0.2f, 0.2f, 1.0f },                           // 鏡面反射係数
  50.0f                                                 // 輝き係数
};

// 背景色
const GLfloat background[] = { 0.2f, 0.3f, 0.4f, 0.0f };
