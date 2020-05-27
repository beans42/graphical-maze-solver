#pragma once
#include "includes.hpp"

auto calculate_path_values(const std::vector<bool>& binary_maze, point_t size, point_t start, point_t end, unsigned* max_path_value, bool* end_reached = nullptr) {
	std::vector<unsigned> path_values(size.x * size.y);
	std::vector<bool>     visited(size.x * size.y);

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
			out.push_back({ x, y, D3DCOLOR_RGBA(color, color, color, 0xFF) });
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
