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

  // デストラクタ
  virtual ~Shape();

  // 描画
  virtual void draw() const = 0;
};
