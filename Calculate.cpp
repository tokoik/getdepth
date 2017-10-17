#include "Calculate.h"

//
// 画像処理
//

// コンストラクタ
Calculate::Calculate(int width, int height, const char *source, int uniforms, int targets)
  : width(width)
  , height(height)
  , program(ggLoadShader("rectangle.vert", source))
  , uniforms(uniforms)
{
  // 計算結果を格納するフレームバッファオブジェクトを作成する
  glGenFramebuffers(1, &fbo);
  glBindFramebuffer(GL_FRAMEBUFFER, fbo);

  for (int i = 0; i < targets; ++i)
  {
    // フレームバッファオブジェクトのターゲットに使うテクスチャを作成する
    GLuint tex;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, width, height, 0, GL_RGB, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

    // フレームバッファオブジェクトにテクスチャを追加する
    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, tex, 0);

    // レンダリングターゲットを設定する
    texture.push_back(tex);
    bufs.push_back(GL_COLOR_ATTACHMENT0 + i);
  }

  // フレームバッファオブジェクトへの描画に使う矩形を作成する
  if (count++ == 0) rectangle = new Rect;
}

// デストラクタ
Calculate::~Calculate()
{
  // シェーダプログラムを削除する
  glDeleteShader(program);

  // フレームバッファオブジェクトを削除する
  glDeleteFramebuffers(1, &fbo);

  // フレームバッファオブジェクトへの描画に使う矩形を削除する
  if (--count == 0) delete rectangle;
}

// 計算を実行する
const std::vector<GLuint> &Calculate::calculate() const
{
  // フレームバッファオブジェクトにレンダリング
  glBindFramebuffer(GL_FRAMEBUFFER, fbo);
  glDrawBuffers(GLsizei(bufs.size()), bufs.data());

  // 隠面消去を無効にする
  glDisable(GL_DEPTH_TEST);
  glDepthMask(GL_FALSE);

  // ビューポートをフレームバッファオブジェクトのサイズに設定する
  glViewport(0, 0, width, height);

  // クリッピング空間いっぱいの矩形をレンダリングする
  rectangle->draw();

  // 元のレンダリング先に戻す
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  glDrawBuffer(GL_BACK);

  // 隠面消去を有効にする
  glEnable(GL_DEPTH_TEST);
  glDepthMask(GL_TRUE);

  return texture;
}

// 計算に使う矩形
const Rect *Calculate::rectangle;

// リファレンスカウント
unsigned int Calculate::count(0);
