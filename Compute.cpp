#include "Compute.h"

//
// 画像処理（コンピュートシェーダ版）
//

// コンストラクタ
Compute::Compute(int width, int height, const char *comp, GLuint targets)
  : width(width)
  , height(height)
  , program(ggLoadComputeShader(comp))
{
  for (GLuint i = 0; i < targets; ++i)
  {
    // 出力側のテクスチャを準備する
    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, width, height, 0, GL_RGB, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

    // 作成したテクスチャを保存する
    textures.push_back(texture);
  }
}

// デストラクタ
Compute::~Compute()
{
  // シェーダプログラムを削除する
  glDeleteShader(program);
}

// 計算を実行する
const std::vector<GLuint> &Compute::execute(int count, const GLuint *sources, GLuint xGroup, GLuint yGroup) const
{
  // 入力側のテクスチャを結合する
  for (int i = 0; i < count; ++i)
  {
    glBindImageTexture(0, sources[i], 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA32F);
  }

  // 出力側のテクスチャを結合する
  for (auto texture : textures)
  {
    glBindImageTexture(count++, texture, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);
  }

  // 計算を実行する
  glDispatchCompute(width / xGroup, height / yGroup, 1);

  return textures;
}
