#pragma once

//
// デプスセンサ関連の基底クラス
//

// 補助プログラム
#include "gg.h"
using namespace gg;

// 計算用のシェーダ
#include "Compute.h"

// 標準ライブラリ
#include <memory>

class DepthCamera
{
  // エラーメッセージ
  const char *message;

protected:

  // 結合ポイント
  enum Binding
  {
    depthBinding = 0,
    pointBinding = 1,
    uvmapBinding = 2,
    filterBinding = 3,
    mapperBinding = 4
  };

  // フィルターのサイズ
  static constexpr int filterSize[] = { 5, 5 };

  // エラーメッセージをセットする
  void setMessage(const char *message)
  {
    this->message = message;
  }

  // デプスセンサのサイズ
  int depthWidth, depthHeight;

  // デプスデータを格納するテクスチャ
  GLuint depthTexture;

  // デプスデータから変換したカメラ座標を格納するテクスチャ
  GLuint pointTexture;

  // カラーセンサのサイズ
  int colorWidth, colorHeight;

  // カラーデータを格納するテクスチャ
  GLuint colorTexture;

  // デプスデータの画素におけるカラーデータのテクスチャ座標を格納するバッファオブジェクト
  GLuint uvmapBuffer;

  // カメラ座標のデータ型
  using Point = std::array<GLfloat, 3>;

  // テクスチャ座標のデータ型
  using Uvmap = std::array<GLfloat, 2>;

  // カラーのデータ型
  using Color = std::array<GLubyte, 3>;

  // バイラテラルフィルタの距離に対する重みと値に対する分散
  struct Weight
  {
    std::array<GLfloat, filterSize[0]> columnWeight;
    std::array<GLfloat, filterSize[1]> rowWeight;
    float variance;
  };

  // バイラテラルフィルタの距離に対する重みを格納する Shader Storage Buffer Objec
  GLuint weightBuffer;

  // depthCount と colorCount を計算してテクスチャとバッファオブジェクトを作成する
  void makeTexture(int *depthCount, int *colorCcount);

public:

  // コンストラクタ
  DepthCamera()
    : depthTexture(0), pointTexture(0), colorTexture(0), uvmapBuffer(0), weightBuffer(0)
    , message(nullptr)
  {
  }

  // コピーコンストラクタ (コピー禁止)
  DepthCamera(const DepthCamera &w) = delete;

  // 代入 (代入禁止)
  DepthCamera &operator=(const DepthCamera &w) = delete;

  // デストラクタ
  virtual ~DepthCamera();

  // エラーメッセージを取り出す
  const char *getMessage() const
  {
    return message;
  }

  // 使用可能なら true を返す
  bool isOpend() const
  {
    return message == nullptr;
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
  GLuint getUvmapBuffer() const
  {
    return uvmapBuffer;
  }

  // バイラテラルフィルタの分散を設定する
  void setVariance(float columnVariance, float rowVariance, float valueVariance);

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
