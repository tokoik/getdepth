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
  const char* message;

  // バイラテラルフィルタのサイズ
  static constexpr int filterSize[]{ 5, 5 };

  // バイラテラルフィルタの距離に対する重みと値に対する分散
  struct Weight
  {
    std::array<GLfloat, filterSize[0]> columnWeight;
    std::array<GLfloat, filterSize[1]> rowWeight;
    float variance;
  };

  // 法線ベクトルのデータ型
  using Normal = std::array<GLfloat, 4>;

  // カメラ座標における法線ベクトルを格納するバッファオブジェクト
  GLuint normalBuffer;

  // 法線ベクトルを計算するシェーダ
  static std::unique_ptr<Compute> normal;

  // 法線ベクトルを求めるカメラ座標のイメージユニットの uniform 変数 point の場所
  static GLint pointLoc;

protected:

  // エラーメッセージを設定する
  void setMessage(const char* message)
  {
    this->message = message;
  }

  // デプスセンサのサイズ
  int depthWidth, depthHeight;

  // デプスデータを格納するテクスチャ
  GLuint depthTexture;

  // 平滑化したデプスデータを格納するテクスチャ
  GLuint smoothTexture;

  // デプスデータから変換したカメラ座標を格納するテクスチャ
  GLuint pointTexture;

  // カメラ座標のデータ型
  using Point = std::array<GLfloat, 3>;

  // カラーセンサのサイズ
  int colorWidth, colorHeight;

  // カラーデータを格納するテクスチャ
  GLuint colorTexture;

  // カラーのデータ型
  using Color = std::array<GLubyte, 3>;

  // カメラ座標に対応したテクスチャ座標を格納するバッファオブジェクト
  GLuint uvmapBuffer;

  // テクスチャ座標のデータ型
  using Uvmap = std::array<GLfloat, 2>;

  // バイラテラルフィルタの距離に対する重みを格納する Shader Storage Buffer Object
  GLuint weightBuffer;

  // テクスチャとバッファオブジェクトを作成してポイント数を返す
  int makeTexture();

  // 描画に使うメッシュ
  static std::unique_ptr<Mesh> mesh;

public:

  // イメージユニット
  enum ImageUnits
  {
    DepthImageUnit = 0,
    SmoothImageUnit,
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
  DepthCamera(int depthWidth, int depthHeight, GLfloat depthFovx, GLfloat depthFovy,
    int colorWidth, int colorHeight, GLfloat colorFovx, GLfloat colorFovy);

  // コピーコンストラクタ (コピー禁止)
  DepthCamera(const DepthCamera& camera) = delete;

  // 代入 (代入禁止)
  DepthCamera& operator=(const DepthCamera& camera) = delete;

  // デストラクタ
  virtual ~DepthCamera();

  // エラーメッセージを取り出す
  const auto getMessage() const
  {
    return message;
  }

  // デプスセンサの画角
  const GLfloat depthFov[2];

  // カラーセンサの画角
  const GLfloat colorFov[2];

  // デプスセンサの姿勢
  GgMatrix attitude;

  // バイラテラルフィルタの分散を設定する
  void setVariance(float column_variance, float row_variance, float value_variance) const;

  // デプスセンサのサイズを得る
  void getDepthResolution(int* width, int* height) const
  {
    *width = depthWidth;
    *height = depthHeight;
  }

  // カラーセンサのサイズを得る
  void getColorResolution(int* width, int* height) const
  {
    *width = colorWidth;
    *height = colorHeight;
  }

  // デプスデータを格納するテクスチャを得る
  auto getDepthTexture() const
  {
    return depthTexture;
  }

  // カラーデータを格納するテクスチャを得る
  auto getColorTexture() const
  {
    return colorTexture;
  }

  // カメラ座標を格納するテクスチャを得る
  auto getPointTexture() const
  {
    return pointTexture;
  }

  // 法線ベクトルを計算する
  GLuint getNormal() const;

  // メッシュを描画する
  void draw() const;

  // 使用中なら true を返す
  bool isOpend() const
  {
    return message == nullptr;
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
