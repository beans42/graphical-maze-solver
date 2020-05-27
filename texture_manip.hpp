#pragma once
#include "includes.hpp"

constexpr int pixel_size_in_bytes = 4; //TODO: add proper method for getting this from the texture's D3DFORMAT
constexpr D3DCOLOR white = D3DCOLOR_RGBA(0xFF, 0xFF, 0xFF, 0xFF);
constexpr D3DCOLOR black = D3DCOLOR_RGBA(0x00, 0x00, 0x00, 0xFF);
constexpr D3DCOLOR gray  = D3DCOLOR_RGBA(0x80, 0x80, 0x80, 0xFF);
constexpr D3DCOLOR red   = D3DCOLOR_RGBA(0xFF, 0x00, 0x00, 0xFF);
constexpr D3DCOLOR green = D3DCOLOR_RGBA(0x00, 0xFF, 0x00, 0xFF);
constexpr D3DCOLOR blue  = D3DCOLOR_RGBA(0x00, 0x00, 0xFF, 0xFF);

struct texture_manip { //class for manipulating d3d9 textures
	IDirect3DTexture9** m_texture_pointer = nullptr;
	IDirect3DSurface9* m_surface = nullptr;
	D3DSURFACE_DESC m_desc{};
	D3DLOCKED_RECT m_locked{};
	
	texture_manip(IDirect3DTexture9** texture) {
		m_texture_pointer = texture;
		(*m_texture_pointer)->GetSurfaceLevel(0, &m_surface);
		(*m_texture_pointer)->GetLevelDesc(0, &m_desc);
	}	 

	void lock_texture(const bool read_only = true) {
		(*m_texture_pointer)->GetSurfaceLevel(0, &m_surface);
		(*m_texture_pointer)->GetLevelDesc(0, &m_desc);
		m_surface->LockRect(&m_locked, 0, read_only ? D3DLOCK_READONLY : NULL);
	}	
	
	void unlock_texture() {
		m_surface->UnlockRect();
		m_surface->Release();
	}

	D3DCOLOR* get_pixel_ptr(const point_t point) {
		return (D3DCOLOR*)((uintptr_t)m_locked.pBits + point.y * m_locked.Pitch + point.x * pixel_size_in_bytes);
	}

	D3DCOLOR get_pixel(const point_t point) {
		return *get_pixel_ptr(point);
	}

	void set_pixel(const point_t point, const D3DCOLOR colour) {
		auto ptr = get_pixel_ptr(point);
		*ptr = colour;
	}

	uint8_t get_luminance(const point_t point) {
		auto cols = (uint8_t*)get_pixel_ptr(point);
		return 0.2126f * cols[1] + 0.7152f * cols[2] + 0.0722f * cols[3]; //relative luminance = 0.2126R + 0.7152G + 0.0722B
	}
	
	void binarize_texture(const int threshold = 200) {
		lock_texture(false);
		if (m_desc.Format == D3DFMT_A8R8G8B8 || m_desc.Format == D3DFMT_X8R8G8B8) {
			for (int x = 0; x < m_desc.Width; x++)
				for (int y = 0; y < m_desc.Height; y++)
					get_luminance({ x, y }) > threshold ? set_pixel({ x, y }, white) : set_pixel({ x, y }, black);
		} else MessageBoxA(NULL, ("Please create a new issue and include this error and file in your post\nFormat: " + std::to_string(m_desc.Format)).c_str(), "Unsupported pixel format!", MB_OK);
		unlock_texture();
	}

	void darken_background() {
		lock_texture(false);
		for (int y = 0; y < m_desc.Height; y++)
			for (int x = 0; x < m_desc.Width; x++)
				if (get_pixel({ x, y }) == white) set_pixel({ x, y }, gray);
		unlock_texture();
	}

	auto get_texture_as_bool_vector() {
		std::vector<bool> out;
		lock_texture();
		for (int y = 0; y < m_desc.Height; y++)
			for (int x = 0; x < m_desc.Width; x++)
				out.push_back(get_pixel({ x, y }) == white);
		unlock_texture();
		return out;
	}

	void draw_points(std::vector<std::tuple<int, int, D3DCOLOR>> pixels) {
		lock_texture(false);
		for (auto &pixel : pixels)
			set_pixel({ std::get<0>(pixel), std::get<1>(pixel) }, std::get<2>(pixel));
		unlock_texture();
	}

	void draw_markers(const point_t start, const point_t end, int marker_size) {
		static std::vector<std::tuple<int, int, D3DCOLOR>> storage;
		lock_texture(false);
		for (auto& pix : storage)
			set_pixel({ std::get<0>(pix), std::get<1>(pix) }, std::get<2>(pix)); //put back the overwritten pixels
		storage.clear();
		for (int y = 0; y < marker_size; y++) {
			for (int x = 0; x < marker_size; x++) {
				storage.push_back({ start.x + x, start.y + y, get_pixel({start.x + x, start.y + y}) }); //save pixels overwritten by start marker
				set_pixel({ start.x + x, start.y + y }, red); //draw start marker
				storage.push_back({ end.x + x, end.y + y, get_pixel({end.x + x, end.y + y}) }); //save pixels overwritten by end marker
				set_pixel({ end.x + x, end.y + y }, green); //draw end marker
			}
		}
		unlock_texture();
	}
};
