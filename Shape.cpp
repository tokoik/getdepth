#include "Shape.h"

//
// 図形描画
//

// コンストラクタ
Shape::Shape()
{
  // 頂点配列オブジェクトを作成する
  glGenVertexArrays(1, &vao);
  glBindVertexArray(vao);
}

// デストラクタ
Shape::~Shape()
{
  // 頂点配列オブジェクトを削除する
  glDeleteVertexArrays(1, &vao);
}

// 描画
void Shape::draw() const
{
  // 頂点配列オブジェクトを指定する
  glBindVertexArray(vao);
}
