#version 150 core
#extension GL_ARB_explicit_attrib_location : enable
#extension GL_ARB_explicit_uniform_location : enable

// テクスチャ
layout (location = 0) uniform sampler2D depth;

// カメラパラメータ
uniform vec2 ds, dc, df;
uniform vec3 dk;

// テクスチャ座標
in vec2 texcoord;

// フレームバッファに出力するデータ
layout (location = 0) out vec4 position;

// 分散
uniform float variance = 0.1;

// 対象画素の値
float base;

// 重み付き画素値の合計と重みの合計を求める
void f(inout vec2 csum, const in float c, const in float w)
{
  float d = c - base;
  float e = exp(-0.5 * d * d / variance) * w;
  csum += vec2(c * e, e);
}

void main(void)
{
  // 対象画素の値を基準値とする
  base = texture(depth, texcoord).r;

  // 対象画素の値とその重みのペアを作る
  vec2 csum = vec2(base, 1.0);

#if 1
  f(csum, textureOffset(depth, texcoord, ivec2(-2, -2)).r, 0.018315639);
  f(csum, textureOffset(depth, texcoord, ivec2(-1, -2)).r, 0.082084999);
  f(csum, textureOffset(depth, texcoord, ivec2( 0, -2)).r, 0.135335283);
  f(csum, textureOffset(depth, texcoord, ivec2( 1, -2)).r, 0.082084999);
  f(csum, textureOffset(depth, texcoord, ivec2( 2, -2)).r, 0.018315639);

  f(csum, textureOffset(depth, texcoord, ivec2(-2, -1)).r, 0.082084999);
  f(csum, textureOffset(depth, texcoord, ivec2(-1, -1)).r, 0.367879441);
  f(csum, textureOffset(depth, texcoord, ivec2( 0, -1)).r, 0.60653066);
  f(csum, textureOffset(depth, texcoord, ivec2( 1, -1)).r, 0.367879441);
  f(csum, textureOffset(depth, texcoord, ivec2( 2, -1)).r, 0.082084999);

  f(csum, textureOffset(depth, texcoord, ivec2(-2,  0)).r, 0.135335283);
  f(csum, textureOffset(depth, texcoord, ivec2(-1,  0)).r, 0.60653066);
  f(csum, textureOffset(depth, texcoord, ivec2( 1,  0)).r, 0.60653066);
  f(csum, textureOffset(depth, texcoord, ivec2( 2,  0)).r, 0.135335283);

  f(csum, textureOffset(depth, texcoord, ivec2(-2,  1)).r, 0.082084999);
  f(csum, textureOffset(depth, texcoord, ivec2(-1,  1)).r, 0.367879441);
  f(csum, textureOffset(depth, texcoord, ivec2( 0,  1)).r, 0.60653066);
  f(csum, textureOffset(depth, texcoord, ivec2( 1,  1)).r, 0.367879441);
  f(csum, textureOffset(depth, texcoord, ivec2( 2,  1)).r, 0.082084999);

  f(csum, textureOffset(depth, texcoord, ivec2(-2,  2)).r, 0.018315639);
  f(csum, textureOffset(depth, texcoord, ivec2(-1,  2)).r, 0.082084999);
  f(csum, textureOffset(depth, texcoord, ivec2( 0,  2)).r, 0.135335283);
  f(csum, textureOffset(depth, texcoord, ivec2( 1,  2)).r, 0.082084999);
  f(csum, textureOffset(depth, texcoord, ivec2( 2,  2)).r, 0.018315639);
#endif

  // デプス値を取り出す
  float z = csum.r / csum.g;

  // 画素のスクリーン座標
  vec2 dp = (texcoord * ds - dc + 0.5f) / df;

  // デプスカメラの歪み補正係数
  float dr = dot(dp, dp);
  float dq = (1.0 + dr * (dk.x + dr * (dk.y + dr * dk.z)));

  // デプス値からカメラ座標値を求める
  position = vec4(dp * z / dq, z, 1.0);
}
