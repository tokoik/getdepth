#include "DepthCamera.h"

//
// 深度センサ関連の基底クラス
//

// depthCount と colorCount を計算してテクスチャとバッファオブジェクトを作成する
void DepthCamera::makeTexture()
{
  // デプスデータとカラーデータのデータ数を求める
  depthCount = depthWidth * depthHeight;
  colorCount = colorWidth * colorHeight;

  // デプスデータを格納するテクスチャを準備する
  glGenTextures(1, &depthTexture);
  glBindTexture(GL_TEXTURE_2D, depthTexture);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, depthWidth, depthHeight, 0, GL_RED, GL_FLOAT, NULL);
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

  // デプスデータの画素位置のカラーのテクスチャ座標を格納するバッファオブジェクトを準備する
  glGenBuffers(1, &coordBuffer);
  glBindBuffer(GL_ARRAY_BUFFER, coordBuffer);
  glBufferData(GL_ARRAY_BUFFER, depthCount * 2 * sizeof (GLfloat), nullptr, GL_DYNAMIC_DRAW);
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
}
