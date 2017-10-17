#pragma once

//
// 描画図形
//

// ウィンドウ関連の処理
#include "Window.h"

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
