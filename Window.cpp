#include "Window.h"

//
// ウィンドウ関連の処理
//

// 標準ライブラリ
#include <cmath>

//
// コンストラクタ
//
Window::Window(int width, int height, const char *title, GLFWmonitor *monitor, GLFWwindow *share)
  : window(glfwCreateWindow(width, height, title, monitor, share))
{
  // ウィンドウが開いていなかったら戻る
  if (!window) return;

  // 現在のウィンドウを処理対象にする
  glfwMakeContextCurrent(window);

  // ゲームグラフィックス特論の都合にもとづく初期化を行う
  ggInit();

  // このインスタンスの this ポインタを記録しておく
  glfwSetWindowUserPointer(window, this);

  // ウィンドウのサイズ変更時に呼び出す処理の登録
  glfwSetFramebufferSizeCallback(window, resize);

  // マウスボタンを操作したときの処理
  glfwSetMouseButtonCallback(window, mouse);

  // マウスホイール操作時に呼び出す処理
  glfwSetScrollCallback(window, wheel);

  // キーボードを操作した時の処理
  glfwSetKeyCallback(window, keyboard);

  // 視点の初期位置を設定する
  eye[0] = objectCenter[0];
  eye[1] = objectCenter[1];
  eye[2] = objectCenter[2];

  // ビューポートとプロジェクション変換行列を初期化する
  resize(window, width, height);
}

//
// デストラクタ
//
Window::~Window()
{
  // ウィンドウを破棄する
  glfwDestroyWindow(window);
}

//
// 画面クリア
//
//   ・図形の描画開始前に呼び出す
//   ・画面の消去などを行う
//
void Window::clear()
{
  // ウィンドウ全体をビューポートにする
  glViewport(0, 0, size[0], size[1]);

  // カラーバッファとデプスバッファを消去する
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

//
// カラーバッファを入れ替えてイベントを取り出す
//
//   ・図形の描画終了後に呼び出す
//   ・ダブルバッファリングのバッファの入れ替えを行う
//   ・キーボード操作等のイベントを取り出す
//
void Window::swapBuffers()
{
  // エラーチェック
  ggError("SwapBuffers");

  // カラーバッファを入れ替える
  glfwSwapBuffers(window);

  // イベントを取り出す
  glfwPollEvents();

  // マウスの位置を調べる
  double x, y;
  glfwGetCursorPos(window, &x, &y);

  // 左ボタンドラッグ
  if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_1))
  {
    // カメラを上下左右に移動する
    const double d(fabs(eye[2]) + 1.0);
    eye[0] += GLfloat(motionFactor[0] * (x - start[0]) * d / GLfloat(size[0]));
    eye[1] += GLfloat(motionFactor[1] * (start[1] - y) * d / GLfloat(size[1]));
    start[0] = x;
    start[1] = y;
  }

  // 右ボタンドラッグ
  if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_2))
  {
    // 視点を回転する
    trackball.motion(float(x), float(y));
  }
}

//
// ウィンドウのサイズ変更時の処理
//
//   ・ウィンドウのサイズ変更時にコールバック関数として呼び出される
//   ・ウィンドウの作成時には明示的に呼び出す
//
void Window::resize(GLFWwindow *window, int width, int height)
{
  // このインスタンスの this ポインタを得る
  Window *const instance(static_cast<Window *>(glfwGetWindowUserPointer(window)));

  if (instance)
  {
    // プロジェクション変換行列を設定する
    instance->mp = ggPerspective(cameraFovy, GLfloat(width) / GLfloat(height), cameraNear, cameraFar);

    // トラックボール処理の範囲を設定する
    instance->trackball.region(width, height);

    // ウィンドウの幅と高さを保存しておく
    instance->size[0] = width;
    instance->size[1] = height;

    // ウィンドウ全体をビューポートにする
    glViewport(0, 0, width, height);
  }
}

//
// マウスボタンを操作したときの処理
//
//   ・マウスボタンを押したときにコールバック関数として呼び出される
//
void Window::mouse(GLFWwindow *window, int button, int action, int mods)
{
  // このインスタンスの this ポインタを得る
  Window *const instance(static_cast<Window *>(glfwGetWindowUserPointer(window)));

  if (instance)
  {
    // マウスの現在位置を取り出す
    double x, y;
    glfwGetCursorPos(window, &x, &y);

    switch (button)
    {
    case GLFW_MOUSE_BUTTON_1:
      // 左ボタンを押した時の処理
      if (action)
      {
        // ドラッグ開始位置を保存する
        instance->start[0] = x;
        instance->start[1] = y;
      }
      break;
    case GLFW_MOUSE_BUTTON_2:
      // 右ボタンを押した時の処理
      if (action)
      {
        // トラックボール処理開始
        instance->trackball.start(float(x), float(y));
      }
      else
      {
        // トラックボール処理終了
        instance->trackball.stop(float(x), float(y));
      }
      break;
    case GLFW_MOUSE_BUTTON_3:
      break;
      break;
    default:
      break;
    }
  }
}

//
// マウスホイール操作時の処理
//
//   ・マウスホイールを操作した時にコールバック関数として呼び出される
//
void Window::wheel(GLFWwindow *window, double x, double y)
{
  // このインスタンスの this ポインタを得る
  Window *const instance(static_cast<Window *>(glfwGetWindowUserPointer(window)));

  if (instance)
  {
    // マウスホイールによる視点の移動量を求める
    const GLfloat move(GLfloat(motionFactor[2] * (fabs(instance->eye[2]) + 1.0) * y));

    // 視点の回転の変換行列を取り出す
    const GLfloat *rotation(instance->trackball.get());

    // 視点が向いている方向に移動する
    instance->eye[0] += rotation[8] * move;
    instance->eye[1] += rotation[9] * move;
    instance->eye[2] -= rotation[10] * move;
  }
}

//
// キーボードをタイプした時の処理
//
//   ．キーボードをタイプした時にコールバック関数として呼び出される
//
void Window::keyboard(GLFWwindow *window, int key, int scancode, int action, int mods)
{
  // このインスタンスの this ポインタを得る
  Window *const instance(static_cast<Window *>(glfwGetWindowUserPointer(window)));

  if (instance)
  {
    if (action == GLFW_PRESS)
    {
      // キーボード操作による処理
      switch (key)
      {
      case GLFW_KEY_R:
        // 視点の回転をリセットする
        instance->trackball.reset();
        break;
      case GLFW_KEY_T:
        //視点の位置をリセットする
        instance->eye[0] = objectCenter[0];
        instance->eye[1] = objectCenter[1];
        instance->eye[2] = objectCenter[2];
        break;
      case GLFW_KEY_SPACE:
        break;
      case GLFW_KEY_BACKSPACE:
      case GLFW_KEY_DELETE:
        break;
      case GLFW_KEY_UP:
        break;
      case GLFW_KEY_DOWN:
        break;
      case GLFW_KEY_RIGHT:
        break;
      case GLFW_KEY_LEFT:
        break;
      default:
        break;
      }
    }
  }
}
