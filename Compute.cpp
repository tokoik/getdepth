#include "Compute.h"

//
// 画像処理（コンピュートシェーダ版）
//

// コンストラクタ
Compute::Compute(int width, int height, const char *source, int targets)
  : width(width)
  , height(height)
  , program(ggLoadComputeShader(source))
{
  for (int i = 0; i < targets; ++i)
  {
    // 出力側のテクスチャを準備する
    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, width, height, 0, GL_RGBA, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

    // 出力側のテクスチャをバインドポイントにバインドする
    glBindImageTexture(i + 1, texture, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);

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
const std::vector<GLuint> &Compute::execute(GLuint texture, GLuint internal) const
{
  // 入力側のテクスチャを選択する
  glBindImageTexture(0, texture, 0, GL_FALSE, 0, GL_READ_ONLY, internal);
  ggError();

  // 計算を実行する
  glDispatchCompute(width / 32, height / 32, 1);
  ggError();

  return textures;
}
