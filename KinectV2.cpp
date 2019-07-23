#include "KinectV2.h"

//
// Kinect V2 関連の処理
//

#if USE_KINECT_V2

// Kinect V2 関連
#pragma comment(lib, "Kinect20.lib")

// コンストラクタ
KinectV2::KinectV2()
  : mapperTexture(0)
{
  // センサが既に使用されていたら戻る
  if (sensor)
  {
    setMessage("Kinect (v2) センサが既に使用されています");
    return;
  }

  // センサを取得する
  if (GetDefaultKinectSensor(&sensor) != S_OK || sensor == nullptr)
  {
    setMessage("Kinect (v2) センサが見つかりません");
    return;
  }

  // センサの使用を開始する
  if (sensor->Open() != S_OK)
  {
    setMessage("Kinect (v2) センサが使用できません");
    return;
  }

  // デプスデータの読み込み設定
  IDepthFrameSource *depthSource;
  if (sensor->get_DepthFrameSource(&depthSource) != S_OK)
  {
    setMessage("Kinect (v2) センサのデプスフレームのソースが取得できません");
    return;
  }
  if (depthSource->OpenReader(&depthReader) != S_OK)
  {
    setMessage("Kinect (v2) センサのデプスフレームのリーダが使用できません");
    return;
  }
  IFrameDescription *depthDescription;
  if (depthSource->get_FrameDescription(&depthDescription) != S_OK)
  {
    setMessage("Kinect (v2) センサのデプスフレームの詳細が取得できません");
    return;
  }
  depthSource->Release();

  // デプスデータのサイズを得る
  depthDescription->get_Width(&depthWidth);
  depthDescription->get_Height(&depthHeight);
  depthDescription->Release();

  // カラーデータの読み込み設定
  IColorFrameSource *colorSource;
  if (sensor->get_ColorFrameSource(&colorSource) != S_OK)
  {
    setMessage("Kinect (v2) センサのカラーフレームのソースが取得できません");
    return;
  }
  if (colorSource->OpenReader(&colorReader) != S_OK)
  {
    setMessage("Kinect (v2) センサのカラーフレームのリーダが使用できません");
    return;
  }
  IFrameDescription *colorDescription;
  if (colorSource->get_FrameDescription(&colorDescription) != S_OK)
  {
    setMessage("Kinect (v2) センサのカラーフレームの詳細が取得できません");
    return;
  }
  colorSource->Release();

  // カラーデータのサイズを得る
  colorDescription->get_Width(&colorWidth);
  colorDescription->get_Height(&colorHeight);
  colorDescription->Release();

  // 座標のマッピング
  if (sensor->get_CoordinateMapper(&coordinateMapper) != S_OK)
  {
    setMessage("Kinect (v2) センサのコーディネートマッパーが取得できません");
    return;
  }

  // まだシェーダが作られていなかったら
  if (shader.get() == nullptr)
  {
    // カメラ座標算出用のシェーダを作成する
    shader.reset(new Compute("position_v2.comp"));
  }

  // テクスチャとバッファオブジェクトを作成してポイント数を返す
  const int depthCount(makeTexture());

  // デプスデータの計測不能点を変換するために用いる一次メモリを確保する
  depth.resize(depthCount);

  // デプスデータからカメラ座標を求めるときに用いる一時メモリを確保する
  point.resize(depthCount);

  // カラーデータを変換する用いる一時メモリを確保する
  color.resize(colorWidth * colorHeight * 4);

  // デプス値に対するカメラ座標の変換テーブルのテクスチャを作成する
  glGenTextures(1, &mapperTexture);
  glBindTexture(GL_TEXTURE_2D, mapperTexture);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RG32F, depthWidth, depthHeight, 0, GL_RG, GL_FLOAT, NULL);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
}

// デストラクタ
KinectV2::~KinectV2()
{
  // コンストラクタが正常に実行されセンサが有効なら
  if (mapperTexture > 0 && sensor)
  {
    // 変換テーブルのテクスチャを削除する
     glDeleteTextures(1, &mapperTexture);

    // センサを開放する
    colorReader->Release();
    depthReader->Release();
    coordinateMapper->Release();
    sensor->Close();
    sensor->Release();

    // センサを開放したことを記録する
    sensor = nullptr;
  }
}

// デプスデータを取得する
GLuint KinectV2::getDepth()
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
    glBindBuffer(GL_ARRAY_BUFFER, uvmapBuffer);
    ColorSpacePoint *const uvmap(static_cast<ColorSpacePoint *>(glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY)));
    coordinateMapper->MapDepthFrameToColorSpace(depthSize, depthBuffer, static_cast<UINT>(depth.size()), uvmap);
    glUnmapBuffer(GL_ARRAY_BUFFER);

    // すべての点について
    for (UINT i = 0; i < depthSize; ++i)
    {
      // その点のデプス値を取り出す
      UINT16 d(depthBuffer[i]);

      // デプス値を (計測不能点は maxDepth にして) 転送する
      if ((depth[i] = d) == 0) depth[i] = maxDepth;
    }

    // デプスデータをテクスチャに転送する
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, depthWidth, depthHeight, GL_RED_INTEGER, GL_UNSIGNED_SHORT, depth.data());

    // カメラ座標への変換テーブルを得る
    UINT32 entry;
    PointF *table;
    coordinateMapper->GetDepthFrameToCameraSpaceTable(&entry, &table);

    // 変換テーブルをテクスチャに転送する
    glBindTexture(GL_TEXTURE_2D, mapperTexture);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, depthWidth, depthHeight, GL_RG, GL_FLOAT, table);

    // テーブルに使ったメモリを開放する
    CoTaskMemFree(table);

    // デプスフレームを開放する
    depthFrame->Release();
  }

  return depthTexture;
}

// カメラ座標を取得する
GLuint KinectV2::getPoint()
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
    glBindBuffer(GL_ARRAY_BUFFER, uvmapBuffer);
    ColorSpacePoint *const uvmap(static_cast<ColorSpacePoint *>(glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY)));
    coordinateMapper->MapDepthFrameToColorSpace(depthSize, depthBuffer, static_cast<UINT>(depth.size()), uvmap);
    glUnmapBuffer(GL_ARRAY_BUFFER);

    // カメラ座標への変換テーブルを得る
    UINT32 entry;
    PointF *table;
    coordinateMapper->GetDepthFrameToCameraSpaceTable(&entry, &table);

    // すべての点について
    for (unsigned int i = 0; i < entry; ++i)
    {
      // その点のデプス値を得る
      const UINT16 d(depthBuffer[i]);

      // デプス値の単位をメートルに換算する (計測不能点は maxDepth / 1000 にする)
      const GLfloat z(-0.001f * static_cast<GLfloat>(d > 0 ? d : maxDepth));

      // その点のスクリーン上の位置を求める
      const GLfloat x(table[i].X);
      const GLfloat y(-table[i].Y);

      // その点のカメラ座標を求める
      point[i][0] = x * z;
      point[i][1] = y * z;
      point[i][2] = z;
    }

    // カメラ座標を転送する
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, depthWidth, depthHeight, GL_RGB, GL_FLOAT, point.data());

    // テーブルに使ったメモリを開放する
    CoTaskMemFree(table);

    // デプスフレームを開放する
    depthFrame->Release();
  }

  return pointTexture;
}

// カメラ座標を算出する
GLuint KinectV2::getPosition()
{
  const GLuint depthTexture(getDepth());
  shader->use();
  glBindImageTexture(depthBinding, depthTexture, 0, GL_FALSE, 0, GL_READ_ONLY, GL_R16UI);
  glBindImageTexture(pointBinding, pointTexture, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);
  glBindImageTexture(mapperBinding, mapperTexture, 0, GL_FALSE, 0, GL_READ_ONLY, GL_RG32F);
  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, filterBinding, weightBuffer);
  shader->execute(depthWidth, depthHeight, 16, 16);

  return pointTexture;
}

// カラーデータを取得する
GLuint KinectV2::getColor()
{
  // カラーのテクスチャを指定する
  glBindTexture(GL_TEXTURE_2D, colorTexture);

  // 次のカラーのフレームデータが到着していれば
  IColorFrame *colorFrame;
  if (colorReader->AcquireLatestFrame(&colorFrame) == S_OK)
  {
    // カラーデータを取得して RGBA 形式に変換する
    colorFrame->CopyConvertedFrameDataToArray(static_cast<UINT>(color.size()),
      static_cast<BYTE *>(color.data()), ColorImageFormat::ColorImageFormat_Bgra);

    // カラーフレームを開放する
    colorFrame->Release();

    // カラーデータをテクスチャに転送する
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, colorWidth, colorHeight, GL_BGRA, GL_UNSIGNED_BYTE, color.data());
  }

  return colorTexture;
}

// センサの識別子
IKinectSensor *KinectV2::sensor(nullptr);

// カメラ座標を計算するシェーダ
std::unique_ptr<Compute> KinectV2::shader(nullptr);

#endif
