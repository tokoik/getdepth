#pragma once

//
// デプスセンサ関連の処理
//

// Kinect V1 を使う
#if !defined(USE_KINECT_V1)
#  define USE_KINECT_V1 1
#  ifdef SENSOR
#    undef SENSOR
#  endif
#  define SENSOR KinectV1
#endif

#if USE_KINECT_V1

// Kinect V1 関連
#include <Windows.h>
#include <NuiApi.h>

// デプスセンサ関連の基底クラス
#include "DepthCamera.h"

// デプス画像のサイズ
#define DEPTH_W 320
#define DEPTH_H 240

// カラー画像のサイズ
#define COLOR_W 640
#define COLOR_H 480

// NUI_IMAGE_RESOLUTION の設定
#define EXPAND_RESOLUTION(width, height) NUI_IMAGE_RESOLUTION_##width##x##height
#define RESOLUTION(width, height) EXPAND_RESOLUTION(width, height)
#define DEPTH_RESOLUTION RESOLUTION(DEPTH_W, DEPTH_H)
#define COLOR_RESOLUTION RESOLUTION(COLOR_W, COLOR_H)

class KinectV1 : public DepthCamera
{
  // 接続しているセンサの数
  static int connected;

  // 使用しているセンサの数
  static int activated;

  // センサの識別子
  INuiSensor *sensor;

  // デプスデータのストリームハンドル
  HANDLE depthStream;

  // デプスデータのイベントハンドル
  const HANDLE nextDepthFrameEvent;

  // デプスデータの計測不能点を変換するために用いる一次メモリ
  GLushort *depth;

  // デプスデータからカメラ座標を求めるときに用いる一時メモリ
  GLfloat (*position)[3];

  // カラーデータのストリームハンドル
  HANDLE colorStream;

  // カラーデータのイベントハンドル
  const HANDLE nextColorFrameEvent;

  // スクリーン座標からカメラ座標に変換する係数
  GLfloat scale[2];

  // カメラ座標を計算するシェーダ
  static std::unique_ptr<Calculate> shader;

  // バイラテラルフィルタの位置と明度の分散の uniform 変数 variance の場所
  static GLint varianceLoc;

  // スクリーン座標からカメラ座標に変換する係数の uniform 変数 scale の場所
  static GLint scaleLoc;

public:

  // コンストラクタ
  KinectV1();

  // デストラクタ
  virtual ~KinectV1();

  // 奥行きの最大値 (mm)
  static constexpr GLushort maxDepth = 10000;

  // 疑似カラー処理の範囲
  static constexpr GLfloat range[2] = { 0.8f, 4.0f };

  // デプスデータを取得する
  GLuint getDepth();

  // カメラ座標を取得する
  GLuint getPoint();

  // カメラ座標を算出する
  GLuint getPosition();

  // カラーデータを取得する
  GLuint getColor();
};

#endif
