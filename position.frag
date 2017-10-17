#version 150 core
#extension GL_ARB_explicit_attrib_location : enable
#extension GL_ARB_explicit_uniform_location : enable

// スケール
uniform vec2 scale;
uniform float depthMax, depthScale;

// テクスチャ
layout (location = 0) uniform sampler2D depth;

// テクスチャ座標
in vec2 texcoord;

// フレームバッファに出力するデータ
layout (location = 0) out vec3 position;

// デプス値をスケーリングする
float s(in float z)
{
  return z == 0.0 ? depthMax : z * depthScale;
}

void main(void)
{
  // デプス値を取り出す
  float z = s(texture(depth, texcoord).r);

  // デプス値からカメラ座標値を求める
  position = vec3((texcoord - 0.5) * scale * z, z);
}
