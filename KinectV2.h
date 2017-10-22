#pragma once

//
// 深度センサ関連の処理
//

// Kinect V2 を使う
#if !defined(USE_KINECT_V2)
#  define USE_KINECT_V2 1
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

  // デプスデータ
  IDepthFrameReader *depthReader;

  // デプスデータからカメラ座標を求めるときに用いる一時メモリ
  GLfloat (*position)[3];

  // カラーデータ
  IColorFrameReader *colorReader;

  // カラーデータの変換に用いる一時メモリ
  GLubyte *color;

  // 座標のマッピング
  ICoordinateMapper *coordinateMapper;

  // コピーコンストラクタ (コピー禁止)
  KinectV2(const KinectV2 &w);

  // 代入 (代入禁止)
  KinectV2 &operator=(const KinectV2 &w);

public:

  // コンストラクタ
  KinectV2();

  // デストラクタ
  virtual ~KinectV2();

  // デプスデータを取得する
  GLuint getDepth() const;

  // カメラ座標を取得する
  GLuint getPoint() const;

  // カラーデータを取得する
  GLuint getColor() const;
};

#endif
