#include "KinectV2.h"

//
// Kinect V2 関連の処理
//

#if USE_KINECT_V2

// 標準ライブラリ
#include <cassert>

// Kinect V2 関連
#pragma comment(lib, "Kinect20.lib")

// 計測不能点のデフォルト距離
const GLfloat maxDepth(10.0f);

// コンストラクタ
KinectV2::KinectV2()
{
  // センサを取得する
  if (sensor == NULL && GetDefaultKinectSensor(&sensor) == S_OK)
  {
    HRESULT hr;

    // センサの使用を開始する
    hr = sensor->Open();
    assert(hr == S_OK);

    // デプスデータの読み込み設定
    IDepthFrameSource *depthSource;
    hr = sensor->get_DepthFrameSource(&depthSource);
    assert(hr == S_OK);
    hr = depthSource->OpenReader(&depthReader);
    assert(hr == S_OK);
    IFrameDescription *depthDescription;
    hr = depthSource->get_FrameDescription(&depthDescription);
    assert(hr == S_OK);
    depthSource->Release();

    // デプスデータのサイズを得る
    depthDescription->get_Width(&depthWidth);
    depthDescription->get_Height(&depthHeight);
    depthDescription->Release();

    // カラーデータの読み込み設定
    IColorFrameSource *colorSource;
    hr = sensor->get_ColorFrameSource(&colorSource);
    assert(hr == S_OK);
    hr = colorSource->OpenReader(&colorReader);
    assert(hr == S_OK);
    IFrameDescription *colorDescription;
    hr = colorSource->get_FrameDescription(&colorDescription);
    assert(hr == S_OK);
    colorSource->Release();

    // カラーデータのサイズを得る
    colorDescription->get_Width(&colorWidth);
    colorDescription->get_Height(&colorHeight);
    colorDescription->Release();

    // 座標のマッピング
    hr = sensor->get_CoordinateMapper(&coordinateMapper);
    assert(hr == S_OK);

    // depthCount と colorCount を計算してテクスチャとバッファオブジェクトを作成する
    makeTexture();

    // デプスデータからカメラ座標を求めるときに用いる一時メモリを確保する
    position = new GLfloat[depthCount][3];

    // カラーデータを変換する用いる一時メモリを確保する
    color = new GLubyte[colorCount * 4];

    // デプスマップのテクスチャ座標に対する頂点座標の拡大率
    scale[0] = 1.546592f;
    scale[1] = 1.222434f;
    scale[2] = -10.0f;
    scale[3] = -65.535f;
  }
}

// デストラクタ
KinectV2::~KinectV2()
{
  if (getActivated() > 0)
  {
    // データ変換用のメモリを削除する
    delete[] position;
    delete[] color;

    // センサを開放する
    colorReader->Release();
    depthReader->Release();
    coordinateMapper->Release();
    sensor->Close();
    sensor->Release();

    // センサを開放したことを記録する
    sensor = NULL;
  }
}

// デプスデータを取得する
GLuint KinectV2::getDepth() const
{
  // デプスのテクスチャを指定する
  glBindTexture(GL_TEXTURE_2D, depthTexture);

  // 次のデプスのフレームデータが到着していれば
  IDepthFrame *depthFrame;
  if (depthReader->AcquireLatestFrame(&depthFrame) == S_OK)
  {
    // デプスデータのサイズと格納場所を得る
    UINT depthSize;
    UINT16 *depthBuffer;
    depthFrame->AccessUnderlyingBuffer(&depthSize, &depthBuffer);

    // カラーのテクスチャ座標を求めてバッファオブジェクトに転送する
    glBindBuffer(GL_ARRAY_BUFFER, coordBuffer);
    ColorSpacePoint *const texcoord(static_cast<ColorSpacePoint *>(glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY)));
    coordinateMapper->MapDepthFrameToColorSpace(depthCount, depthBuffer, depthCount, texcoord);
    glUnmapBuffer(GL_ARRAY_BUFFER);

    // デプスデータをテクスチャに転送する
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, depthWidth, depthHeight, GL_RED, GL_UNSIGNED_SHORT, depthBuffer);

    // デプスフレームを開放する
    depthFrame->Release();
  }

  return depthTexture;
}

// カメラ座標を取得する
GLuint KinectV2::getPoint() const
{
  // カメラ座標のテクスチャを指定する
  glBindTexture(GL_TEXTURE_2D, pointTexture);

  // 次のデプスのフレームデータが到着していれば
  IDepthFrame *depthFrame;
  if (depthReader->AcquireLatestFrame(&depthFrame) == S_OK)
  {
    // デプスデータのサイズと格納場所を得る
    UINT depthSize;
    UINT16 *depthBuffer;
    depthFrame->AccessUnderlyingBuffer(&depthSize, &depthBuffer);

    // カラーのテクスチャ座標を求めてバッファオブジェクトに転送する
    glBindBuffer(GL_ARRAY_BUFFER, coordBuffer);
    ColorSpacePoint *const texcoord(static_cast<ColorSpacePoint *>(glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY)));
    coordinateMapper->MapDepthFrameToColorSpace(depthCount, depthBuffer, depthCount, texcoord);
    glUnmapBuffer(GL_ARRAY_BUFFER);

    // カメラ座標への変換テーブルを得る
    UINT32 entry;
    PointF *table;
    coordinateMapper->GetDepthFrameToCameraSpaceTable(&entry, &table);

    // すべての点について
    for (unsigned int i = 0; i < entry; ++i)
    {
      // デプス値の単位をメートルに換算する係数
      static const GLfloat zScale(-0.001f);

      // その点のデプス値を得る
      const unsigned short d(depthBuffer[i]);

      // デプス値の単位をメートルに換算する (計測不能点は maxDepth にする)
      const GLfloat z(d == 0 ? -maxDepth : GLfloat(d) * zScale);

      // その点のスクリーン上の位置を求める
      const GLfloat x(table[i].X);
      const GLfloat y(-table[i].Y);

      // その点のカメラ座標を求める
      position[i][0] = x * z;
      position[i][1] = y * z;
      position[i][2] = z;
    }

    // カメラ座標を転送する
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, depthWidth, depthHeight, GL_RGB, GL_FLOAT, position);

    // テーブルに使ったメモリを開放する
    CoTaskMemFree(table);

    // デプスフレームを開放する
    depthFrame->Release();
  }

  return pointTexture;
}

// カラーデータを取得する
GLuint KinectV2::getColor() const
{
  // カラーのテクスチャを指定する
  glBindTexture(GL_TEXTURE_2D, colorTexture);

  // 次のカラーのフレームデータが到着していれば
  IColorFrame *colorFrame;
  if (colorReader->AcquireLatestFrame(&colorFrame) == S_OK)
  {
    // カラーデータを取得して RGBA 形式に変換する
    colorFrame->CopyConvertedFrameDataToArray(colorCount * 4,
      static_cast<BYTE *>(color), ColorImageFormat::ColorImageFormat_Bgra);

    // カラーフレームを開放する
    colorFrame->Release();

    // カラーデータをテクスチャに転送する
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, colorWidth, colorHeight, GL_BGRA, GL_UNSIGNED_BYTE, color);
  }

  return colorTexture;
}

// センサの識別子
IKinectSensor *KinectV2::sensor(NULL);

#endif
