#version 430 core

// 光源
layout (std140) uniform Light
{
  vec4 lamb;                                                // 環境光成分
  vec4 ldiff;                                               // 拡散反射光成分
  vec4 lspec;                                               // 鏡面反射光成分
  vec4 lpos;                                                // 位置
};

// 材質
layout (std140) uniform Material
{
  vec4 kamb;                                                // 環境光の反射係数
  vec4 kdiff;                                               // 拡散反射係数
  vec4 kspec;                                               // 鏡面反射係数
  float kshi;                                               // 輝き係数
};

// 変換行列
uniform mat4 mv;                                            // 視点座標系への変換行列
uniform mat4 mp;                                            // 投影変換行列
uniform mat4 mn;                                            // 法線ベクトルの変換行列

// テクスチャ
uniform sampler2D position;                                 // 頂点位置のテクスチャ
uniform sampler2D normal;                                   // 法線ベクトルのテクスチャ
uniform sampler2D back;                                     // 背景のテクスチャ

// 頂点属性
layout (location = 0) in vec2 pc;                           // 頂点のテクスチャ座標
layout (location = 1) in vec2 cc;                           // カラーのテクスチャ座標

// ラスタライザに送る頂点属性
out vec4 nv;                                                // 法線ベクトル
out vec4 idiff;                                             // 拡散反射光強度
out vec4 ispec;                                             // 鏡面反射光強度
out vec2 texcoord;                                          // テクスチャ座標

void main(void)
{
  // 頂点位置
  vec4 pv = texture(position, pc);

  // 座標計算
  vec4 p = mv * pv;                                         // 視点座標系の頂点の位置

  // クリッピング座標系における座標値
  gl_Position = mp * p;

  // テクスチャ座標
  texcoord = gl_Position.xy * 0.5 + 0.5;

  // 法線ベクトル
  nv = texture(normal, pc);

  // 陰影計算
  vec3 v = normalize(p.xyz);                                // 視線ベクトル
  vec3 l = normalize((lpos * p.w - p * lpos.w).xyz);        // 光線ベクトル
  vec3 n = normalize((mn * nv).xyz);                        // 法線ベクトル
  vec3 h = normalize(l - v);                                // 中間ベクトル

  // 拡散反射光強度
  idiff = max(dot(n, l), 0.0) * kdiff * ldiff + kamb * lamb;

  // 鏡面反射光強度
  ispec = pow(max(dot(n, h), 0.0), kshi) * kspec * lspec;
}
