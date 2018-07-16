#include "KinectV1.h"

//
// Kinect V1 関連の処理
//

#if USE_KINECT_V1

// Kinect V1 関連
#pragma comment(lib, "Kinect10.lib")

// コンストラクタ
KinectV1::KinectV1()
  : nextDepthFrameEvent(CreateEvent(NULL, TRUE, FALSE, NULL))
  , nextColorFrameEvent(CreateEvent(NULL, TRUE, FALSE, NULL))
  , sensor(nullptr)
  , depth(nullptr)
  , position(nullptr)
{
  // 最初のインスタンスを生成するときだけ
  if (activated == 0)
  {
    // 接続されているセンサの数を調べる
    if (NuiGetSensorCount(&connected) != S_OK || connected == 0)
      throw std::runtime_error("Kinect (v1) センサが接続されていません");
  }

  // センサが接続されており使用台数が接続台数に達していないかどうか確認する
  if (activated >= connected)
    throw std::runtime_error("Kinect (v1) センサの数が足りません");

  // 使用できるセンサを見つける
  if (NuiCreateSensorByIndex(activated, &sensor) != S_OK || sensor == nullptr)
    throw std::runtime_error("Kinect (v1) センサが見つかりません");

  // センサの使用を開始する
  if (sensor->NuiStatus() != S_OK)
    throw std::runtime_error("Kinect (v1) センサが使用できません");

  // センサを初期化する (カラーとデプスを取得する)
  if (sensor->NuiInitialize(NUI_INITIALIZE_FLAG_USES_COLOR | NUI_INITIALIZE_FLAG_USES_DEPTH) != S_OK)
    throw std::runtime_error("Kinect (v1) センサを初期化できません");

  // センサの仰角を初期化する
  if (sensor->NuiCameraElevationSetAngle(0L) != S_OK)
    throw std::runtime_error("Kinect (v1) センサの仰角を設定できません");

  // デプスストリームの取得設定
  if (sensor->NuiImageStreamOpen(NUI_IMAGE_TYPE_DEPTH, DEPTH_RESOLUTION, 0, 2, nextDepthFrameEvent, &depthStream) != S_OK)
    throw std::runtime_error("Kinect (v1) センサのデプスストリームが取得できません");

  // カラーストリームの取得設定
  if (sensor->NuiImageStreamOpen(NUI_IMAGE_TYPE_COLOR, COLOR_RESOLUTION, 0, 2, nextColorFrameEvent, &colorStream) != S_OK)
    throw std::runtime_error("Kinect (v1) センサのカラーストリームが取得できません");

  // 有効にしたセンサの数を数える
  ++activated;

  // デプスカメラの解像度を初期化する
  depthWidth = DEPTH_W;
  depthHeight = DEPTH_H;

  // カラーカメラの解像度を初期化する
  colorWidth = COLOR_W;
  colorHeight = COLOR_H;

  // depthCount と colorCount を計算してテクスチャとバッファオブジェクトを作成する
  makeTexture();

  // デプスデータの計測不能点を変換するために用いる一次メモリを確保する
  depth = new GLfloat[depthCount];

  // デプスデータからカメラ座標を求めるときに用いる一時メモリを確保する
  position = new GLfloat[depthCount][3];

  // デプスマップのテクスチャ座標に対する頂点座標の拡大率
  scale[0] = static_cast<GLfloat>(NUI_CAMERA_DEPTH_NOMINAL_INVERSE_FOCAL_LENGTH_IN_PIXELS * depthWidth);
  scale[1] = static_cast<GLfloat>(NUI_CAMERA_DEPTH_NOMINAL_INVERSE_FOCAL_LENGTH_IN_PIXELS * depthHeight);

  // まだシェーダが作られていなかったら
  if (shader.get() == nullptr)
  {
    // カメラ座標算出用のシェーダを作成する
    shader.reset(new Calculate(depthWidth, depthHeight, "position_v1" SHADER_EXT));

    // シェーダの uniform 変数の場所を調べる
    varianceLoc = glGetUniformLocation(shader->get(), "variance");
    scaleLoc = glGetUniformLocation(shader->get(), "scale");
  }
}

// デストラクタ
KinectV1::~KinectV1()
{
  // イベントハンドルを閉じる
  CloseHandle(nextDepthFrameEvent);
  CloseHandle(nextColorFrameEvent);

  // データ変換用のメモリを削除する
  delete[] depth;
  delete[] position;

  // センサをシャットダウンする
  if (sensor)  sensor->NuiShutdown();
}

// デプスデータを取得する
GLuint KinectV1::getDepth()
{
  // カラーのテクスチャを指定する
  glBindTexture(GL_TEXTURE_2D, depthTexture);

  // 次のフレームデータが到着していれば
  if (WaitForSingleObject(nextDepthFrameEvent, 0) != WAIT_TIMEOUT)
  {
    // 取得したフレームデータの格納場所
    NUI_IMAGE_FRAME frame;

    // フレームデータの格納場所を frame に取得する
    if (sensor->NuiImageStreamGetNextFrame(depthStream, 0, &frame) == S_OK)
    {
      // これから処理完了までデータが変更されないようにロックする
      NUI_LOCKED_RECT rect;
      frame.pFrameTexture->LockRect(0, &rect, NULL, 0);

      // ロックに成功したら
      if (rect.Pitch)
      {
        // すべての点について
        for (int i = 0; i < depthCount; ++i)
        {
          // その点のデプス値を取り出す
          const USHORT d(reinterpret_cast<USHORT *>(rect.pBits)[i] >> NUI_IMAGE_PLAYER_INDEX_SHIFT);

          // デプス値の単位をメートルに換算して (計測不能点は maxDepth にして) 転送する
          depth[i] = d == 0 ? -maxDepth : -0.001f * static_cast<GLfloat>(d);
        }

        // pBits に入っているデータをテクスチャに転送する
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, depthWidth, depthHeight, GL_RED, GL_FLOAT, depth);

        // カラーデータのテクスチャ座標を求めてバッファオブジェクトに転送する
        glBindBuffer(GL_ARRAY_BUFFER, coordBuffer);
        GLfloat(*const uvmap)[2](static_cast<GLfloat(*)[2]>(glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY)));
        for (int i = 0; i < depthCount; ++i)
        {
          // デプスの画素位置からカラーの画素位置を求める
          LONG x, y;
          sensor->NuiImageGetColorPixelCoordinatesFromDepthPixel(COLOR_RESOLUTION,
            NULL, i % depthWidth, i / depthWidth, reinterpret_cast<USHORT *>(rect.pBits)[i], &x, &y);

          // カラーデータのテクスチャ座標に変換する
          uvmap[i][0] = static_cast<GLfloat>(x) + 0.5f;
          uvmap[i][1] = static_cast<GLfloat>(y) + 0.5f;
        }
        glUnmapBuffer(GL_ARRAY_BUFFER);
      }
    }

    // データをアンロックする
    sensor->NuiImageStreamReleaseFrame(depthStream, &frame);
  }

  return depthTexture;
}

// カメラ座標を取得する
GLuint KinectV1::getPoint()
{
  // カラーのテクスチャを指定する
  glBindTexture(GL_TEXTURE_2D, pointTexture);

  // 次のフレームデータが到着していれば
  if (WaitForSingleObject(nextDepthFrameEvent, 0) != WAIT_TIMEOUT)
  {
    // 取得したフレームデータの格納場所
    NUI_IMAGE_FRAME frame;

    // フレームデータの格納場所を frame に取得する
    if (sensor->NuiImageStreamGetNextFrame(depthStream, 0, &frame) == S_OK)
    {
      // これから処理完了までデータが変更されないようにロックする
      NUI_LOCKED_RECT rect;
      frame.pFrameTexture->LockRect(0, &rect, NULL, 0);

      // ロックに成功したら
      if (rect.Pitch)
      {
        // カラーデータのテクスチャ座標を求めてバッファオブジェクトに転送する
        glBindBuffer(GL_ARRAY_BUFFER, coordBuffer);
        GLfloat (*const uvmap)[2](static_cast<GLfloat (*)[2]>(glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY)));
        for (int i = 0; i < depthCount; ++i)
        {
          // デプスの画素位置からカラーの画素位置を求める
          LONG x, y;
          sensor->NuiImageGetColorPixelCoordinatesFromDepthPixel(COLOR_RESOLUTION,
            NULL, i % depthWidth, i / depthWidth, reinterpret_cast<USHORT *>(rect.pBits)[i], &x, &y);

          // カラーデータのテクスチャ座標に変換する
          uvmap[i][0] = static_cast<GLfloat>(x) + 0.5f;
          uvmap[i][1] = static_cast<GLfloat>(y) + 0.5f;
        }
        glUnmapBuffer(GL_ARRAY_BUFFER);

        // すべての点について
        for (int i = 0; i < depthCount; ++i)
        {
          // その点のデプス値を得る
          const USHORT d(reinterpret_cast<USHORT *>(rect.pBits)[i] >> NUI_IMAGE_PLAYER_INDEX_SHIFT);

          // デプス値の単位をメートルに換算する (計測不能点は maxDepth にする)
          const GLfloat z(d == 0 ? -maxDepth : -0.001f * static_cast<GLfloat>(d));

          // レンズの画角にもとづくスケールを求める
          const GLfloat s(NUI_CAMERA_DEPTH_NOMINAL_INVERSE_FOCAL_LENGTH_IN_PIXELS * z);

          // その点のスクリーン上の位置を求める
          const GLfloat x(float(i % depthWidth - depthWidth / 2) + 0.5f);
          const GLfloat y(float(i / depthWidth - depthHeight / 2) + 0.5f);

          // その点のカメラ座標を求める
          position[i][0] = x * s;
          position[i][1] = y * s;
          position[i][2] = z;
        }

        // カメラ座標をテクスチャに転送する
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, depthWidth, depthHeight, GL_RGB, GL_FLOAT, position);
      }
    }

    // データをアンロックする
    sensor->NuiImageStreamReleaseFrame(depthStream, &frame);
  }

  return pointTexture;
}

// カメラ座標を算出する
GLuint KinectV1::getPosition()
{
  shader->use();
  glUniform2fv(scaleLoc, 1, scale);
  glUniform1f(varianceLoc, variance);
  const GLuint depthTexture(getDepth());
  const GLenum depthFormat(GL_R32F);
  return shader->execute(1, &depthTexture, &depthFormat)[0];
}

// カラーデータを取得する
GLuint KinectV1::getColor()
{

  // カラーのテクスチャを指定する
  glBindTexture(GL_TEXTURE_2D, colorTexture);

  // 次のフレームデータが到着していれば
  if (WaitForSingleObject(nextColorFrameEvent, 0) != WAIT_TIMEOUT)
  {
    // 取得したフレームデータの格納場所
    NUI_IMAGE_FRAME frame;

    // フレームデータの格納場所を frame に取得する
    if (sensor->NuiImageStreamGetNextFrame(colorStream, 0, &frame) == S_OK)
    {
      // これから処理完了までデータが変更されないようにロックする
      NUI_LOCKED_RECT rect;
      frame.pFrameTexture->LockRect(0, &rect, NULL, 0);

      // ロックに成功したら
      if (rect.Pitch)
      {
        // pBits に入っているデータをテクスチャに転送する
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, colorWidth, colorHeight, GL_BGRA, GL_UNSIGNED_BYTE, rect.pBits);
      }

      // データをアンロックする
      sensor->NuiImageStreamReleaseFrame(colorStream, &frame);
    }
  }

  return colorTexture;
}

// 接続しているセンサの数
int KinectV1::connected(0);

// 使用しているセンサの数
int KinectV1::activated(0);

// カメラ座標を計算するシェーダ
std::unique_ptr<Calculate> KinectV1::shader(nullptr);

//バイラテラルフィルタの分散の uniform 変数 variance の場所
GLint KinectV1::varianceLoc;

// スクリーン座標からカメラ座標に変換する係数の uniform 変数 scale の場所
GLint KinectV1::scaleLoc;

#endif
