#include "Rs400.h"

//
// RealSense 関連の処理
//

#if USE_REAL_SENSE

// RealSense 関連
#pragma comment (lib, "realsense2.lib")

	// パイプラインの設定
constexpr int depth_width = 640;		// depth_intr.width;
constexpr int depth_height = 480;		// depth_intr.height;
constexpr int depth_fps = 30;				// 30 or 60 due to resolution
constexpr int color_width = 640;		// color_intr.width
constexpr int color_height = 480;		// color_intr.height
constexpr int color_fps = 30;				// 30 or 60

// 標準ライブラリ
#include <iostream>
#include <climits>
#include <cassert>

#if defined(DEBUG)
static void check_format(int format)
{
	switch (format)
	{
	case RS2_FORMAT_ANY: std::cout << "RS2_FORMAT_ANY\n"; break;
	case RS2_FORMAT_Z16: std::cout << "RS2_FORMAT_Z16\n"; break;
	case RS2_FORMAT_DISPARITY16: std::cout << "RS2_FORMAT_DISPARITY16\n"; break;
	case RS2_FORMAT_XYZ32F: std::cout << "RS2_FORMAT_XYZ32F\n"; break;
	case RS2_FORMAT_YUYV: std::cout << "RS2_FORMAT_YUYV\n"; break;
	case RS2_FORMAT_RGB8: std::cout << "RS2_FORMAT_RGB8\n"; break;
	case RS2_FORMAT_BGR8: std::cout << "RS2_FORMAT_BGR8\n"; break;
	case RS2_FORMAT_RGBA8: std::cout << "RS2_FORMAT_RGBA8\n"; break;
	case RS2_FORMAT_BGRA8: std::cout << "RS2_FORMAT_BGRA8\n"; break;
	case RS2_FORMAT_Y8: std::cout << "RS2_FORMAT_Y8\n"; break;
	case RS2_FORMAT_Y16: std::cout << "RS2_FORMAT_Y16\n"; break;
	case RS2_FORMAT_RAW10: std::cout << "RS2_FORMAT_RAW10\n"; break;
	case RS2_FORMAT_RAW16: std::cout << "RS2_FORMAT_RAW16\n"; break;
	case RS2_FORMAT_RAW8: std::cout << "RS2_FORMAT_RAW8\n"; break;
	case RS2_FORMAT_UYVY: std::cout << "RS2_FORMAT_UYVY\n"; break;
	case RS2_FORMAT_MOTION_RAW: std::cout << "RS2_FORMAT_MOTION_RAW\n"; break;
	case RS2_FORMAT_MOTION_XYZ32F: std::cout << "RS2_FORMAT_MOTION_XYZ32F\n"; break;
	case RS2_FORMAT_GPIO_RAW: std::cout << "RS2_FORMAT_GPIO_RAW\n"; break;
	case RS2_FORMAT_6DOF: std::cout << "RS2_FORMAT_6DOF\n"; break;
	case RS2_FORMAT_DISPARITY32: std::cout << "RS2_FORMAT_DISPARITY32\n"; break;
	case RS2_FORMAT_COUNT: std::cout << "RS2_FORMAT_COUNT\n"; break;
	default: break;
	}
}
#  define CHECK_FRAME_FORMAT(format) check_format(format)
#else
#  define CHECK_FRAME_FORMAT(format)
#endif

// コンストラクタ
Rs400::Rs400()
	: run(false)
	, depth(nullptr)
	, color(nullptr)
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
		for (auto &&device : context->query_devices())
		{
			// それを有効にする
			enable_device(device);
		}
	}

	// RealSense の使用台数が接続台数に達していれば戻る
	if (++activated > device_count())
	{
		setMessage("RealSense の数が足りません");
		return;
	}

	// パイプラインの設定
	conf.enable_stream(RS2_STREAM_DEPTH, depth_width, depth_height, RS2_FORMAT_Z16, depth_fps);
	conf.enable_stream(RS2_STREAM_COLOR, color_width, color_height, RS2_FORMAT_RGB8, color_fps);

	// パイプラインをその設定で開始する
	profile = pipe.start(conf);

	// デプスストリーム
	const auto dstream(profile.get_stream(RS2_STREAM_DEPTH).as<rs2::video_stream_profile>());

	// デプスストリームの内部パラメータ
  depthIntrinsics = dstream.get_intrinsics();

#if defined(DEBUG)
	// 内部パラメータの確認
	std::cerr << "Width = " << depthIntrinsics.width << ", Height = " << depthIntrinsics.height << "\n";
	std::cerr << "Principal Point = (" << depthIntrinsics.ppx << ", " << depthIntrinsics.ppy << ")\n";
	std::cerr << "Focal Length = (" << depthIntrinsics.fx << ", " << depthIntrinsics.fy << ")\n";
	std::cerr << "Distortion coefficients =";
	for (auto c : depthIntrinsics.coeffs) std::cout << " " << c;
	std::cerr << "\n";

	const auto sensor = profile.get_device().first<rs2::depth_sensor>();
	std::cerr << "Scale = " << sensor.get_depth_scale() <<"\n";
#endif

	// デプスフレームの幅と高さ
	depthWidth = depthIntrinsics.width;
	depthHeight = depthIntrinsics.height;

	// カラーストリーム
	const auto cstream(profile.get_stream(RS2_STREAM_COLOR).as<rs2::video_stream_profile>());

	// カラーストリームの内部パラメータ
	colorIntrinsics = cstream.get_intrinsics();

#if defined(DEBUG)
	// 内部パラメータの確認
	std::cerr << "Width = " << colorIntrinsics.width << ", Height = " << colorIntrinsics.height << "\n";
	std::cerr << "Principal Point = (" << colorIntrinsics.ppx << ", " << colorIntrinsics.ppy << ")\n";
	std::cerr << "Focal Length = (" << colorIntrinsics.fx << ", " << colorIntrinsics.fy << ")\n";
	std::cerr << "Distortion coefficients =";
	for (auto c : colorIntrinsics.coeffs) std::cout << " " << c;
	std::cerr << "\n";
#endif

	// カラーフレームの幅と高さ
	colorWidth = colorIntrinsics.width;
	colorHeight = colorIntrinsics.height;

  // カラーテクスチャのスケールを求める
  colorScale[0] = 1.0f;
  colorScale[1] = 1.0f;

  // depthCount と colorCount を計算してテクスチャとバッファオブジェクトを作成する
	makeTexture();

	// データ転送用のメモリを確保する
	depth = new GLushort[depthCount];
	point = new GLfloat[depthCount][3];
	color = new GLubyte[colorCount][3];

	// まだシェーダが作られていなかったら
	if (shader.get() == nullptr)
	{
		// カメラ座標算出用のシェーダを作成する
		shader.reset(new Calculate(depthWidth, depthHeight, "position_rs" SHADER_EXT));

		// シェーダの uniform 変数の場所を調べる
		varianceLoc = glGetUniformLocation(shader->get(), "variance");
    dppLoc = glGetUniformLocation(shader->get(), "dpp");
    dfLoc = glGetUniformLocation(shader->get(), "df");
  }

	// まだスレッドが走っていなかったら
	if (!worker.joinable())
	{
		// ループが回るようにする
		run = true;

		// イベントループを開始する
		worker = std::thread([&]()
		{
			while (run)
			{
				// フレームを取り出す
				frames = pipe.wait_for_frames();

				// デバイスをロックする
				std::lock_guard<std::mutex> lock(deviceMutex);

				// デプスフレームを取り出す
				const auto dframe(frames.get_depth_frame());

        // カメラ座標を求める
        const GLushort *const d(static_cast<const unsigned short *>(dframe.get_data()));
        for (int i = 0; i < depthCount; ++i)
        {
          // デプスを取り出す
          depth[i] = d[i];

          // 計測不能点だったら最遠点に飛ばす
          if (depth[i] == 0) depth[i] = maxDepth;
        }

				// デプスデータの新着を知らせる
				depthPtr = depth;

				// カラーフレームを取り出す
				const auto cframe(frames.get_color_frame());

				// カラーフレームを確保したメモリに格納する
				memcpy(color, cframe.get_data(), colorCount * sizeof *color);

				// カラーデータの新着を知らせる
				colorPtr = color;

				// テクスチャ座標を求める
        const rs2::points points(pc.calculate(dframe));
        pc.map_to(cframe);
        uvmap = reinterpret_cast<const GLfloat(*)[2]>(points.get_texture_coordinates());
			}
		});
	}
}

// デストラクタ
Rs400::~Rs400()
{
	// イベントループを停止する
	if (worker.joinable())
	{
		// イベントループを停止する
		run = false;

		// イベントループのスレッドが終了するのを待つ
		worker.join();
	}

	// データ転送用のメモリの開放
	delete[] depth;
	delete[] color;
}

// RealSense を有効にする
void Rs400::enable_device(rs2::device dev)
{
	// RealSense のシリアル番号を調べる
	std::string serial_number(dev.get_info(RS2_CAMERA_INFO_SERIAL_NUMBER));

	// デバイスをロックする
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

	// このデバイスのシリアル番号の RealSense をパイプラインで使用できるようにする
	conf.enable_device(serial_number);

	// このデバイスを登録する
	devices.emplace(serial_number, this);
}

// RealSense を無効にする
void Rs400::remove_devices(const rs2::event_information& info)
{
	// デバイスをロックする
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
int Rs400::device_count()
{
	// デバイスをロックする
	std::lock_guard<std::mutex> lock(deviceMutex);

	// デバイスリストの数を返す
	return static_cast<int>(devices.size());
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
		// デバイスごとのストリームについて
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
			for (size_t i = 0; i < frameset.size(); ++i)
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
	// デプスデータのテクスチャを指定する
	glBindTexture(GL_TEXTURE_2D, depthTexture);

	// デプスデータが更新されておりデプスデータの取得中でなければ
	if (depthPtr && deviceMutex.try_lock())
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
		deviceMutex.unlock();
	}

	return depthTexture;
}

// カメラ座標を取得する
GLuint Rs400::getPoint()
{
	// カメラ座標のテクスチャを指定する
	glBindTexture(GL_TEXTURE_2D, pointTexture);

	// デプスデータが更新されており RealSense がデプスデータの取得中でなければ
	if (depthPtr && deviceMutex.try_lock())
	{
    // カメラ座標を求める
    for (int i = 0; i < depthCount; ++i)
    {
      // カメラ座標系の z 座標値を m 単位で求める
      point[i][2] = -0.001f * depth[i];

      // カメラ座標系の x, y 座標値を m 単位で求める
      point[i][0] = (i % depthWidth - depthIntrinsics.ppx) * point[i][2] / depthIntrinsics.fx;
      point[i][1] = (i / depthWidth - depthIntrinsics.ppy) * point[i][2] / depthIntrinsics.fy;
    }

    // カメラ座標をテクスチャに転送する
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, depthWidth, depthHeight, GL_RGB, GL_FLOAT, point);

		// テクスチャ座標のバッファオブジェクトを指定する
		glBindBuffer(GL_ARRAY_BUFFER, coordBuffer);

		// テクスチャ座標をバッファオブジェクトに転送する
		glBufferSubData(GL_ARRAY_BUFFER, 0, depthCount * 2 * sizeof(GLfloat), uvmap);

		// 一度送ってしまえば更新されるまで送る必要がないのでデータは不要
		depthPtr = nullptr;

		// デプスデータをアンロックする
		deviceMutex.unlock();
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
  glUniform2f(dppLoc, depthIntrinsics.ppx, depthIntrinsics.ppy);
  glUniform2f(dfLoc, depthIntrinsics.fx, depthIntrinsics.fy);
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
	if (colorPtr && deviceMutex.try_lock())
	{
		// カラーデータをテクスチャに転送する
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, colorWidth, colorHeight, GL_RGB, GL_UNSIGNED_BYTE, colorPtr);

    // 一度送ってしまえば更新されるまで送る必要がないのでデータは不要
		colorPtr = nullptr;

		// カラーデータをアンロックする
		deviceMutex.unlock();
	}

	return colorTexture;
}

// 使用しているセンサの数
int Rs400::activated(0);

// カメラ座標を計算するシェーダ
std::unique_ptr<Calculate> Rs400::shader(nullptr);

// バイラテラルフィルタの分散の uniform 変数 variance の場所
GLint Rs400::varianceLoc;

// カメラパラメータの uniform 変数の場所
GLint Rs400::dppLoc, Rs400::dfLoc;

// RealSense のデバイスリスト
std::map<std::string, Rs400 *> Rs400::devices;

#endif
