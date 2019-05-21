#pragma once

//
// デプスセンサ関連の基底クラス
//

// 補助プログラム
#include "gg.h"
using namespace gg;

// 計算用のシェーダ
//#include "Fragment.h"
#include "Compute.h"

// 標準ライブラリ
#include <memory>

class DepthCamera
{
protected:

  // デプスセンサのサイズと画素数
  int depthWidth, depthHeight, depthCount;

  // デプスデータを格納するテクスチャ
  GLuint depthTexture;

  // デプスデータから変換したポイントのカメラ座標を格納するテクスチャ
  GLuint pointTexture;

  // カラーセンサのサイズと画素数
  int colorWidth, colorHeight, colorCount;

  // カラーテクスチャのスケール
  GLfloat colorScale[2];

  // カラーデータを格納するテクスチャ
  GLuint colorTexture;

  // デプスデータの画素におけるカラーデータのテクスチャ座標を格納するバッファオブジェクト
  GLuint coordBuffer;

  // バイラテラルフィルタの分散
  GLfloat variance;

  // depthCount と colorCount を計算してテクスチャとバッファオブジェクトを作成する
  void makeTexture();

  // コピーコンストラクタ (コピー禁止)
  DepthCamera(const DepthCamera &w) {}

  // 代入 (代入禁止)
  DepthCamera &operator=(const DepthCamera &w) {}

  // エラーメッセージ
  const char *message;

protected:

  // エラーメッセージをセットする
  void setMessage(const char *message)
  {
    this->message = message;
  }

public:

  // コンストラクタ
  DepthCamera()
    : depthTexture(0)
    , pointTexture(0)
    , colorTexture(0)
    , coordBuffer(0)
    , variance(0.1f)
    , message(nullptr)
  {
  }

  // デストラクタ
  virtual ~DepthCamera();

  // 使用可能なら true を返す
  bool isOpend() const
  {
    return message == nullptr;
  }

  // エラーメッセージを取り出す
  const char *getMessage() const
  {
    return message;
  }

  // デプスセンサのサイズを得る
  void getDepthResolution(int *width, int *height) const
  {
    *width = depthWidth;
    *height = depthHeight;
  }

  // カラーセンサのサイズを得る
  void getColorResolution(int *width, int *height) const
  {
    *width = colorWidth;
    *height = colorHeight;
  }

  // カラーテクスチャのスケールを得る
  const GLfloat *getColorScale() const
  {
    return colorScale;
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

  // デプスデータを取得する
  virtual GLuint getDepth()
  {
    glBindTexture(GL_TEXTURE_2D, depthTexture);
    return depthTexture;
  }

  // カメラ座標を取得する
  virtual GLuint getPoint()
  {
    glBindTexture(GL_TEXTURE_2D, pointTexture);
    return pointTexture;
  }

  // カメラ座標を算出する
  virtual GLuint getPosition()
  {
    glBindTexture(GL_TEXTURE_2D, pointTexture);
    return pointTexture;
  }

  // カラーデータを取得する
  virtual GLuint getColor()
  {
    glBindTexture(GL_TEXTURE_2D, colorTexture);
    return colorTexture;
  }
};
