#version 150 core
#extension GL_ARB_explicit_attrib_location : enable
#extension GL_ARB_explicit_uniform_location : enable

// テクスチャ
layout (location = 2) uniform sampler2D color;      // カラーのテクスチャ

// ラスタライザから受け取る頂点属性の補間値
in vec4 idiff;                                      // 拡散反射光強度
in vec4 ispec;                                      // 鏡面反射光強度
in vec2 texcoord;                                   // テクスチャ座標

// フレームバッファに出力するデータ
layout (location = 0) out vec4 fc;                  // フラグメントの色

void main(void)
{
  // テクスチャマッピングを行って陰影を求める
  //fc = idiff + ispec;
  //fc = texture(color, texcoord);
  fc = texture(color, texcoord) * idiff + ispec;
}
