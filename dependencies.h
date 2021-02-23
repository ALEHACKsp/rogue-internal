#define M_PI	3.14159265358979323846264338327950288419716939937510
#include <stdio.h>
#include <windows.h>
#include <math.h>

#include <string>

#include "imgui/imgui.h"
#include "imgui/imgui_impl_dx11.h"
#include "imgui/imgui_internal.h"

#include "minhook/MinHook.h"
#pragma comment(lib, "minhook.lib")

#include <d3d11.h>
#include <d3dx9math.h>
#pragma comment(lib, "d3d11.lib")

ImGuiWindow& BeginScene();
VOID EndScene(ImGuiWindow& window);

uintptr_t uworld_r;
uint64_t base_address;
uintptr_t GetObjectNames;
uintptr_t BoneMatrix;
uintptr_t LineOfSight;
uintptr_t FreeFN;
uintptr_t ProjectWorldToScreen;
uintptr_t PlayerController;
uintptr_t LocaPawn;

namespace Offsets {
	uintptr_t OwningGameInstance = 0x188;
	uintptr_t LocalPlayers = 0x38;
	uintptr_t PlayerController = 0x30;
	uintptr_t AcknowledgedPawn = 0x2A0;
	uintptr_t ROOTCOMPONENT = 0x130;
	uintptr_t AACTORS = 0x98;
	uintptr_t ACTORCOUNT = 0xA0;
	uintptr_t Levels = 0x148;
	uintptr_t PersistentLevel = 0x30;
	uintptr_t RELATIVELOCATION = 0x11C;
	uintptr_t bDowned = 0x488;
}

template<typename T>
T read(DWORD_PTR address, const T& def = T())
{
	return *(T*)address;
}

template<typename T>
T write(DWORD_PTR address, DWORD_PTR address2, const T& def = T())
{
	return *(T*)address = address2;
}

namespace Settings
{
	float FOV = 80.0f;
	bool BoxEsp = true;
	bool SkeletonESP = true;
	bool NameESP = false;
	bool NoSpread = false;
	bool SnapLines = true;
	bool bVisible = true;
	bool bIgnoreDown = true;
	bool Aimbot = true;
}

class Matrix4
{
public:
	// Constructors
	Matrix4()
	{

	}
	~Matrix4()
	{

	}

	// Variables
	union {
		struct {
			float _11, _12, _13, _14;
			float _21, _22, _23, _24;
			float _31, _32, _33, _34;
			float _41, _42, _43, _44;
		};
		float m[4][4];
	};
};

typedef struct
{
	DWORD R;
	DWORD G;
	DWORD B;
	DWORD A;
}RGBA;

class Color
{
public:

	RGBA NiggaGreen = { 128, 224, 0, 200 };
	RGBA red = { 255,0,0,255 };
	RGBA Magenta = { 255,0,255,255 };
	RGBA yellow = { 255,255,0,255 };
	RGBA grayblue = { 128,128,255,255 };
	RGBA green = { 128,224,0,255 };
	RGBA darkgreen = { 0,224,128,255 };
	RGBA brown = { 192,96,0,255 };
	RGBA pink = { 255,168,255,255 };
	RGBA DarkYellow = { 216,216,0,255 };
	RGBA SilverWhite = { 236,236,236,255 };
	RGBA purple = { 144,0,255,255 };
	RGBA Navy = { 88,48,224,255 };
	RGBA skyblue = { 0,136,255,255 };
	RGBA graygreen = { 128,160,128,255 };
	RGBA blue = { 0,96,192,255 };
	RGBA orange = { 255,128,0,255 };
	RGBA peachred = { 255,80,128,255 };
	RGBA reds = { 255,128,192,255 };
	RGBA darkgray = { 96,96,96,255 };
	RGBA Navys = { 0,0,128,255 };
	RGBA darkgreens = { 0,128,0,255 };
	RGBA darkblue = { 0,128,128,255 };
	RGBA redbrown = { 128,0,0,255 };
	RGBA purplered = { 128,0,128,255 };
	RGBA greens = { 0,255,0,255 };
	RGBA envy = { 0,255,255,255 };
	RGBA black = { 0,0,0,255 };
	RGBA gray = { 128,128,128,255 };
	RGBA white = { 255,255,255,255 };
	RGBA blues = { 30,144,255,255 };
	RGBA lightblue = { 135,206,250,160 };
	RGBA Scarlet = { 220, 20, 60, 160 };
	RGBA white_ = { 255,255,255,200 };
	RGBA gray_ = { 128,128,128,200 };
	RGBA black_ = { 0,0,0,200 };
	RGBA red_ = { 255,0,0,200 };
	RGBA Magenta_ = { 255,0,255,200 };
	RGBA yellow_ = { 255,255,0,200 };
	RGBA grayblue_ = { 128,128,255,200 };
	RGBA green_ = { 128,224,0,200 };
	RGBA darkgreen_ = { 0,224,128,200 };
	RGBA brown_ = { 192,96,0,200 };
	RGBA pink_ = { 255,168,255,200 };
	RGBA darkyellow_ = { 216,216,0,200 };
	RGBA silverwhite_ = { 236,236,236,200 };
	RGBA purple_ = { 144,0,255,200 };
	RGBA Blue_ = { 88,48,224,200 };
	RGBA skyblue_ = { 0,136,255,200 };
	RGBA graygreen_ = { 128,160,128,200 };
	RGBA blue_ = { 0,96,192,200 };
	RGBA orange_ = { 255,128,0,200 };
	RGBA pinks_ = { 255,80,128,200 };
	RGBA Fuhong_ = { 255,128,192,200 };
	RGBA darkgray_ = { 96,96,96,200 };
	RGBA Navy_ = { 0,0,128,200 };
	RGBA darkgreens_ = { 0,128,0,200 };
	RGBA darkblue_ = { 0,128,128,200 };
	RGBA redbrown_ = { 128,0,0,200 };
	RGBA purplered_ = { 128,0,128,200 };
	RGBA greens_ = { 0,255,0,200 };
	RGBA envy_ = { 0,255,255,200 };
	RGBA glassblack = { 0, 0, 0, 160 };
	RGBA GlassBlue = { 65,105,225,80 };
	RGBA glassyellow = { 255,255,0,160 };
	RGBA glass = { 200,200,200,60 };
	RGBA Plum = { 221,160,221,160 };

};
Color Col;

template<class T>
struct TArray
{
	friend class FString;

public:
	inline TArray()
	{
		Data = NULL;
		Count = Max = 0;
	};

	inline INT Num() const
	{
		return Count;
	};

	inline T& operator[](int i)
	{
		return Data[i];
	};

	inline T& operator[](int i) const
	{
		return Data[i];
	};

	inline BOOL IsValidIndex(int i) const
	{
		return i < Num();
	}

private:
	T* Data;
	INT32 Count;
	INT32 Max;
};


class FString : private TArray<wchar_t>
{
public:
	inline FString() {

	};

	inline int Count() {
		return Num();
	}

	inline bool IsValid() {
		return Data != NULL;
	}

	inline const wchar_t* data() const {
		return Data;
	}

	inline const char* c_str() const {
		return std::string(Data, &Data[wcslen(Data)]).c_str();
	}
};

struct FVector2D {
	float X;
	float Y;

	inline BOOL IsValid() {
		return X != NULL && Y != NULL;
	}

	inline float distance() {
		return sqrtf(this->X * this->X + this->Y * this->Y);
	}
};

struct FVector {
	float X;
	float Y;
	float Z;

	FVector operator-(FVector buffer) {

		return FVector
		(
			{ this->X - buffer.X, this->Y - buffer.Y, this->Z - buffer.Z }
		);
	}

	FVector operator+(FVector buffer) {

		return FVector
		(
			{ this->X + buffer.X, this->Y + buffer.Y, this->Z + buffer.Z }
		);
	}

	inline float distance() {
		return sqrtf(this->X * this->X + this->Y * this->Y + this->Z * this->Z);
	}
};

class Vector3
{
public:
	float x;
	float y;
	float z;

	inline Vector3()
	{
		x = y = z = 0.0f;
	}

	inline Vector3(float X, float Y, float Z)
	{
		x = X; y = Y; z = Z;
	}

	inline float operator[](int i) const
	{
		return ((float*)this)[i];
	}

	inline Vector3& operator+=(float v)
	{
		x += v; y += v; z += v; return *this;
	}

	inline Vector3& operator-=(float v)
	{
		x -= v; y -= v; z -= v; return *this;
	}

	inline Vector3& operator-=(const Vector3& v)
	{
		x -= v.x; y -= v.y; z -= v.z; return *this;
	}

	inline Vector3 operator*(float v) const
	{
		return Vector3(x * v, y * v, z * v);
	}

	inline Vector3 operator/(float v) const
	{
		return Vector3(x / v, y / v, z / v);
	}

	inline Vector3& operator+=(const Vector3& v)
	{
		x += v.x; y += v.y; z += v.z; return *this;
	}

	inline Vector3 operator-(const Vector3& v) const
	{
		return Vector3(x - v.x, y - v.y, z - v.z);
	}

	inline Vector3 operator+(const Vector3& v) const
	{
		return Vector3(x + v.x, y + v.y, z + v.z);
	}

	inline Vector3& operator/=(float v)
	{
		x /= v; y /= v; z /= v; return *this;
	}

	inline bool Zero() const
	{
		return (x > -0.1f && x < 0.1f && y > -0.1f && y < 0.1f && z > -0.1f && z < 0.1f);
	}

	inline float Dot(Vector3 v)
	{
		return x * v.x + y * v.y + z * v.z;
	}

	inline float Distance(Vector3 v)
	{
		return float(sqrtf(powf(v.x - x, 2.0) + powf(v.y - y, 2.0) + powf(v.z - z, 2.0)));
	}

	inline double Length() {
		return sqrt(x * x + y * y + z * z);
	}

	Vector3 operator+(Vector3 v)
	{
		return Vector3(x + v.x, y + v.y, z + v.z);
	}
	Vector3 operator-(Vector3 v)
	{
		return Vector3(x - v.x, y - v.y, z - v.z);
	}
	Vector3 operator*(float flNum) { return Vector3(x * flNum, y * flNum, z * flNum); }
};

bool WorldToScreen(Vector3 vWorldPos, Vector3* vScreenPos)
{
	auto WorldToScreen = reinterpret_cast<bool(__fastcall*)(uintptr_t pPlayerController, Vector3 vWorldPos, Vector3 * vScreenPosOut, char)>(ProjectWorldToScreen);
	return WorldToScreen(PlayerController, vWorldPos, vScreenPos, (char)0);
}

class FRotator
{
public:
	FRotator() : Pitch(0.f), Yaw(0.f), Roll(0.f)
	{

	}

	FRotator(float _Pitch, float _Yaw, float _Roll) : Pitch(_Pitch), Yaw(_Yaw), Roll(_Roll)
	{

	}
	~FRotator()
	{

	}

	float Pitch;
	float Yaw;
	float Roll;


	double Length() {
		return sqrt(Pitch * Pitch + Yaw * Yaw + Roll * Roll);
	}

	FRotator operator+(FRotator angB) { return FRotator(Pitch + angB.Pitch, Yaw + angB.Yaw, Roll + angB.Roll); }
	FRotator operator-(FRotator angB) { return FRotator(Pitch - angB.Pitch, Yaw - angB.Yaw, Roll - angB.Roll); }
	FRotator operator/(float flNum) { return FRotator(Pitch / flNum, Yaw / flNum, Roll / flNum); }
	FRotator operator*(float flNum) { return FRotator(Pitch * flNum, Yaw * flNum, Roll * flNum); }
	bool operator==(FRotator angB) { return Pitch == angB.Pitch && Yaw == angB.Yaw && Yaw == angB.Yaw; }
	bool operator!=(FRotator angB) { return Pitch != angB.Pitch || Yaw != angB.Yaw || Yaw != angB.Yaw; }

};

Vector3 CamLoc, CamRot;
float X, Y;