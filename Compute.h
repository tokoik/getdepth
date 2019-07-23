#pragma once

//
// 画像処理（コンピュートシェーダ版）
//

// 補助プログラム
#include "gg.h"
using namespace gg;

// 標準ライブラリ
#include <vector>

class Compute
{
  // 計算用のシェーダプログラム
  const GLuint program;

public:

  // コンストラクタ
  Compute(const char *comp);

  // デストラクタ
  virtual ~Compute();

  // シェーダプログラムを得る
  GLuint get() const
  {
    return program;
  }

  // 計算用のシェーダプログラムの使用を開始する
  void use() const
  {
    glUseProgram(program);
  }

  // 計算を実行する
  void execute(GLuint width, GLuint height, GLuint local_size_x = 1, GLuint local_size_y = 1) const;
};
