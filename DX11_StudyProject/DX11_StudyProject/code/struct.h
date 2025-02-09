#pragma once

#include <DirectXMath.h>
#include <DirectXPackedVector.h>

using namespace DirectX;

struct VERTEX
{
	XMFLOAT4 pos;
	XMFLOAT4 col;
};

struct NORMALVERTEX
{
	XMFLOAT4 pos;
	XMFLOAT3 nrm;
};

struct TEXVERTEX
{
	XMFLOAT4 pos;
	XMFLOAT3 nrm;
	XMFLOAT2 uv;
};

struct TBNVERTEX
{
	XMFLOAT4 pos;
	XMFLOAT3 nrm;
	XMFLOAT3 tan;
	XMFLOAT2 uv;
};

struct GEOVERTEX
{
	XMFLOAT4 pos;
	XMFLOAT2 size;
};

struct PARTICLEVERTEX
{
	XMFLOAT4 pos;
	XMFLOAT3 vel;
	XMFLOAT2 size;
	float age;
	UINT type;
};

struct TRANSMATRIX
{
	XMFLOAT4X4 matWorld;
	XMFLOAT4X4 matWorldRT;
	XMFLOAT4X4 matWVP;
	XMFLOAT4X4 matTex;
};

struct TRANSMATRIX_TEX2
{
	XMFLOAT4X4 matWorld;
	XMFLOAT4X4 matWorldRT;
	XMFLOAT4X4 matWVP;
	XMFLOAT4X4 matTex;
	XMFLOAT4X4 matTex2;
};

struct LIGHT
{
	XMFLOAT3A direction;
	XMFLOAT4 diffuse;
	XMFLOAT4 ambient;
	XMFLOAT4 specular;
};

struct POINTLIGHT
{
	XMFLOAT3 position;
	float	 Range;
	XMFLOAT4 diffuse;
	XMFLOAT4 ambient;
	XMFLOAT4 specular;
};

struct MATERIAL
{
	XMFLOAT4 diffuse;
	XMFLOAT4 ambient;
	XMFLOAT4 specular; // w = power
	XMFLOAT4 Env;
};

struct PERFRAME
{
	XMFLOAT3A viewPos;
	XMFLOAT4 fogColor;
	float fogStart;
	float fogRange;
};

struct TESS
{
	float maxDistance;
	float minDistance;
	float maxFactor;
	float minFactor;
	XMFLOAT4 heightScale;
};