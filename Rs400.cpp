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
	// 最初のオブジェクトの時だけ
	// 
	context.set_devices_changed_callback([&](rs2::event_information& info)
	{
		std::lock_guard<std::mutex> lock(_mutex);
		// Go over the list of devices and check if it was disconnected
		auto itr = devices.begin();
		while (itr != devices.end())
		{
			if (info.was_removed(itr->second.profile.get_device()))
			{
				itr = devices.erase(itr);
			}
			else
			{
				++itr;
			}
		}

		connected_devices.remove_devices(info);
		for (auto&& dev : info.get_new_devices())
		{
			connected_devices.enable_device(dev);
		}
	});

  // スレッドが走っていなかったら
  if (!worker.joinable())
  {
		// Start streaming with default recommended configuration
		pipe.start();
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

// RealSense のコンテキスト
rs2::context Rs400::context;

// データ取得用のスレッド
std::thread Rs400::worker;

// カメラ座標を計算するシェーダ
std::unique_ptr<Calculate> Rs400::shader(nullptr);

// バイラテラルフィルタの分散の uniform 変数 variance の場所
GLint Rs400::varianceLoc;

#endif
