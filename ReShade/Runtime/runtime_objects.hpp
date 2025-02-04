/**
* Copyright (C) 2014 Patrick Mours. All rights reserved.
* License: https://github.com/crosire/reshade#license
*
* Copyright (C) 2023 Elisha Riedlinger
*
* This software is  provided 'as-is', without any express  or implied  warranty. In no event will the
* authors be held liable for any damages arising from the use of this software.
* Permission  is granted  to anyone  to use  this software  for  any  purpose,  including  commercial
* applications, and to alter it and redistribute it freely, subject to the following restrictions:
*
*   1. The origin of this software must not be misrepresented; you must not claim that you  wrote the
*      original  software. If you use this  software  in a product, an  acknowledgment in the product
*      documentation would be appreciated but is not required.
*   2. Altered source versions must  be plainly  marked as such, and  must not be  misrepresented  as
*      being the original software.
*   3. This notice may not be removed or altered from any source distribution.
*/

#pragma once

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include "External\reshade\source\effect_module.hpp"
#include <filesystem>

namespace reshade
{
	enum class special_uniform
	{
		none,
		frame_time,
		frame_count,
		random,
		ping_pong,
		date,
		timer,
		key,
		mouse_point,
		mouse_delta,
		mouse_button,
		mouse_wheel,
		freepie,
		overlay_open,
		bufready_depth,
	};

	enum class texture_reference
	{
		none,
		back_buffer,
		depth_buffer
	};

	template <typename T, size_t SAMPLES>
	class moving_average
	{
	public:
		moving_average() : _index(0), _average(0), _tick_sum(0), _tick_list() {}

		inline operator T() const { return _average; }

		void clear()
		{
			_index = 0;
			_average = 0;
			_tick_sum = 0;

			for (size_t i = 0; i < SAMPLES; i++)
				_tick_list[i] = 0;
		}
		void append(T value)
		{
			_tick_sum -= _tick_list[_index];
			_tick_sum += _tick_list[_index] = value;
			_index = ++_index % SAMPLES;
			_average = _tick_sum / SAMPLES;
		}

	private:
		size_t _index;
		T _average, _tick_sum, _tick_list[SAMPLES];
	};

	struct texture final : reshadefx::texture_info
	{
		texture() {} // For standalone textures like the font atlas
		texture(const reshadefx::texture_info &init) : texture_info(init) {}

		auto annotation_as_int(const char *ann_name, size_t i = 0) const
		{
			const auto it = std::find_if(annotations.begin(), annotations.end(),
				[ann_name](const auto &annotation) { return annotation.name == ann_name; });
			if (it == annotations.end()) return 0;
			return it->type.is_integral() ? it->value.as_int[i] : static_cast<int>(it->value.as_float[i]);
		}
		auto annotation_as_float(const char *ann_name, size_t i = 0) const
		{
			const auto it = std::find_if(annotations.begin(), annotations.end(),
				[ann_name](const auto &annotation) { return annotation.name == ann_name; });
			if (it == annotations.end()) return 0.0f;
			return it->type.is_floating_point() ? it->value.as_float[i] : static_cast<float>(it->value.as_int[i]);
		}
		auto annotation_as_string(const char *ann_name) const
		{
			const auto it = std::find_if(annotations.begin(), annotations.end(),
				[ann_name](const auto &annotation) { return annotation.name == ann_name; });
			if (it == annotations.end()) return std::string_view();
			return std::string_view(it->value.string_data);
		}

		bool matches_description(const reshadefx::texture_info &desc) const
		{
			return width == desc.width && height == desc.height && levels == desc.levels && format == desc.format;
		}

		void *impl = nullptr;
		size_t effect_index = std::numeric_limits<size_t>::max();
		texture_reference impl_reference = texture_reference::none;
		std::vector<size_t> shared;
		bool loaded = false;
	};

	struct uniform final : reshadefx::uniform_info
	{
		uniform(const reshadefx::uniform_info &init) : uniform_info(init) {}

		auto annotation_as_int(const char *ann_name, size_t i = 0, int default_value = 0) const
		{
			const auto it = std::find_if(annotations.begin(), annotations.end(),
				[ann_name](const auto &annotation) { return annotation.name == ann_name; });
			if (it == annotations.end()) return default_value;
			return it->type.is_integral() ? it->value.as_int[i] : static_cast<int>(it->value.as_float[i]);
		}
		auto annotation_as_float(const char *ann_name, size_t i = 0, float default_value = 0.0f) const
		{
			const auto it = std::find_if(annotations.begin(), annotations.end(),
				[ann_name](const auto &annotation) { return annotation.name == ann_name; });
			if (it == annotations.end()) return default_value;
			return it->type.is_floating_point() ? it->value.as_float[i] : static_cast<float>(it->value.as_int[i]);
		}
		auto annotation_as_string(const char *ann_name, const std::string_view &default_value = std::string_view()) const
		{
			const auto it = std::find_if(annotations.begin(), annotations.end(),
				[ann_name](const auto &annotation) { return annotation.name == ann_name; });
			if (it == annotations.end()) return default_value;
			return std::string_view(it->value.string_data);
		}

		bool supports_toggle_key() const
		{
			if (type.base == reshadefx::type::t_bool)
				return true;
			if (type.base != reshadefx::type::t_int && type.base != reshadefx::type::t_uint)
				return false;
			const std::string_view ui_type = annotation_as_string("ui_type");
			return ui_type == "list" || ui_type == "combo" || ui_type == "radio";
		}

		size_t effect_index = std::numeric_limits<size_t>::max();
		special_uniform special = special_uniform::none;
		uint32_t toggle_key_data[4] = {};
	};

	struct technique final : reshadefx::technique_info
	{
		technique(const reshadefx::technique_info &init) : technique_info(init) {}

		auto annotation_as_int(const char *ann_name, size_t i = 0) const
		{
			const auto it = std::find_if(annotations.begin(), annotations.end(),
				[ann_name](const auto &annotation) { return annotation.name == ann_name; });
			if (it == annotations.end()) return 0;
			return it->type.is_integral() ? it->value.as_int[i] : static_cast<int>(it->value.as_float[i]);
		}
		auto annotation_as_float(const char *ann_name, size_t i = 0) const
		{
			const auto it = std::find_if(annotations.begin(), annotations.end(),
				[ann_name](const auto &annotation) { return annotation.name == ann_name; });
			if (it == annotations.end()) return 0.0f;
			return it->type.is_floating_point() ? it->value.as_float[i] : static_cast<float>(it->value.as_int[i]);
		}
		auto annotation_as_string(const char *ann_name) const
		{
			const auto it = std::find_if(annotations.begin(), annotations.end(),
				[ann_name](const auto &annotation) { return annotation.name == ann_name; });
			if (it == annotations.end()) return std::string_view();
			return std::string_view(it->value.string_data);
		}

		void *impl = nullptr;
		size_t effect_index = std::numeric_limits<size_t>::max();
		bool hidden = false;
		bool enabled = false;
		int64_t time_left = 0;
		uint32_t toggle_key_data[4] = {};
		moving_average<uint64_t, 60> average_cpu_duration;
		moving_average<uint64_t, 60> average_gpu_duration;
	};

	struct effect final
	{
		unsigned int rendering = 0;
		bool skipped = false;
		bool compiled = false;
		std::string errors;
		std::string preamble;
		reshadefx::module module;
		std::filesystem::path source_file;
		std::vector<std::filesystem::path> included_files;
		std::vector<std::pair<std::string, std::string>> definitions;
		std::unordered_map<std::string, std::string> assembly;
		std::vector<uniform> uniforms;
		std::vector<unsigned char> uniform_data_storage;
	};
}
