#include "Rs400.h"

//
// RealSense 関連の処理
//

#if USE_REAL_SENSE

// RealSense 関連
#ifdef _WIN32
#  pragma comment (lib, "realsense2.lib")
#endif

// 標準ライブラリ
#include <iostream>
#include <climits>
#include <cassert>

// デプスデータをカラーデータに合わせる場合 1
#define ALIGN_TO_COLOR 0

#define D415 0
#define D435 1
#define D455 2
#define L515 3

#define REALSENSE D415

#if REALSENSE == D415

// センサの画角
constexpr GLfloat depth_fovx{ 1.1345f };  // 65°
constexpr GLfloat depth_fovy{ 0.6981f };  // 40°
constexpr GLfloat color_fovx{ 1.2043f };  // 69°
constexpr GLfloat color_fovy{ 0.7330f };  // 42°


// パイプラインの設定
constexpr int depth_width = 1280;		      // depth_intr.width;
constexpr int depth_height = 720;		      // depth_intr.height;
constexpr int depth_fps = 30;				      // 30 or 60 due to resolution
constexpr int color_width = 1920;		      // color_intr.width
constexpr int color_height = 1080;	      // color_intr.height
constexpr int color_fps = 30;				      // 30 or 60

#elif REALSENSE == D435

// センサの画角
constexpr GLfloat depth_fovx{ 1.5184f };  // 87°
constexpr GLfloat depth_fovy{ 1.0123f };  // 58°
constexpr GLfloat color_fovx{ 1.2043f };  // 69°
constexpr GLfloat color_fovy{ 0.7330f };  // 42°

// パイプラインの設定
constexpr int depth_width = 1280;		      // depth_intr.width;
constexpr int depth_height = 720;		      // depth_intr.height;
constexpr int depth_fps = 30;				      // 30 or 60 due to resolution
constexpr int color_width = 1920;		      // color_intr.width
constexpr int color_height = 1080;        // color_intr.height
constexpr int color_fps = 30;				      // 30 or 60

#elif REALSENSE == D455

// センサの画角
constexpr GLfloat depth_fovx{ 1.5184f };  // 87°
constexpr GLfloat depth_fovy{ 1.0123f };  // 58°
constexpr GLfloat color_fovx{ 1.5708f };  // 90°
constexpr GLfloat color_fovy{ 1.1335f };  // 65°

// パイプラインの設定
constexpr int depth_width = 1280;		      // depth_intr.width;
constexpr int depth_height = 720;		      // depth_intr.height;
constexpr int depth_fps = 30;				      // 30 or 60 due to resolution
constexpr int color_width = 1280;		      // color_intr.width
constexpr int color_height = 720;	        // color_intr.height
constexpr int color_fps = 30;				      // 30 or 60

#elif REALSENSE == L515

// センサの画角
constexpr GLfloat depth_fovx{ 1.2217f };  // 70°
constexpr GLfloat depth_fovy{ 0.9599f };  // 55°
constexpr GLfloat color_fovx{ 1.1345f };  // 65°
constexpr GLfloat color_fovy{ 0.6981f };  // 40°

// パイプラインの設定
constexpr int depth_width = 1024;		      // depth_intr.width;
constexpr int depth_height = 768;		      // depth_intr.height;
constexpr int depth_fps = 30;				      // 30 or 60 due to resolution
constexpr int color_width = 1920;		      // color_intr.width
constexpr int color_height = 1080;	      // color_intr.height
constexpr int color_fps = 30;				      // 30 or 60

#endif

#if defined(DEBUG)
static void check_frame_format(int format)
{
  std::cerr << "\n";

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
#  define CHECK_FRAME_FORMAT(format) check_frame_format(format)
static void check_intrinsics(const rs2_intrinsics &intrinsics)
{
  std::cerr << "\n";
  std::cerr << "Width = " << intrinsics.width << ", Height = " << intrinsics.height << "\n";
  std::cerr << "Principal Point = (" << intrinsics.ppx << ", " << intrinsics.ppy << ")\n";
  std::cerr << "Focal Length = (" << intrinsics.fx << ", " << intrinsics.fy << ")\n";
  std::cerr << "Distortion coefficients =";
  for (auto c : intrinsics.coeffs) std::cout << " " << c;
  std::cerr << "\n";
  switch (intrinsics.model)
  {
  case RS2_DISTORTION_INVERSE_BROWN_CONRADY:
    std::cerr << "RS2_DISTORTION_INVERSE_BROWN_CONRADY\n";
    break;
  case RS2_DISTORTION_KANNALA_BRANDT4:
    std::cerr << "RS2_DISTORTION_KANNALA_BRANDT4\n";
    break;
  case RS2_DISTORTION_FTHETA:
    std::cerr << "RS2_DISTORTION_FTHETA\n";
    break;
  default:
    break;
  }
}
#  define CHECK_INTRINSICS(intrinsics) check_intrinsics(intrinsics)
#else
#  define CHECK_FRAME_FORMAT(format)
#  define CHECK_INTRINSICS(intrinsics)
#endif

// コンストラクタ
Rs400::Rs400()
  : DepthCamera{ depth_width, depth_height, depth_fovx, depth_fovy, color_width, color_height, color_fovx, color_fovy }
  , depthPtr{ nullptr }
  , colorPtr{ nullptr }
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
			remove_device(info);

			// すべての新しく取り付けられたデバイスについて
			for (auto &&device : info.get_new_devices())
			{
				// それを有効にする
				add_device(device);
			}
		});

		// 最初からつながってるすべてのデバイスについて
		for (auto &&device : context->query_devices())
		{
			// それを有効にする
			add_device(device);
		}
	}

	// RealSense の使用台数が接続台数に達していれば戻る
	if (activated >= count_device())
	{
		setMessage("RealSense の数が足りません");
		return;
	}

  // デバイスの番号
  int id(0);

  for (const auto &device : devices)
  {
    // 未使用のデバイスがあったら
    if (id++ == activated)
    {
      // 使用中のデバイスをカウントする
      ++activated;

      // パイプラインの設定
      rs2::config conf;

      // このデバイスのシリアル番号の RealSense をパイプラインで使用できるようにする
      conf.enable_device(device.first);

      // キャプチャするデータのフォーマットを指定する
      conf.enable_stream(RS2_STREAM_DEPTH, depth_width, depth_height, RS2_FORMAT_Z16, depth_fps);
      conf.enable_stream(RS2_STREAM_COLOR, color_width, color_height, RS2_FORMAT_RGB8, color_fps);

      // この設定でパイプラインを開始する
      profile = pipe.start(conf);
      break;
    }
  }

	// デプスストリーム
	const auto dstream(profile.get_stream(RS2_STREAM_DEPTH).as<rs2::video_stream_profile>());

	// デプスストリームの内部パラメータ
  depthIntrinsics = dstream.get_intrinsics();

#if defined(DEBUG)
	// デプスセンサの内部パラメータの確認
  check_intrinsics(depthIntrinsics);
  const auto sensor(profile.get_device().first<rs2::depth_sensor>());
  std::cerr << "Scale = " << sensor.get_depth_scale() << "\n";
#endif

	// カラーストリーム
	const auto cstream(profile.get_stream(RS2_STREAM_COLOR).as<rs2::video_stream_profile>());

	// カラーストリームの内部パラメータ
	colorIntrinsics = cstream.get_intrinsics();

#if defined(DEBUG)
	// カラーセンサの内部パラメータの確認
  check_intrinsics(colorIntrinsics);
#endif

	// カラーフレームの幅と高さ
	colorWidth = colorIntrinsics.width;
	colorHeight = colorIntrinsics.height;

  // デプスフレームの幅と高さ
#if ALIGN_TO_COLOR
  depthWidth = colorWidth;
  depthHeight = colorHeight;
#else
  depthWidth = depthIntrinsics.width;
  depthHeight = depthIntrinsics.height;
#endif

  // カラーセンサに対するデプスセンサの外部パラメータ
  extrinsics = dstream.get_extrinsics_to(cstream);

  // まだ深度を平滑化するシェーダが作られていなかったら
  if (smooth.get() == nullptr)
  {
    // 深度平滑用のシェーダを作成する
    smooth.reset(new Compute("smooth_rs.comp"));

    // uniform 変数の場所を調べる
    currentLoc = glGetUniformLocation(smooth->get(), "current");
    previousLoc = glGetUniformLocation(smooth->get(), "previous");
    attenuationLoc = glGetUniformLocation(smooth->get(), "attenuation");
  }

  // まだカメラ座標を計算するシェーダが作られていなかったら
  if (position.get() == nullptr)
  {
    // カメラ座標算出用のシェーダを作成する
    position.reset(new Compute("position_rs.comp"));

    // uniform 変数の場所を調べる
    depthLoc = glGetUniformLocation(position->get(), "depth");
    pointLoc = glGetUniformLocation(position->get(), "point");
    dppLoc = glGetUniformLocation(position->get(), "dpp");
    dfLoc = glGetUniformLocation(position->get(), "df");
    cppLoc = glGetUniformLocation(position->get(), "cpp");
    cfLoc = glGetUniformLocation(position->get(), "cf");
    maxDepthLoc = glGetUniformLocation(position->get(), "maxDepth");
    extRotationLoc = glGetUniformLocation(position->get(), "extRotation");
    extTranslationLoc = glGetUniformLocation(position->get(), "extTranslation");

    // シェーダストレージブロックに結合ポイントを割り当てる
    const GLuint weightIndex{ glGetProgramResourceIndex(position->get(), GL_SHADER_STORAGE_BLOCK, "Weight") };
    glShaderStorageBlockBinding(position->get(), weightIndex, WeightBinding);
    const GLuint uvmapIndex{ glGetProgramResourceIndex(position->get(), GL_SHADER_STORAGE_BLOCK, "Uvmap") };
    glShaderStorageBlockBinding(position->get(), uvmapIndex, UvmapBinding);
  }

  // テクスチャとバッファオブジェクトを作成してポイント数を返す
  const int depthCount(makeTexture());

  // データ転送用のメモリを確保する
  point.resize(depthCount);
  uvmap.resize(depthCount);
	color.resize(colorWidth * colorHeight);
}

// デストラクタ
Rs400::~Rs400()
{
}

// RealSense を追加する
void Rs400::add_device(rs2::device &dev)
{
	// RealSense 以外のカメラでないか調べる
	if (dev.get_info(RS2_CAMERA_INFO_NAME) == "Platform Camera")
	{
		// RealSense ではない
		return;
	}

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

	// このデバイスをリストに登録する
	devices.emplace(serial_number, &dev);
}

// RealSense を無効にする
void Rs400::remove_device(const rs2::event_information &info)
{
	// デバイスをロックする
	std::lock_guard<std::mutex> lock(deviceMutex);

	// デバイスリストのすべてのデバイスについて
	for (auto device = devices.begin(); device != devices.end();)
	{
		// そのデバイスが削除されていたら
		if (info.was_removed(*device->second))
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
int Rs400::count_device()
{
	// デバイスをロックする
	std::lock_guard<std::mutex> lock(deviceMutex);

	// デバイスリストの数を返す
	return static_cast<int>(devices.size());
}

// デプスデータを取得する
GLuint Rs400::getDepth()
{
	// デプスデータのテクスチャを指定する
	glBindTexture(GL_TEXTURE_2D, depthTexture);

  // センサから取得するフレームの格納先
  rs2::frameset frames;

  // 新しいフレームが到着していれば
  if (pipe.poll_for_frames(&frames) && deviceMutex.try_lock())
  {
#if ALIGN_TO_COLOR
    // デプスデータをカラーデータに合わせる
    rs2::align align_to_color(RS2_STREAM_COLOR);
    frames = align_to_color.process(frames);
#endif

    // デプスフレームを取り出す
    const auto dframe{ frames.get_depth_frame() };
    depthPtr = static_cast<const GLushort *>(dframe.get_data());

    // デプスデータをテクスチャに転送する
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, depthWidth, depthHeight, GL_RED_INTEGER, GL_UNSIGNED_SHORT, depthPtr);

    // カラーフレームを取り出す
    const auto cframe{ frames.get_color_frame() };
    colorPtr = static_cast<const Color *>(cframe.get_data());

    // デプスデータをアンロックする
    deviceMutex.unlock();
  }

  return depthTexture;
}

// カメラ座標を取得する
GLuint Rs400::getPoint()
{
  // デプスデータの読み込み
  getDepth();

  // カメラ座標のテクスチャを指定する
	glBindTexture(GL_TEXTURE_2D, pointTexture);

	// デプスデータが更新されており RealSense がデプスデータの取得中でなければ
	if (depthPtr && deviceMutex.try_lock())
	{
    // デプスデータからテクスチャ座標を求める
    for (int i = 0; i < depthWidth * depthHeight; ++i)
    {
      // 画素位置
      const int x{ i % depthWidth };
      const int y{ i / depthWidth };

      // 格納先
      const int j{ (depthHeight - y - 1) * depthWidth + x };

      // カラーデータを格納する
      color[j] = colorPtr[i];

#if ALIGN_TO_COLOR
      // テクスチャ座標を求める
      uvmap[j][0] = x + 0.5f;
      uvmap[j][1] = y + 0.5f;
#else
      // デプスセンサのカメラ座標を m 単位で求める (計測不能点だったら最遠点に飛ばす)
      const GLfloat dz{ 0.001f * ((depthPtr[i] != 0 && depthPtr[i] < maxDepth) ? depthPtr[i] : maxDepth) };
      const GLfloat dx{ dz * (x - depthIntrinsics.ppx) / depthIntrinsics.fx };
      const GLfloat dy{ dz * (y - depthIntrinsics.ppy) / depthIntrinsics.fy };

      // デプスセンサのカメラ座標を保存する
      point[j][0] = dx;
      point[j][1] = -dy;
      point[j][2] = -dz;

      // カラーセンサから見たカメラ座標を求める
      const GLfloat cx{ extrinsics.rotation[0] * dx + extrinsics.rotation[3] * dy + extrinsics.rotation[6] * dz + extrinsics.translation[0] };
      const GLfloat cy{ extrinsics.rotation[1] * dx + extrinsics.rotation[4] * dy + extrinsics.rotation[7] * dz + extrinsics.translation[1] };
      const GLfloat cz{ extrinsics.rotation[2] * dx + extrinsics.rotation[5] * dy + extrinsics.rotation[8] * dz + extrinsics.translation[2] };

      // カラーセンサのカメラ座標をテクスチャ座標に変換して保存する
      uvmap[j][0] = cx * colorIntrinsics.fx / cz + colorIntrinsics.ppx;
      uvmap[j][1] = cy * colorIntrinsics.fy / cz + colorIntrinsics.ppy;
#endif
    }

    // カメラ座標をテクスチャに転送する
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, depthWidth, depthHeight, GL_RGB, GL_FLOAT, point.data());

		// テクスチャ座標のバッファオブジェクトを指定する
		glBindBuffer(GL_ARRAY_BUFFER, uvmapBuffer);

		// テクスチャ座標をバッファオブジェクトに転送する
		glBufferSubData(GL_ARRAY_BUFFER, 0, uvmap.size() * sizeof uvmap[0], uvmap.data());

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
  // デプスデータを平滑化する
  const GLuint depthTexture{ getDepth() };
  smooth->use();
  glUniform1i(currentLoc, DepthImageUnit);
  glUniform1i(previousLoc, SmoothImageUnit);
  glUniform1f(attenuationLoc, 0.3f);
  glBindImageTexture(DepthImageUnit, depthTexture, 0, GL_FALSE, 0, GL_READ_ONLY, GL_R16UI);
  glBindImageTexture(SmoothImageUnit, smoothTexture, 0, GL_FALSE, 0, GL_READ_WRITE, GL_R16UI);
  smooth->execute(depthWidth, depthHeight);

  // カメラ座標をシェーダで算出する
  position->use();
  glUniform1i(depthLoc, DepthImageUnit);
  glUniform1i(pointLoc, PointImageUnit);
  glUniform2f(dppLoc, depthIntrinsics.ppx, depthIntrinsics.ppy);
  glUniform2f(dfLoc, depthIntrinsics.fx, depthIntrinsics.fy);
  glUniform2f(cppLoc, colorIntrinsics.ppx, colorIntrinsics.ppy);
  glUniform2f(cfLoc, colorIntrinsics.fx, colorIntrinsics.fy);
  glUniform1f(maxDepthLoc, maxDepth);
  glUniformMatrix3fv(extRotationLoc, 1, GL_FALSE, extrinsics.rotation);
  glUniform3fv(extTranslationLoc, 1, extrinsics.translation);
  glBindImageTexture(DepthImageUnit, depthTexture, 0, GL_FALSE, 0, GL_READ_ONLY, GL_R16UI);
  glBindImageTexture(PointImageUnit, pointTexture, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);
  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, WeightBinding, weightBuffer);
  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, UvmapBinding, uvmapBuffer);
  position->execute(depthWidth, depthHeight, 16, 16);

  return pointTexture;
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
int Rs400::activated{ 0 };

// デプスデータを計算するシェーダ
std::unique_ptr<Compute> Rs400::smooth{ nullptr };

// 現在のデプスデータのイメージユニットの uniform 変数の場所
GLint Rs400::currentLoc;

// 以前のデプスデータのイメージユニットの uniform 変数の場所
GLint Rs400::previousLoc;

// デプスデータの減衰率
GLint Rs400::attenuationLoc;

// カメラ座標を計算するシェーダ
std::unique_ptr<Compute> Rs400::position{ nullptr };

// デプスデータのイメージユニットの uniform 変数 depth の場所
GLint Rs400::depthLoc;

// カメラ座標のイメージユニットの uniform 変数 point の場所
GLint Rs400::pointLoc;

// デプスセンサのカメラパラメータの uniform 変数の場所
GLint Rs400::dppLoc, Rs400::dfLoc;

// カラーセンサのカメラパラメータの uniform 変数の場所
GLint Rs400::cppLoc, Rs400::cfLoc;

// 奥行きの最大値 の uniform 変数 maxDepth の場所
GLint Rs400::maxDepthLoc;

// RealSense のカラーセンサに対するデプスセンサの外部パラメータの uniform 変数の場所
GLint Rs400::extRotationLoc, Rs400::extTranslationLoc;

// RealSense のデバイスリスト
std::map<std::string, rs2::device *> Rs400::devices;

#endif
