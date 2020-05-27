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

auto calculate_path_values(const std::vector<bool>& binary_maze, point_t size, point_t start, point_t end, unsigned* max_path_value, bool* end_reached = nullptr) {
	std::vector<unsigned> path_values(size.x * size.y);
	std::vector<bool>     visited    (size.x * size.y);

	std::queue<point_t> current_points;
	current_points.push({ start.x, start.y });
	path_values[start.x + start.y * size.x] = 1;
	visited[start.x + start.y * size.x] = true;

	bool local_end_reached = false;
	unsigned local_max_path_value = 0;
	while (current_points.size() > 0) {
		auto current_point = current_points.front();
		current_points.pop();

		unsigned current_path_value = path_values[current_point.x + current_point.y * size.x];
		if (current_path_value > local_max_path_value)
			local_max_path_value = current_path_value;

		if (current_point.x == end.x && current_point.y == end.y) {
			local_end_reached = true;
			break;
		}

		for (int i = -1; i < 2; i++) {
			for (int j = -1; j < 2; j++) {
				if (current_point.x + j < 0 || current_point.y + i < 0 || current_point.x + j >= size.x || current_point.y + i >= size.y)
					continue;
				if (binary_maze[(current_point.x + j) + (current_point.y + i) * size.x] && !visited[(current_point.x + j) + (current_point.y + i) * size.x]) {
					path_values[(current_point.x + j) + (current_point.y + i) * size.x] = current_path_value + 1;
					visited[(current_point.x + j) + (current_point.y + i) * size.x] = true;
					current_points.push({ current_point.x + j, current_point.y + i });
				}
			}
		}
	}
	
	*max_path_value = local_max_path_value;
	if (end_reached) *end_reached = local_end_reached;
	return path_values;
}

auto get_cost_map(const std::vector<bool>& binary_maze, point_t size, point_t start, point_t end) {
	std::vector<std::tuple<int, int, D3DCOLOR>> out;
	unsigned max_path_value; bool end_reached;
	auto path_values = calculate_path_values(binary_maze, size, start, end, &max_path_value, &end_reached);

	if (!end_reached) {
		MessageBoxA(NULL, "no solution found", "alert!", MB_OK);
		return out;
	}

	double multiplier = 1.f / (double)max_path_value;
	for (int x = 0; x < size.x; x++) {
		for (int y = 0; y < size.y; y++) {
			uint8_t color = 255 * multiplier * path_values[x + y * size.x];
			out.push_back({ x, y, D3DCOLOR_RGBA(color, color, color, 0xFF ) });
		}
	}
	return out;
}

auto get_path(const std::vector<bool>& binary_maze, point_t size, point_t start, point_t end, unsigned* max_path_value = nullptr) {
	std::vector<std::tuple<int, int, unsigned>> out; //tuple = (x coordinate, y coordinate, path value)
	
	unsigned local_max_path_value; bool end_reached;
	auto path_values = calculate_path_values(binary_maze, size, start, end, &local_max_path_value, &end_reached);
	if (max_path_value) *max_path_value = local_max_path_value;

	if (!end_reached) {
		MessageBoxA(NULL, "no solution found", "alert!", MB_OK);
		return out;
	}

	unsigned current_path_value = path_values[end.x + end.y * size.x];
	point_t temp = end;
	while (current_path_value != 1) {
		out.push_back({ temp.x, temp.y, current_path_value });

		point_t best_move = { 0, 0 };
		unsigned best_move_path_value = local_max_path_value;
		for (int i = -1; i < 2; i++) {
			for (int j = -1; j < 2; j++) {
				if (i == 0 && j == 0)
					continue;
				if (temp.x + j < 0 || temp.y + i < 0 || temp.x + j >= size.x || temp.y + i >= size.y)
					continue;
				if (path_values[(temp.x + j) + (temp.y + i) * size.x] == 0)
					continue;
				if (path_values[(temp.x + j) + (temp.y + i) * size.x] < best_move_path_value) {
					best_move_path_value = path_values[(temp.x + j) + (temp.y + i) * size.x];
					best_move = { temp.x + j, temp.y + i };
				}
			}
		}
		temp = best_move;
		current_path_value = path_values[temp.x + temp.y * size.x];
	}

	return out;
}