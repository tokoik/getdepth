#pragma once

//
// ウィンドウ関連の処理
//

// 各種設定
#include "config.h"

//
// ウィンドウ関連の処理を担当するクラス
//
class Window
{
  // ウィンドウの識別子
  GLFWwindow *const window;

  // ウィンドウの幅と高さ
  GLsizei size[2];

  // カメラに対するオブジェクトの中心位置
  GLfloat eye[3];

  // ドラッグ開始位置
  double start[2];

  // トラックボール処理
  GgTrackball trackball;

  // プロジェクション変換行列
  GgMatrix mp;

  //
  // コピーコンストラクタ (コピー禁止)
  //
  Window(const Window &w);

  //
  // 代入 (代入禁止)
  //
  Window &operator=(const Window &w);

public:

  //
  // コンストラクタ
  //
  Window(int width = 640, int height = 480, const char *title = "GLFW Window",
    GLFWmonitor *monitor = NULL, GLFWwindow *share = NULL);

  //
  // デストラクタ
  //
  virtual ~Window();

  //
  // ウィンドウの識別子の取得
  //
  const GLFWwindow *get() const
  {
    return window;
  }

  //
  // ウィンドウを閉じるべきかを判定する
  //
  //   ・描画ループの継続条件として使う
  //
  bool shouldClose() const
  {
    // ウィンドウを閉じるか ESC キーがタイプされていれば真
    return glfwWindowShouldClose(window) || glfwGetKey(window, GLFW_KEY_ESCAPE);
  }

  //
  // 画面クリア
  //
  //   ・図形の描画開始前に呼び出す
  //   ・画面の消去などを行う
  //
  void clear();

  //
  // カラーバッファを入れ替えてイベントを取り出す
  //
  //   ・図形の描画終了後に呼び出す
  //   ・ダブルバッファリングのバッファの入れ替えを行う
  //   ・キーボード操作等のイベントを取り出す
  //
  void swapBuffers();

  //
  // ウィンドウのサイズ変更時の処理
  //
  //   ・ウィンドウのサイズ変更時にコールバック関数として呼び出される
  //   ・ウィンドウの作成時には明示的に呼び出す
  //
  static void resize(GLFWwindow *window, int width, int height);

  //
  // マウスボタンを操作したときの処理
  //
  //   ・マウスボタンを押したときにコールバック関数として呼び出される
  //
  static void mouse(GLFWwindow *window, int button, int action, int mods);

  //
  // マウスホイール操作時の処理
  //
  //   ・マウスホイールを操作した時にコールバック関数として呼び出される
  //
  static void wheel(GLFWwindow *window, double x, double y);

  //
  // キーボードをタイプした時の処理
  //
  //   ．キーボードをタイプした時にコールバック関数として呼び出される
  //
  static void keyboard(GLFWwindow *window, int key, int scancode, int action, int mods);

  //
  // 現在のウィンドウのサイズを得る
  //
  const GLsizei *getSize() const
  {
    return size;
  }

  //
  // モデルビュー変換行列を得る
  //
  GgMatrix getMw() const
  {
    return trackball.getMatrix() * ggTranslate(eye);
  }

  //
  // プロジェクション変換行列を得る
  //
  const GgMatrix &getMp() const
  {
    return mp;
  }
};
