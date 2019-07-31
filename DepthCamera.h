#pragma once

//
// デプスセンサ関連の基底クラス
//

// 補助プログラム
#include "gg.h"
using namespace gg;

// 計算用のシェーダ
#include "Compute.h"

// メッシュの描画
#include "Mesh.h"

// 標準ライブラリ
#include <memory>

class DepthCamera
{
  // エラーメッセージ
  const char *message;

  // フィルタのサイズ
  static constexpr int filterSize[] = { 5, 5 };

  // バイラテラルフィルタの距離に対する重みと値に対する分散
  struct Weight
  {
    std::array<GLfloat, filterSize[0]> columnWeight;
    std::array<GLfloat, filterSize[1]> rowWeight;
    float variance;
  };

  // カメラ座標における法線ベクトルを格納するバッファオブジェクト
  GLuint normalBuffer;

  // 法線ベクトルを計算するシェーダ
  static std::unique_ptr<Compute> normal;

  // 法線ベクトルを求めるカメラ座標のイメージユニットの uniform 変数 point の場所
  static GLuint pointLoc;

protected:

  // エラーメッセージを設定する
  void setMessage(const char *message)
  {
    this->message = message;
  }

  // カメラ座標のデータ型
  using Point = std::array<GLfloat, 3>;

  // テクスチャ座標のデータ型
  using Uvmap = std::array<GLfloat, 2>;

  // 法線ベクトルのデータ型
  using Normal = std::array<GLfloat, 4>;

  // カラーのデータ型
  using Color = std::array<GLubyte, 3>;

  // デプスセンサのサイズ
  int depthWidth, depthHeight;

  // デプスデータを格納するテクスチャ
  GLuint depthTexture;

  // カラーセンサのサイズ
  int colorWidth, colorHeight;

  // カラーデータを格納するテクスチャ
  GLuint colorTexture;

  // デプスデータから変換したカメラ座標を格納するテクスチャ
  GLuint pointTexture;

  // カメラ座標に対応したテクスチャ座標を格納するバッファオブジェクト
  GLuint uvmapBuffer;

  // バイラテラルフィルタの距離に対する重みを格納する Shader Storage Buffer Object
  GLuint weightBuffer;

  // テクスチャとバッファオブジェクトを作成してポイント数を返す
  int makeTexture();

  // 描画するメッシュ
  static std::unique_ptr<Mesh> mesh;

public:

  // エラーメッセージを取り出す
  const char *getMessage() const
  {
    return message;
  }

  // イメージユニット
  enum ImageUnits
  {
    DepthImageUnit = 0,
    PointImageUnit,
    MapperImageUnit
  };

  // 結合ポイント
  enum BindingPoints
  {
    // 0, 1 は GgSimpleShader で使っている
    UvmapBinding = 2,
    WeightBinding,
    NormalBinding
  };

  // コンストラクタ
  DepthCamera();

  // コピーコンストラクタ (コピー禁止)
  DepthCamera(const DepthCamera &w) = delete;

  // 代入 (代入禁止)
  DepthCamera &operator=(const DepthCamera &w) = delete;

  // デストラクタ
  virtual ~DepthCamera();

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

  // デプスデータを格納するテクスチャを得る
  GLuint getDepthTexture() const
  {
    return depthTexture;
  }

  // カラーデータを格納するテクスチャを得る
  GLuint getColorTexture() const
  {
    return colorTexture;
  }

  // カメラ座標を格納するテクスチャを得る
  GLuint getPointTexture() const
  {
    return pointTexture;
  }

  // テクスチャ座標を格納するバッファオブジェクトを得る
  GLuint getUvmapBuffer() const
  {
    return uvmapBuffer;
  }

  // 法線ベクトルを格納するバッファオブジェクトを得る
  GLuint getNormalBuffer() const
  {
    return normalBuffer;
  }

  // 法線ベクトルの計算
  GLuint getNormal() const;

  // バイラテラルフィルタの分散を設定する
  void setVariance(float columnVariance, float rowVariance, float valueVariance) const;

  // デプスセンサの姿勢
  GgMatrix attitude;

  // 目種の描画
  void draw()
  {
    mesh->draw(depthWidth, depthHeight);
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
