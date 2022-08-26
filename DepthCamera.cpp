﻿#include "DepthCamera.h"
#include <iostream>
//
// デプスセンサ関連の基底クラス
//

// コンストラクタ
DepthCamera::DepthCamera(int depthWidth, int depthHeight, GLfloat depthFovx, GLfloat depthFovy,
  int colorWidth, int colorHeight, GLfloat colorFovx, GLfloat colorFovy)
: message(nullptr)
, depthTexture{ 0 }, smoothTexture{ 0 }, pointTexture{ 0 }, colorTexture{ 0 }
, uvmapBuffer{ 0 }, weightBuffer{ 0 }, normalBuffer{ 0 }
, depthWidth{ depthWidth }, depthHeight{ depthHeight }
, depthFov{ depthFovx, depthFovy }
, colorWidth{ colorWidth }, colorHeight{ colorHeight }
, colorFov{ colorFovx, colorFovy }
{
  // まだシェーダが作られていなかったら
  if (normal.get() == nullptr)
  {
    // 法線ベクトル算出用のシェーダを作成する
    normal.reset(new Compute("normal.comp"));

    // カメラ座標のイメージユニットの uniform 変数の場所を求める
    pointLoc = glGetUniformLocation(normal->get(), "point");

    // 法線ベクトルのバッファオブジェクトを参照する結合ポイントを指定する
    const GLuint normalIndex(glGetProgramResourceIndex(normal->get(), GL_SHADER_STORAGE_BLOCK, "Normal"));
    glShaderStorageBlockBinding(normal->get(), normalIndex, NormalBinding);
  }

  // まだメッシュが作られていなかったら
  if (mesh.get() == nullptr)
  {
    // 描画用のメッシュを作成する
    mesh.reset(new Mesh);
  }
}

// デストラクタ
DepthCamera::~DepthCamera()
{
  // テクスチャを削除する
  if (depthTexture > 0) glDeleteTextures(1, &depthTexture);
  if (pointTexture > 0) glDeleteTextures(1, &pointTexture);
  if (colorTexture > 0) glDeleteTextures(1, &colorTexture);

  // バッファオブジェクトを削除する
  if (uvmapBuffer > 0) glDeleteBuffers(1, &uvmapBuffer);
  if (normalBuffer > 0) glDeleteBuffers(1, &normalBuffer);
  if (weightBuffer > 0) glDeleteBuffers(1, &weightBuffer);
}

// テクスチャとバッファオブジェクトを作成してポイント数を返す
int DepthCamera::makeTexture()
{
  // カラーデータの境界色
  static const GLfloat border[]{ 0.5f, 0.5f, 0.5f, 0.0f };

  // デプスデータを格納するテクスチャを準備する
  glGenTextures(1, &depthTexture);
  glBindTexture(GL_TEXTURE_2D, depthTexture);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_R16UI, depthWidth, depthHeight, 0, GL_RED_INTEGER, GL_UNSIGNED_SHORT, NULL);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

  // 平滑したデプスデータを格納するテクスチャを準備する
  glGenTextures(1, &smoothTexture);
  glBindTexture(GL_TEXTURE_2D, smoothTexture);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_R16UI, depthWidth, depthHeight, 0, GL_RED_INTEGER, GL_UNSIGNED_SHORT, NULL);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  const GLushort clear{ 0 };
  glClearTexImage(smoothTexture, 0, GL_RED_INTEGER, GL_UNSIGNED_SHORT, &clear);

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

  // ポイント数を求める
  const int depthCount(depthWidth * depthHeight);

  // デプスデータの画素位置のカラーのテクスチャ座標を格納するバッファオブジェクトを準備する
  glGenBuffers(1, &uvmapBuffer);
  glBindBuffer(GL_ARRAY_BUFFER, uvmapBuffer);
  glBufferData(GL_ARRAY_BUFFER, depthCount * sizeof (Uvmap), NULL, GL_DYNAMIC_DRAW);

  // カメラ座標の法線ベクトルを格納するバッファオブジェクトを準備する
  glGenBuffers(1, &normalBuffer);
  glBindBuffer(GL_ARRAY_BUFFER, normalBuffer);
  glBufferData(GL_ARRAY_BUFFER, depthCount * sizeof (Normal), nullptr, GL_DYNAMIC_DRAW);

  // バイラテラルフィルタの重みを格納するバッファオブジェクトを準備する
  glGenBuffers(1, &weightBuffer);
  glBindBuffer(GL_SHADER_STORAGE_BUFFER, weightBuffer);
  glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof (Weight), NULL, GL_STATIC_DRAW);

  // ポイント数を返す
  return depthCount;
}

// 法線ベクトルの計算
GLuint DepthCamera::getNormal() const
{
  normal->use();
  glUniform1i(pointLoc, PointImageUnit);
  glBindImageTexture(PointImageUnit, pointTexture, 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA32F);
  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, NormalBinding, normalBuffer);
  normal->execute(depthWidth, depthHeight);

  return normalBuffer;
}

// バイラテラルフィルタの分散を設定する
void DepthCamera::setVariance(float columnVariance, float rowVariance, float valueVariance) const
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

// カメラ座標を計算するシェーダ
std::unique_ptr<Compute> DepthCamera::normal(nullptr);

// 描画するメッシュ
std::unique_ptr<Mesh> DepthCamera::mesh(nullptr);

// 法線ベクトルを求めるカメラ座標のイメージユニットの uniform 変数 point の場所
GLuint DepthCamera::pointLoc;
