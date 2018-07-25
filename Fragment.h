#pragma once

//
// 画像処理
//

// 補助プログラム
#include "gg.h"
using namespace gg;

// 矩形
#include "Rect.h"

// 標準ライブラリ
#include <vector>

class Fragment
{
  // 画像処理に使うフレームバッファオブジェクト
  GLuint fbo;

  // 計算結果を保存するフレームバッファのターゲットに使うテクスチャ
  std::vector<GLuint> textures;

  // レンダリングターゲット
  std::vector<GLenum> bufs;

  // フレームバッファオブジェクトのサイズ
  const GLsizei width, height;

  // 計算用のシェーダプログラム
  const GLuint program;

  // 計算に使う矩形
  static const Rect *rectangle;

  // リファレンスカウント
  static unsigned int count;

public:

  // コンストラクタ
  Fragment(int width, int height, const char *frag, GLuint targets = 1);

  // デストラクタ
  virtual ~Fragment();

  // シェーダプログラムを得る
  GLuint get() const
  {
    return program;
  }

  // 計算結果を取り出すテクスチャ名を得る
  const std::vector<GLuint> &getTexture() const
  {
    return textures;
  }

  // 計算用のシェーダプログラムの使用を開始する
  void use() const
  {
    glUseProgram(program);
  }

  // 計算を実行する
  const std::vector<GLuint> &execute(GLuint count, const GLuint *sources, ...) const;
};

using Calculate = Fragment;
#define SHADER_EXT ".frag"
