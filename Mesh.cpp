#include "Mesh.h"

//
// メッシュ
//

// テクスチャ座標の生成してバッファオブジェクトに転送する
void Mesh::genCoord()
{
  GLfloat (*const coord)[2](static_cast<GLfloat(*)[2]>(glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY)));
  for (int i = 0; i < vertices; ++i)
  {
    coord[i][0] = (GLfloat(i % slices) + 0.5f) / GLfloat(slices);
    coord[i][1] = (GLfloat(i / slices) + 0.5f) / GLfloat(stacks);
  }
  glUnmapBuffer(GL_ARRAY_BUFFER);
}

// コンストラクタ
Mesh::Mesh(int slices, int stacks, GLuint coordBuffer)
  : slices(slices)
  , stacks(stacks)
  , vertices(slices * stacks)
  , indexes((slices - 1) * (stacks - 1) * 3 * 2)
{
  // デプスデータのサンプリング用のバッファオブジェクトを準備する
  glGenBuffers(1, &depthCoord);
  glBindBuffer(GL_ARRAY_BUFFER, depthCoord);
  glBufferData(GL_ARRAY_BUFFER, vertices * 2 * sizeof (GLfloat), NULL, GL_STATIC_DRAW);

  // デプスデータのテクスチャ座標を求めてバッファオブジェクトに転送する
  genCoord();

  // インデックスが 0 の varying 変数に割り当てる
  glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, NULL);
  glEnableVertexAttribArray(0);

  // カラーデータのテクスチャ座標を格納するバッファオブジェクトが指定されていたら
  if (coordBuffer > 0)
  {
    // カラーデータのテクスチャ座標を格納するバッファオブジェクトを結合する
    glBindBuffer(GL_ARRAY_BUFFER, coordBuffer);

    // カラーデータのテクスチャ座標の初期値を求めてバッファオブジェクトに転送する
    genCoord();

    // インデックスが 1 の varying 変数に割り当てる
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, NULL);
    glEnableVertexAttribArray(1);
  }

  // インデックス用のバッファオブジェクト
  glGenBuffers(1, &indexBuffer);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, indexes * sizeof (GLuint), NULL, GL_STATIC_DRAW);

  // インデックスを求めてバッファオブジェクトに転送する
  GLuint *index(static_cast<GLuint *>(glMapBuffer(GL_ELEMENT_ARRAY_BUFFER, GL_WRITE_ONLY)));
  for (int j = 0; j < stacks - 1; ++j)
  {
    for (int i = 0; i < slices - 1; ++i)
    {
      index[0] = slices * j + i;
      index[1] = index[5] = index[0] + 1;
      index[2] = index[4] = index[0] + slices;
      index[3] = index[2] + 1;
      index += 6;
    }
  }
  glUnmapBuffer(GL_ELEMENT_ARRAY_BUFFER);
}

// デストラクタ
Mesh::~Mesh()
{
  // 頂点バッファオブジェクトを削除する
  glDeleteBuffers(1, &depthCoord);
  glDeleteBuffers(1, &indexBuffer);
}

// 描画
void Mesh::draw() const
{
  // 頂点配列オブジェクトを指定して描画する
  Shape::draw();
  glDrawElements(GL_TRIANGLES, indexes, GL_UNSIGNED_INT, NULL);
}
