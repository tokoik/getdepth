﻿#include "DepthCamera.h"
#include <iostream>
//
// デプスセンサ関連の基底クラス
//

// depthCount と colorCount を計算してテクスチャとバッファオブジェクトを作成する
void DepthCamera::makeTexture(int *depthCount, int *colorCount)
{
  // カラーデータの境界色
  static const GLfloat border[] = { 0.5f, 0.5f, 0.5f, 0.0f };

  // デプスデータとカラーデータのデータ数を求める
  *depthCount = depthWidth * depthHeight;
  *colorCount = colorWidth * colorHeight;

  // デプスデータを格納するテクスチャを準備する
  glGenTextures(1, &depthTexture);
  glBindTexture(GL_TEXTURE_2D, depthTexture);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_R16UI, depthWidth, depthHeight, 0, GL_RED_INTEGER, GL_UNSIGNED_SHORT, NULL);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

  // デプスデータから求めたカメラ座標を格納するテクスチャを準備する
  glGenTextures(1, &pointTexture);
  glBindTexture(GL_TEXTURE_2D, pointTexture);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, depthWidth, depthHeight, 0, GL_RGB, GL_FLOAT, NULL);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

  // カラーデータを格納するテクスチャを準備する
  glGenTextures(1, &colorTexture);
  glBindTexture(GL_TEXTURE_2D, colorTexture);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, colorWidth, colorHeight, 0, GL_BGRA, GL_UNSIGNED_BYTE, NULL);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, border);

  // デプスデータの画素位置のカラーのテクスチャ座標を格納するバッファオブジェクトを準備する
  glGenBuffers(1, &coordBuffer);
  glBindBuffer(GL_ARRAY_BUFFER, coordBuffer);
  glBufferData(GL_ARRAY_BUFFER, *depthCount * 2 * sizeof (GLfloat), nullptr, GL_DYNAMIC_DRAW);

  // バイラテラルフィルタの重みを格納するバッファオブジェクトを準備する
  glGenBuffers(1, &weightBuffer);
  glBindBuffer(GL_SHADER_STORAGE_BUFFER, weightBuffer);
  glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof (Weight), nullptr, GL_STATIC_DRAW);
}

// デストラクタ
DepthCamera::~DepthCamera()
{
  // テクスチャを削除する
  if (depthTexture > 0) glDeleteTextures(1, &depthTexture);
  if (pointTexture > 0) glDeleteTextures(1, &pointTexture);
  if (colorTexture > 0) glDeleteTextures(1, &colorTexture);

  // バッファオブジェクトを削除する
  if (coordBuffer > 0) glDeleteBuffers(1, &coordBuffer);
  if (weightBuffer > 0) glDeleteBuffers(1, &weightBuffer);
}

// バイラテラルフィルタの分散を設定する
void DepthCamera::setVariance(float columnVariance, float rowVariance, float valueVariance)
{
  // バイラテラルフィルタの距離に対する重み
  Weight weight;

  // バイラテラルフィルタの距離に対する桁方向の重みを求める
  const float offset1x(0.5f * static_cast<float>(weight.columnWeight.size() - 1));
  for (size_t i = 0; i < weight.columnWeight.size(); ++i)
  {
    const float d(static_cast<float>(i) - offset1x);
    weight.columnWeight[i] = exp(-0.5f * d * d / columnVariance);
  }

  // バイラテラルフィルタの距離に対する行方向の重みを求める
  const float offset1y(0.5f * static_cast<float>(weight.rowWeight.size() - 1));
  for (size_t i = 0; i < weight.rowWeight.size(); ++i)
  {
    const float d(static_cast<float>(i) - offset1y);
    weight.rowWeight[i] = exp(-0.5f * d * d / rowVariance);
  }

  // バイラテラルフィルタの値に対する分散を設定する
  weight.variance = valueVariance;

  // バイラテラルフィルタの距離に対する重みと値に対する分散を転送する
  glBindBuffer(GL_SHADER_STORAGE_BUFFER, weightBuffer);
  glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof weight, &weight);
}
