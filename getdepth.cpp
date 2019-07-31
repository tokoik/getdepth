//
// RGB-D カメラからのデプスマップ取得
//

// 標準ライブラリ
#include <Windows.h>

// OpenCV
#include <opencv2/highgui/highgui.hpp>
#ifdef _WIN32
#  define CV_VERSION_STR CVAUX_STR(CV_MAJOR_VERSION) CVAUX_STR(CV_MINOR_VERSION) CVAUX_STR(CV_SUBMINOR_VERSION)
#  ifdef _DEBUG
#    define CV_EXT_STR "d.lib"
#  else
#    define CV_EXT_STR ".lib"
#  endif
#  pragma comment(lib, "opencv_world" CV_VERSION_STR CV_EXT_STR)
#endif

// ウィンドウ関連の処理
#include "GgApplication.h"

// センサ関連の処理
//#include "KinectV1.h"
//#include "KinectV2.h"
//#include "Ds325.h"
#include "Rs400.h"

// センサの数
constexpr int sensorCount(3);

// OpenCV によるビデオキャプチャに使うカメラ
#define CAPTURE_DEVICE 1

// 頂点位置の生成をシェーダで行うなら 1
#define USE_SHADER 1

// 透明人間にするなら 1
#define USE_REFRACTION 0

// カメラパラメータ
constexpr GLfloat cameraFovy(1.0f);                     // 画角
constexpr GLfloat cameraNear(0.1f);                     // 前方面までの距離
constexpr GLfloat cameraFar(50.0f);                     // 後方面までの距離

// 光源
constexpr GgSimpleShader::Light lightData =
{
  { 0.2f, 0.2f, 0.2f, 1.0f },                           // 環境光成分
  { 1.0f, 1.0f, 1.0f, 1.0f },                           // 拡散反射光成分
  { 1.0f, 1.0f, 1.0f, 1.0f },                           // 鏡面光成分
  { 0.0f, 0.0f, 5.0f, 1.0f }                            // 位置
};

// 材質
constexpr GgSimpleShader::Material materialData =
{
  { 0.8f, 0.8f, 0.8f, 1.0f },                           // 環境光の反射係数
  { 0.8f, 0.8f, 0.8f, 1.0f },                           // 拡散反射係数
  { 0.2f, 0.2f, 0.2f, 1.0f },                           // 鏡面反射係数
  50.0f                                                 // 輝き係数
};

// 背景色
constexpr GLfloat background[] = { 0.2f, 0.3f, 0.4f, 0.0f };

// バイラテラルフィルタのデフォルトの位置の標準偏差
constexpr float deviation1(2.0f);

// バイラテラルフィルタのデフォルトの明度の標準偏差
constexpr float deviation2(10.0f);

// すべてのバイラテラルフィルタの分散を設定するコールバック関数
static void updateVariance(const GgApplication::Window *window, int key, int scancode, int action, int mods)
{
  // センサのリストを取り出す
  void *const sensors(window->getUserPointer());

  // センサのリストが有効のときにキー操作が行われたら
  if (sensors && action)
  {
    // バイラテラルフィルタの分散を求める
    const GLfloat sd1(window->getArrowX() * deviation1 * 0.1f + deviation1);
    const GLfloat sd2(window->getArrowY() * deviation2 * 0.1f + deviation2);
    const GLfloat variance1(sd1 * sd1), variance2(sd2 * sd2);

#if defined(_DEBUG)
    std::cerr << "sd1 =" << sd1 << ", sd2 =" << sd2 << "\n";
#endif

    // すべてのバイラテラルフィルタの分散を設定する
    for (auto &sensor : *static_cast<std::vector<std::unique_ptr<SENSOR>> *>(sensors))
    {
      sensor->setVariance(variance1, variance1, variance2);
    }
  }
}

//
// アプリケーションの実行
//
void GgApplication::run()
{
  // ウィンドウを開く
  Window window("Depth Map Viewer", 1280, 720);
  if (!window.get())
  {
    throw std::runtime_error("GLFW のウィンドウが開けません");
  }

  // デプスセンサのリスト
  std::vector<std::unique_ptr<SENSOR>> sensors;

  // センサの数の分だけ
  for (int i = 0; i < sensorCount; ++i)
  {
    // センサを起動する
    std::unique_ptr<SENSOR> sensor(std::make_unique<SENSOR>());

    // センサが起動できなかったら終わる
    if (!sensor->isOpend()) break;

    // バイラテラルフィルタの初期値を設定する
    sensor->setVariance(deviation1 * deviation1, deviation1 * deviation1, deviation2 * deviation2);

    // センサの姿勢を設定する
    sensor->attitude = ggRotateY(6.2831853f * i / sensorCount) * ggTranslate(0.0f, 0.0f, 0.6f);

    // センサを追加する
    sensors.emplace_back(std::move(sensor));
  }

  // センサが一つもなければエラーにする
  if (sensors.empty())
  {
    throw std::runtime_error("センサが起動できません");
  }

  // キーボード操作のコールバック関数を登録する
  window.setUserPointer(&sensors);
  window.setKeyboardFunc(updateVariance);

#if USE_REFRACTION
  // 背景画像のキャプチャに使う OpenCV のビデオキャプチャを初期化する
  cv::VideoCapture camera(CAPTURE_DEVICE);
  if (!camera.isOpened())
  {
    throw std::runtime_error("ビデオカメラが見つかりません");
  }

  // カメラの初期設定
  camera.grab();
  const GLsizei capture_env_width(GLsizei(camera.get(CV_CAP_PROP_FRAME_WIDTH)));
  const GLsizei capture_env_height(GLsizei(camera.get(CV_CAP_PROP_FRAME_HEIGHT)));

  // 背景画像のテクスチャ
  GLuint bmap;
  glGenTextures(1, &bmap);
  glBindTexture(GL_TEXTURE_2D, bmap);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, capture_env_width, capture_env_height, 0, GL_BGR, GL_UNSIGNED_BYTE, nullptr);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

  // 透明人間用のシェーダ
  const GgSimpleShader simple("refraction.vert", "refraction.frag");
  const GLint pointLoc(glGetUniformLocation(simple.get(), "point"));
  const GLint backLoc(glGetUniformLocation(simple.get(), "back"));
  const GLint windowSizeLoc(glGetUniformLocation(simple.get(), "windowSize"));
#else
  // 描画用のシェーダ
  const GgSimpleShader simple("simple.vert", "simple.frag");
  const GLint pointLoc(glGetUniformLocation(simple.get(), "point"));
  const GLint colorLoc(glGetUniformLocation(simple.get(), "color"));
  const GLint rangeLoc(glGetUniformLocation(simple.get(), "range"));
  const GLuint uvmapIndex(glGetProgramResourceIndex(simple.get(), GL_SHADER_STORAGE_BLOCK, "Uvmap"));
  glShaderStorageBlockBinding(simple.get(), uvmapIndex, DepthCamera::UvmapBinding);
#endif
  const GLuint normalIndex(glGetProgramResourceIndex(simple.get(), GL_SHADER_STORAGE_BLOCK, "Normal"));
  glShaderStorageBlockBinding(simple.get(), normalIndex, DepthCamera::NormalBinding);

  // 光源データ
  const GgSimpleShader::LightBuffer light(lightData);

  // 材質データ
  const GgSimpleShader::MaterialBuffer material(materialData);

  // 背景色を設定する
  glClearColor(background[0], background[1], background[2], background[3]);

  // 隠面消去処理を有効にする
  glEnable(GL_DEPTH_TEST);
  glEnable(GL_CULL_FACE);

  // ウィンドウが開いている間くり返し描画する
  while (window)
  {
#if USE_REFRACTION
    // 画像のキャプチャ
    if (camera.grab())
    {
      // キャプチャ映像から画像を切り出す
      cv::Mat frame;
      camera.retrieve(frame, 3);

      // 切り出した画像をテクスチャに転送する
      cv::Mat flipped;
      cv::flip(frame, flipped, 0);
      glBindTexture(GL_TEXTURE_2D, bmap);
      glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, frame.cols, flipped.rows, GL_BGR, GL_UNSIGNED_BYTE, flipped.data);
    }
#endif

    // すべてのセンサについて
    for (auto &sensor : sensors)
    {
      // 頂点位置の取得
#if USE_SHADER
      sensor->getPosition();
#else
      sensor->getPoint();
#endif

      // カラーデータの取得
      sensor->getColor();

      // 法線ベクトルの計算
      sensor->getNormal();
    }

    // モデル変換行列
    const GgMatrix mm(window.getTrackball(1) * window.getTranslation(0));

    // 投影変換行列
    const GgMatrix mp(ggPerspective(cameraFovy, window.getAspect(), cameraNear, cameraFar));

    // 画面消去
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // すべてのセンサについて
    for (auto &sensor : sensors)
    {
      // 描画用のシェーダプログラムの使用開始
      simple.use(mp, mm * sensor->attitude, light);
      material.select();

      // カメラ座標のテクスチャ
      glUniform1i(pointLoc, 0);
      glActiveTexture(GL_TEXTURE0);
      glBindTexture(GL_TEXTURE_2D, sensor->getPointTexture());

#if USE_REFRACTION
      // 背景テクスチャ
      glUniform1i(backLoc, 1);
      glActiveTexture(GL_TEXTURE1);
      glBindTexture(GL_TEXTURE_2D, bmap);

      // ウィンドウサイズ
      glUniform2iv(windowSizeLoc, 1, window.getSize());
#else
      // 前景テクスチャ
      glUniform1i(colorLoc, 1);
      glActiveTexture(GL_TEXTURE1);
      glBindTexture(GL_TEXTURE_2D, sensor->getColorTexture());

      // テクスチャ座標のシェーダストレージバッファオブジェクト
      glBindBufferBase(GL_SHADER_STORAGE_BUFFER, DepthCamera::UvmapBinding, sensor->getUvmapBuffer());

      // 疑似カラー処理
      glUniform2fv(rangeLoc, 1, sensor->range);
#endif

      // 法線ベクトルののシェーダストレージバッファオブジェクト
      glBindBufferBase(GL_SHADER_STORAGE_BUFFER, DepthCamera::NormalBinding, sensor->getNormalBuffer());

      // 図形描画
      sensor->draw();
    }

    // バッファを入れ替える
    window.swapBuffers();
  }
}
