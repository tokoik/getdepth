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

	// 新着のデプスデータ
	const GLushort *depthPtr;

	// カメラ座標転送用のメモリ
	std::vector<Point> point;

	// テクスチャ座標転送用のメモリ
	std::vector<Uvmap> uvmap;

	// カラーデータ転送用のメモリ
	std::vector<Color> color;

	// 新着のカラーデータ
	const Color *colorPtr;

	// カメラ座標を計算するシェーダ
	static std::unique_ptr<Compute> shader;

  // デプスデータのイメージユニットの uniform 変数 depth の場所
  static GLint depthLoc;

  // カメラ座標のイメージユニットの uniform 変数 point の場所
  static GLint pointLoc;

  // デプスセンサの主点位置の uniform 変数 dpp の場所
  static GLint dppLoc;

  // デプスセンサの焦点距離の unform 変数 df の場所
  static GLint dfLoc;

  // カラーセンサの主点位置の uniform 変数 dpp の場所
  static GLint cppLoc;

  // カラーセンサの焦点距離の unform 変数 df の場所
  static GLint cfLoc;

  // 奥行きの最大値の uniform 変数 maxDepth の場所
  static GLint maxDepthLoc;

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

  // RealSense のカラーセンサに対するデプスセンサの外部パラメータの uniform 変数の場所
  static GLint extRotationLoc, extTranslationLoc;

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
  static constexpr GLushort maxDepth = 0;

  // 疑似カラー処理の範囲
  static constexpr GLfloat range[2] = { 0.3f, 5.0f };

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
