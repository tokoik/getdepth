#pragma once

//
// デプスセンサ関連の処理
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

// デプスセンサ関連の基底クラス
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
  std::vector<GLushort> depth;

  // デプスデータからカメラ座標を求めるときに用いる一時メモリ
  std::vector<Point> point;

  // デプス値に対するカメラ座標の変換テーブルのテクスチャ
  GLuint mapperTexture;

  // カラーデータ
  IColorFrameReader *colorReader;

  // カラーデータの変換に用いる一時メモリ
  std::vector<GLubyte> color;

  // カメラ座標を計算するシェーダ
  static std::unique_ptr<Compute> shader;

public:

  // コンストラクタ
  KinectV2();

  // デストラクタ
  virtual ~KinectV2();

  // 奥行きの最大値
  static constexpr GLushort maxDepth = 10000;

  // 疑似カラー処理の範囲
  static constexpr GLfloat range[2] = { 0.5f, 8.0f };

  // デプスデータを取得する
  GLuint getDepth();

  // カメラ座標を取得する
  GLuint getPoint();

  // カメラ座標を算出する
  GLuint KinectV2::getPosition();

  // カラーデータを取得する
  GLuint getColor();
};

#endif
