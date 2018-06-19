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
    // フレームバッファオブジェクトのターゲットに使うテクスチャを作成する
    GLuint t;
    glGenTextures(1, &t);
    glBindTexture(GL_TEXTURE_2D, t);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, width, height, 0, GL_RGB, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

    // 作成したテクスチャを保存する
    textures.push_back(t);
  }
  ggError();
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

  // 出力側のテクスチャを選択する
  for (size_t i = 0; i < textures.size(); ++i)
  {
    glBindImageTexture(i + 1, textures[i], 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);
    ggError();
  }

  // 計算を実行する
  glDispatchCompute(width / 32, height / 32, 1);
  ggError();

  return textures;
}
