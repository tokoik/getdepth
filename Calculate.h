#pragma once

//
// 画像処理
//

// ウィンドウ関連の処理
#include "Window.h"

// 矩形
#include "Rect.h"

// 標準ライブラリ
#include <vector>

class Calculate
{
  // 画像処理に使うフレームバッファオブジェクト
  GLuint fbo;

  // 計算結果を保存するフレームバッファのターゲットに使うテクスチャ
  std::vector<GLuint> texture;

  // レンダリングターゲット
  std::vector<GLenum> bufs;

  // フレームバッファオブジェクトのサイズ
  const GLsizei width, height;

  // 計算用のシェーダプログラム
  const GLuint program;

  // 計算用のシェーダプログラムで使用しているサンプラの uniform 変数の数
  const int uniforms;

  // 計算に使う矩形
  static const Rect *rectangle;

  // リファレンスカウント
  static unsigned int count;

public:

  // コンストラクタ
  Calculate(int width, int height, const char *source, int uniforms = 1, int targets = 1);

  // デストラクタ
  virtual ~Calculate();

  // シェーダプログラムを得る
  GLuint get() const
  {
    return program;
  }

  // 計算結果を取り出すテクスチャ名を得る
  const std::vector<GLuint> &getTexture() const
  {
    return texture;
  }

  // 計算用のシェーダプログラムの使用を開始する
  void use() const
  {
    glUseProgram(program);
  }

  // 計算を実行する
  const std::vector<GLuint> &calculate() const;
};
