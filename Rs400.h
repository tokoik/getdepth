#pragma once

//
// デプスセンサ関連の処理
//

// デプスセンサ関連の基底クラス
#include "DepthCamera.h"

// RealSense を使う
#if !defined(USE_REAL_SENSE)
#  define USE_REAL_SENSE 1
#  ifdef SENSOR
#    undef SENSOR
#  endif
#  define SENSOR Rs400
#endif

#if USE_REAL_SENSE

// RealSense 関連
#include <librealsense2/rs.hpp>

// 標準ライブラリ
#include <thread>
#include <mutex>
#include <string>
#include <map>

class Rs400 : public DepthCamera
{
  // 使用しているセンサの数
  static int activated;

	// デプスデータ転送用のメモリ
	std::vector<GLushort> depth;

	// 新着のデプスデータ
	const GLushort *depthPtr;

	// カメラ座標転送用のメモリ
	std::vector<std::array<GLfloat, 3>> point;

	// テクスチャ座標転送用のメモリ
	std::vector<std::array<GLfloat, 2>> uvmap;

	// カラーデータ転送用のメモリ
	std::vector<std::array<GLubyte, 3>> color;

	// 新着のカラーデータ
	const std::array<GLubyte, 3> *colorPtr;

	// カメラ座標を計算するシェーダ
	static std::unique_ptr<Calculate> shader;

  // デプスセンサの主点位置の uniform 変数 dpp の場所
  static GLint dppLoc;

  // デプスセンサの焦点距離の unform 変数 df の場所
  static GLint dfLoc;

	// データ取得用のスレッド
	std::thread worker;

	// スレッドの継続フラグ
	bool run;

	// デバイスの mutex
	std::mutex deviceMutex;

	// RealSense のデバイスリスト
	static std::map<std::string, rs2::device *> devices;

	// RealSense のパイプライン
	rs2::pipeline pipe;

	// RealSense のパイプラインのプロファイル
	rs2::pipeline_profile profile;

  // RealSense のデプスストリームの内部パラメータ
  rs2_intrinsics depthIntrinsics;

  // RealSense のカラーストリームの内部パラメータ
  rs2_intrinsics colorIntrinsics;

  // RealSense のカラーセンサに対するデプスセンサの外部パラメータ
  rs2_extrinsics extrinsics;

	// RealSense を有効にする
	void add_device(rs2::device &dev);

	// RealSense を無効にする
	void remove_device(const rs2::event_information &info);

	// 接続されている RealSense の数を調べる
	int count_device();

public:

  // コンストラクタ
	Rs400();

  // デストラクタ
  virtual ~Rs400();

  // 計測不能点のデフォルト距離
  static constexpr GLushort maxDepth = 5000;

  // 疑似カラー処理の範囲
  static constexpr GLfloat range[2] = { 0.4f, 4.0f };

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
