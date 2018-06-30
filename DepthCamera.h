#pragma once

//
// 深度センサ関連の基底クラス
//

// 補助プログラム
#include "gg.h"
using namespace gg;

// 計算用のシェーダ
#include "Calculate.h"

// 標準ライブラリ
#include <memory>

class DepthCamera
{
  // 有効化されたデプスカメラの台数
  static int activated;

protected:

  // 接続しているセンサの数
  static int connected;

  // デプスカメラのサイズと画素数
  int depthWidth, depthHeight, depthCount;

  // デプスデータを格納するテクスチャ
  GLuint depthTexture;

  // デプスデータから変換したポイントのカメラ座標を格納するテクスチャ
  GLuint pointTexture;

  // カラーカメラのサイズと画素数
  int colorWidth, colorHeight, colorCount;

  // カラーデータを格納するテクスチャ
  GLuint colorTexture;

  // デプスデータの画素におけるカラーデータのテクスチャ座標を格納するバッファオブジェクト
  GLuint coordBuffer;

  // カメラ座標を計算するシェーダ
  std::unique_ptr<Calculate> shader;

  // バイラテラルフィルタの分散
  GLfloat variance;

  // depthCount と colorCount を計算してテクスチャとバッファオブジェクトを作成する
  void makeTexture();

public:

  // コンストラクタ
  DepthCamera()
    : shader(nullptr)
    , variance(0.1f)
  {
  }
  DepthCamera(int depthWidth, int depthHeight, int colorWidth, int colorHeight)
    : shader(nullptr)
    , variance(0.1f)
    , depthWidth(depthWidth)
    , depthHeight(depthHeight)
    , colorWidth(colorWidth)
    , colorHeight(colorHeight)
  {
  }

  // デストラクタ
  virtual ~DepthCamera();

  // 接続されているセンサの数を調べる
  static int getConnected()
  {
    return connected;
  }

  // 使用しているセンサの数を調べる
  static int getActivated()
  {
    return activated;
  }

  // デプスデータを取得する
  virtual GLuint getDepth() const
  {
    glBindTexture(GL_TEXTURE_2D, depthTexture);
    return depthTexture;
  }

  // カメラ座標を取得する
  virtual GLuint getPoint() const
  {
    glBindTexture(GL_TEXTURE_2D, pointTexture);
    return pointTexture;
  }

  // カメラ座標を算出する
  virtual GLuint getPosition() const
  {
    glBindTexture(GL_TEXTURE_2D, pointTexture);
    return pointTexture;
  }

  // カラーデータを取得する
  virtual GLuint getColor() const
  {
    glBindTexture(GL_TEXTURE_2D, colorTexture);
    return colorTexture;
  }

  // デプスカメラのサイズを得る
  void getDepthResolution(int *width, int *height) const
  {
    *width = depthWidth;
    *height = depthHeight;
  }

  // カラーカメラのサイズを得る
  void getColorResolution(int *width, int *height) const
  {
    *width = colorWidth;
    *height = colorHeight;
  }

  // カラーデータのテクスチャ座標を格納するバッファオブジェクトを得る
  GLuint getCoordBuffer() const
  {
    return coordBuffer;
  }

  // バイラテラルフィルタの分散を設定する
  void setVariance(GLfloat v)
  {
    variance = v;
  }
};
