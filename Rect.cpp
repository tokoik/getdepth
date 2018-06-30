#include "Rect.h"

//
// 矩形
//

// 描画
void Rect::draw() const
{
  // 頂点配列オブジェクトを指定して描画する
  Shape::draw();
  glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}
