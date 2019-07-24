#pragma once

//
// メッシュ
//

// 補助プログラム
#include "gg.h"
using namespace gg;

class Mesh
{
  // 頂点配列オブジェクト
  GLuint vao;

  // メッシュの幅
  const GLsizei slices;

  // メッシュの高さ
  const GLsizei stacks;

public:

  // コンストラクタ
  Mesh(int slices, int stacks)
    : slices(slices), stacks(stacks)
  {
    // 頂点配列オブジェクトを作成する
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
  }

  // コピーコンストラクタを封じる
  Mesh(const Mesh &mesh) = delete;

  // 代入演算子を封じる
  Mesh &operator=(const Mesh &mesh) = delete;

  // デストラクタ
  virtual ~Mesh()
  {
    // 頂点配列オブジェクトを削除する
    glDeleteVertexArrays(1, &vao);
  }

  // 描画
  virtual void draw(GLuint meshSizeLoc) const
  {
    // 描画するメッシュのサイズを設定する
    glUniform2i(meshSizeLoc, slices, stacks);

    // 頂点配列オブジェクトを指定する
    glBindVertexArray(vao);

    // 描画する
    glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, slices * 2, stacks - 1);
  }
};
