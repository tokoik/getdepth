#version 430 core

// 疑似カラー処理を行う場合は 1
#define PSEUDO_COLOR 1

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
uniform mat4 mv;                                            // モデルビュー変換行列
uniform mat4 mp;                                            // 投影変換行列
uniform mat4 mn;                                            // 法線ベクトルの変換行列

// テクスチャ
uniform sampler2D point;                                    // 頂点位置のテクスチャ
uniform sampler2D color;                                    // カラーのテクスチャ

// バッファオブジェクト
layout (std430) readonly buffer Uvmap
{
  vec2 uvmap[];                                             // テクスチャ座標
};
layout (std430) readonly buffer Normal
{
  vec4 normal[];                                            // 法線ベクトル
};

// 疑似カラー処理
uniform vec2 range = vec2(0.3, 6.0);

// ラスタライザに送る頂点属性
out vec4 idiff;                                             // 拡散反射光強度
out vec4 ispec;                                             // 鏡面反射光強度
out vec2 texcoord;                                          // テクスチャ座標

void main(void)
{
  // 頂点位置のテクスチャのサンプリング位置
  //   各頂点において gl_VertexID が 0, 1, 2, 3, ... のように割り当てられるから、
  //     x = gl_VertexID >> 1      = 0, 0, 1, 1, 2, 2, 3, 3, ...
  //     y = 1 - (gl_VertexID & 1) = 1, 0, 1, 0, 1, 0, 1, 0, ...
  //   のように GL_TRIANGLE_STRIP 向けの頂点座標値が得られる。
  //   y に gl_InstaceID を足せば glDrawArrayInstanced() のインスタンスごとに y が変化する。
  //   これをメッシュのサイズで割れば縦横 (0, 1) の範囲の点群が得られる。
  const int x = gl_VertexID >> 1;
  const int y = gl_InstanceID + 1 - (gl_VertexID & 1);
  const vec2 pc = (vec2(x, y) + 0.5) / vec2(textureSize(point, 0));

  // 頂点位置のサンプリング
  const vec4 pv = texture(point, pc);

  // 座標計算
  const vec4 p = mv * pv;                                   // 視点座標系の頂点の位置

  // クリッピング座標系における座標値
  gl_Position = mp * p;

  // 頂点インデックス
  const int i = y * textureSize(point, 0).x + x;

  // テクスチャ座標の取り出し
  texcoord = uvmap[i] / vec2(textureSize(color, 0));

  // 法線ベクトルの取り出し
  vec3 nv = vec3(normal[i]);

  // 陰影計算
  const vec3 v = normalize(vec3(p));                        // 視線ベクトル
  const vec3 l = normalize(vec3(lpos * p.w - p * lpos.w));  // 光線ベクトル
  const vec3 n = normalize(mat3(mn) * nv);                  // 法線ベクトル
  const vec3 h = normalize(l - v);                          // 中間ベクトル

#if PSEUDO_COLOR
  // 疑似カラー処理
  const float z = -6.0 * (pv.z + range.s) / (range.t - range.s);
  const vec4 c = clamp(vec4(z - 2.0, 2.0 - abs(z - 2.0), 2.0 - z, 1.0), 0.0, 1.0);

  // 拡散反射光強度
  idiff = c * max(dot(n, l), 0.0) * kdiff * ldiff + kamb * lamb;
#else
  // 拡散反射光強度
  idiff = max(dot(n, l), 0.0) * kdiff * ldiff + kamb * lamb;
#endif

  // 鏡面反射光強度
  ispec = pow(max(dot(n, h), 0.0), kshi) * kspec * lspec;
}
