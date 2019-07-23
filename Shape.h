#pragma once

//
// 描画図形
//

// 補助プログラム
#include "gg.h"
using namespace gg;

class Shape
{
  // 頂点配列オブジェクト
  GLuint vao;

public:

  // コンストラクタ
  Shape();

  // コピーコンストラクタを封じる
  Shape(const Shape &shape) = delete;

  // 代入演算子を封じる
  Shape &operator=(const Shape &shape) = delete;

  // デストラクタ
  virtual ~Shape();

  // 描画
  virtual void draw() const = 0;
};
