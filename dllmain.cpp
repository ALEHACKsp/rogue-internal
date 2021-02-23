#define _CRT_SECURE_NO_WARNINGS
#include "dependencies.h"
#include <sstream>
#include "xor.hpp"
#include "Scanner.h"
#define xorthis(str) _xor_(str).c_str()
#define RELATIVE_ADDR(addr, size) ((PBYTE)((UINT_PTR)(addr) + *(PINT)((UINT_PTR)(addr) + ((size) - sizeof(INT))) + (size)))

uint64_t uworld;
bool ShowMenu = true;
ID3D11Device* device = nullptr;
ID3D11DeviceContext* immediateContext = nullptr;
ID3D11RenderTargetView* renderTargetView = nullptr;

HRESULT(*PresentOriginal)(IDXGISwapChain* swapChain, UINT syncInterval, UINT flags) = nullptr;

WNDPROC oriWndProc = NULL;
extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	if (GetAsyncKeyState(VK_INSERT))
		ShowMenu = !ShowMenu;
	if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam) && ShowMenu == true)
	{
		return true;
	}
	return CallWindowProc(oriWndProc, hWnd, msg, wParam, lParam);
}

BOOL valid_pointer(DWORD64 address)
{
	if (!IsBadWritePtr((LPVOID)address, (UINT_PTR)8)) return TRUE;
	else return FALSE;
}

VOID FreeMemory(PVOID buffer) {
	return reinterpret_cast<VOID(*)(PVOID)>(FreeFN)(buffer);
}

char* GetObjectName(uintptr_t Actor) {
	auto GetObjectName = reinterpret_cast<FString * (*)(FString*, uintptr_t)>(GetObjectNames);
	FString buffer;
	GetObjectName(&buffer, Actor);
	if (!buffer.c_str()) {
		return {};
	}
	CHAR result[256];
	strcpy_s(result, sizeof(result), buffer.c_str());
	FreeMemory((PVOID)buffer.data());
	return result;
}

class FMinimalViewInfo
{
public:
	Vector3 Location;
	Vector3 Rotation;
	float FOV;
};

void ClientSetRotation(Vector3 Angle)
{
	auto ClientSetRotation = (*(void(__fastcall**)(uint64_t, Vector3, char))(*(uint64_t*)PlayerController + 0x640));
	ClientSetRotation(PlayerController, Angle, (char)0);
}

void inline DrawItems(uintptr_t pawn, std::string object_name, ImGuiWindow& window)
{
	float olddistance = 0x7FFF;
	if (!pawn) return;
	uintptr_t RootComponent = read<uint64_t>(pawn + 0x130);//rootcomp
	if (!RootComponent) return;
	Vector3 CenterLocation = *(Vector3*)(RootComponent + 0x11c);//relativelocation
	Vector3 vScreenPos;
	WorldToScreen(CenterLocation, &vScreenPos);
	if (vScreenPos.x == 0 && vScreenPos.y == 0) return;
	window.DrawList->AddText(ImVec2(vScreenPos.x, vScreenPos.y), ImGui::GetColorU32({ 255.f, 255.f, 255.f, 1.f }), object_name.c_str());
}

Vector3 GetBoneMatrix(uintptr_t CurrentActorMesh, int boneid, Vector3* out)
{
	auto GetBoneMatrix = reinterpret_cast<Matrix4 * (__fastcall*)(uintptr_t, Matrix4*, int)>(BoneMatrix);

	Matrix4 BoneMatrix;
	if (!GetBoneMatrix((uintptr_t)CurrentActorMesh, &BoneMatrix, boneid)) return Vector3(0, 0, 0);

	out->x = BoneMatrix._41;
	out->y = BoneMatrix._42;
	out->z = BoneMatrix._43;
}

uintptr_t GetCurrentMesh(uintptr_t base)
{
	static uintptr_t returnval = 0;

	returnval = read<uintptr_t>(base + 0x280);//mesh

	return returnval;
}

uintptr_t GetCurrentState(uintptr_t base)
{
	static uintptr_t returnval = 0;

	returnval = read<uintptr_t>(base + 0x240);//state

	return returnval;
}

bool cLineOfSight(uintptr_t CurrentActor)
{
	Vector3 tmp = { 0,0,0 };

	auto fLineOfSight = ((BOOL(__fastcall*)(uintptr_t, uintptr_t, Vector3*))(LineOfSight));
	return fLineOfSight(PlayerController, CurrentActor, &tmp);
}

bool CheckIfInFOV(uintptr_t CurrentActor, float& max)
{
	uintptr_t LocalState = GetCurrentState(LocaPawn);
	uintptr_t EnemyState = GetCurrentState(CurrentActor);
	Vector3 rootHead;

	GetBoneMatrix(GetCurrentMesh(CurrentActor), 110, &rootHead); WorldToScreen(rootHead, &rootHead);

	if (rootHead.x <= 0 || rootHead.y <= 0) return false;
	if (rootHead.x >= X || rootHead.y >= Y) return false;

	if (Settings::bVisible && !cLineOfSight(CurrentActor)) {
		return false;
	}

	//else if (read<uintptr_t>(EnemyState + Offsets::bDowned)) {
	///	return false;
	//}

	else {
		float Dist = sqrtf(powf((float)(rootHead.x - (X / 2)), (float)2) + powf((float)(rootHead.y - (Y / 2)), (float)2));

		if (Dist < max)
		{
			float Radius = (Settings::FOV * X / 80.0f) / 2;

			if (rootHead.x <= ((X / 2) + Radius) && rootHead.x >= ((X / 2) - Radius) && rootHead.y <= ((Y / 2) + Radius) && rootHead.y >= ((Y / 2) - Radius))
			{
				max = Dist;
				return true;
			}

			return false;
		}
	}
}


ULONG(__fastcall* GetViewPoint)(uintptr_t, Vector3*, Vector3*) = NULL;
ULONG GetPlayerViewPointHook(uintptr_t Controller, Vector3* pLocation, Vector3* pRotation) {

	GetViewPoint(Controller, pLocation, pRotation);

	CamLoc = *pLocation;
	CamRot = *pRotation;

	return GetViewPoint(Controller, pLocation, pRotation);
}

bool updatestuff(ImGuiWindow& window)
{
	float FOVmax = 9999.f;
	uintptr_t GWorld = read<uintptr_t>(uworld_r);
	if (!GWorld) return false;

	//std::cout << "1\n\n";

	uintptr_t Gameinstance = read<uintptr_t>(GWorld + Offsets::OwningGameInstance);
	if (!Gameinstance) return false;

	//std::cout << "2\n\n";
	uintptr_t LocalPlayers = read<uintptr_t>(Gameinstance + Offsets::LocalPlayers);
	if (!LocalPlayers) return false;
	//
		//std::cout << "3\n\n";
	uintptr_t Localplayer = read<uintptr_t>(LocalPlayers);
	if (!Localplayer) return false;

	//std::cout << "4\n\n";
	PlayerController = read<uintptr_t>(Localplayer + Offsets::PlayerController);
	if (!PlayerController) return false;

	LocaPawn = read<uintptr_t>(PlayerController + Offsets::AcknowledgedPawn);
	if (!LocaPawn) return false;
	//LocaPawn

	X = GetSystemMetrics(SM_CXSCREEN);
	Y = GetSystemMetrics(SM_CYSCREEN);

	//std::cout << "5\n\n";

	//std::cout << "6\n\n";
	uintptr_t Ulevel = read<uint64_t>(GWorld + Offsets::PersistentLevel);
	uintptr_t AActors = read<uint64_t>(Ulevel + Offsets::AACTORS);
	uintptr_t ActorCount = read<int>(Ulevel + Offsets::ACTORCOUNT);


	//	std::cout << "7\n\n";
	for (int i = 0; i < ActorCount; i++)
	{
		//std::cout << "8\n\n";
		uintptr_t CurrentActor = read<uint64_t>(AActors + i * sizeof(uintptr_t));

		//std::cout << "9\n\n";
		std::string NameCurrentActor = GetObjectName(CurrentActor);

		//std::cout << "10\n\n";
		//DWORD64 Root = read<DWORD64>(CurrentActor + Offsets::ROOTCOMPONENT);
	//	Vector3 loc = read<Vector3>(Root + Offsets::RELATIVELOCATION);

		//std::cout << "11\n\n";
	//	Vector3 vScreenPos;
		//WorldToScreen(loc, &vScreenPos, PlayerController);
//
	//	window.DrawList->AddText(ImVec2(vScreenPos.x, vScreenPos.y), ImGui::GetColorU32({ 255.f, 255.f, 255.f, 1.f }), NameCurrentActor.c_str());
	//	DrawItems(CurrentActor, NameCurrentActor.c_str(), window);
		//std::cout << "12\n\n";
		//110 head id
		if (strstr(NameCurrentActor.c_str(), xorthis("DefaultPVPBotCharacter_C")) || strstr(NameCurrentActor.c_str(), xorthis("DefaultBotCharacter_C")) || strstr(NameCurrentActor.c_str(), xorthis("MainCharacter_C"))) {
			//std::cout << NameCurrentActor.c_str() << "\n\n";

			if (valid_pointer(LocaPawn))
				if (CurrentActor == LocaPawn) continue;

			//DrawItems(CurrentActor, xorthis("[PLAYER]"), window);

			Vector3 Headbox, bottom;

			GetBoneMatrix(GetCurrentMesh(CurrentActor), 110, &Headbox);
			WorldToScreen(Vector3(Headbox.x, Headbox.y, Headbox.z + 15), &Headbox);

			GetBoneMatrix(GetCurrentMesh(CurrentActor), 0, &bottom);
			WorldToScreen(bottom, &bottom);

			if (Settings::SnapLines)
				window.DrawList->AddLine(ImVec2(bottom.x, bottom.y), ImVec2(X / 2, Y / 2), ImGui::GetColorU32({ 255.f, 0.f, 0.f, 1.f }));

			ImU32 color = ImGui::GetColorU32({ 255.f, 0.f, 0.f, 1.f });

			float Height = Headbox.y - bottom.y;
			if (Height < 0)
				Height = Height * (-1.f);
			float Width = Height * 0.65;
			Headbox.x = Headbox.x - (Width / 2);

			float linewidth = (Width / 3);
			float lineheight = (Height / 3);

			if (Settings::BoxEsp)
			{
				window.DrawList->AddLine(ImVec2(Headbox.x, Headbox.y), ImVec2(Headbox.x, Headbox.y + lineheight), color);
				window.DrawList->AddLine(ImVec2(Headbox.x, Headbox.y), ImVec2(Headbox.x + linewidth, Headbox.y), color);
				window.DrawList->AddLine(ImVec2(Headbox.x + Width - linewidth, Headbox.y), ImVec2(Headbox.x + Width, Headbox.y), color);
				window.DrawList->AddLine(ImVec2(Headbox.x + Width, Headbox.y), ImVec2(Headbox.x + Width, Headbox.y + lineheight), color);
				window.DrawList->AddLine(ImVec2(Headbox.x, Headbox.y + Height - lineheight), ImVec2(Headbox.x, Headbox.y + Height), color);
				window.DrawList->AddLine(ImVec2(Headbox.x, Headbox.y + Height), ImVec2(Headbox.x + linewidth, Headbox.y + Height), color);
				window.DrawList->AddLine(ImVec2(Headbox.x + Width - linewidth, Headbox.y + Height), ImVec2(Headbox.x + Width, Headbox.y + Height), color);
				window.DrawList->AddLine(ImVec2(Headbox.x + Width, Headbox.y + Height - lineheight), ImVec2(Headbox.x + Width, Headbox.y + Height), color);
			}

			if (Settings::Aimbot)
			{
				if (CheckIfInFOV(CurrentActor, FOVmax) && valid_pointer(LocaPawn))
				{
					if (GetAsyncKeyState(VK_RBUTTON))
					{
						Vector3 RetVector = { 0,0,0 };

						Vector3 rootHead;
						GetBoneMatrix(GetCurrentMesh(CurrentActor), 110, &rootHead);

						Vector3 VectorPos = rootHead - CamLoc;

						float distance = VectorPos.Length();
						RetVector.x = -(((float)acos(VectorPos.z / distance) * (float)(180.0f / M_PI)) - 90.f);
						RetVector.y = (float)atan2(VectorPos.y, VectorPos.x) * (float)(180.0f / M_PI);

						ClientSetRotation(RetVector);

					}
				}
			}

			if (Settings::NameESP)
			{
				FString playerName = read<FString>(GetCurrentState(CurrentActor) + 0x300);
				CHAR buffer[32];
				strcpy_s(buffer, sizeof(buffer), playerName.c_str());

				window.DrawList->AddText(ImVec2(Headbox.x + lineheight, Headbox.y - linewidth), color, buffer);
			}

			if (Settings::SkeletonESP)
			{
				Vector3 vHip, vNeck, vUpperArmLeft, vUpperArmRight, vUpperArmRight2, vMiddleArm, vLeftHand, vRightHand, vLeftHand1,
					vRightHand1, vRightThigh, vLeftThigh, vRightCalf, vLeftCalf, vLeftFoot, vRightFoot;

				GetBoneMatrix(GetCurrentMesh(CurrentActor), 2, &vHip); WorldToScreen(vHip, &vHip);//
				GetBoneMatrix(GetCurrentMesh(CurrentActor), 108, &vNeck); WorldToScreen(vNeck, &vNeck); //lets see
				GetBoneMatrix(GetCurrentMesh(CurrentActor), 66, &vUpperArmLeft); WorldToScreen(vUpperArmLeft, &vUpperArmLeft);//lets see
				GetBoneMatrix(GetCurrentMesh(CurrentActor), 65, &vUpperArmRight); WorldToScreen(vUpperArmRight, &vUpperArmRight);//lets see
				GetBoneMatrix(GetCurrentMesh(CurrentActor), 101, &vUpperArmRight2); WorldToScreen(vUpperArmRight2, &vUpperArmRight2);//lets see
				GetBoneMatrix(GetCurrentMesh(CurrentActor), 39, &vLeftHand); WorldToScreen(vLeftHand, &vLeftHand);//
				GetBoneMatrix(GetCurrentMesh(CurrentActor), 75, &vRightHand); WorldToScreen(vRightHand, &vRightHand);//
				GetBoneMatrix(GetCurrentMesh(CurrentActor), 74, &vMiddleArm); WorldToScreen(vMiddleArm, &vMiddleArm);//middle arm
				GetBoneMatrix(GetCurrentMesh(CurrentActor), 38, &vLeftHand1); WorldToScreen(vLeftHand1, &vLeftHand1);//lets see
				GetBoneMatrix(GetCurrentMesh(CurrentActor), 196, &vRightHand1); WorldToScreen(vRightHand1, &vRightHand1);//lets see
				GetBoneMatrix(GetCurrentMesh(CurrentActor), 3, &vRightThigh); WorldToScreen(vRightThigh, &vRightThigh);//
				GetBoneMatrix(GetCurrentMesh(CurrentActor), 15, &vLeftThigh); WorldToScreen(vLeftThigh, &vLeftThigh);//
				GetBoneMatrix(GetCurrentMesh(CurrentActor), 4, &vRightCalf); WorldToScreen(vRightCalf, &vRightCalf);//
				GetBoneMatrix(GetCurrentMesh(CurrentActor), 16, &vLeftCalf); WorldToScreen(vLeftCalf, &vLeftCalf);//
				GetBoneMatrix(GetCurrentMesh(CurrentActor), 17, &vLeftFoot); WorldToScreen(vLeftFoot, &vLeftFoot);//
				GetBoneMatrix(GetCurrentMesh(CurrentActor), 5, &vRightFoot); WorldToScreen(vRightFoot, &vRightFoot);//

				window.DrawList->AddLine(ImVec2(vMiddleArm.x, vMiddleArm.y), ImVec2(vUpperArmRight2.x, vUpperArmRight2.y), color);
				window.DrawList->AddLine(ImVec2(vRightHand.x, vRightHand.y), ImVec2(vMiddleArm.x, vMiddleArm.y), color);
				window.DrawList->AddLine(ImVec2(vNeck.x, vNeck.y), ImVec2(vUpperArmRight2.x, vUpperArmRight2.y), color);
				window.DrawList->AddLine(ImVec2(vUpperArmRight.x, vUpperArmRight.y), ImVec2(vNeck.x, vNeck.y), color);
				window.DrawList->AddLine(ImVec2(vUpperArmRight.x, vUpperArmRight.y), ImVec2(vUpperArmLeft.x, vUpperArmLeft.y), color);
				window.DrawList->AddLine(ImVec2(vLeftHand.x, vLeftHand.y), ImVec2(vUpperArmLeft.x, vUpperArmLeft.y), color);
				window.DrawList->AddLine(ImVec2(vHip.x, vHip.y), ImVec2(vNeck.x, vNeck.y), color);//straight line from dick to neck
				window.DrawList->AddLine(ImVec2(vLeftThigh.x, vLeftThigh.y), ImVec2(vHip.x, vHip.y), color);
				window.DrawList->AddLine(ImVec2(vRightThigh.x, vRightThigh.y), ImVec2(vHip.x, vHip.y), color);
				window.DrawList->AddLine(ImVec2(vLeftCalf.x, vLeftCalf.y), ImVec2(vLeftThigh.x, vLeftThigh.y), color);
				window.DrawList->AddLine(ImVec2(vRightCalf.x, vRightCalf.y), ImVec2(vRightThigh.x, vRightThigh.y), color);
				window.DrawList->AddLine(ImVec2(vLeftFoot.x, vLeftFoot.y), ImVec2(vLeftCalf.x, vLeftCalf.y), color);
				window.DrawList->AddLine(ImVec2(vRightFoot.x, vRightFoot.y), ImVec2(vRightCalf.x, vRightCalf.y), color);
			}
		}

		//std::cout << NameCurrentActor.c_str() << "\n\n"; //debug get all  names
		//std::cout << "13\n\n";
	}
}
static int Active_Tab = 1;

void Menu(ImGuiWindow& window)
{
	{
		if (ShowMenu)
		{

			static float childHeight = 0.0f;
			ImGui::Begin(xorthis("rogue"), &ShowMenu, ImVec2{ 550, 250 }, ImGuiWindowFlags_NoCollapse);
			ImGui::Checkbox(xorthis("enable aimbot"), &Settings::Aimbot);
		//	ImGui::Checkbox(xorthis("enable nospread"), &Settings::NoSpread);
			ImGui::Checkbox(_xor_("box esp").c_str(), &Settings::BoxEsp);
			ImGui::Checkbox(_xor_("line esp").c_str(), &Settings::SnapLines);
			ImGui::Checkbox(_xor_("skeleton esp").c_str(), &Settings::SkeletonESP);
			ImGui::SliderFloat(xorthis("aim fov##slider"), &Settings::FOV, 0.0f, 1000.0f, ("%.2f"));
			ImGui::End();
			//	std::cout << "start\n\n";
		}
		try {
			updatestuff(window);
		}
		catch (...) {}
	}
}


__declspec(dllexport) HRESULT PresentHook(IDXGISwapChain* swapChain, UINT syncInterval, UINT flags) {
	if (!device) {
		swapChain->GetDevice(__uuidof(device), reinterpret_cast<PVOID*>(&device));
		device->GetImmediateContext(&immediateContext);

		ID3D11Texture2D* renderTarget = nullptr;
		swapChain->GetBuffer(0, __uuidof(renderTarget), reinterpret_cast<PVOID*>(&renderTarget));
		device->CreateRenderTargetView(renderTarget, nullptr, &renderTargetView);
		renderTarget->Release();

		HWND targetWindow = 0;
		EnumWindows([](HWND hWnd, LPARAM lParam) -> BOOL {
			DWORD pid = 0;
			GetWindowThreadProcessId(hWnd, &pid);
			if (pid == GetCurrentProcessId()) {
				*reinterpret_cast<HWND*>(lParam) = hWnd;
				return FALSE;
			}

			return TRUE;
			}, reinterpret_cast<LPARAM>(&targetWindow));

		ImGui_ImplDX11_Init(targetWindow, device, immediateContext);
		ImGui_ImplDX11_CreateDeviceObjects();
	}

	immediateContext->OMSetRenderTargets(1, &renderTargetView, nullptr);

	auto& window = BeginScene();
	window.DrawList->AddCircle(ImVec2((GetSystemMetrics)(0) / 2, (GetSystemMetrics)(1) / 2), Settings::FOV, ImGui::GetColorU32({ 0.0f, 0.0f, 0.0f, 1.00f }), 35, 2.f);

	Menu(window);
	EndScene(window);
	return PresentOriginal(swapChain, syncInterval, flags);
}

ImGuiWindow& BeginScene() {
	ImGui_ImplDX11_NewFrame();
	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
	ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0, 0, 0, 0));
	ImGui::Begin("##scene", nullptr, ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoTitleBar);

	auto& io = ImGui::GetIO();
	ImGui::SetWindowPos(ImVec2(0, 0), ImGuiCond_Always);
	ImGui::SetWindowSize(ImVec2(io.DisplaySize.x, io.DisplaySize.y), ImGuiCond_Always);

	return *ImGui::GetCurrentWindow();
}

VOID EndScene(ImGuiWindow& window) {
	window.DrawList->PushClipRectFullScreen();
	ImGui::End();
	ImGui::PopStyleColor();
	ImGui::PopStyleVar(2);
	ImGui::Render();
}

VOID CreateConsole()
{
	AllocConsole();
	static_cast<VOID>(freopen("CONIN$", "r", stdin));
	static_cast<VOID>(freopen("CONOUT$", "w", stdout));
	static_cast<VOID>(freopen("CONOUT$", "w", stderr));
}

#include "minhook/MinHook.h"

__int64(*CalculateSpread)(uintptr_t, int*, int*) = nullptr;
__int64 __fastcall CalculateSpreadHook(__int64 a1, DWORD* a2, DWORD* a3)
{

	__int64 result = 0;
	if (Settings::NoSpread)
	{
		return result;
	}
	else if (*(DWORD*)(a1 + 120))
	{
		*a2 = *(DWORD*)(*(DWORD64*)(a1 + 112) + 4i64);
		result = *(DWORD64*)(a1 + 112);
		*a3 = *(DWORD*)(28i64 * *(int*)(a1 + 120) + result - 24);
	}
	else
	{
		*a2 = 0;
		*a3 = 0;
	}

}

bool Main() {
	//CreateConsole();
	HWND window = FindWindow(0, _xor_(L"Rogue Company  ").c_str()); //you can make it find UnrealWindow if you want doesnt change anything

	oriWndProc = (WNDPROC)SetWindowLongPtr(window, GWLP_WNDPROC, (LONG_PTR)WndProc);
	IDXGISwapChain* swapChain = nullptr;
	ID3D11Device* device = nullptr;
	ID3D11DeviceContext* context = nullptr;
	auto                 featureLevel = D3D_FEATURE_LEVEL_11_0;

	DXGI_SWAP_CHAIN_DESC sd = { 0 };
	sd.BufferCount = 1;
	sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
	sd.OutputWindow = FindWindow((L"UnrealWindow"), (L"Rogue Company  "));
	sd.SampleDesc.Count = 1;
	sd.Windowed = TRUE;

	if (FAILED(D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, 0, 0, &featureLevel, 1, D3D11_SDK_VERSION, &sd, &swapChain, &device, nullptr, &context))) {
		MessageBox(0, L"Failed to create D3D11 device and swap chain", L"Failure", MB_ICONERROR);
		return FALSE;
	}

	auto table = *reinterpret_cast<PVOID**>(swapChain);
	auto present = table[8];
	auto resize = table[13];

	context->Release();
	device->Release();
	swapChain->Release();

	MH_CreateHook(present, PresentHook, reinterpret_cast<PVOID*>(&PresentOriginal));
	MH_EnableHook(present);

	auto addr = Scanners::PatternScan(xorthis("48 89 5C 24 10 48 89 74 24 18 55 41 56 41 57 48 8B EC"));

	MH_CreateHook((LPVOID)addr, GetPlayerViewPointHook, reinterpret_cast<PVOID*>(&GetViewPoint));
	MH_EnableHook((LPVOID)addr);

	addr = Scanners::PatternScan(xorthis("83 79 78 ? 4C 8B C9 75 0F 0F 57 C0 C7 02 ? ? ? ? F3 41 0F 11 ? C3 48 8B 41 70 8B 48 04 89 0A 49 63 41 78 48 6B C8 1C 49 8B 41 70 F3 0F 10 44 01 ? F3 41 0F 11 ? C3"));

	MH_CreateHook((LPVOID)addr, CalculateSpreadHook, reinterpret_cast<PVOID*>(&CalculateSpread));
	MH_EnableHook((LPVOID)addr);
}

BOOL APIENTRY DllMain(HMODULE module, DWORD reason, LPVOID reserved) {
	if (reason == DLL_PROCESS_ATTACH) {
		MH_Initialize();
		if (!uworld_r)
		{
			uworld_r = Scanners::PatternScan(_xor_("48 8B 1D ? ? ? ? 48 85 DB 74 3B 41").c_str());
			uworld_r = reinterpret_cast<uintptr_t>(RELATIVE_ADDR(uworld_r, 7));
			if (!uworld_r)
			{
				MessageBoxA(0, _xor_("UWORLD FAILED").c_str(), 0, 0);
			}
		}

		if (!GetObjectNames)
		{
			GetObjectNames = Scanners::PatternScan(_xor_("40 53 48 83 EC 20 48 8B D9 48 85 D2 75 45 33 C0 48 89 01 48 89 41 08 8D 50 05 E8 ? ? ? ? 8B 53 08 8D 42 05 89 43 08 3B 43 0C 7E 08 48 8B CB E8 ? ? ? ? 48 8B 0B 48 8D 15 ? ? ? ? 41 B8 ? ? ? ? E8 ? ? ? ? 48 8B C3 48 83 C4 20 5B C3 48 8B 42 18").c_str());
			if (!GetObjectNames)
			{
				MessageBoxA(0, _xor_("GETOBJECTNAMES FAILED").c_str(), 0, 0);
			}
		}

		if (!FreeFN)
		{
			FreeFN = Scanners::PatternScan(_xor_("48 85 C9 74 2E 53 48 83 EC 20 48 8B D9 48 8B 0D ? ? ? ? 48 85 C9 75 0C").c_str());
			if (!FreeFN)
			{
				MessageBoxA(0, _xor_("FREE FAILED").c_str(), 0, 0);
			}
		}

		if (!ProjectWorldToScreen)
		{
			ProjectWorldToScreen = Scanners::PatternScan(_xor_("48 89 5C 24 ? 48 89 6C 24 ? 48 89 74 24 ? 57 48 81 EC ? ? ? ? 41 0F B6 E9 49 8B D8").c_str());
			if (!ProjectWorldToScreen)
			{
				MessageBoxA(0, _xor_("ProjectWorldToScreen FAILED").c_str(), 0, 0);
			}
		}

		if (!BoneMatrix)
		{
			BoneMatrix = Scanners::PatternScan(_xor_("48 8B C4 55 53 56 57 41 54 41 56 41 57 48 8D 68 ?? 48 81 EC ?? ?? ?? ?? 0F 29 78").c_str());
			if (!BoneMatrix)
			{
				MessageBoxA(0, _xor_("BoneMatrix FAILED").c_str(), 0, 0);
			}
		}

		if (!LineOfSight)
		{
			LineOfSight = Scanners::PatternScan(_xor_("40 55 53 56 57 48 8D 6C 24 ? 48 81 EC ? ? ? ? 48 8B 05 ? ? ? ? 48 33 C4 48 89 45 E0 49").c_str());
			if (!LineOfSight)
			{
				MessageBoxA(0, _xor_("LineOfSight FAILED").c_str(), 0, 0);
			}
		}
		Main();
	}

	return TRUE;
}
