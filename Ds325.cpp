#include "Ds325.h"

//
// DepthSense 関連の処理
//

#if USE_DEPTH_SENSE

// 標準ライブラリ
#include <iostream>

// DepthSense 関連
#pragma comment (lib, "DepthSense.lib")

// 計測不能点のデフォルト距離
#if CAPTURE_CAMERA == DS325
const GLfloat maxDepth(3.0f);
#else
const GLfloat maxDepth(6.0f);
#endif

// コンストラクタ
Ds325::Ds325(
  FrameFormat depth_format,           // デプスカメラの解像度
  unsigned int depth_fps,             // デプスカメラのフレームレート
  DepthNode::CameraMode depth_mode,   // デプスカメラのモード
  FrameFormat color_format,           // カラーカメラの解像度
  unsigned int color_fps,             // カラーカメラのフレームレート
  CompressionType color_compression,  // カラーカメラの圧縮方式
  PowerLineFrequency frequency        // 電源周波数
  )
  : depth_format(depth_format)
  , depth_fps(depth_fps)
  , depth_mode(depth_mode)
  , color_format(color_format)
  , color_fps(color_fps)
  , color_compression(color_compression)
  , power_line_frequency(frequency)
{
  // イベントループを開始して接続されている DepthSense の数を求める
  startLoop();

  // DepthSense が接続されており使用台数が接続台数に達していなければ
  if (getActivated() < connected)
  {
    // 未使用の DepthSense を取り出す
    Device device(context.getDevices()[getActivated()]);

    // DepthSense のノードのイベントハンドラを登録する
    device.nodeAddedEvent().connect(&onNodeConnected, this);
    device.nodeRemovedEvent().connect(&onNodeDisconnected, this);

    // DepthSense のフレームフォーマットから解像度を求める
    FrameFormat_toResolution(depth_format, &depthWidth, &depthHeight);
    FrameFormat_toResolution(color_format, &colorWidth, &colorHeight);

    // depthCount と colorCount を計算してテクスチャとバッファオブジェクトを作成する
    makeTexture();

    // データ転送用のメモリを確保する
    depthBuffer = new GLshort[depthCount];
    point = new GLfloat[depthCount * 3];
    uvmap = new GLfloat[depthCount * 2];
    colorBuffer = new GLubyte[colorCount * 3];


    // デプスマップのテクスチャ座標に対する頂点座標の拡大率
    scale[0] = -0.753554f * 2.0f;
    scale[1] = -0.554309f * 2.0f;
    scale[2] = -5.0f;
    scale[3] = -32.767f;

    // DepthSense の各ノードを初期化する
    for (Node &node : device.getNodes()) configureNode(node);
  }
}

// デストラクタ
Ds325::~Ds325()
{
  // DepthSense が有効になっていたら
  if (getActivated() > 0)
  {
    // ノードの登録解除
    unregisterNode(colorNode);
    unregisterNode(depthNode);

    // データ転送用のメモリの開放
    delete[] depthBuffer;
    delete[] point;
    delete[] uvmap;
    delete[] colorBuffer;
  }

  // 最後の DepthSense を削除するときはイベントループを停止する
  if (getActivated() <= 1 && worker.joinable())
  {
    // イベントループを停止する
    context.quit();

    // イベントループのスレッドが終了するのを待つ
    worker.join();

    // ストリーミングを停止する
    context.stopNodes();
  }
}

// イベントループが停止していたらイベントループを開始する
void Ds325::startLoop()
{
  // スレッドが走っていなかったら
  if (!worker.joinable())
  {
    // DepthSense のサーバに接続する
    context = Context::create();

    // DepthSense の取り付け／取り外し時の処理を登録する
    context.deviceAddedEvent().connect(&onDeviceConnected);
    context.deviceRemovedEvent().connect(&onDeviceDisconnected);

    // 現在接続されている DepthSense の数を数える
    connected = context.getDevices().size();

    // ストリーミングを開始する
    context.startNodes();

    // イベントループを開始する
    worker = std::thread([&]() { context.run(); });
  }
}

// DepthSense のノードの登録
void Ds325::configureNode(Node &node)
{
  if (node.is<DepthNode>() && !depthNode.isSet())
  {
    // デプスデータの取得前なのでテクスチャやバッファオブジェクトへの転送は行わない
    depth = nullptr;

    // デプスノード
    depthNode = node.as<DepthNode>();

    // デプスノードのイベント発生時の処理の登録
    depthNode.newSampleReceivedEvent().connect(&onNewDepthSample, this);

    // デプスノードの初期設定
    configureDepthNode(depthNode);

    // デプスノードの登録
    context.registerNode(depthNode);
  }
  else if (node.is<ColorNode>() && !colorNode.isSet())
  {
    // カラーデータの取得前なのでテクスチャへの転送は行わない
    color = nullptr;

    // カラーノード
    colorNode = node.as<ColorNode>();

    // カラーノードのイベント発生時の処理の登録
    colorNode.newSampleReceivedEvent().connect(&onNewColorSample, this);

    // カラーノードの初期設定
    configureColorNode(colorNode);

    // カラーノードの登録
    context.registerNode(colorNode);
  }
}

// ノードの削除
void Ds325::unregisterNode(Node node)
{
  if (node.is<DepthNode>() && depthNode.isSet())
  {
    // デプスデータの取得終了
    depthNode.newSampleReceivedEvent().disconnect(&onNewDepthSample, this);
    context.unregisterNode(depthNode);
    depthNode.unset();

    // デプスデータの取得を終了したのでテクスチャやバッファオブジェクトへの転送は行わない
    depth = nullptr;
  }
  else if (node.is<ColorNode>() && colorNode.isSet())
  {
    // カラーデータの取得終了
    colorNode.newSampleReceivedEvent().disconnect(&onNewColorSample, this);
    context.unregisterNode(colorNode);
    colorNode.unset();

    // カラーデータの取得を終了したのでなのでテクスチャへの転送は行わない
    color = nullptr;
  }
}

// DepthSense が取り付けられた時の処理
void Ds325::onDeviceConnected(Context context, Context::DeviceAddedData data)
{
  MessageBox(NULL, TEXT("DepthSense が取り付けられました。"), TEXT("そうですか"), MB_OK);

  // イベントループを開始して接続されている DepthSense の数を更新する
  startLoop();
}

// DepthSense が取り外されたときの処理
void Ds325::onDeviceDisconnected(Context context, Context::DeviceRemovedData data)
{
  MessageBox(NULL, TEXT("DepthSense が取り外されました。"), TEXT("そうですか"), MB_OK);

  // スレッドが走っていれば接続されている DepthSense の数を更新する
  if (worker.joinable()) connected = context.getDevices().size();
}

// ノードが接続された時の処理
void Ds325::onNodeConnected(Device device, Device::NodeAddedData data, Ds325 *sensor)
{
  sensor->configureNode(data.node);
}

// ノードの接続が解除された時の処理
void Ds325::onNodeDisconnected(Device device, Device::NodeRemovedData data, Ds325 *sensor)
{
  if (data.node.is<DepthNode>() && (data.node.as<DepthNode>() == sensor->depthNode))
    sensor->depthNode.unset();
  if (data.node.is<ColorNode>() && (data.node.as<ColorNode>() == sensor->colorNode))
    sensor->colorNode.unset();
}

// DepthSense のデプスノードの初期化
void Ds325::configureDepthNode(DepthNode &dnode)
{
  // デプスノードの初期設定
  DepthNode::Configuration config(dnode.getConfiguration());
  config.frameFormat = depth_format;
  config.framerate = depth_fps;
  config.mode = depth_mode;
  config.saturation = true;

  // 頂点データ取得の有効化
  dnode.setEnableDepthMap(true);
  dnode.setEnableVerticesFloatingPoint(true);
  dnode.setEnableUvMap(true);

  // 設定実行
  try
  {
    context.requestControl(dnode, 0);
    dnode.setConfiguration(config);
  }
  catch (ArgumentException &e)
  {
    std::cout << "Argument Exception: " << e.what() << std::endl;
  }
  catch (UnauthorizedAccessException &e)
  {
    std::cout << "Unauthorized Access Exception: " << e.what() << std::endl;
  }
  catch (IOException& e)
  {
    std::cout << "IO Exception: " << e.what() << std::endl;
  }
  catch (InvalidOperationException &e)
  {
    std::cout << "Invalid Operation Exception: " << e.what() << std::endl;
  }
  catch (ConfigurationException &e)
  {
    std::cout << "Configuration Exception: " << e.what() << std::endl;
  }
  catch (StreamingException &e)
  {
    std::cout << "Streaming Exception: " << e.what() << std::endl;
  }
  catch (TimeoutException &)
  {
    std::cout << "TimeoutException" << std::endl;
  }
}

// DepthSense のデプスノードのイベント発生時の処理
void Ds325::onNewDepthSample(DepthNode node, DepthNode::NewSampleReceivedData data, Ds325 *sensor)
{
  // デプスカメラの内部パラメータ
  const int &dw(data.stereoCameraParameters.depthIntrinsics.width);
  const int &dh(data.stereoCameraParameters.depthIntrinsics.height);
  const float &dcx(data.stereoCameraParameters.depthIntrinsics.cx);
  const float &dcy(data.stereoCameraParameters.depthIntrinsics.cy);
  const float &dfx(data.stereoCameraParameters.depthIntrinsics.fx);
  const float &dfy(data.stereoCameraParameters.depthIntrinsics.fy);
  const float &dk1(data.stereoCameraParameters.depthIntrinsics.k1);
  const float &dk2(data.stereoCameraParameters.depthIntrinsics.k2);
  const float &dk3(data.stereoCameraParameters.depthIntrinsics.k3);

  // カラーカメラの内部パラメータ
  const int &cw(data.stereoCameraParameters.colorIntrinsics.width);
  const int &ch(data.stereoCameraParameters.colorIntrinsics.height);
  const float &ccx(data.stereoCameraParameters.colorIntrinsics.cx);
  const float &ccy(data.stereoCameraParameters.colorIntrinsics.cy);
  const float &cfx(data.stereoCameraParameters.colorIntrinsics.fx);
  const float &cfy(data.stereoCameraParameters.colorIntrinsics.fy);
  const float &ck1(data.stereoCameraParameters.colorIntrinsics.k1);
  const float &ck2(data.stereoCameraParameters.colorIntrinsics.k2);
  const float &ck3(data.stereoCameraParameters.colorIntrinsics.k3);

  // デプスカメラの
  const GLfloat fovx(dfx * maxDepth / dcx);
  const GLfloat fovy(dfy * maxDepth / dcy);

  // データ転送
  sensor->depthMutex.lock();
  for (int i = 0; i < sensor->depthCount; ++i)
  {
    // デプスマップの画素位置
    const int u(i % dw);
    const int t(i / dw);
    const int v(dh - t - 1);

    // 転送先のマップは上下を反転する
    const int j(v * dw + u);

    // デプス値を転送する
    sensor->depthBuffer[j] = data.depthMap[i];

    // 画素が計測不能なら
    if (sensor->depthBuffer[j] > 32000)
    {
      // 画素位置からデプスマップのスクリーン座標を求める
      const GLfloat dx((static_cast<GLfloat>(u) - dcx + 0.5f) / dfx);
      const GLfloat dy((static_cast<GLfloat>(v) - dcy + 0.5f) / dfy);

      // デプスカメラの歪み補正係数
      const GLfloat dr(dx * dx + dy * dy);
      const GLfloat dq(1.0f + dr * (dk1 + dr * (dk2 + dr * dk3)));

      // 歪みを補正したポイントのスクリーン座標値
      const GLfloat x(dx / dq);
      const GLfloat y(dy / dq);

      // 計測不能点を最遠点にしてカメラ座標を求める
      sensor->point[j * 3 + 0] = x * maxDepth;
      sensor->point[j * 3 + 1] = y * maxDepth;
      sensor->point[j * 3 + 2] = -maxDepth;

      // カラーカメラの歪み補正係数
      const GLfloat cr(x * x + y * y);
      const GLfloat cq(1.0f + cr * (ck1 + cr * (ck2 + cr * ck3)));

      // ポイントのカメラ座標
      const GLfloat cx((x + 0.0508f) / cq);
      const GLfloat cy(y / cq);

      // 歪みを補正したポイントのテクスチャ座標値
      sensor->uvmap[j * 2 + 0] = ccx + cx * cfx;
      sensor->uvmap[j * 2 + 1] = ccy - cy * cfy;
    }
    else
    {
      sensor->point[j * 3 + 0] = data.verticesFloatingPoint[i].x;
      sensor->point[j * 3 + 1] = data.verticesFloatingPoint[i].y;
      sensor->point[j * 3 + 2] = -data.verticesFloatingPoint[i].z;
      sensor->uvmap[j * 2 + 0] = data.uvMap[i].u * static_cast<GLfloat>(cw);
      sensor->uvmap[j * 2 + 1] = data.uvMap[i].v * static_cast<GLfloat>(ch);
    }
  }
  sensor->depth = sensor->depthBuffer;
  sensor->depthMutex.unlock();
}

// DepthSense のカラーノードの初期化
void Ds325::configureColorNode(ColorNode &cnode)
{
  // カラーノードの初期設定
  ColorNode::Configuration config(cnode.getConfiguration());
  config.frameFormat = color_format;
  config.compression = color_compression;
  config.powerLineFrequency = power_line_frequency;
  config.framerate = color_fps;

  // 色データの取得の有効化
  cnode.setEnableColorMap(true);

  try
  {
    context.requestControl(cnode, 0);
    cnode.setConfiguration(config);
  }
  catch (ArgumentException& e)
  {
    std::cout << "Argument Exception: " << e.what() << std::endl;
  }
  catch (UnauthorizedAccessException &e)
  {
    std::cout << "Unauthorized Access Exception:" << e.what() << std::endl;
  }
  catch (IOException &e)
  {
    std::cout << "IO Exception: " << e.what() << std::endl;
  }
  catch (InvalidOperationException &e)
  {
    std::cout << "Invalid Operation Exception: " << e.what() << std::endl;
  }
  catch (ConfigurationException &e)
  {
    std::cout << "Configuration Exception: " << e.what() << std::endl;
  }
  catch (StreamingException &e)
  {
    std::cout << "Streaming Exception: " << e.what() << std::endl;
  }
  catch (TimeoutException &)
  {
    std::cout << "TimeoutException" << std::endl;
  }
}

// DepthSense のカラーノードのイベント発生時の処理
void Ds325::onNewColorSample(ColorNode node, ColorNode::NewSampleReceivedData data, Ds325 *sensor)
{
  // カラーデータを転送する
  sensor->colorMutex.lock();
  if (sensor->color_compression == COMPRESSION_TYPE_MJPEG)
  {
    memcpy(sensor->colorBuffer, data.colorMap, sensor->colorCount * 3 * sizeof(GLubyte));
  }
  else
  {
    // ITU-R BT.601 / ITU-R BT.709 (1250/50/2:1)
    //
    //   R = Y + 1.402 × Cr
    //   G = Y - 0.344136 × Cb - 0.714136 × Cr
    //   B = Y + 1.772 × Cb

    // ITU - R BT.709 (1125 / 60 / 2:1)
    //
    //   R = Y + 1.5748 × Cr
    //   G = Y - 0.187324 × Cb - 0.468124 × Cr
    //   B = Y + 1.8556 × Cb

    for (int i = 0; i < sensor->colorCount; ++i)
    {
      const int iy(i * 2), iu((iy & ~3) + 1), iv(iu + 2), j(i * 3);
      const float y(static_cast<float>(data.colorMap[iy] - 16));
      const float u(static_cast<float>(data.colorMap[iu] - 128));
      const float v(static_cast<float>(data.colorMap[iv] - 128));
      const float r(y + 1.402f * v);
      const float g(y - 0.344136f * u - 0.714136f * v);
      const float b(y + 1.772f * u);
      sensor->colorBuffer[j + 0] = b > 0.0f ? static_cast<GLubyte>(b) : 0;
      sensor->colorBuffer[j + 1] = g > 0.0f ? static_cast<GLubyte>(g) : 0;
      sensor->colorBuffer[j + 2] = r > 0.0f ? static_cast<GLubyte>(r) : 0;
    }
  }
  sensor->color = sensor->colorBuffer;
  sensor->colorMutex.unlock();
}

// デプスデータを取得する
GLuint Ds325::getDepth()
{
  // デプスデータのテクスチャを指定する
  glBindTexture(GL_TEXTURE_2D, depthTexture);

  // デプスデータが更新されていれば
  if (depth)
  {
    // DepthSense がデプスデータの取得中でなければ
    if (depthMutex.try_lock())
    {
      // テクスチャ座標のバッファオブジェクトを指定する
      glBindBuffer(GL_ARRAY_BUFFER, coordBuffer);

      // テクスチャ座標をバッファオブジェクトに転送する
      glBufferSubData(GL_ARRAY_BUFFER, 0, depthCount * 2 * sizeof(GLfloat), uvmap);

      // デプスデータをテクスチャに転送する
      glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, depthWidth, depthHeight, GL_RED, GL_SHORT, depth);

      // 一度送ってしまえば更新されるまで送る必要がないのでデータは不要
      depth = nullptr;

      // デプスデータのロックを開放する
      depthMutex.unlock();
    }
  }

  return depthTexture;
}

// カメラ座標を取得する
GLuint Ds325::getPoint()
{
  // カメラ座標のテクスチャを指定する
  glBindTexture(GL_TEXTURE_2D, pointTexture);

  // デプスデータが更新されていれば
  if (depth)
  {
    // DepthSense がデプスデータの取得中でなければ
    if (depthMutex.try_lock())
    {
      // テクスチャ座標のバッファオブジェクトを指定する
      glBindBuffer(GL_ARRAY_BUFFER, coordBuffer);

      // テクスチャ座標をバッファオブジェクトに転送する
      glBufferSubData(GL_ARRAY_BUFFER, 0, depthCount * 2 * sizeof(GLfloat), uvmap);

      // カメラ座標をテクスチャに転送する
      glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, depthWidth, depthHeight, GL_RGB, GL_FLOAT, point);

      // 一度送ってしまえば更新されるまで送る必要がないのでデータは不要
      depth = nullptr;

      // デプスデータのロックを開放する
      depthMutex.unlock();
    }
  }

  return pointTexture;
}

// カラーデータを取得する
GLuint Ds325::getColor()
{
  // カラーデータのテクスチャを指定する
  glBindTexture(GL_TEXTURE_2D, colorTexture);

  // カラーデータが更新されていれば
  if (color)
  {
    // DepthSense がカラーデータの取得中でなければ
    if (colorMutex.try_lock())
    {
      // カラーデータをテクスチャに転送する
      glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, colorWidth, colorHeight, GL_BGR, GL_UNSIGNED_BYTE, color);

      // 一度送ってしまえば更新されるまで送る必要がないのでデータは不要
      color = nullptr;

      // カラーデータのロックを開放する
      colorMutex.unlock();
    }
  }

  return colorTexture;
}

// DepthSense のコンテキスト
Context Ds325::context;

// データ取得用のスレッド
std::thread Ds325::worker;

#endif
