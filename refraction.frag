#version 430 core

// テクスチャ
uniform sampler2D color;                                    // カラーのテクスチャ
uniform sampler2D back;		                                  // 背景のテクスチャ
uniform float alpha = 0.3;                                  // 透明度

// ラスタライザから受け取る頂点属性の補間値
in vec3 nv;                                                 // 法線ベクトル
in vec4 idiff;                                              // 拡散反射光強度
in vec4 ispec;                                              // 鏡面反射光強度
in vec2 texcoord;                                           // テクスチャ座標
in vec2 tc;                                                 // メッシュのテクスチャ座標

// フレームバッファに出力するデータ
layout (location = 0) out vec4 fc;                          // フラグメントの色

void main(void)
{
  // 屈折方向のテクスチャ座標
  if (gl_FragCoord.z < 0.99) {
    const vec2 offset = refract(vec3(0.0, 0.0, -1.0), normalize(nv), 0.8).xy * 0.2;
    fc = mix(texture(back, tc + offset) + ispec, texture(color, texcoord), alpha);
  }
  else
  {
    fc = texture(back, tc);
  }

  //fc = texture(back, tc);
  //fc = idiff + ispec;
  //fc = texture(back, tc) * idiff + ispec;
}

