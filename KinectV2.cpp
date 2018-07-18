#include "KinectV2.h"

//
// Kinect V2 関連の処理
//

#if USE_KINECT_V2

// Kinect V2 関連
#pragma comment(lib, "Kinect20.lib")

// コンストラクタ
KinectV2::KinectV2()
  : depth(nullptr)
  , point(nullptr)
  , color(nullptr)
  , mapperTexture(0)
{
  // センサが既に使用されていたら戻る
  if (sensor)
    throw std::runtime_error("Kinect (v2) センサが既に使用されています");

  // センサを取得する
  if (GetDefaultKinectSensor(&sensor) != S_OK || sensor == nullptr)
    throw std::runtime_error("Kinect (v2) センサのドライバがインストールされていません");

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
  depth = new GLfloat[depthCount];

  // デプスデータからカメラ座標を求めるときに用いる一時メモリを確保する
  point = new GLfloat[depthCount][3];

  // カラーデータを変換する用いる一時メモリを確保する
  color = new GLubyte[colorCount * 4];

  // カメラ座標算出用のシェーダを作成する
  shader.reset(new Calculate(depthWidth, depthHeight, "position_v2" SHADER_EXT));

  // シェーダの uniform 変数の場所を調べる
  varianceLoc = glGetUniformLocation(shader->get(), "variance");

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
    // データ変換用のメモリを削除する
    delete[] depth;
    delete[] point;
    delete[] color;

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
    glBindBuffer(GL_ARRAY_BUFFER, coordBuffer);
    ColorSpacePoint *const uvmap(static_cast<ColorSpacePoint *>(glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY)));
    coordinateMapper->MapDepthFrameToColorSpace(depthSize, depthBuffer, depthCount, uvmap);
    glUnmapBuffer(GL_ARRAY_BUFFER);

    // すべての点について
    for (UINT i = 0; i < depthSize; ++i)
    {
      // その点のデプス値を取り出す
      UINT16 d(depthBuffer[i]);

      // デプス値の単位をメートルに換算して (計測不能点は maxDepth にして) 転送する
      depth[i] = d == 0 ? -maxDepth : -0.001f * static_cast<GLfloat>(d);
    }

    // デプスデータをテクスチャに転送する
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, depthWidth, depthHeight, GL_RED, GL_FLOAT, depth);

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
    glBindBuffer(GL_ARRAY_BUFFER, coordBuffer);
    ColorSpacePoint *const uvmap(static_cast<ColorSpacePoint *>(glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY)));
    coordinateMapper->MapDepthFrameToColorSpace(depthSize, depthBuffer, depthCount, uvmap);
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

      // デプス値の単位をメートルに換算する (計測不能点は maxDepth にする)
      const GLfloat z(d == 0 ? -maxDepth : -0.001f * static_cast<GLfloat>(d));

      // その点のスクリーン上の位置を求める
      const GLfloat x(table[i].X);
      const GLfloat y(-table[i].Y);

      // その点のカメラ座標を求める
      point[i][0] = x * z;
      point[i][1] = y * z;
      point[i][2] = z;
    }

    // カメラ座標を転送する
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, depthWidth, depthHeight, GL_RGB, GL_FLOAT, point);

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
  shader->use();
  glUniform1f(varianceLoc, variance);
  const GLuint texture[] = { getDepth(), mapperTexture };
  const GLenum format[] = { GL_R32F, GL_RG32F };
  return shader->execute(2, texture, format, 16, 16)[0];
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
IKinectSensor *KinectV2::sensor(nullptr);

// カメラ座標を計算するシェーダ
std::unique_ptr<Calculate> KinectV2::shader(nullptr);

//バイラテラルフィルタの分散の uniform 変数 variance の場所
GLint KinectV2::varianceLoc;

#endif
