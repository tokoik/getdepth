#version 150 core
#extension GL_ARB_explicit_attrib_location : enable
#extension GL_ARB_explicit_uniform_location : enable

// テクスチャ
layout (location = 1) uniform sampler2D normal;     // 法線ベクトルのテクスチャ
layout (location = 3) uniform sampler2D back;		// 背景のテクスチャ

// ウィンドウサイズ
uniform vec2 size;

// ラスタライザから受け取る頂点属性の補間値
in vec3 s;                                          // 視線の屈折ベクトル
in vec4 idiff;                                      // 拡散反射光強度
in vec4 ispec;                                      // 鏡面反射光強度
in vec2 texcoord;                                   // テクスチャ座標

// フレームバッファに出力するデータ
layout (location = 0) out vec4 fc;                  // フラグメントの色

void main(void)
{
  // 法線ベクトル
  vec3 nv = texture(normal, texcoord).xyz;

  // 屈折マッピング
  fc = texture(back, gl_FragCoord.xy / size + refract(vec3(0.0, 0.0, -1.0), nv, 0.67).xy * 0.2) * idiff + ispec;
}
