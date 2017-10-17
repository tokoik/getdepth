#pragma once

//
// 矩形
//

// 図形描画
#include "Shape.h"

class Rect : public Shape
{
  // 頂点バッファオブジェクト
  GLuint vbo;

public:

  // コンストラクタ
  Rect();

  // デストラクタ
  virtual ~Rect();

  // 描画
  virtual void draw() const;
};
