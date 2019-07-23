#pragma once

//
// デプスセンサ関連の処理
//

// DepthSense を使う
#if !defined(USE_DEPTH_SENSE)
#  define USE_DEPTH_SENSE 1
#  ifdef SENSOR
#    undef SENSOR
#  endif
#  define SENSOR Ds325
#endif

#if USE_DEPTH_SENSE

// DepthSense 関連
#include <Windows.h>
#include <DepthSense.hxx>
using namespace DepthSense;
#undef INT64_C
#undef UINT64_C

// 標準ライブラリ
#include <thread>
#include <mutex>

// デプスセンサ関連の基底クラス
#include "DepthCamera.h"

// キャプチャ中のメッセージ出力
#define CAPTURE_VERBOSE         0

// DepthSense の種類
#define DS311                   0
#define DS325                   1

// 使用する DepthSense 
#define DEPTHSENSE_MODEL        DS325

// DepthSense の動作モード
#if DEPTHSENSE_MODEL == DS325
const FrameFormat capture_depth_format(FRAME_FORMAT_QVGA);
const DepthNode::CameraMode capture_depth_mode(DepthNode::CAMERA_MODE_CLOSE_MODE);
const FrameFormat capture_color_format(FRAME_FORMAT_VGA);
const CompressionType capture_color_compression(COMPRESSION_TYPE_MJPEG);
#else
const FrameFormat capture_depth_format(FRAME_FORMAT_QQVGA);
const DepthNode::CameraMode capture_depth_mode(DepthNode::CAMERA_MODE_LONG_RANGE);
const FrameFormat capture_color_format(FRAME_FORMAT_VGA);
const CompressionType capture_color_compression(COMPRESSION_TYPE_YUY2);
#endif

// デプスセンサのフレームレート
const unsigned int capture_depth_fps(60);

// カラーセンサのフレームレート
const unsigned int capture_color_fps(30);

// 電源周波数
const PowerLineFrequency color_frequency(POWER_LINE_FREQUENCY_60HZ);    // 関西
//const PowerLineFrequency color_frequency(POWER_LINE_FREQUENCY_50HZ);    // 関東

class Ds325 : public DepthCamera
{
  // 接続しているセンサの数
  static int connected;

  // 使用しているセンサの数
  static int activated;

  // デプスセンサの解像度
  const FrameFormat depth_format;

  // デプスセンサのフレームレート
  const unsigned int depth_fps;

  // デプスセンサのモード
  const DepthNode::CameraMode depth_mode;

  // カラーセンサの解像度
  const FrameFormat color_format;

  // カラーセンサのフレームレート
  const unsigned int color_fps;

  // カラーセンサのデータ圧縮方式
  const CompressionType color_compression;

  // 電源周波数
  const PowerLineFrequency power_line_frequency;

  // DepthSense のコンテキスト
  static Context context;

  // データ取得用のスレッド
  static std::thread worker;

  // ノードを登録する
  void configureNode(Node &node);

  // ノードを削除する
  void unregisterNode(Node node);

  // DepthSense が取り付けられた時の処理
  static void onDeviceConnected(Context context, Context::DeviceAddedData data);

  // DepthSense が取り外されたときの処理
  static void Ds325::onDeviceDisconnected(Context context, Context::DeviceRemovedData data);

  // ノードが接続された時の処理
  static void onNodeConnected(Device device, Device::NodeAddedData data, Ds325 *sensor);

  // ノードの接続が解除された時の処理
  static void onNodeDisconnected(Device device, Device::NodeRemovedData data, Ds325 *sensor);

  // デプスノード
  DepthNode depthNode;

  // デプスノードを初期化する
  void configureDepthNode(DepthNode &dnode);

  // デプスノードのイベント発生時の処理
  static void onNewDepthSample(DepthNode node, DepthNode::NewSampleReceivedData data, Ds325 *sensor);

  // デプスノード用の mutex
  std::mutex depthMutex;

  // デプスセンサの内部パラメータ
  IntrinsicParameters depthIntrinsics;

  // デプスデータ転送用のメモリ
	std::vector<GLushort> depth;

	// 新着のデプスデータ
	const GLushort *depthPtr;

  // カメラ座標転送用のメモリ
  std::vector<Point> point;

  // テクスチャ座標転送用のメモリ
  std::vector<Uvmap> uvmap;

  // カラーノード
  ColorNode colorNode;

  // カラーノードを初期化する
  void configureColorNode(ColorNode &cnode);

  // カラーノードのイベント発生時の処理
  static void onNewColorSample(ColorNode node, ColorNode::NewSampleReceivedData data, Ds325 *sensor);

  // カラーノード用の mutex
  std::mutex colorMutex;

  // カラーセンサの内部パラメータ
  IntrinsicParameters colorIntrinsics;

  // カラーデータ転送用のメモリ
	std::vector<Color> color;

	// 新着のカラーデータ
	const Color *colorPtr;

  // カメラ座標を計算するシェーダ
  static std::unique_ptr<Compute> shader;

  // カメラパラメータの uniform 変数の場所
  static GLint dcLoc, dfLoc, dkLoc;

public:

  // コンストラクタ
  Ds325(
    FrameFormat depth_format = capture_depth_format,  // デプスセンサの解像度
    unsigned int depth_fps = capture_depth_fps,       // デプスセンサのフレームレート
    DepthNode::CameraMode = capture_depth_mode,       // デプスセンサのモード
    FrameFormat color_format = capture_color_format,  // カラーセンサの解像度
    unsigned int color_fps = capture_color_fps,       // カラーセンサのフレームレート
    CompressionType color_compression = capture_color_compression,  // カラーセンサの圧縮方式
    PowerLineFrequency frequency = color_frequency    // 電源周波数
    );

  // デストラクタ
  virtual ~Ds325();

#if DEPTHSENSE_MODEL == DS325
  // 計測不能点のデフォルト距離 (mm)
  static constexpr GLushort maxDepth = 4000;

  // 疑似カラー処理の範囲
  static constexpr GLfloat range[2] = { 0.2f, 1.0f };
#else
  // 計測不能点のデフォルト距離
  static constexpr GLushort maxDepth = 10000;

  // 疑似カラー処理の範囲
  static constexpr GLfloat range[2] = { 0.4f, 6.0f };
#endif

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
