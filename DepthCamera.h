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

  // カラーデータを格納するテクスチャ
  GLuint colorTexture;

  // デプスデータの画素におけるカラーデータのテクスチャ座標を格納するバッファオブジェクト
  GLuint coordBuffer;

  // バイラテラルフィルタの位置と明度の分散
  GLfloat variance[2];

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
    , variance{ 1.0f, 0.01f }
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

  // カラーデータのテクスチャ座標を格納するバッファオブジェクトを得る
  GLuint getCoordBuffer() const
  {
    return coordBuffer;
  }

  // バイラテラルフィルタの位置と明度の分散を設定する
  void setVariance(GLfloat v1, GLfloat v2)
  {
    variance[0] = v1;
    variance[1] = v2;
  }

  // バイラテラルフィルタの位置の分散を設定する
  void setVariance1(GLfloat v)
  {
    variance[0] = v;
  }

  // バイラテラルフィルタの明度の分散を設定する
  void setVariance2(GLfloat v)
  {
    variance[1] = v;
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
