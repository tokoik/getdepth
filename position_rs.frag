#version 150 core
#extension GL_ARB_explicit_attrib_location : enable
#extension GL_ARB_explicit_uniform_location : enable

// テクスチャ
layout (location = 0) uniform sampler2D depth;

// テクスチャのサイズ - 1
ivec2 ds = textureSize(depth, 0) - 1;

// フレームバッファに出力するデータ
layout (location = 0) out vec4 position;

// テクスチャ座標
in vec2 texcoord;

// カメラパラメータ
uniform vec2 dpp, df;

// バイラテラルフィルタの位置と明度の分散
uniform vec2 variance = vec2(1.0, 0.01);

// 対象画素の値
float base;

// 重み付き画素値の合計と重みの合計を求める
void f(inout vec2 csum, const in float c, const in float w)
{
  float d = c - base;
  float e = exp(-0.5 * d * d / variance[1]) * w;
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

  // 画素のスクリーン座標 (D415/D435 はデプスセンサのゆがみ補正をする必要がない)
  vec2 dp = (dpp - texcoord * ds) / df;

  // デプス値からカメラ座標値を求める
  position = vec4(dp * z, z, 1.0);
}
