#include "includes.hpp"
#include "dijkstra.hpp"
#include "texture_manip.hpp"

#pragma region imgui_stuff
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

static IDirect3D9* g_pD3D = nullptr;
static IDirect3DDevice9* g_pd3dDevice = nullptr;
static D3DPRESENT_PARAMETERS g_d3dpp{};

bool CreateDeviceD3D(HWND hWnd) {
	if ((g_pD3D = Direct3DCreate9(D3D_SDK_VERSION)) == NULL) return false;

	ZeroMemory(&g_d3dpp, sizeof(g_d3dpp));
	g_d3dpp.Windowed = TRUE;
	g_d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
	g_d3dpp.BackBufferFormat = D3DFMT_UNKNOWN;
	g_d3dpp.EnableAutoDepthStencil = TRUE;
	g_d3dpp.AutoDepthStencilFormat = D3DFMT_D16;
	g_d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_ONE;
	//g_d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;   // Present without vsync, maximum unthrottled framerate
	if (g_pD3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hWnd, D3DCREATE_HARDWARE_VERTEXPROCESSING, &g_d3dpp, &g_pd3dDevice) < 0) return false;

	return true;
}

void CleanupDeviceD3D() {
	if (g_pd3dDevice) { g_pd3dDevice->Release(); g_pd3dDevice = nullptr; }
	if (g_pD3D) { g_pD3D->Release(); g_pD3D = nullptr; }
}

void ResetDevice() {
	ImGui_ImplDX9_InvalidateDeviceObjects();
	HRESULT hr = g_pd3dDevice->Reset(&g_d3dpp);
	if (hr == D3DERR_INVALIDCALL)
		IM_ASSERT(0);
	ImGui_ImplDX9_CreateDeviceObjects();
}

LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
		return true;

	switch (msg) {
	case WM_SIZE:
		if (g_pd3dDevice != NULL && wParam != SIZE_MINIMIZED) {
			g_d3dpp.BackBufferWidth = LOWORD(lParam);
			g_d3dpp.BackBufferHeight = HIWORD(lParam);
			ResetDevice();
		}
		return 0;
	case WM_SYSCOMMAND:
		if ((wParam & 0xfff0) == SC_KEYMENU) //disable ALT application menu
			return 0;
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	}
	return DefWindowProc(hWnd, msg, wParam, lParam);
}
#pragma endregion

int WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR pCmdLine, int nCmdShow) {
	WNDCLASSEX wc = { sizeof(WNDCLASSEX), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandleA(NULL), NULL, NULL, NULL, NULL, "mz slv", NULL };
	RegisterClassEx(&wc);
	HWND hwnd = CreateWindowExA(WS_EX_APPWINDOW, wc.lpszClassName, "maze solver | SexOffenderSally#0660", WS_OVERLAPPEDWINDOW | WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT, 800, 600, NULL, NULL, wc.hInstance, NULL);

	if (!CreateDeviceD3D(hwnd)) {
		CleanupDeviceD3D();
		UnregisterClass(wc.lpszClassName, wc.hInstance);
		return 1;
	}
	
	ShowWindow(hwnd, SW_SHOWDEFAULT);
	UpdateWindow(hwnd);

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	io.IniFilename = NULL;

	ImGuiStyle& style = ImGui::GetStyle();
	style.WindowRounding = 0.0f;
	style.FrameRounding = 5.f;
	style.GrabRounding = style.FrameRounding;

	ImGui::StyleColorsDark();

	ImGui_ImplWin32_Init(hwnd);
	ImGui_ImplDX9_Init(g_pd3dDevice);

	RECT rect = { -1 };
	bool main_wnd_init = false;
	
	IDirect3DTexture9* pic_texture = nullptr;
	int pic_width = 0;
	int pic_height = 0;
	std::string file_name;
	
	bool pic_chosen = false;
	
	bool cost_map = false;
	point_t start = { 0, 0 }, end = { 0, 0 };

	bool solved = false;

	bool show_whole_image = false;

	int marker_size = 10;

	texture_manip* tex = nullptr;

	MSG msg{};
	while (msg.message != WM_QUIT) {
		if (PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
			continue;
		}

		GetWindowRect(hwnd, &rect);

		ImGui_ImplDX9_NewFrame();
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();
		
		ImGui::SetNextWindowSize({ (float)(rect.right - rect.left - 14), (float)(rect.bottom - rect.top - 37) });

		{
			ImGui::Begin("##main window", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_HorizontalScrollbar);
			if (!main_wnd_init) {
				main_wnd_init = true;
				ImGui::SetWindowPos(ImVec2(-1, -1));
			}

			if (ImGui::Button("browse...")) {
				file_name = get_file_name();
				if (!pic_chosen) {
					if (load_texture_from_file(g_pd3dDevice, file_name.c_str(), &pic_texture, &pic_width, &pic_height)) {
						tex = new texture_manip(&pic_texture);
						pic_chosen = true;
						solved = false;
					}
				} else {
					if (load_texture_from_file(g_pd3dDevice, file_name.c_str(), &pic_texture, &pic_width, &pic_height)) {
						solved = false;
					}
				}
			}
			
			ImGui::SameLine();

			if (ImGui::Button("github"))
				ShellExecute(0, 0, "https://github.com/beans42", 0, 0, SW_SHOW);

			if (pic_chosen) {
				if (!solved)
					tex->draw_markers(start, end, marker_size);

				ImGui::SameLine();
				ImGui::Text(file_name.c_str());
				
				ImGui::Checkbox("show entire image", &show_whole_image);
				ImGui::SameLine();
				ImGui::Text("size: %dx%d", pic_width, pic_height);

				ImGui::SliderInt("marker size", &marker_size, 1, 100);
				ImGui::SliderInt("start x    ", &start.x, 0, pic_width  - 1);
				ImGui::SliderInt("start y    ", &start.y, 0, pic_height - 1);
				ImGui::SliderInt("end x      ", &end.x,   0, pic_width  - 1);
				ImGui::SliderInt("end y      ", &end.y,   0, pic_height - 1);

				ImGui::Checkbox("draw cost map?", &cost_map);
				ImGui::SameLine();
				if (ImGui::Button("solve")) {
					load_texture_from_file(g_pd3dDevice, file_name.c_str(), &pic_texture, &pic_width, &pic_height);
					tex->binarize_texture();
					if (cost_map) {
						const auto binary_maze = tex->get_texture_as_bool_vector();
						const auto temp = get_cost_map(binary_maze, { pic_width, pic_height }, start, end);
						tex->darken_background();
						tex->draw_points(temp);
					} else {
						const auto binary_maze = tex->get_texture_as_bool_vector();
						unsigned max_path_value;
						const auto temp = get_path(binary_maze, { pic_width, pic_height }, start, end, &max_path_value);
						std::vector<std::tuple<int, int, D3DCOLOR>> points;
						for (auto& point : temp) {
							int r = ((float)std::get<2>(point) / (float)max_path_value) * 255.f;
							points.push_back({ std::get<0>(point), std::get<1>(point), D3DCOLOR_RGBA(0xFF - r, r, 0x00, 0xFF) });
						}
						tex->darken_background();
						tex->draw_points(points);
					}
					solved = true;
				}

				if (solved) {
					ImGui::SameLine();
					if (ImGui::Button("save as jpg")) {
						size_t last_period = file_name.find_last_of(".");
						std::string raw_name = file_name.substr(0, last_period);
						raw_name += "_output.jpg";
						D3DXSaveTextureToFileA(raw_name.c_str(), D3DXIFF_JPG, pic_texture, nullptr);
					}
				}

				ImGui::Separator();

				if (show_whole_image) {
					const auto pos = ImGui::GetWindowPos();
					const auto size = ImGui::GetWindowSize();
					//internal imgui function that lets you specify corners of image (p_min, p_max)
					ImGui::GetCurrentContext()->CurrentWindow->DrawList->AddImage(pic_texture, ImGui::GetCursorScreenPos(), ImVec2(pos.x + size.x - 5, pos.y + size.y - 5));
				} else {
					ImGui::Image(pic_texture, ImVec2(pic_width, pic_height));
				}
			}

			ImGui::End();
		}

		ImGui::EndFrame();
		g_pd3dDevice->SetRenderState(D3DRS_ZENABLE, FALSE);
		g_pd3dDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
		g_pd3dDevice->SetRenderState(D3DRS_SCISSORTESTENABLE, FALSE);
		g_pd3dDevice->Clear(0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, D3DCOLOR_RGBA(0, 0, 0, 0), 1.0f, 0);
		if (g_pd3dDevice->BeginScene() >= 0) {
			ImGui::Render();
			ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());
			g_pd3dDevice->EndScene();
		}
		HRESULT result = g_pd3dDevice->Present(NULL, NULL, NULL, NULL);
		if (result == D3DERR_DEVICELOST && g_pd3dDevice->TestCooperativeLevel() == D3DERR_DEVICENOTRESET) ResetDevice();
	}

	ImGui_ImplDX9_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();

	CleanupDeviceD3D();
	DestroyWindow(hwnd);
	UnregisterClassA(wc.lpszClassName, wc.hInstance);

	return 0;
}
