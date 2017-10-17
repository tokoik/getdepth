#include "Rect.h"

//
// 矩形
//

// コンストラクタ
Rect::Rect()
{
  // 頂点バッファオブジェクトを作成する
  glGenBuffers(1, &vbo);
  glBindBuffer(GL_ARRAY_BUFFER, vbo);

  // 頂点バッファオブジェクトのメモリを確保してデータを転送する
  static const GLfloat points[] = { -1.0f, -1.0f, 1.0f, -1.0f, 1.0f, 1.0f, -1.0f, 1.0f };
  glBufferData(GL_ARRAY_BUFFER, sizeof points, points, GL_STATIC_DRAW);

  // 0 番の attribute 変数からデータを入力する
  glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, NULL);
  glEnableVertexAttribArray(0);
}

// デストラクタ
Rect::~Rect()
{
  // 頂点バッファオブジェクトを削除する
  glDeleteBuffers(1, &vbo);
}

// 描画
void Rect::draw() const
{
  // 頂点配列オブジェクトを指定して描画する
  Shape::draw();
  glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
}
