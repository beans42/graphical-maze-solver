#pragma once
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <Commdlg.h>
#include <shellapi.h>

#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"
#include "imgui/imgui_impl_dx9.h"
#include "imgui/imgui_impl_win32.h"

#include <C:\Program Files (x86)\Microsoft DirectX SDK (June 2010)\Include\d3d9.h>
#include <C:\Program Files (x86)\Microsoft DirectX SDK (June 2010)\Include\D3dx9tex.h>

#if _WIN64
#pragma comment(lib, "C:\\Program Files (x86)\\Microsoft DirectX SDK (June 2010)\\Lib\\x64\\d3d9.lib")
#pragma comment(lib, "C:\\Program Files (x86)\\Microsoft DirectX SDK (June 2010)\\Lib\\x64\\D3dx9.lib")
#else
#pragma comment(lib, "C:\\Program Files (x86)\\Microsoft DirectX SDK (June 2010)\\Lib\\x86\\d3d9.lib")
#pragma comment(lib, "C:\\Program Files (x86)\\Microsoft DirectX SDK (June 2010)\\Lib\\x86\\D3dx9.lib")
#endif //_WIN64

#include <string>
#include <vector>
#include <queue>

struct point_t {
	int x;
	int y;
};

std::string get_file_name() {
	char szFile[MAX_PATH];
	OPENFILENAME ofn; ZeroMemory(&ofn, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = NULL;
	ofn.lpstrFile = szFile;
	ofn.lpstrFile[0] = '\0';
	ofn.nMaxFile = sizeof(szFile);
	ofn.lpstrFilter = "All\0*.*\0Picture\0*.BMP;*.DDS;*.DIB;*.HDR;*.JPG;*.PFM;*.PNG;*.PPM;*.TGA\0"; //formats supported by D3DXCreateTextureFromFileEx()
	ofn.nFilterIndex = 2;
	ofn.lpstrFileTitle = NULL;
	ofn.nMaxFileTitle = 0;
	ofn.lpstrInitialDir = NULL;
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
	GetOpenFileNameA(&ofn);
	return std::string(szFile);
}

bool load_texture_from_file(IDirect3DDevice9* pd3dDevice, const char* filename, PDIRECT3DTEXTURE9* out_texture, int* out_width, int* out_height) {
	IDirect3DTexture9* texture;
	HRESULT hr = D3DXCreateTextureFromFileExA(pd3dDevice, filename, D3DX_DEFAULT_NONPOW2, D3DX_DEFAULT_NONPOW2, D3DX_DEFAULT, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, D3DX_DEFAULT, D3DX_DEFAULT, 0, nullptr, nullptr, &texture);
	if (hr != S_OK) return false;

	D3DSURFACE_DESC my_image_desc;
	texture->GetLevelDesc(0, &my_image_desc);
	*out_texture = texture;
	*out_width = (int)my_image_desc.Width;
	*out_height = (int)my_image_desc.Height;
	return true;
}