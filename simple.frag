#version 430 core

// テクスチャ
uniform sampler2D color;                                    // カラーのテクスチャ

// ラスタライザから受け取る頂点属性の補間値
in vec4 id;                                                 // 拡散反射光強度
in vec4 is;                                                 // 鏡面反射光強度
in vec2 tc;                                                 // テクスチャ座標

// フレームバッファに出力するデータ
layout (location = 0) out vec4 fc;                          // フラグメントの色

void main(void)
{
  // テクスチャマッピングを行って陰影を求める
  //fc = id + is;
  fc = texture(color, tc);
  //fc = texture(color, tc) * id + is;
}
