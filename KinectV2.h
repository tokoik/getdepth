#pragma once

//
// 深度センサ関連の処理
//

// Kinect V2 を使う
#if !defined(USE_KINECT_V2)
#  define USE_KINECT_V2 1
#  ifdef SENSOR
#    undef SENSOR
#  endif
#  define SENSOR KinectV2
#endif

#if USE_KINECT_V2

// Kinect 関連
#include <Windows.h>
#include <Kinect.h>

// 深度センサ関連の基底クラス
#include "DepthCamera.h"

class KinectV2 : public DepthCamera
{
  // センサの識別子
  static IKinectSensor *sensor;

  // 座標のマッピング
  ICoordinateMapper *coordinateMapper;

  // デプスデータ
  IDepthFrameReader *depthReader;

  // デプスデータの計測不能点を変換するために用いる一次メモリ
  GLfloat *depth;

  // デプスデータからカメラ座標を求めるときに用いる一時メモリ
  GLfloat (*position)[3];

  // デプス値に対するカメラ座標の変換テーブルのテクスチャ
  GLuint mapperTexture;

  // シェーダの uniform 変数 variance の場所
  GLint varianceLoc;

  // カラーデータ
  IColorFrameReader *colorReader;

  // カラーデータの変換に用いる一時メモリ
  GLubyte *color;

  // コピーコンストラクタ (コピー禁止)
  KinectV2(const KinectV2 &w);

  // 代入 (代入禁止)
  KinectV2 &operator=(const KinectV2 &w);

public:

  // コンストラクタ
  KinectV2();

  // デストラクタ
  virtual ~KinectV2();

  // 奥行きの最大値
  static constexpr GLfloat maxDepth = 10.0f;

  // 疑似カラー処理の範囲
  static constexpr GLfloat range[2] = { 0.5f, 8.0f };

  // デプスデータを取得する
  GLuint getDepth() const;

  // カメラ座標を取得する
  GLuint getPoint() const;

  // カメラ座標を算出する
  GLuint KinectV2::getPosition() const;

  // カラーデータを取得する
  GLuint getColor() const;
};

#endif
