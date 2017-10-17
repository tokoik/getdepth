#pragma once

//
// メッシュ
//

// 図形描画
#include "Shape.h"

class Mesh : public Shape
{
  // メッシュの幅
  const GLsizei slices;

  // メッシュの高さ
  const GLsizei stacks;

  // データとして保持する頂点数
  const GLsizei vertices;

  // 実際に描画する頂点数
  const GLsizei indexes;

  // デプスデータのサンプリングに使うテクスチャ座標を格納するバッファオブジェクト
  GLuint depthCoord;

  // 頂点のインデックスを格納するバッファオブジェクト
  GLuint indexBuffer;

  // テクスチャ座標を生成してバインドされているバッファオブジェクトに転送する
  void genCoord();

public:

  // コンストラクタ
  Mesh(int stacks, int slices, GLuint coordBuffer = 0);

  // デストラクタ
  virtual ~Mesh();

  // 描画
  virtual void draw() const;
};
