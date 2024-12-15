#pragma once

struct float3
{
  float x, y, z;
};

inline float3 operator+(const float3& a, const float3& b)
{
  return {a.x + b.x, a.y + b.y, a.z + b.z};
}

inline float3 operator-(const float3& a, const float3& b)
{
  return {a.x - b.x, a.y - b.y, a.z - b.z};
}

inline float3 operator*(const float3& a, float b)
{
  return {a.x * b, a.y * b, a.z * b};
}