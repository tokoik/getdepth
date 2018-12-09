#pragma once

//
// 深度センサ関連の処理
//

// RealSense を使う
#if !defined(USE_REAL_SENSE)
#  define USE_REAL_SENSE 1
#  ifdef SENSOR
#    undef SENSOR
#  endif
#  define SENSOR Rs400
#endif

#if USE_REAL_SENSE

// DepthSense 関連
#include <librealsense2/rs.hpp>

// 標準ライブラリ
#include <thread>
#include <mutex>
#include <string>
#include <map>

// 深度センサ関連の基底クラス
#include "DepthCamera.h"

class Rs400 : public DepthCamera
{
  // 接続しているセンサの数
  static int connected;

  // 使用しているセンサの数
  static int activated;

  // データ取得用のスレッド
  static std::thread worker;

  // デプスノード用の mutex
  std::mutex depthMutex;

	// デプスデータ転送用のメモリ
	GLushort *depth, *depthPtr;

	// カメラ座標転送用のメモリ
	GLfloat(*point)[3];

	// テクスチャ座標転送用のメモリ
	GLfloat(*uvmap)[2];

	// カラーノード用の mutex
	std::mutex colorMutex;

	// カラーデータ転送用のメモリ
	GLubyte(*color)[3], (*colorPtr)[3];

	// カメラ座標を計算するシェーダ
	static std::unique_ptr<Calculate> shader;

	// バイラテラルフィルタの分散の uniform 変数 variance の場所
	static GLint varianceLoc;

	// デバイスの mutex
	std::mutex deviceMutex;

	// RealSense のデバイスリスト
	static std::map<std::string, Rs400 *> devices;

	// RealSense の設定情報
	rs2::config conf;

	// RealSense のパイプライン
	rs2::pipeline pipe;

	// RealSense のパイプラインのプロファイル
	rs2::pipeline_profile profile;

	// ストームごとのフレーム数
	std::map<int, rs2::frame> frames_per_stream;

	// カラー化されたフレーム
	rs2::colorizer colorize_frame;

	// RealSense から取り出したフレーム
	rs2::frameset frames;

	// Declare pointcloud object, for calculating pointclouds and texture mappings
	rs2::pointcloud pc;

	// We want the points object to be persistent so we can display the last cloud when a frame drops
	rs2::points points;

	// RealSense を有効にする
	void enable_device(rs2::device dev);

	// RealSense を無効にする
	void remove_devices(const rs2::event_information& info);

	// 接続されている RealSense の数を調べる
	size_t device_count();

	// RealSense のストリーム数を調べる
	int stream_count();

	// RealSense からフレームを取り出す
	void poll_frames();


public:

  // コンストラクタ
	Rs400();

  // デストラクタ
  virtual ~Rs400();

  // 計測不能点のデフォルト距離
  static constexpr GLushort maxDepth = 10000;

  // 疑似カラー処理の範囲
  static constexpr GLfloat range[2] = { 0.4f, 10.0f };

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
