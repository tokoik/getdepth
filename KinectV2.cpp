#include "KinectV2.h"

//
// Kinect V2 関連の処理
//

#if USE_KINECT_V2

// Kinect V2 関連
#pragma comment(lib, "Kinect20.lib")

// 標準ライブラリ
#include <climits>

// コンストラクタ
KinectV2::KinectV2()
{
  // センサが既に使用されていたら戻る
  if (sensor)
    throw std::runtime_error("Kinect (v2) センサが既に使用されています");

  // センサを取得する
  if (GetDefaultKinectSensor(&sensor) != S_OK || sensor == nullptr)
    throw std::runtime_error("Kinect (v2) センサが接続されていません");

  // センサの使用を開始する
  if (sensor->Open() != S_OK)
    throw std::runtime_error("Kinect (v2) センサが使用できません");

  // デプスデータの読み込み設定
  IDepthFrameSource *depthSource;
  if (sensor->get_DepthFrameSource(&depthSource) != S_OK)
    throw std::runtime_error("Kinect (v2) センサのデプスフレームのソースが取得できません");
  if (depthSource->OpenReader(&depthReader) != S_OK)
    throw std::runtime_error("Kinect (v2) センサのデプスフレームのリーダが使用できません");
  IFrameDescription *depthDescription;
  if (depthSource->get_FrameDescription(&depthDescription) != S_OK)
    throw std::runtime_error("Kinect (v2) センサのデプスフレームの詳細が取得できません");
  depthSource->Release();

  // デプスデータのサイズを得る
  depthDescription->get_Width(&depthWidth);
  depthDescription->get_Height(&depthHeight);
  depthDescription->Release();

  // カラーデータの読み込み設定
  IColorFrameSource *colorSource;
  if (sensor->get_ColorFrameSource(&colorSource) != S_OK)
    throw std::runtime_error("Kinect (v2) センサのカラーフレームのソースが取得できません");
  if (colorSource->OpenReader(&colorReader) != S_OK)
    throw std::runtime_error("Kinect (v2) センサのカラーフレームのリーダが使用できません");
  IFrameDescription *colorDescription;
  if (colorSource->get_FrameDescription(&colorDescription) != S_OK)
    throw std::runtime_error("Kinect (v2) センサのカラーフレームの詳細が取得できません");
  colorSource->Release();

  // カラーデータのサイズを得る
  colorDescription->get_Width(&colorWidth);
  colorDescription->get_Height(&colorHeight);
  colorDescription->Release();

  // 座標のマッピング
  if (sensor->get_CoordinateMapper(&coordinateMapper) != S_OK)
    throw std::runtime_error("Kinect (v2) センサのコーディネートマッパーが取得できません");

  // depthCount と colorCount を計算してテクスチャとバッファオブジェクトを作成する
  makeTexture();

  // デプスデータの計測不能点を変換するために用いる一次メモリを確保する
  depth = new GLushort[depthCount];

  // デプスデータからカメラ座標を求めるときに用いる一時メモリを確保する
  position = new GLfloat[depthCount][3];

  // カラーデータを変換する用いる一時メモリを確保する
  color = new GLubyte[colorCount * 4];

  // デプスマップのテクスチャ座標に対する頂点座標の拡大率
  scale[0] = 1.546592f;
  scale[1] = 1.222434f;
  scale[2] = -65.535f;
  scale[3] = -maxDepth;
}

// デストラクタ
KinectV2::~KinectV2()
{
  if (getActivated() > 0)
  {
    // データ変換用のメモリを削除する
    delete[] depth;
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

    // デプスデータの計測不能点を最遠点に移動する
    for (UINT i = 0; i < depthSize; ++i) if ((depth[i] = depthBuffer[i]) == 0) depthBuffer[i] = USHRT_MAX;

    // デプスフレームを開放する
    depthFrame->Release();

    // デプスデータをテクスチャに転送する
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, depthWidth, depthHeight, GL_RED, GL_UNSIGNED_SHORT, depth);

    // カラーのテクスチャ座標を求めてバッファオブジェクトに転送する
    glBindBuffer(GL_ARRAY_BUFFER, coordBuffer);
    ColorSpacePoint *const texcoord(static_cast<ColorSpacePoint *>(glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY)));
    coordinateMapper->MapDepthFrameToColorSpace(depthSize, depth, depthCount, texcoord);
    glUnmapBuffer(GL_ARRAY_BUFFER);
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
    coordinateMapper->MapDepthFrameToColorSpace(depthSize, depthBuffer, depthCount, texcoord);
    glUnmapBuffer(GL_ARRAY_BUFFER);

    // カメラ座標への変換テーブルを得る
    UINT32 entry;
    PointF *table;
    coordinateMapper->GetDepthFrameToCameraSpaceTable(&entry, &table);

    // すべての点について
    for (unsigned int i = 0; i < entry; ++i)
    {
      // デプス値の単位をメートルに換算する係数
      static constexpr GLfloat zScale(-0.001f);

      // その点のデプス値を得る
      const unsigned short d(depthBuffer[i]);

      // デプス値の単位をメートルに換算する (計測不能点は maxDepth にする)
      const GLfloat z(d == 0 ? -maxDepth : static_cast<GLfloat>(d) * zScale);

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
