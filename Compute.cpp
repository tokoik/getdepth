#include "Compute.h"

//
// 画像処理（コンピュートシェーダ版）
//

// コンストラクタ
Compute::Compute(const char *comp)
  : program(ggLoadComputeShader(comp))
{
}

// デストラクタ
Compute::~Compute()
{
  // シェーダプログラムを削除する
  glDeleteShader(program);
}

// 計算を実行する
void Compute::execute(GLuint width, GLuint height, GLuint local_size_x, GLuint local_size_y) const
{
  glDispatchCompute((width + local_size_x - 1) / local_size_x, (height + local_size_y - 1) / local_size_y, 1);
}
