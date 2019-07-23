﻿#pragma once

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
  // 計算結果を保存するフレームバッファのターゲットに使うテクスチャ
  std::vector<GLuint> textures;

  // テクスチャのサイズ
  const GLsizei width, height;

  // 計算用のシェーダプログラム
  const GLuint program;

public:

  // コンストラクタ
  Compute(int width, int height, const char *comp, GLuint targets = 1);

  // デストラクタ
  virtual ~Compute();

  // シェーダプログラムを得る
  GLuint get() const
  {
    return program;
  }

  // 計算結果を取り出すテクスチャ名を得る
  const std::vector<GLuint> &getTexture() const
  {
    return textures;
  }

  // 計算用のシェーダプログラムの使用を開始する
  void use() const
  {
    glUseProgram(program);
  }

  // 計算を実行する
  const std::vector<GLuint> &execute(GLuint count, const GLuint *sources, const GLenum *format = nullptr, GLuint local_size_x = 1, GLuint local_size_y = 1) const;
};
