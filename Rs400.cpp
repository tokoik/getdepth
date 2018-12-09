#include "Rs400.h"

//
// DepthSense 関連の処理
//

#if USE_REAL_SENSE

// RealSense 関連
#pragma comment (lib, "realsense2.lib")

// 標準ライブラリ
#include <iostream>
#include <climits>

// コンストラクタ
Rs400::Rs400()
{
	// RealSense のコンテキスト
	static std::unique_ptr<rs2::context> context(nullptr);

	// 最初のオブジェクトの時だけ
	if (!context)
	{
		// コンテキストを作成して
		context.reset(new rs2::context);

		// デバイスが変更されたときに呼び出すコールバック関数を指定する
		context->set_devices_changed_callback([&](rs2::event_information &info)
		{
			// 取り外されたデバイスがあればそれを削除する
			remove_devices(info);

			// すべての新しく取り付けられたデバイスについて
			for (auto &&device : info.get_new_devices())
			{
				// それを有効にする
				enable_device(device);
			}
		});

		// 最初からつながってるすべてのデバイスについて
		for (auto &&dev : context->query_devices())
		{
			// それを有効にする
			enable_device(dev);
		}
	}

  // DepthSense の使用台数が接続台数に達していれば戻る
  if (++activated > connected)
  {
    setMessage("RealSense の数が足りません");
    return;
  }

  // depthCount と colorCount を計算してテクスチャとバッファオブジェクトを作成する
  makeTexture();

  // データ転送用のメモリを確保する
  depth = new GLushort[depthCount];
  point = new GLfloat[depthCount][3];
  uvmap = new GLfloat[depthCount][2];
  color = new GLubyte[colorCount][3];
}

// デストラクタ
Rs400::~Rs400()
{
  // RealSense が有効になっていたら
  if (--activated >= 0)
  {
    // 最後の RealSense を削除するときはイベントループを停止する
    if (activated == 0 && worker.joinable())
    {
      // イベントループのスレッドが終了するのを待つ
      worker.join();
    }

    // データ転送用のメモリの開放
    delete[] depth;
    delete[] point;
    delete[] uvmap;
    delete[] color;
  }
}

// RealSense を有効にする
void Rs400::enable_device(rs2::device dev)
{
	// RealSense のシリアル番号を調べる
	std::string serial_number(dev.get_info(RS2_CAMERA_INFO_SERIAL_NUMBER));

	// デバイスリストをロックする
	std::lock_guard<std::mutex> lock(deviceMutex);

	// デバイスリストのなかにデバイスがあるか調べる
	if (devices.find(serial_number) != devices.end())
	{
		// 既にデバイスリストの中にある
		return;
	}

	// RealSense 以外のカメラかどうか調べる
	const std::string platform_camera_name = "Platform Camera";
	if (platform_camera_name == dev.get_info(RS2_CAMERA_INFO_NAME))
	{
		// RealSense ではない
		return;
	}

	// 指定したシリアル番号の RealSense をパイプラインで使用できるようにする
	conf.enable_device(serial_number);

	// パイプラインをその設定で開始する
	profile = pipe.start(conf);

	// このデバイスを登録する
	devices.emplace(serial_number, this);
}

// RealSense を無効にする
void Rs400::remove_devices(const rs2::event_information& info)
{
	// デバイスリストをロックする
	std::lock_guard<std::mutex> lock(deviceMutex);

	// デバイスリストのすべてのデバイスについて
	for (auto device = devices.begin(); device != devices.end();)
	{
		// そのデバイスが削除されていたら
		if (info.was_removed(device->second->profile.get_device()))
		{
			// そのデバイスをデバイスリストから削除して先に進む
			device = devices.erase(device);
		}
		else
		{
			// そのデバイスをデバイスとリストに残して先に進む
			++device;
		}
	}
}

// 接続されている RealSense の数を調べる
size_t Rs400::device_count()
{
	// デバイスリストをロックする
	std::lock_guard<std::mutex> lock(deviceMutex);

	// デバイスリストの数を返す
	return devices.size();
}

// RealSense のストリーム数を調べる
int Rs400::stream_count()
{
	// デバイスリストをロックする
	std::lock_guard<std::mutex> lock(deviceMutex);

	// ストリームの数
	int count(0);

	// すべてのデバイスについて
	for (auto &&sn_to_dev : devices)
	{
		// デバイスごとのストームについて
		for (auto &&stream : sn_to_dev.second->frames_per_stream)
		{
			// 存在するストリームの数を数える
			if (stream.second) count++;
		}
	}

	// ストリームの数を返す
	return count;
}

// RealSense からフレームを取り出す
void Rs400::poll_frames()
{
	// デバイスリストをロックする
	std::lock_guard<std::mutex> lock(deviceMutex);

	// すべてのデバイスについて
	for (auto &&view : devices)
	{
		// そのパイプラインからフレームセット全体を取り出す
		rs2::frameset frameset;
		if (view.second->pipe.poll_for_frames(&frameset))
		{
			// フレームセットの個々のフレームについて
			for (int i = 0; i < frameset.size(); ++i)
			{
				// 一つのフレームを取り出す
				rs2::frame new_frame = frameset[i];

				// ストリームの番号を求める
				int stream_id = new_frame.get_profile().unique_id();

				// そのストリームのフレームを更新する
				view.second->frames_per_stream[stream_id] = view.second->colorize_frame.process(new_frame);
			}
		}
	}
}

// デプスデータを取得する
GLuint Rs400::getDepth()
{
	// RealSense からフレームを取り出す
	frames = pipe.wait_for_frames();

	// 取り出したフレームからデプスデータを切り出す
	rs2::depth_frame depth = frames.get_depth_frame();
	std::cout << depth.get_bits_per_pixel() << "\n";

  // デプスデータのテクスチャを指定する
  glBindTexture(GL_TEXTURE_2D, depthTexture);

  // デプスデータが更新されておりデプスデータの取得中でなければ
  if (depthPtr && depthMutex.try_lock())
  {
    // テクスチャ座標のバッファオブジェクトを指定する
    glBindBuffer(GL_ARRAY_BUFFER, coordBuffer);

    // テクスチャ座標をバッファオブジェクトに転送する
    glBufferSubData(GL_ARRAY_BUFFER, 0, depthCount * 2 * sizeof (GLfloat), uvmap);

    // デプスデータをテクスチャに転送する
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, depthWidth, depthHeight, GL_RED_INTEGER, GL_UNSIGNED_SHORT, depthPtr);

    // 一度送ってしまえば更新されるまで送る必要がないのでデータは不要
    depthPtr = nullptr;

    // デプスデータをアンロックする
    depthMutex.unlock();
  }

  return depthTexture;
}

// カメラ座標を取得する
GLuint Rs400::getPoint()
{
  // カメラ座標のテクスチャを指定する
  glBindTexture(GL_TEXTURE_2D, pointTexture);

  // デプスデータが更新されており DepthSense がデプスデータの取得中でなければ
  if (depthPtr && depthMutex.try_lock())
  {
    // カメラ座標をテクスチャに転送する
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, depthWidth, depthHeight, GL_RGB, GL_FLOAT, point);

    // テクスチャ座標のバッファオブジェクトを指定する
    glBindBuffer(GL_ARRAY_BUFFER, coordBuffer);

    // テクスチャ座標をバッファオブジェクトに転送する
    glBufferSubData(GL_ARRAY_BUFFER, 0, depthCount * 2 * sizeof (GLfloat), uvmap);

    // 一度送ってしまえば更新されるまで送る必要がないのでデータは不要
    depthPtr = nullptr;

    // デプスデータをアンロックする
    depthMutex.unlock();
  }

  return pointTexture;
}

// カメラ座標を算出する
GLuint Rs400::getPosition()
{
  // テクスチャ座標のバッファオブジェクトを指定する
  glBindBuffer(GL_ARRAY_BUFFER, coordBuffer);

  // テクスチャ座標をバッファオブジェクトに転送する
  glBufferSubData(GL_ARRAY_BUFFER, 0, depthCount * 2 * sizeof (GLfloat), uvmap);

  // カメラ座標をシェーダで算出する
  shader->use();
  glUniform1f(varianceLoc, variance);
  const GLuint depthTexture(getDepth());
  const GLenum depthFormat(GL_R16UI);
  return shader->execute(1, &depthTexture, &depthFormat, 16, 16)[0];
}

// カラーデータを取得する
GLuint Rs400::getColor()
{
  // カラーデータのテクスチャを指定する
  glBindTexture(GL_TEXTURE_2D, colorTexture);

  // カラーデータが更新されておりカラーデータの取得中でなければ
  if (colorPtr && colorMutex.try_lock())
  {
    // カラーデータをテクスチャに転送する
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, colorWidth, colorHeight, GL_BGR, GL_UNSIGNED_BYTE, colorPtr);

    // 一度送ってしまえば更新されるまで送る必要がないのでデータは不要
    colorPtr = nullptr;

    // カラーデータをアンロックする
    colorMutex.unlock();
  }

  return colorTexture;
}

// 接続しているセンサの数
int Rs400::connected(-1);

// 使用しているセンサの数
int Rs400::activated(0);

// データ取得用のスレッド
std::thread Rs400::worker;

// カメラ座標を計算するシェーダ
std::unique_ptr<Calculate> Rs400::shader(nullptr);

// バイラテラルフィルタの分散の uniform 変数 variance の場所
GLint Rs400::varianceLoc;

// RealSense のデバイスリスト
std::map<std::string, Rs400 *> Rs400::devices;

#endif
