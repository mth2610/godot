/*************************************************************************/
/*  particles_material.cpp                                               */
/*************************************************************************/
/*                       This file is part of:                           */
/*                           GODOT ENGINE                                */
/*                      https://godotengine.org                          */
/*************************************************************************/
/* Copyright (c) 2007-2018 Juan Linietsky, Ariel Manzur.                 */
/* Copyright (c) 2014-2018 Godot Engine contributors (cf. AUTHORS.md)    */
/*                                                                       */
/* Permission is hereby granted, free of charge, to any person obtaining */
/* a copy of this software and associated documentation files (the       */
/* "Software"), to deal in the Software without restriction, including   */
/* without limitation the rights to use, copy, modify, merge, publish,   */
/* distribute, sublicense, and/or sell copies of the Software, and to    */
/* permit persons to whom the Software is furnished to do so, subject to */
/* the following conditions:                                             */
/*                                                                       */
/* The above copyright notice and this permission notice shall be        */
/* included in all copies or substantial portions of the Software.       */
/*                                                                       */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,       */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF    */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.*/
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY  */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,  */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE     */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                */
/*************************************************************************/

#include "particles_material.h"

Mutex *ParticlesMaterial::material_mutex = NULL;
SelfList<ParticlesMaterial>::List ParticlesMaterial::dirty_materials;
Map<ParticlesMaterial::MaterialKey, ParticlesMaterial::ShaderData> ParticlesMaterial::shader_map;
ParticlesMaterial::ShaderNames *ParticlesMaterial::shader_names = NULL;

void ParticlesMaterial::init_shaders() {

#ifndef NO_THREADS
	material_mutex = Mutex::create();
#endif

	shader_names = memnew(ShaderNames);

	shader_names->spread = "spread";
	shader_names->flatness = "flatness";
	shader_names->initial_linear_velocity = "initial_linear_velocity";
	shader_names->initial_angle = "initial_angle";
	shader_names->angular_velocity = "angular_velocity";
	shader_names->orbit_velocity = "orbit_velocity";
	shader_names->linear_accel = "linear_accel";
	shader_names->radial_accel = "radial_accel";
	shader_names->tangent_accel = "tangent_accel";
	shader_names->damping = "damping";
	shader_names->scale = "scale";
	shader_names->hue_variation = "hue_variation";
	shader_names->anim_speed = "anim_speed";
	shader_names->anim_offset = "anim_offset";

	shader_names->initial_linear_velocity_random = "initial_linear_velocity_random";
	shader_names->initial_angle_random = "initial_angle_random";
	shader_names->angular_velocity_random = "angular_velocity_random";
	shader_names->orbit_velocity_random = "orbit_velocity_random";
	shader_names->linear_accel_random = "linear_accel_random";
	shader_names->radial_accel_random = "radial_accel_random";
	shader_names->tangent_accel_random = "tangent_accel_random";
	shader_names->damping_random = "damping_random";
	shader_names->scale_random = "scale_random";
	shader_names->hue_variation_random = "hue_variation_random";
	shader_names->anim_speed_random = "anim_speed_random";
	shader_names->anim_offset_random = "anim_offset_random";

	shader_names->angle_texture = "angle_texture";
	shader_names->angular_velocity_texture = "angular_velocity_texture";
	shader_names->orbit_velocity_texture = "orbit_velocity_texture";
	shader_names->linear_accel_texture = "linear_accel_texture";
	shader_names->radial_accel_texture = "radial_accel_texture";
	shader_names->tangent_accel_texture = "tangent_accel_texture";
	shader_names->damping_texture = "damping_texture";
	shader_names->scale_texture = "scale_texture";
	shader_names->hue_variation_texture = "hue_variation_texture";
	shader_names->anim_speed_texture = "anim_speed_texture";
	shader_names->anim_offset_texture = "anim_offset_texture";

	shader_names->color = "color_value";
	shader_names->color_ramp = "color_ramp";

	shader_names->emission_sphere_radius = "emission_sphere_radius";
	shader_names->emission_box_extents = "emission_box_extents";
	shader_names->emission_texture_point_count = "emission_texture_point_count";
	shader_names->emission_texture_points = "emission_texture_points";
	shader_names->emission_texture_normal = "emission_texture_normal";
	shader_names->emission_texture_color = "emission_texture_color";

	shader_names->trail_divisor = "trail_divisor";
	shader_names->trail_size_modifier = "trail_size_modifier";
	shader_names->trail_color_modifier = "trail_color_modifier";

	shader_names->gravity = "gravity";

	shader_names->noise_scale = "noise_scale";
	shader_names->noise_strength = "noise_strength";
	shader_names->noise_timefactor = "noise_timefactor";
}

void ParticlesMaterial::finish_shaders() {

#ifndef NO_THREADS
	memdelete(material_mutex);
#endif

	memdelete(shader_names);
}

void ParticlesMaterial::_update_shader() {

	dirty_materials.remove(&element);

	MaterialKey mk = _compute_key();
	if (mk.key == current_key.key)
		return; //no update required in the end

	if (shader_map.has(current_key)) {
		shader_map[current_key].users--;
		if (shader_map[current_key].users == 0) {
			//deallocate shader, as it's no longer in use
			VS::get_singleton()->free(shader_map[current_key].shader);
			shader_map.erase(current_key);
		}
	}

	current_key = mk;

	if (shader_map.has(mk)) {

		VS::get_singleton()->material_set_shader(_get_material(), shader_map[mk].shader);
		shader_map[mk].users++;
		return;
	}

	//must create a shader!

	String code = "shader_type particles;\n";

	code += "uniform float spread;\n";
	code += "uniform float flatness;\n";
	code += "uniform float initial_linear_velocity;\n";
	code += "uniform float initial_angle;\n";
	code += "uniform float angular_velocity;\n";
	code += "uniform float orbit_velocity;\n";
	code += "uniform float linear_accel;\n";
	code += "uniform float radial_accel;\n";
	code += "uniform float tangent_accel;\n";
	code += "uniform float damping;\n";
	code += "uniform float scale;\n";
	code += "uniform float hue_variation;\n";
	code += "uniform float anim_speed;\n";
	code += "uniform float anim_offset;\n";

	code += "uniform float initial_linear_velocity_random;\n";
	code += "uniform float initial_angle_random;\n";
	code += "uniform float angular_velocity_random;\n";
	code += "uniform float orbit_velocity_random;\n";
	code += "uniform float linear_accel_random;\n";
	code += "uniform float radial_accel_random;\n";
	code += "uniform float tangent_accel_random;\n";
	code += "uniform float damping_random;\n";
	code += "uniform float scale_random;\n";
	code += "uniform float hue_variation_random;\n";
	code += "uniform float anim_speed_random;\n";
	code += "uniform float anim_offset_random;\n";

	code += "uniform float noise_scale;\n";
	code += "uniform float noise_strength;\n";
	code += "uniform float noise_timefactor;\n";

	switch (emission_shape) {
		case EMISSION_SHAPE_POINT: {
			//do none
		} break;
		case EMISSION_SHAPE_SPHERE: {
			code += "uniform float emission_sphere_radius;\n";
		} break;
		case EMISSION_SHAPE_BOX: {
			code += "uniform vec3 emission_box_extents;\n";
		} break;
		case EMISSION_SHAPE_DIRECTED_POINTS: {
			code += "uniform sampler2D emission_texture_normal : hint_black;\n";
		} //fallthrough
		case EMISSION_SHAPE_POINTS: {
			code += "uniform sampler2D emission_texture_points : hint_black;\n";
			code += "uniform int emission_texture_point_count;\n";
			if (emission_color_texture.is_valid()) {
				code += "uniform sampler2D emission_texture_color : hint_white;\n";
			}
		} break;
	}

	code += "uniform vec4 color_value : hint_color;\n";

	code += "uniform int trail_divisor;\n";

	code += "uniform vec3 gravity;\n";

	if (color_ramp.is_valid())
		code += "uniform sampler2D color_ramp;\n";

	if (tex_parameters[PARAM_INITIAL_LINEAR_VELOCITY].is_valid())
		code += "uniform sampler2D linear_velocity_texture;\n";
	if (tex_parameters[PARAM_ORBIT_VELOCITY].is_valid())
		code += "uniform sampler2D orbit_velocity_texture;\n";
	if (tex_parameters[PARAM_ANGULAR_VELOCITY].is_valid())
		code += "uniform sampler2D angular_velocity_texture;\n";
	if (tex_parameters[PARAM_LINEAR_ACCEL].is_valid())
		code += "uniform sampler2D linear_accel_texture;\n";
	if (tex_parameters[PARAM_RADIAL_ACCEL].is_valid())
		code += "uniform sampler2D radial_accel_texture;\n";
	if (tex_parameters[PARAM_TANGENTIAL_ACCEL].is_valid())
		code += "uniform sampler2D tangent_accel_texture;\n";
	if (tex_parameters[PARAM_DAMPING].is_valid())
		code += "uniform sampler2D damping_texture;\n";
	if (tex_parameters[PARAM_ANGLE].is_valid())
		code += "uniform sampler2D angle_texture;\n";
	if (tex_parameters[PARAM_SCALE].is_valid())
		code += "uniform sampler2D scale_texture;\n";
	if (tex_parameters[PARAM_HUE_VARIATION].is_valid())
		code += "uniform sampler2D hue_variation_texture;\n";
	if (tex_parameters[PARAM_ANIM_SPEED].is_valid())
		code += "uniform sampler2D anim_speed_texture;\n";
	if (tex_parameters[PARAM_ANIM_OFFSET].is_valid())
		code += "uniform sampler2D anim_offset_texture;\n";

	if (trail_size_modifier.is_valid()) {
		code += "uniform sampler2D trail_size_modifier;\n";
	}

	if (trail_color_modifier.is_valid()) {
		code += "uniform sampler2D trail_color_modifier;\n";
	}

	//need a random function
	code += "\n\n";
	code += "float rand_from_seed(inout uint seed) {\n";
	code += "	int k;\n";
	code += "	int s = int(seed);\n";
	code += "	if (s == 0)\n";
	code += "	s = 305420679;\n";
	code += "	k = s / 127773;\n";
	code += "	s = 16807 * (s - k * 127773) - 2836 * k;\n";
	code += "	if (s < 0)\n";
	code += "		s += 2147483647;\n";
	code += "	seed = uint(s);\n";
	code += "	return float(seed % uint(65536)) / 65535.0;\n";
	code += "}\n";
	code += "\n";

	code += "float rand_from_seed_m1_p1(inout uint seed) {\n";
	code += "	return rand_from_seed(seed) * 2.0 - 1.0;\n";
	code += "}\n";
	code += "\n";

	//improve seed quality
	code += "uint hash(uint x) {\n";
	code += "	x = ((x >> uint(16)) ^ x) * uint(73244475);\n";
	code += "	x = ((x >> uint(16)) ^ x) * uint(73244475);\n";
	code += "	x = (x >> uint(16)) ^ x;\n";
	code += "	return x;\n";
	code += "}\n";
	code += "\n";

	//functions to create simplex noise, curl noise in 3D and 4D
	code += "vec4 permute(vec4 x) {\n";
	code += "	return mod(((x * 34.0) + 1.0) * x, 289.0);\n";
	code += "}\n";
	code += "\n";

	code += "vec4 taylor_inv_sqrt(vec4 r) {\n";
	code += "	return 1.79284291400159 - 0.85373472095314 * r;\n";
	code += "}\n";
	code += "\n";

	code += "\n\n";
	code += "float permute_float(float x) {\n";
	code += "	return floor(mod(((x * 34.0) + 1.0) * x, 289.0));\n";
	code += "}\n";
	code += "\n";

	code += "float taylor_inv_sqrt_float(float r) {\n";
	code += "	return 1.79284291400159 - 0.85373472095314 * r;\n";
	code += "}\n";
	code += "\n";

	code += "float snoise_3d(vec3 v) {\n";
	code += "	vec2 C = vec2(1.0 / 6.0, 1.0 / 3.0) ;\n";
	code += "	vec4 D = vec4(0.0, 0.5, 1.0, 2.0);\n";
	code += "	vec3 i = floor(v + dot(v, vec3(C.y)) );\n";
	code += "	vec3 x0 = v - i + dot(i, vec3(C.x)) ;\n";
	code += "	vec3 g = step(x0.yzx, x0.xyz);\n";
	code += "	vec3 l = 1.0 - g;\n";
	code += "	vec3 i1 = min(g.xyz, l.zxy);\n";
	code += "	vec3 i2 = max(g.xyz, l.zxy);\n";
	code += "	vec3 x1 = x0 - i1 + vec3(C.x);\n";
	code += "	vec3 x2 = x0 - i2 + vec3(C.y);\n";
	code += "	vec3 x3 = x0 - vec3(D.y);\n";
	code += "	i = mod(i, 289.0);\n";
	code += "	vec4 p = permute(permute(permute(\n";
	code += "			i.z + vec4(0.0, i1.z, i2.z, 1.0 ))\n";
	code += "			+ i.y + vec4(0.0, i1.y, i2.y, 1.0 ))\n";
	code += "			+ i.x + vec4(0.0, i1.x, i2.x, 1.0 ));\n";
	code += "	float n_ = 0.142857142857;\n";
	code += "	vec3 ns = n_ * D.wyz - D.xzx;\n";
	code += "	vec4 j = p - 49.0 * floor(p * ns.z * ns.z);\n";
	code += "	vec4 x_ = floor(j * ns.z);\n";
	code += "	vec4 y_ = floor(j - 7.0 * x_ );\n";
	code += "	vec4 x = x_ * ns.x + vec4(ns.y);\n";
	code += "	vec4 y = y_ * ns.x + vec4(ns.y);\n";
	code += "	vec4 h = 1.0 - abs(x) - abs(y);\n";
	code += "	vec4 b0 = vec4(x.xy, y.xy);\n";
	code += "	vec4 b1 = vec4(x.zw, y.zw);\n";
	code += "	vec4 s0 = floor(b0) * 2.0 + 1.0;\n";
	code += "	vec4 s1 = floor(b1) * 2.0 + 1.0;\n";
	code += "	vec4 sh = -step(h, vec4(0.0));\n";
	code += "	vec4 a0 = b0.xzyw + s0.xzyw * sh.xxyy ;\n";
	code += "	vec4 a1 = b1.xzyw + s1.xzyw * sh.zzww ;\n";
	code += "	vec3 p0 = vec3(a0.xy, h.x);\n";
	code += "	vec3 p1 = vec3(a0.zw, h.y);\n";
	code += "	vec3 p2 = vec3(a1.xy, h.z);\n";
	code += "	vec3 p3 = vec3(a1.zw, h.w);\n";
	code += "	vec4 norm = taylor_inv_sqrt(vec4(dot(p0, p0), dot(p1, p1), dot(p2, p2), dot(p3, p3)));\n";
	code += "	p0 *= norm.x;\n";
	code += "	p1 *= norm.y;\n";
	code += "	p2 *= norm.z;\n";
	code += "	p3 *= norm.w;\n";
	code += "	vec4 m = max(0.6 - vec4(dot(x0, x0), dot(x1, x1), dot(x2, x2), dot(x3, x3)), vec4(0.0));\n";
	code += "	m = m * m;\n";
	code += "	return 42.0 * dot(m * m, vec4(dot(p0, x0), dot(p1, x1), dot(p2, x2), dot(p3, x3)));\n";
	code += " }\n";
	code += "\n";

	code += "vec4 grad4(float j, vec4 ip) {\n";
	code += "	vec4 ones = vec4(1.0, 1.0, 1.0, -1.0);\n";
	code += "	vec4 p,s;\n";
	code += "	p.xyz = floor(fract(vec3(j) * ip.xyz) * 7.0) * ip.z - 1.0;\n";
	code += "	p.w = 1.5 - dot(abs(p.xyz), ones.xyz);\n";
	code += "	s = vec4(lessThan(p, vec4(0.0)));\n";
	code += "	p.xyz = p.xyz + (s.xyz * 2.0 - 1.0) * s.www; \n";
	code += "	return p;\n";
	code += "}\n";
	code += "\n";

	code += "float snoise_4d(vec4 v) {\n";
	code += "	vec2 C = vec2(0.138196601125010504, 0.309016994374947451);\n";
	code += "	vec4 i = floor(v + dot(v, vec4(C.y)));\n";
	code += "	vec4 x0 = v - i + dot(i, vec4(C.x));\n";
	code += "	vec4 i0;\n";
	code += "	vec3 isX = step(x0.yzw, x0.xxx);\n";
	code += "	vec3 isYZ = step(x0.zww, x0.yyz);\n";
	code += "	i0.x = isX.x + isX.y + isX.z;\n";
	code += "	i0.yzw = 1.0 - isX;\n";
	code += "	i0.y += isYZ.x + isYZ.y;\n";
	code += "	i0.zw += 1.0 - isYZ.xy;\n";
	code += "	i0.z += isYZ.z;\n";
	code += "	i0.w += 1.0 - isYZ.z;\n";
	code += "	vec4 i3 = clamp(i0, 0.0, 1.0);\n";
	code += "	vec4 i2 = clamp(i0-1.0, 0.0, 1.0);\n";
	code += "	vec4 i1 = clamp(i0-2.0, 0.0, 1.0);\n";
	code += "	vec4 x1 = x0 - i1 + 1.0 * vec4(C.y);\n";
	code += "	vec4 x2 = x0 - i2 + 2.0 * vec4(C.y);\n";
	code += "	vec4 x3 = x0 - i3 + 3.0 * vec4(C.y);\n";
	code += "	vec4 x4 = x0 - 1.0 + 4.0 * vec4(C.y);\n";
	code += "	i = mod(i, 289.0);\n";
	code += "	float j0 = permute_float(permute_float(permute_float(permute_float(i.w) + i.z) + i.y) + i.x);\n";
	code += "	vec4 j1 = permute( permute( permute( permute (\n";
	code += "			i.w + vec4(i1.w, i2.w, i3.w, 1.0 ))\n";
	code += "			+ i.z + vec4(i1.z, i2.z, i3.z, 1.0 ))\n";
	code += "			+ i.y + vec4(i1.y, i2.y, i3.y, 1.0 ))\n";
	code += "			+ i.x + vec4(i1.x, i2.x, i3.x, 1.0 ));\n";
	code += "	vec4 ip = vec4(1.0/294.0, 1.0/49.0, 1.0/7.0, 0.0) ;\n";
	code += "	vec4 p0 = grad4(j0, ip);\n";
	code += "	vec4 p1 = grad4(j1.x, ip);\n";
	code += "	vec4 p2 = grad4(j1.y, ip);\n";
	code += "	vec4 p3 = grad4(j1.z, ip);\n";
	code += "	vec4 p4 = grad4(j1.w, ip);\n";
	code += "	vec4 norm = taylor_inv_sqrt(vec4(dot(p0, p0), dot(p1, p1), dot(p2, p2), dot(p3, p3)));\n";
	code += "	p0 *= norm.x;\n";
	code += "	p1 *= norm.y;\n";
	code += "	p2 *= norm.z;\n";
	code += "	p3 *= norm.w;\n";
	code += "	p4 *= taylor_inv_sqrt_float(dot(p4, p4));\n";
	code += "	vec3 m0 = max(0.6 - vec3(dot(x0, x0), dot(x1, x1), dot(x2, x2)), vec3(0.0));\n";
	code += "	vec2 m1 = max(0.6 - vec2(dot(x3, x3), dot(x4, x4)), vec2(0.0));\n";
	code += "	m0 = m0 * m0;\n";
	code += "	m1 = m1 * m1;\n";
	code += "	return 49.0 * (dot(m0 * m0, vec3( dot(p0, x0), dot(p1, x1), dot(p2, x2)))\n";
	code += "			+ dot(m1 * m1, vec2(dot(p3, x3), dot(p4, x4)))) ;\n";
	code += "}\n";
	code += "\n";

	code += "vec3 snoise_vec3(vec3 p) {\n";
	code += "	float s = snoise_3d(p);\n";
	code += "	float s1 = snoise_3d(vec3(p.y - 19.1, p.z + 33.4, p.x + 47.2));\n";
	code += "	float s2 = snoise_3d(vec3(p.z + 74.2, p.x - 124.5, p.y + 99.4));\n";
	code += "	vec3 c = vec3(s, s1, s2);\n";
	code += "	return c;\n";
	code += "}\n";
	code += "\n";

	code += "\n";
	code += "vec3 snoise_vec4(vec4 p) {\n";
	code += "	float s = snoise_4d(p);\n";
	code += "	float s1 = snoise_4d(vec4(p.xyz + vec3(123.4, 129845.6, -1239.1), p.w));\n";
	code += "	float s2 = snoise_4d(vec4(p.xyz + vec3(-9519.0, 9051.0, -123.0), p.w));\n";
	code += "	vec3 c = vec3(s, s1, s2);\n";
	code += "	return c;\n";
	code += "}\n";
	code += "\n";

	code += "vec3 curl_3d(vec3 p ) {\n";
	code += "	float epsilon = 0.001;\n";
	code += "	vec3 dx = vec3(epsilon, 0.0, 0.0);\n";
	code += "	vec3 dy = vec3(0.0, epsilon, 0.0);\n";
	code += "	vec3 dz = vec3(0.0, 0.0, epsilon);\n";
	code += "	vec3 x0 = snoise_vec3(p - dx).xyz;\n";
	code += "	vec3 x1 = snoise_vec3(p + dx).xyz;\n";
	code += "	vec3 y0 = snoise_vec3(p - dy).xyz;\n";
	code += "	vec3 y1 = snoise_vec3(p + dy).xyz;\n";
	code += "	vec3 z0 = snoise_vec3(p - dz).xyz;\n";
	code += "	vec3 z1 = snoise_vec3(p + dz).xyz;\n";
	code += "	float x = y1.z - y0.z - z1.y + z0.y;\n";
	code += "	float y = z1.x - z0.x - x1.z + x0.z;\n";
	code += "	float z = x1.y - x0.y - y1.x + y0.x;\n";
	code += "	float divisor = 1.0 / (2.0 * epsilon);\n";
	code += "	return vec3(normalize(vec3(x, y, z) * divisor));\n";
	code += "}\n";
	code += "\n";

	code += "vec3 curl_4d(vec4 p ) {\n";
	code += "	float epsilon = 0.001;\n";
	code += "	vec4 dx = vec4(epsilon, 0.0, 0.0, 0.0);\n";
	code += "	vec4 dy = vec4(0.0, epsilon, 0.0, 0.0);\n";
	code += "	vec4 dz = vec4(0.0, 0.0, epsilon, 0.0);\n";
	code += "	vec4 dw = vec4(0.0, 0.0, 0.0, epsilon);\n";
	code += "	vec3 x0 = snoise_vec4(p - dx).xyz;\n";
	code += "	vec3 x1 = snoise_vec4(p + dx).xyz;\n";
	code += "	vec3 y0 = snoise_vec4(p - dy).xyz;\n";
	code += "	vec3 y1 = snoise_vec4(p + dy).xyz;\n";
	code += "	vec3 z0 = snoise_vec4(p - dz).xyz;\n";
	code += "	vec3 z1 = snoise_vec4(p + dz).xyz;\n";
	code += "	float x = y1.z - y0.z - z1.y + z0.y;\n";
	code += "	float y = z1.x - z0.x - x1.z + x0.z;\n";
	code += "	float z = x1.y - x0.y - y1.x + y0.x;\n";
	code += "	float divisor = 1.0 / (2.0 * epsilon);\n";
	code += "	return vec3(normalize(vec3(x, y, z) * divisor));\n";
	code += "}\n";
	code += "\n";

	code += "\n\n";
	code += "void vertex() {\n";
	code += "	uint base_number = NUMBER / uint(trail_divisor);\n";
	code += "	uint alt_seed = hash(base_number + uint(1) + RANDOM_SEED);\n";
	code += "	float angle_rand = rand_from_seed(alt_seed);\n";
	code += "	float scale_rand = rand_from_seed(alt_seed);\n";
	code += "	float hue_rot_rand = rand_from_seed(alt_seed);\n";
	code += "	float anim_offset_rand = rand_from_seed(alt_seed);\n";
	code += "	float pi = 3.14159;\n";
	code += "	float degree_to_rad = pi / 180.0;\n";
	code += "\n";

	if (emission_shape >= EMISSION_SHAPE_POINTS) {
		code += "	int point = min(emission_texture_point_count - 1, int(rand_from_seed(alt_seed) * float(emission_texture_point_count)));\n";
		code += "	ivec2 emission_tex_size = textureSize(emission_texture_points, 0);\n";
		code += "	ivec2 emission_tex_ofs = ivec2(point % emission_tex_size.x, point / emission_tex_size.x);\n";
	}
	code += "	if (RESTART) {\n";

	if (tex_parameters[PARAM_INITIAL_LINEAR_VELOCITY].is_valid())
		code += "		float tex_linear_velocity = textureLod(linear_velocity_texture, vec2(0.0, 0.0), 0.0).r;\n";
	else
		code += "		float tex_linear_velocity = 0.0;\n";

	if (tex_parameters[PARAM_ANGLE].is_valid())
		code += "		float tex_angle = textureLod(angle_texture, vec2(0.0, 0.0), 0.0).r;\n";
	else
		code += "		float tex_angle = 0.0;\n";

	if (tex_parameters[PARAM_ANIM_OFFSET].is_valid())
		code += "		float tex_anim_offset = textureLod(anim_offset_texture, vec2(0.0, 0.0), 0.0).r;\n";
	else
		code += "		float tex_anim_offset = 0.0;\n";

	code += "		float spread_rad = spread * degree_to_rad;\n";

	if (flags[FLAG_DISABLE_Z]) {

		code += "		float angle1_rad = rand_from_seed_m1_p1(alt_seed) * spread_rad;\n";
		code += "		vec3 rot = vec3(cos(angle1_rad), sin(angle1_rad), 0.0);\n";
		code += "		VELOCITY = rot * initial_linear_velocity * mix(1.0, rand_from_seed(alt_seed), initial_linear_velocity_random);\n";

	} else {
		//initiate velocity spread in 3D
		code += "		float angle1_rad = rand_from_seed_m1_p1(alt_seed) * spread_rad;\n";
		code += "		float angle2_rad = rand_from_seed_m1_p1(alt_seed) * spread_rad * (1.0 - flatness);\n";
		code += "		vec3 direction_xz = vec3(sin(angle1_rad), 0, cos(angle1_rad));\n";
		code += "		vec3 direction_yz = vec3(0, sin(angle2_rad), cos(angle2_rad));\n";
		code += "		direction_yz.z = direction_yz.z / sqrt(direction_yz.z); // better uniform distribution\n";
		code += "		vec3 direction = vec3(direction_xz.x * direction_yz.z, direction_yz.y, direction_xz.z * direction_yz.z);\n";
		code += "		direction = normalize(direction);\n";
		code += "		VELOCITY = direction * initial_linear_velocity * mix(1.0, rand_from_seed(alt_seed), initial_linear_velocity_random);\n";
	}

	code += "		float base_angle = (initial_angle + tex_angle) * mix(1.0, angle_rand, initial_angle_random);\n";
	code += "		CUSTOM.x = base_angle * degree_to_rad;\n"; // angle
	code += "		CUSTOM.y = 0.0;\n"; // phase
	code += "		CUSTOM.z = (anim_offset + tex_anim_offset) * mix(1.0, anim_offset_rand, anim_offset_random);\n"; // animation offset (0-1)

	switch (emission_shape) {
		case EMISSION_SHAPE_POINT: {
			//do none
		} break;
		case EMISSION_SHAPE_SPHERE: {
			code += "		TRANSFORM[3].xyz = normalize(vec3(rand_from_seed(alt_seed) * 2.0 - 1.0, rand_from_seed(alt_seed) * 2.0 - 1.0, rand_from_seed(alt_seed) * 2.0 - 1.0)) * emission_sphere_radius;\n";
		} break;
		case EMISSION_SHAPE_BOX: {
			code += "		TRANSFORM[3].xyz = vec3(rand_from_seed(alt_seed) * 2.0 - 1.0, rand_from_seed(alt_seed) * 2.0 - 1.0, rand_from_seed(alt_seed) * 2.0 - 1.0) * emission_box_extents;\n";
		} break;
		case EMISSION_SHAPE_POINTS:
		case EMISSION_SHAPE_DIRECTED_POINTS: {
			code += "		TRANSFORM[3].xyz = texelFetch(emission_texture_points, emission_tex_ofs, 0).xyz;\n";

			if (emission_shape == EMISSION_SHAPE_DIRECTED_POINTS) {
				if (flags[FLAG_DISABLE_Z]) {

					code += "		mat2 rotm;";
					code += "		rotm[0] = texelFetch(emission_texture_normal, emission_tex_ofs, 0).xy;\n";
					code += "		rotm[1] = rotm[0].yx * vec2(1.0, -1.0);\n";
					code += "		VELOCITY.xy = rotm * VELOCITY.xy;\n";
				} else {
					code += "		vec3 normal = texelFetch(emission_texture_normal, emission_tex_ofs, 0).xyz;\n";
					code += "		vec3 v0 = abs(normal.z) < 0.999 ? vec3(0.0, 0.0, 1.0) : vec3(0, 1.0, 0.0);\n";
					code += "		vec3 tangent = normalize(cross(v0, normal));\n";
					code += "		vec3 bitangent = normalize(cross(tangent, normal));\n";
					code += "		VELOCITY = mat3(tangent, bitangent, normal) * VELOCITY;\n";
				}
			}
		} break;
	}
	code += "		VELOCITY = (EMISSION_TRANSFORM * vec4(VELOCITY, 0.0)).xyz;\n";
	code += "		TRANSFORM = EMISSION_TRANSFORM * TRANSFORM;\n";
	if (flags[FLAG_DISABLE_Z]) {
		code += "		VELOCITY.z = 0.0;\n";
		code += "		TRANSFORM[3].z = 0.0;\n";
	}

	code += "	} else {\n";

	code += "		CUSTOM.y += DELTA / LIFETIME;\n";

	if (tex_parameters[PARAM_INITIAL_LINEAR_VELOCITY].is_valid())
		code += "		float tex_linear_velocity = textureLod(linear_velocity_texture, vec2(CUSTOM.y, 0.0), 0.0).r;\n";
	else
		code += "		float tex_linear_velocity = 0.0;\n";

	if (flags[FLAG_DISABLE_Z]) {

		if (tex_parameters[PARAM_ORBIT_VELOCITY].is_valid())
			code += "		float tex_orbit_velocity = textureLod(orbit_velocity_texture, vec2(CUSTOM.y, 0.0), 0.0).r;\n";
		else
			code += "		float tex_orbit_velocity = 0.0;\n";
	}

	if (tex_parameters[PARAM_ANGULAR_VELOCITY].is_valid())
		code += "		float tex_angular_velocity = textureLod(angular_velocity_texture, vec2(CUSTOM.y, 0.0), 0.0).r;\n";
	else
		code += "		float tex_angular_velocity = 0.0;\n";

	if (tex_parameters[PARAM_LINEAR_ACCEL].is_valid())
		code += "		float tex_linear_accel = textureLod(linear_accel_texture, vec2(CUSTOM.y, 0.0), 0.0).r;\n";
	else
		code += "		float tex_linear_accel = 0.0;\n";

	if (tex_parameters[PARAM_RADIAL_ACCEL].is_valid())
		code += "		float tex_radial_accel = textureLod(radial_accel_texture, vec2(CUSTOM.y, 0.0), 0.0).r;\n";
	else
		code += "		float tex_radial_accel = 0.0;\n";

	if (tex_parameters[PARAM_TANGENTIAL_ACCEL].is_valid())
		code += "		float tex_tangent_accel = textureLod(tangent_accel_texture, vec2(CUSTOM.y, 0.0), 0.0).r;\n";
	else
		code += "		float tex_tangent_accel = 0.0;\n";

	if (tex_parameters[PARAM_DAMPING].is_valid())
		code += "		float tex_damping = textureLod(damping_texture, vec2(CUSTOM.y, 0.0), 0.0).r;\n";
	else
		code += "		float tex_damping = 0.0;\n";

	if (tex_parameters[PARAM_ANGLE].is_valid())
		code += "		float tex_angle = textureLod(angle_texture, vec2(CUSTOM.y, 0.0), 0.0).r;\n";
	else
		code += "		float tex_angle = 0.0;\n";

	if (tex_parameters[PARAM_ANIM_SPEED].is_valid())
		code += "		float tex_anim_speed = textureLod(anim_speed_texture, vec2(CUSTOM.y, 0.0), 0.0).r;\n";
	else
		code += "		float tex_anim_speed = 0.0;\n";

	if (tex_parameters[PARAM_ANIM_OFFSET].is_valid())
		code += "		float tex_anim_offset = textureLod(anim_offset_texture, vec2(CUSTOM.y, 0.0), 0.0).r;\n";
	else
		code += "		float tex_anim_offset = 0.0;\n";

	code += "		vec3 force = gravity;\n";
	code += "		vec3 pos = TRANSFORM[3].xyz;\n";
	if (flags[FLAG_DISABLE_Z]) {
		code += "		pos.z = 0.0;\n";
	}
	code += "		// apply linear acceleration\n";
	code += "		force += length(VELOCITY) > 0.0 ? normalize(VELOCITY) * (linear_accel + tex_linear_accel) * mix(1.0, rand_from_seed(alt_seed), linear_accel_random) : vec3(0.0);\n";
	code += "		// apply radial acceleration\n";
	code += "		vec3 org = EMISSION_TRANSFORM[3].xyz;\n";
	code += "		vec3 diff = pos - org;\n";
	code += "		force += length(diff) > 0.0 ? normalize(diff) * (radial_accel + tex_radial_accel) * mix(1.0, rand_from_seed(alt_seed), radial_accel_random) : vec3(0.0);\n";
	code += "		// apply tangential acceleration;\n";
	if (flags[FLAG_DISABLE_Z]) {
		code += "		force += length(diff.yx) > 0.0 ? vec3(normalize(diff.yx * vec2(-1.0, 1.0)), 0.0) * ((tangent_accel + tex_tangent_accel) * mix(1.0, rand_from_seed(alt_seed), tangent_accel_random)) : vec3(0.0);\n";

	} else {
		code += "		vec3 crossDiff = cross(normalize(diff), normalize(gravity));\n";
		code += "		force += length(crossDiff) > 0.0 ? normalize(crossDiff) * ((tangent_accel + tex_tangent_accel) * mix(1.0, rand_from_seed(alt_seed), tangent_accel_random)) : vec3(0.0);\n";
	}
	code += "		// apply attractor forces\n";
	code += "		VELOCITY += force * DELTA;\n";
	code += "		// orbit velocity\n";
	if (flags[FLAG_DISABLE_Z]) {

		code += "		float orbit_amount = (orbit_velocity + tex_orbit_velocity) * mix(1.0, rand_from_seed(alt_seed), orbit_velocity_random);\n";
		code += "		if (orbit_amount != 0.0) {\n";
		code += "		     float ang = orbit_amount * DELTA * pi * 2.0;\n";
		code += "		     mat2 rot = mat2(vec2(cos(ang), -sin(ang)), vec2(sin(ang), cos(ang)));\n";
		code += "		     TRANSFORM[3].xy -= diff.xy;\n";
		code += "		     TRANSFORM[3].xy += rot * diff.xy;\n";
		code += "		}\n";
	}

	if (tex_parameters[PARAM_INITIAL_LINEAR_VELOCITY].is_valid()) {
		code += "		VELOCITY = normalize(VELOCITY) * tex_linear_velocity;\n";
	}
	code += "		if (damping + tex_damping > 0.0) {\n";
	code += "			float v = length(VELOCITY);\n";
	code += "			float damp = (damping + tex_damping) * mix(1.0, rand_from_seed(alt_seed), damping_random);\n";
	code += "			v -= damp * DELTA;\n";
	code += "			if (v < 0.0) {\n";
	code += "				VELOCITY = vec3(0.0);\n";
	code += "			} else {\n";
	code += "				VELOCITY = normalize(VELOCITY) * v;\n";
	code += "			}\n";
	code += "		}\n";
	code += "		float base_angle = (initial_angle + tex_angle) * mix(1.0, angle_rand, initial_angle_random);\n";
	code += "		base_angle += CUSTOM.y * LIFETIME * (angular_velocity + tex_angular_velocity) * mix(1.0, rand_from_seed(alt_seed) * 2.0 - 1.0, angular_velocity_random);\n";
	code += "		CUSTOM.x = base_angle * degree_to_rad;\n"; // angle
	code += "		CUSTOM.z = (anim_offset + tex_anim_offset) * mix(1.0, anim_offset_rand, anim_offset_random) + CUSTOM.y * (anim_speed + tex_anim_speed) * mix(1.0, rand_from_seed(alt_seed), anim_speed_random);\n"; // angle
	if (flags[FLAG_ANIM_LOOP]) {
		code += "		CUSTOM.z = mod(CUSTOM.z, 1.0);\n"; // loop

	} else {
		code += "		CUSTOM.z = clamp(CUSTOM.z, 0.0, 1.0);\n"; // 0 to 1 only
	}
	code += "	}\n";

	// add noise to velocity
	if (flags[FLAG_NOISE]) {
		if (flags[FLAG_DISABLE_Z]) {
			switch (noise_type) {
				case RANDOM_NOISE: {
					code += "	uint random_seed = hash(base_number + RANDOM_SEED + uint(int(TIME)));\n";
					code += "	float random_angle = 2.0 * 3.14 * rand_from_seed(random_seed);\n";
					code += "	vec3 noise_direction = vec3(cos(random_angle), sin(random_angle), 0.0);\n";
				} break;
				case SIMPLEX_NOISE: {
					code += "	vec3 noise_direction = snoise_vec3(noise_scale * vec3(TRANSFORM[3].xy, noise_timefactor * TIME));\n";
				} break;
				case CURL_NOISE: {
					code += "	vec3 noise_direction = curl_3d(noise_scale * vec3(TRANSFORM[3].xy, noise_timefactor * TIME));\n";
				} break;
			}
			code += "	VELOCITY += noise_strength * noise_direction*DELTA; \n";
			code += "	VELOCITY.z= 0.0; \n";
		} else {
			switch (noise_type) {
				case RANDOM_NOISE: {
					code += "	uint random_seed1 = hash(base_number + uint(1) + RANDOM_SEED + uint(int(TIME)));\n";
					code += "	uint random_seed2 = hash(base_number + uint(2) + RANDOM_SEED + uint(int(TIME)));\n";
					code += "	uint random_seed3 = hash(base_number + uint(3) + RANDOM_SEED + uint(int(TIME)));\n";
					code += "	float random_num1 = rand_from_seed_m1_p1(random_seed1);\n";
					code += "	float random_num2 = rand_from_seed_m1_p1(random_seed2);\n";
					code += "	float random_num3 = rand_from_seed_m1_p1(random_seed2);\n";
					code += "	vec3 noise_direction = normalize(vec3(random_num1, random_num2, random_num3));\n";
				} break;
				case SIMPLEX_NOISE: {
					code += "	vec3 noise_direction = snoise_vec4(noise_scale * vec4(TRANSFORM[3].xyz, noise_timefactor * TIME));\n";
				} break;
				case CURL_NOISE: {
					code += "	vec3 noise_direction = curl_4d(noise_scale * vec4(TRANSFORM[3].xyz, noise_timefactor * TIME));\n";
				} break;
			}
			code += "	VELOCITY += noise_strength * noise_direction*DELTA; \n";
		}
	}

	// apply color
	// apply hue rotation
	if (tex_parameters[PARAM_SCALE].is_valid())
		code += "	float tex_scale = textureLod(scale_texture, vec2(CUSTOM.y, 0.0), 0.0).r;\n";
	else
		code += "	float tex_scale = 1.0;\n";

	if (tex_parameters[PARAM_HUE_VARIATION].is_valid())
		code += "	float tex_hue_variation = textureLod(hue_variation_texture, vec2(CUSTOM.y, 0.0), 0.0).r;\n";
	else
		code += "	float tex_hue_variation = 0.0;\n";

	code += "	float hue_rot_angle = (hue_variation + tex_hue_variation) * pi * 2.0 * mix(1.0, hue_rot_rand * 2.0 - 1.0, hue_variation_random);\n";
	code += "	float hue_rot_c = cos(hue_rot_angle);\n";
	code += "	float hue_rot_s = sin(hue_rot_angle);\n";
	code += "	mat4 hue_rot_mat = mat4(vec4(0.299, 0.587, 0.114, 0.0),\n";
	code += "			vec4(0.299, 0.587, 0.114, 0.0),\n";
	code += "			vec4(0.299, 0.587, 0.114, 0.0),\n";
	code += "			vec4(0.000, 0.000, 0.000, 1.0)) +\n";
	code += "		mat4(vec4(0.701, -0.587, -0.114, 0.0),\n";
	code += "			vec4(-0.299, 0.413, -0.114, 0.0),\n";
	code += "			vec4(-0.300, -0.588, 0.886, 0.0),\n";
	code += "			vec4(0.000, 0.000, 0.000, 0.0)) * hue_rot_c +\n";
	code += "		mat4(vec4(0.168, 0.330, -0.497, 0.0),\n";
	code += "			vec4(-0.328, 0.035,  0.292, 0.0),\n";
	code += "			vec4(1.250, -1.050, -0.203, 0.0),\n";
	code += "			vec4(0.000, 0.000, 0.000, 0.0)) * hue_rot_s;\n";
	if (color_ramp.is_valid()) {
		code += "	COLOR = hue_rot_mat * textureLod(color_ramp, vec2(CUSTOM.y, 0.0), 0.0);\n";
	} else {
		code += "	COLOR = hue_rot_mat * color_value;\n";
	}
	if (emission_color_texture.is_valid() && emission_shape >= EMISSION_SHAPE_POINTS) {
		code += "	COLOR *= texelFetch(emission_texture_color, emission_tex_ofs, 0);\n";
	}
	if (trail_color_modifier.is_valid()) {
		code += "	if (trail_divisor > 1) {\n";
		code += "		COLOR *= textureLod(trail_color_modifier, vec2(float(int(NUMBER) % trail_divisor) / float(trail_divisor - 1), 0.0), 0.0);\n";
		code += "	}\n";
	}
	code += "\n";

	if (flags[FLAG_DISABLE_Z]) {

		if (flags[FLAG_ALIGN_Y_TO_VELOCITY]) {
			code += "	if (length(VELOCITY) > 0.0) {\n";
			code += "		TRANSFORM[1].xyz = normalize(VELOCITY);\n";
			code += "	} else {\n";
			code += "		TRANSFORM[1].xyz = normalize(TRANSFORM[1].xyz);\n";
			code += "	}\n";
			code += "	TRANSFORM[0].xyz = normalize(cross(TRANSFORM[1].xyz, TRANSFORM[2].xyz));\n";
			code += "	TRANSFORM[2] = vec4(0.0, 0.0, 1.0, 0.0);\n";
		} else {
			code += "	TRANSFORM[0] = vec4(cos(CUSTOM.x), -sin(CUSTOM.x), 0.0, 0.0);\n";
			code += "	TRANSFORM[1] = vec4(sin(CUSTOM.x), cos(CUSTOM.x), 0.0, 0.0);\n";
			code += "	TRANSFORM[2] = vec4(0.0, 0.0, 1.0, 0.0);\n";
		}

	} else {
		// orient particle Y towards velocity
		if (flags[FLAG_ALIGN_Y_TO_VELOCITY]) {
			code += "	if (length(VELOCITY) > 0.0) {\n";
			code += "		TRANSFORM[1].xyz = normalize(VELOCITY);\n";
			code += "	} else {\n";
			code += "		TRANSFORM[1].xyz = normalize(TRANSFORM[1].xyz);\n";
			code += "	}\n";
			code += "	if (TRANSFORM[1].xyz == normalize(TRANSFORM[0].xyz)) {\n";
			code += "		TRANSFORM[0].xyz = normalize(cross(normalize(TRANSFORM[1].xyz), normalize(TRANSFORM[2].xyz)));\n";
			code += "		TRANSFORM[2].xyz = normalize(cross(normalize(TRANSFORM[0].xyz), normalize(TRANSFORM[1].xyz)));\n";
			code += "	} else {\n";
			code += "		TRANSFORM[2].xyz = normalize(cross(normalize(TRANSFORM[0].xyz), normalize(TRANSFORM[1].xyz)));\n";
			code += "		TRANSFORM[0].xyz = normalize(cross(normalize(TRANSFORM[1].xyz), normalize(TRANSFORM[2].xyz)));\n";
			code += "	}\n";
		} else {
			code += "	TRANSFORM[0].xyz = normalize(TRANSFORM[0].xyz);\n";
			code += "	TRANSFORM[1].xyz = normalize(TRANSFORM[1].xyz);\n";
			code += "	TRANSFORM[2].xyz = normalize(TRANSFORM[2].xyz);\n";
		}
		// turn particle by rotation in Y
		if (flags[FLAG_ROTATE_Y]) {
			code += "	TRANSFORM = TRANSFORM * mat4(vec4(cos(CUSTOM.x), 0.0, -sin(CUSTOM.x), 0.0), vec4(0.0, 1.0, 0.0, 0.0), vec4(sin(CUSTOM.x), 0.0, cos(CUSTOM.x), 0.0), vec4(0.0, 0.0, 0.0, 1.0));\n";
		}
	}
	//scale by scale
	code += "	float base_scale = mix(scale * tex_scale, 1.0, scale_random * scale_rand);\n";
	code += "	if (base_scale == 0.0) {\n";
	code += "		base_scale = 0.000001;\n";
	code += "	}\n";
	if (trail_size_modifier.is_valid()) {
		code += "	if (trail_divisor > 1) {\n";
		code += "		base_scale *= textureLod(trail_size_modifier, vec2(float(int(NUMBER) % trail_divisor) / float(trail_divisor - 1), 0.0), 0.0).r;\n";
		code += "	}\n";
	}

	code += "	TRANSFORM[0].xyz *= base_scale;\n";
	code += "	TRANSFORM[1].xyz *= base_scale;\n";
	code += "	TRANSFORM[2].xyz *= base_scale;\n";

	if (flags[FLAG_DISABLE_Z]) {
		code += "	VELOCITY.z = 0.0;\n";
		code += "	TRANSFORM[3].z = 0.0;\n";
	}

	code += "}\n";
	code += "\n";

	ShaderData shader_data;
	shader_data.shader = VS::get_singleton()->shader_create();
	shader_data.users = 1;

	VS::get_singleton()->shader_set_code(shader_data.shader, code);

	shader_map[mk] = shader_data;

	VS::get_singleton()->material_set_shader(_get_material(), shader_data.shader);
}

void ParticlesMaterial::flush_changes() {

	if (material_mutex)
		material_mutex->lock();

	while (dirty_materials.first()) {

		dirty_materials.first()->self()->_update_shader();
	}

	if (material_mutex)
		material_mutex->unlock();
}

void ParticlesMaterial::_queue_shader_change() {

	if (material_mutex)
		material_mutex->lock();

	if (!element.in_list()) {
		dirty_materials.add(&element);
	}

	if (material_mutex)
		material_mutex->unlock();
}

bool ParticlesMaterial::_is_shader_dirty() const {

	bool dirty = false;

	if (material_mutex)
		material_mutex->lock();

	dirty = element.in_list();

	if (material_mutex)
		material_mutex->unlock();

	return dirty;
}

void ParticlesMaterial::set_spread(float p_spread) {

	spread = p_spread;
	VisualServer::get_singleton()->material_set_param(_get_material(), shader_names->spread, p_spread);
}

float ParticlesMaterial::get_spread() const {

	return spread;
}

void ParticlesMaterial::set_flatness(float p_flatness) {

	flatness = p_flatness;
	VisualServer::get_singleton()->material_set_param(_get_material(), shader_names->flatness, p_flatness);
}
float ParticlesMaterial::get_flatness() const {

	return flatness;
}

void ParticlesMaterial::set_param(Parameter p_param, float p_value) {

	ERR_FAIL_INDEX(p_param, PARAM_MAX);

	parameters[p_param] = p_value;

	switch (p_param) {
		case PARAM_INITIAL_LINEAR_VELOCITY: {
			VisualServer::get_singleton()->material_set_param(_get_material(), shader_names->initial_linear_velocity, p_value);
		} break;
		case PARAM_ANGULAR_VELOCITY: {
			VisualServer::get_singleton()->material_set_param(_get_material(), shader_names->angular_velocity, p_value);
		} break;
		case PARAM_ORBIT_VELOCITY: {
			VisualServer::get_singleton()->material_set_param(_get_material(), shader_names->orbit_velocity, p_value);
		} break;
		case PARAM_LINEAR_ACCEL: {
			VisualServer::get_singleton()->material_set_param(_get_material(), shader_names->linear_accel, p_value);
		} break;
		case PARAM_RADIAL_ACCEL: {
			VisualServer::get_singleton()->material_set_param(_get_material(), shader_names->radial_accel, p_value);
		} break;
		case PARAM_TANGENTIAL_ACCEL: {
			VisualServer::get_singleton()->material_set_param(_get_material(), shader_names->tangent_accel, p_value);
		} break;
		case PARAM_DAMPING: {
			VisualServer::get_singleton()->material_set_param(_get_material(), shader_names->damping, p_value);
		} break;
		case PARAM_ANGLE: {
			VisualServer::get_singleton()->material_set_param(_get_material(), shader_names->initial_angle, p_value);
		} break;
		case PARAM_SCALE: {
			VisualServer::get_singleton()->material_set_param(_get_material(), shader_names->scale, p_value);
		} break;
		case PARAM_HUE_VARIATION: {
			VisualServer::get_singleton()->material_set_param(_get_material(), shader_names->hue_variation, p_value);
		} break;
		case PARAM_ANIM_SPEED: {
			VisualServer::get_singleton()->material_set_param(_get_material(), shader_names->anim_speed, p_value);
		} break;
		case PARAM_ANIM_OFFSET: {
			VisualServer::get_singleton()->material_set_param(_get_material(), shader_names->anim_offset, p_value);
		} break;
		case PARAM_NOISE_SCALE: {
			VisualServer::get_singleton()->material_set_param(_get_material(), shader_names->noise_scale, p_value);
		} break;
		case PARAM_NOISE_STRENGTH: {
			VisualServer::get_singleton()->material_set_param(_get_material(), shader_names->noise_strength, p_value);
		} break;
		case PARAM_NOISE_TIMEFACTOR: {
			VisualServer::get_singleton()->material_set_param(_get_material(), shader_names->noise_timefactor, p_value);
		} break;
	}
}
float ParticlesMaterial::get_param(Parameter p_param) const {

	ERR_FAIL_INDEX_V(p_param, PARAM_MAX, 0);

	return parameters[p_param];
}

void ParticlesMaterial::set_param_randomness(Parameter p_param, float p_value) {

	ERR_FAIL_INDEX(p_param, PARAM_MAX);

	randomness[p_param] = p_value;

	switch (p_param) {
		case PARAM_INITIAL_LINEAR_VELOCITY: {
			VisualServer::get_singleton()->material_set_param(_get_material(), shader_names->initial_linear_velocity_random, p_value);
		} break;
		case PARAM_ANGULAR_VELOCITY: {
			VisualServer::get_singleton()->material_set_param(_get_material(), shader_names->angular_velocity_random, p_value);
		} break;
		case PARAM_ORBIT_VELOCITY: {
			VisualServer::get_singleton()->material_set_param(_get_material(), shader_names->orbit_velocity_random, p_value);
		} break;
		case PARAM_LINEAR_ACCEL: {
			VisualServer::get_singleton()->material_set_param(_get_material(), shader_names->linear_accel_random, p_value);
		} break;
		case PARAM_RADIAL_ACCEL: {
			VisualServer::get_singleton()->material_set_param(_get_material(), shader_names->radial_accel_random, p_value);
		} break;
		case PARAM_TANGENTIAL_ACCEL: {
			VisualServer::get_singleton()->material_set_param(_get_material(), shader_names->tangent_accel_random, p_value);
		} break;
		case PARAM_DAMPING: {
			VisualServer::get_singleton()->material_set_param(_get_material(), shader_names->damping_random, p_value);
		} break;
		case PARAM_ANGLE: {
			VisualServer::get_singleton()->material_set_param(_get_material(), shader_names->initial_angle_random, p_value);
		} break;
		case PARAM_SCALE: {
			VisualServer::get_singleton()->material_set_param(_get_material(), shader_names->scale_random, p_value);
		} break;
		case PARAM_HUE_VARIATION: {
			VisualServer::get_singleton()->material_set_param(_get_material(), shader_names->hue_variation_random, p_value);
		} break;
		case PARAM_ANIM_SPEED: {
			VisualServer::get_singleton()->material_set_param(_get_material(), shader_names->anim_speed_random, p_value);
		} break;
		case PARAM_ANIM_OFFSET: {
			VisualServer::get_singleton()->material_set_param(_get_material(), shader_names->anim_offset_random, p_value);
		} break;
	}
}
float ParticlesMaterial::get_param_randomness(Parameter p_param) const {

	ERR_FAIL_INDEX_V(p_param, PARAM_MAX, 0);

	return randomness[p_param];
}

static void _adjust_curve_range(const Ref<Texture> &p_texture, float p_min, float p_max) {

	Ref<CurveTexture> curve_tex = p_texture;
	if (!curve_tex.is_valid())
		return;

	curve_tex->ensure_default_setup(p_min, p_max);
}

void ParticlesMaterial::set_param_texture(Parameter p_param, const Ref<Texture> &p_texture) {

	ERR_FAIL_INDEX(p_param, PARAM_MAX);

	tex_parameters[p_param] = p_texture;

	switch (p_param) {
		case PARAM_INITIAL_LINEAR_VELOCITY: {
			//do none for this one
		} break;
		case PARAM_ANGULAR_VELOCITY: {
			VisualServer::get_singleton()->material_set_param(_get_material(), shader_names->angular_velocity_texture, p_texture);
			_adjust_curve_range(p_texture, -360, 360);
		} break;
		case PARAM_ORBIT_VELOCITY: {
			VisualServer::get_singleton()->material_set_param(_get_material(), shader_names->orbit_velocity_texture, p_texture);
			_adjust_curve_range(p_texture, -500, 500);
		} break;
		case PARAM_LINEAR_ACCEL: {
			VisualServer::get_singleton()->material_set_param(_get_material(), shader_names->linear_accel_texture, p_texture);
			_adjust_curve_range(p_texture, -200, 200);
		} break;
		case PARAM_RADIAL_ACCEL: {
			VisualServer::get_singleton()->material_set_param(_get_material(), shader_names->radial_accel_texture, p_texture);
			_adjust_curve_range(p_texture, -200, 200);
		} break;
		case PARAM_TANGENTIAL_ACCEL: {
			VisualServer::get_singleton()->material_set_param(_get_material(), shader_names->tangent_accel_texture, p_texture);
			_adjust_curve_range(p_texture, -200, 200);
		} break;
		case PARAM_DAMPING: {
			VisualServer::get_singleton()->material_set_param(_get_material(), shader_names->damping_texture, p_texture);
			_adjust_curve_range(p_texture, 0, 100);
		} break;
		case PARAM_ANGLE: {
			VisualServer::get_singleton()->material_set_param(_get_material(), shader_names->angle_texture, p_texture);
			_adjust_curve_range(p_texture, -360, 360);
		} break;
		case PARAM_SCALE: {
			VisualServer::get_singleton()->material_set_param(_get_material(), shader_names->scale_texture, p_texture);

			Ref<CurveTexture> curve_tex = p_texture;
			if (curve_tex.is_valid()) {
				curve_tex->ensure_default_setup();
			}

		} break;
		case PARAM_HUE_VARIATION: {
			VisualServer::get_singleton()->material_set_param(_get_material(), shader_names->hue_variation_texture, p_texture);
			_adjust_curve_range(p_texture, -1, 1);
		} break;
		case PARAM_ANIM_SPEED: {
			VisualServer::get_singleton()->material_set_param(_get_material(), shader_names->anim_speed_texture, p_texture);
			_adjust_curve_range(p_texture, 0, 200);
		} break;
		case PARAM_ANIM_OFFSET: {
			VisualServer::get_singleton()->material_set_param(_get_material(), shader_names->anim_offset_texture, p_texture);
		} break;
	}

	_queue_shader_change();
}
Ref<Texture> ParticlesMaterial::get_param_texture(Parameter p_param) const {

	ERR_FAIL_INDEX_V(p_param, PARAM_MAX, Ref<Texture>());

	return tex_parameters[p_param];
}

void ParticlesMaterial::set_color(const Color &p_color) {

	VisualServer::get_singleton()->material_set_param(_get_material(), shader_names->color, p_color);
	color = p_color;
}

Color ParticlesMaterial::get_color() const {

	return color;
}

void ParticlesMaterial::set_color_ramp(const Ref<Texture> &p_texture) {

	color_ramp = p_texture;
	VisualServer::get_singleton()->material_set_param(_get_material(), shader_names->color_ramp, p_texture);
	_queue_shader_change();
	_change_notify();
}

Ref<Texture> ParticlesMaterial::get_color_ramp() const {

	return color_ramp;
}

void ParticlesMaterial::set_flag(Flags p_flag, bool p_enable) {
	ERR_FAIL_INDEX(p_flag, FLAG_MAX);
	flags[p_flag] = p_enable;
	_queue_shader_change();
	if (p_flag == FLAG_DISABLE_Z) {
		_change_notify();
	}
}

bool ParticlesMaterial::get_flag(Flags p_flag) const {
	ERR_FAIL_INDEX_V(p_flag, FLAG_MAX, false);
	return flags[p_flag];
}

void ParticlesMaterial::set_emission_shape(EmissionShape p_shape) {

	emission_shape = p_shape;
	_change_notify();
	_queue_shader_change();
}

void ParticlesMaterial::set_emission_sphere_radius(float p_radius) {

	emission_sphere_radius = p_radius;
	VisualServer::get_singleton()->material_set_param(_get_material(), shader_names->emission_sphere_radius, p_radius);
}

void ParticlesMaterial::set_emission_box_extents(Vector3 p_extents) {

	emission_box_extents = p_extents;
	VisualServer::get_singleton()->material_set_param(_get_material(), shader_names->emission_box_extents, p_extents);
}

void ParticlesMaterial::set_emission_point_texture(const Ref<Texture> &p_points) {

	emission_point_texture = p_points;
	VisualServer::get_singleton()->material_set_param(_get_material(), shader_names->emission_texture_points, p_points);
}

void ParticlesMaterial::set_emission_normal_texture(const Ref<Texture> &p_normals) {

	emission_normal_texture = p_normals;
	VisualServer::get_singleton()->material_set_param(_get_material(), shader_names->emission_texture_normal, p_normals);
}

void ParticlesMaterial::set_emission_color_texture(const Ref<Texture> &p_colors) {

	emission_color_texture = p_colors;
	VisualServer::get_singleton()->material_set_param(_get_material(), shader_names->emission_texture_color, p_colors);
	_queue_shader_change();
}

void ParticlesMaterial::set_emission_point_count(int p_count) {

	emission_point_count = p_count;
	VisualServer::get_singleton()->material_set_param(_get_material(), shader_names->emission_texture_point_count, p_count);
}

ParticlesMaterial::EmissionShape ParticlesMaterial::get_emission_shape() const {

	return emission_shape;
}

float ParticlesMaterial::get_emission_sphere_radius() const {

	return emission_sphere_radius;
}
Vector3 ParticlesMaterial::get_emission_box_extents() const {

	return emission_box_extents;
}
Ref<Texture> ParticlesMaterial::get_emission_point_texture() const {

	return emission_point_texture;
}
Ref<Texture> ParticlesMaterial::get_emission_normal_texture() const {

	return emission_normal_texture;
}

Ref<Texture> ParticlesMaterial::get_emission_color_texture() const {

	return emission_color_texture;
}

int ParticlesMaterial::get_emission_point_count() const {

	return emission_point_count;
}

void ParticlesMaterial::set_trail_divisor(int p_divisor) {

	trail_divisor = p_divisor;
	VisualServer::get_singleton()->material_set_param(_get_material(), shader_names->trail_divisor, p_divisor);
}

int ParticlesMaterial::get_trail_divisor() const {

	return trail_divisor;
}

void ParticlesMaterial::set_trail_size_modifier(const Ref<CurveTexture> &p_trail_size_modifier) {

	trail_size_modifier = p_trail_size_modifier;

	Ref<CurveTexture> curve = trail_size_modifier;
	if (curve.is_valid()) {
		curve->ensure_default_setup();
	}

	VisualServer::get_singleton()->material_set_param(_get_material(), shader_names->trail_size_modifier, curve);
	_queue_shader_change();
}

Ref<CurveTexture> ParticlesMaterial::get_trail_size_modifier() const {

	return trail_size_modifier;
}

void ParticlesMaterial::set_trail_color_modifier(const Ref<GradientTexture> &p_trail_color_modifier) {

	trail_color_modifier = p_trail_color_modifier;
	VisualServer::get_singleton()->material_set_param(_get_material(), shader_names->trail_color_modifier, p_trail_color_modifier);
	_queue_shader_change();
}

Ref<GradientTexture> ParticlesMaterial::get_trail_color_modifier() const {

	return trail_color_modifier;
}

void ParticlesMaterial::set_gravity(const Vector3 &p_gravity) {

	gravity = p_gravity;
	Vector3 gset = gravity;
	if (gset == Vector3()) {
		gset = Vector3(0, -0.000001, 0); //as gravity is used as upvector in some calculations
	}
	VisualServer::get_singleton()->material_set_param(_get_material(), shader_names->gravity, gset);
}

Vector3 ParticlesMaterial::get_gravity() const {

	return gravity;
}

RID ParticlesMaterial::get_shader_rid() const {

	ERR_FAIL_COND_V(!shader_map.has(current_key), RID());
	return shader_map[current_key].shader;
}

void ParticlesMaterial::set_noise_type(NoiseType p_type) {

	noise_type = p_type;
	_change_notify();
	_queue_shader_change();
}

ParticlesMaterial::NoiseType ParticlesMaterial::get_noise_type() const {

	return noise_type;
}

void ParticlesMaterial::_validate_property(PropertyInfo &property) const {

	if (property.name == "color" && color_ramp.is_valid()) {
		property.usage = 0;
	}

	if (property.name == "emission_sphere_radius" && emission_shape != EMISSION_SHAPE_SPHERE) {
		property.usage = 0;
	}

	if (property.name == "emission_box_extents" && emission_shape != EMISSION_SHAPE_BOX) {
		property.usage = 0;
	}

	if ((property.name == "emission_point_texture" || property.name == "emission_color_texture") && (emission_shape < EMISSION_SHAPE_POINTS)) {
		property.usage = 0;
	}

	if (property.name == "emission_normal_texture" && emission_shape != EMISSION_SHAPE_DIRECTED_POINTS) {
		property.usage = 0;
	}

	if (property.name == "emission_point_count" && (emission_shape != EMISSION_SHAPE_POINTS && emission_shape != EMISSION_SHAPE_DIRECTED_POINTS)) {
		property.usage = 0;
	}

	if (property.name.begins_with("orbit_") && !flags[FLAG_DISABLE_Z]) {
		property.usage = 0;
	}
}

Shader::Mode ParticlesMaterial::get_shader_mode() const {

	return Shader::MODE_PARTICLES;
}

void ParticlesMaterial::_bind_methods() {

	ClassDB::bind_method(D_METHOD("set_spread", "degrees"), &ParticlesMaterial::set_spread);
	ClassDB::bind_method(D_METHOD("get_spread"), &ParticlesMaterial::get_spread);

	ClassDB::bind_method(D_METHOD("set_flatness", "amount"), &ParticlesMaterial::set_flatness);
	ClassDB::bind_method(D_METHOD("get_flatness"), &ParticlesMaterial::get_flatness);

	ClassDB::bind_method(D_METHOD("set_param", "param", "value"), &ParticlesMaterial::set_param);
	ClassDB::bind_method(D_METHOD("get_param", "param"), &ParticlesMaterial::get_param);

	ClassDB::bind_method(D_METHOD("set_param_randomness", "param", "randomness"), &ParticlesMaterial::set_param_randomness);
	ClassDB::bind_method(D_METHOD("get_param_randomness", "param"), &ParticlesMaterial::get_param_randomness);

	ClassDB::bind_method(D_METHOD("set_param_texture", "param", "texture"), &ParticlesMaterial::set_param_texture);
	ClassDB::bind_method(D_METHOD("get_param_texture", "param"), &ParticlesMaterial::get_param_texture);

	ClassDB::bind_method(D_METHOD("set_color", "color"), &ParticlesMaterial::set_color);
	ClassDB::bind_method(D_METHOD("get_color"), &ParticlesMaterial::get_color);

	ClassDB::bind_method(D_METHOD("set_color_ramp", "ramp"), &ParticlesMaterial::set_color_ramp);
	ClassDB::bind_method(D_METHOD("get_color_ramp"), &ParticlesMaterial::get_color_ramp);

	ClassDB::bind_method(D_METHOD("set_flag", "flag", "enable"), &ParticlesMaterial::set_flag);
	ClassDB::bind_method(D_METHOD("get_flag", "flag"), &ParticlesMaterial::get_flag);

	ClassDB::bind_method(D_METHOD("set_emission_shape", "shape"), &ParticlesMaterial::set_emission_shape);
	ClassDB::bind_method(D_METHOD("get_emission_shape"), &ParticlesMaterial::get_emission_shape);

	ClassDB::bind_method(D_METHOD("set_emission_sphere_radius", "radius"), &ParticlesMaterial::set_emission_sphere_radius);
	ClassDB::bind_method(D_METHOD("get_emission_sphere_radius"), &ParticlesMaterial::get_emission_sphere_radius);

	ClassDB::bind_method(D_METHOD("set_emission_box_extents", "extents"), &ParticlesMaterial::set_emission_box_extents);
	ClassDB::bind_method(D_METHOD("get_emission_box_extents"), &ParticlesMaterial::get_emission_box_extents);

	ClassDB::bind_method(D_METHOD("set_emission_point_texture", "texture"), &ParticlesMaterial::set_emission_point_texture);
	ClassDB::bind_method(D_METHOD("get_emission_point_texture"), &ParticlesMaterial::get_emission_point_texture);

	ClassDB::bind_method(D_METHOD("set_emission_normal_texture", "texture"), &ParticlesMaterial::set_emission_normal_texture);
	ClassDB::bind_method(D_METHOD("get_emission_normal_texture"), &ParticlesMaterial::get_emission_normal_texture);

	ClassDB::bind_method(D_METHOD("set_emission_color_texture", "texture"), &ParticlesMaterial::set_emission_color_texture);
	ClassDB::bind_method(D_METHOD("get_emission_color_texture"), &ParticlesMaterial::get_emission_color_texture);

	ClassDB::bind_method(D_METHOD("set_emission_point_count", "point_count"), &ParticlesMaterial::set_emission_point_count);
	ClassDB::bind_method(D_METHOD("get_emission_point_count"), &ParticlesMaterial::get_emission_point_count);

	ClassDB::bind_method(D_METHOD("set_trail_divisor", "divisor"), &ParticlesMaterial::set_trail_divisor);
	ClassDB::bind_method(D_METHOD("get_trail_divisor"), &ParticlesMaterial::get_trail_divisor);

	ClassDB::bind_method(D_METHOD("set_trail_size_modifier", "texture"), &ParticlesMaterial::set_trail_size_modifier);
	ClassDB::bind_method(D_METHOD("get_trail_size_modifier"), &ParticlesMaterial::get_trail_size_modifier);

	ClassDB::bind_method(D_METHOD("set_trail_color_modifier", "texture"), &ParticlesMaterial::set_trail_color_modifier);
	ClassDB::bind_method(D_METHOD("get_trail_color_modifier"), &ParticlesMaterial::get_trail_color_modifier);

	ClassDB::bind_method(D_METHOD("get_gravity"), &ParticlesMaterial::get_gravity);
	ClassDB::bind_method(D_METHOD("set_gravity", "accel_vec"), &ParticlesMaterial::set_gravity);

	ClassDB::bind_method(D_METHOD("get_noise_type"), &ParticlesMaterial::get_noise_type);
	ClassDB::bind_method(D_METHOD("set_noise_type", "type"), &ParticlesMaterial::set_noise_type);

	ADD_GROUP("Trail", "trail_");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "trail_divisor", PROPERTY_HINT_RANGE, "1,1000000,1"), "set_trail_divisor", "get_trail_divisor");
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "trail_size_modifier", PROPERTY_HINT_RESOURCE_TYPE, "CurveTexture"), "set_trail_size_modifier", "get_trail_size_modifier");
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "trail_color_modifier", PROPERTY_HINT_RESOURCE_TYPE, "GradientTexture"), "set_trail_color_modifier", "get_trail_color_modifier");
	ADD_GROUP("Emission Shape", "emission_");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "emission_shape", PROPERTY_HINT_ENUM, "Point,Sphere,Box,Points,Directed Points"), "set_emission_shape", "get_emission_shape");
	ADD_PROPERTY(PropertyInfo(Variant::REAL, "emission_sphere_radius", PROPERTY_HINT_RANGE, "0.01,128,0.01,or_greater"), "set_emission_sphere_radius", "get_emission_sphere_radius");
	ADD_PROPERTY(PropertyInfo(Variant::VECTOR3, "emission_box_extents"), "set_emission_box_extents", "get_emission_box_extents");
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "emission_point_texture", PROPERTY_HINT_RESOURCE_TYPE, "Texture"), "set_emission_point_texture", "get_emission_point_texture");
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "emission_normal_texture", PROPERTY_HINT_RESOURCE_TYPE, "Texture"), "set_emission_normal_texture", "get_emission_normal_texture");
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "emission_color_texture", PROPERTY_HINT_RESOURCE_TYPE, "Texture"), "set_emission_color_texture", "get_emission_color_texture");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "emission_point_count", PROPERTY_HINT_RANGE, "0,1000000,1"), "set_emission_point_count", "get_emission_point_count");
	ADD_GROUP("Flags", "flag_");
	ADD_PROPERTYI(PropertyInfo(Variant::BOOL, "flag_align_y"), "set_flag", "get_flag", FLAG_ALIGN_Y_TO_VELOCITY);
	ADD_PROPERTYI(PropertyInfo(Variant::BOOL, "flag_rotate_y"), "set_flag", "get_flag", FLAG_ROTATE_Y);
	ADD_PROPERTYI(PropertyInfo(Variant::BOOL, "flag_disable_z"), "set_flag", "get_flag", FLAG_DISABLE_Z);
	ADD_GROUP("Spread", "");
	ADD_PROPERTY(PropertyInfo(Variant::REAL, "spread", PROPERTY_HINT_RANGE, "0,180,0.01"), "set_spread", "get_spread");
	ADD_PROPERTY(PropertyInfo(Variant::REAL, "flatness", PROPERTY_HINT_RANGE, "0,1,0.01"), "set_flatness", "get_flatness");
	ADD_GROUP("Gravity", "");
	ADD_PROPERTY(PropertyInfo(Variant::VECTOR3, "gravity"), "set_gravity", "get_gravity");
	ADD_GROUP("Initial Velocity", "initial_");
	ADD_PROPERTYI(PropertyInfo(Variant::REAL, "initial_velocity", PROPERTY_HINT_RANGE, "0,1000,0.01,or_lesser,or_greater"), "set_param", "get_param", PARAM_INITIAL_LINEAR_VELOCITY);
	ADD_PROPERTYI(PropertyInfo(Variant::REAL, "initial_velocity_random", PROPERTY_HINT_RANGE, "0,1,0.01"), "set_param_randomness", "get_param_randomness", PARAM_INITIAL_LINEAR_VELOCITY);
	ADD_GROUP("Angular Velocity", "angular_");
	ADD_PROPERTYI(PropertyInfo(Variant::REAL, "angular_velocity", PROPERTY_HINT_RANGE, "-360,360,0.01"), "set_param", "get_param", PARAM_ANGULAR_VELOCITY);
	ADD_PROPERTYI(PropertyInfo(Variant::REAL, "angular_velocity_random", PROPERTY_HINT_RANGE, "0,1,0.01"), "set_param_randomness", "get_param_randomness", PARAM_ANGULAR_VELOCITY);
	ADD_PROPERTYI(PropertyInfo(Variant::OBJECT, "angular_velocity_curve", PROPERTY_HINT_RESOURCE_TYPE, "CurveTexture"), "set_param_texture", "get_param_texture", PARAM_ANGULAR_VELOCITY);
	ADD_GROUP("Orbit Velocity", "orbit_");
	ADD_PROPERTYI(PropertyInfo(Variant::REAL, "orbit_velocity", PROPERTY_HINT_RANGE, "-1000,1000,0.01,or_lesser,or_greater"), "set_param", "get_param", PARAM_ORBIT_VELOCITY);
	ADD_PROPERTYI(PropertyInfo(Variant::REAL, "orbit_velocity_random", PROPERTY_HINT_RANGE, "0,1,0.01"), "set_param_randomness", "get_param_randomness", PARAM_ORBIT_VELOCITY);
	ADD_PROPERTYI(PropertyInfo(Variant::OBJECT, "orbit_velocity_curve", PROPERTY_HINT_RESOURCE_TYPE, "CurveTexture"), "set_param_texture", "get_param_texture", PARAM_ORBIT_VELOCITY);
	ADD_GROUP("Linear Accel", "linear_");
	ADD_PROPERTYI(PropertyInfo(Variant::REAL, "linear_accel", PROPERTY_HINT_RANGE, "-100,100,0.01,or_lesser,or_greater"), "set_param", "get_param", PARAM_LINEAR_ACCEL);
	ADD_PROPERTYI(PropertyInfo(Variant::REAL, "linear_accel_random", PROPERTY_HINT_RANGE, "0,1,0.01"), "set_param_randomness", "get_param_randomness", PARAM_LINEAR_ACCEL);
	ADD_PROPERTYI(PropertyInfo(Variant::OBJECT, "linear_accel_curve", PROPERTY_HINT_RESOURCE_TYPE, "CurveTexture"), "set_param_texture", "get_param_texture", PARAM_LINEAR_ACCEL);
	ADD_GROUP("Radial Accel", "radial_");
	ADD_PROPERTYI(PropertyInfo(Variant::REAL, "radial_accel", PROPERTY_HINT_RANGE, "-100,100,0.01,or_lesser,or_greater"), "set_param", "get_param", PARAM_RADIAL_ACCEL);
	ADD_PROPERTYI(PropertyInfo(Variant::REAL, "radial_accel_random", PROPERTY_HINT_RANGE, "0,1,0.01"), "set_param_randomness", "get_param_randomness", PARAM_RADIAL_ACCEL);
	ADD_PROPERTYI(PropertyInfo(Variant::OBJECT, "radial_accel_curve", PROPERTY_HINT_RESOURCE_TYPE, "CurveTexture"), "set_param_texture", "get_param_texture", PARAM_RADIAL_ACCEL);
	ADD_GROUP("Tangential Accel", "tangential_");
	ADD_PROPERTYI(PropertyInfo(Variant::REAL, "tangential_accel", PROPERTY_HINT_RANGE, "-100,100,0.01,or_lesser,or_greater"), "set_param", "get_param", PARAM_TANGENTIAL_ACCEL);
	ADD_PROPERTYI(PropertyInfo(Variant::REAL, "tangential_accel_random", PROPERTY_HINT_RANGE, "0,1,0.01"), "set_param_randomness", "get_param_randomness", PARAM_TANGENTIAL_ACCEL);
	ADD_PROPERTYI(PropertyInfo(Variant::OBJECT, "tangential_accel_curve", PROPERTY_HINT_RESOURCE_TYPE, "CurveTexture"), "set_param_texture", "get_param_texture", PARAM_TANGENTIAL_ACCEL);
	ADD_GROUP("Damping", "");
	ADD_PROPERTYI(PropertyInfo(Variant::REAL, "damping", PROPERTY_HINT_RANGE, "0,100,0.01,or_greater"), "set_param", "get_param", PARAM_DAMPING);
	ADD_PROPERTYI(PropertyInfo(Variant::REAL, "damping_random", PROPERTY_HINT_RANGE, "0,1,0.01"), "set_param_randomness", "get_param_randomness", PARAM_DAMPING);
	ADD_PROPERTYI(PropertyInfo(Variant::OBJECT, "damping_curve", PROPERTY_HINT_RESOURCE_TYPE, "CurveTexture"), "set_param_texture", "get_param_texture", PARAM_DAMPING);
	ADD_GROUP("Angle", "");
	ADD_PROPERTYI(PropertyInfo(Variant::REAL, "angle", PROPERTY_HINT_RANGE, "-720,720,0.1,or_lesser,or_greater"), "set_param", "get_param", PARAM_ANGLE);
	ADD_PROPERTYI(PropertyInfo(Variant::REAL, "angle_random", PROPERTY_HINT_RANGE, "0,1,0.01"), "set_param_randomness", "get_param_randomness", PARAM_ANGLE);
	ADD_PROPERTYI(PropertyInfo(Variant::OBJECT, "angle_curve", PROPERTY_HINT_RESOURCE_TYPE, "CurveTexture"), "set_param_texture", "get_param_texture", PARAM_ANGLE);
	ADD_GROUP("Scale", "");
	ADD_PROPERTYI(PropertyInfo(Variant::REAL, "scale", PROPERTY_HINT_RANGE, "0,1000,0.01,or_greater"), "set_param", "get_param", PARAM_SCALE);
	ADD_PROPERTYI(PropertyInfo(Variant::REAL, "scale_random", PROPERTY_HINT_RANGE, "0,1,0.01"), "set_param_randomness", "get_param_randomness", PARAM_SCALE);
	ADD_PROPERTYI(PropertyInfo(Variant::OBJECT, "scale_curve", PROPERTY_HINT_RESOURCE_TYPE, "CurveTexture"), "set_param_texture", "get_param_texture", PARAM_SCALE);
	ADD_GROUP("Color", "");
	ADD_PROPERTY(PropertyInfo(Variant::COLOR, "color"), "set_color", "get_color");
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "color_ramp", PROPERTY_HINT_RESOURCE_TYPE, "GradientTexture"), "set_color_ramp", "get_color_ramp");

	ADD_GROUP("Hue Variation", "hue_");
	ADD_PROPERTYI(PropertyInfo(Variant::REAL, "hue_variation", PROPERTY_HINT_RANGE, "-1,1,0.1"), "set_param", "get_param", PARAM_HUE_VARIATION);
	ADD_PROPERTYI(PropertyInfo(Variant::REAL, "hue_variation_random", PROPERTY_HINT_RANGE, "0,1,0.01"), "set_param_randomness", "get_param_randomness", PARAM_HUE_VARIATION);
	ADD_PROPERTYI(PropertyInfo(Variant::OBJECT, "hue_variation_curve", PROPERTY_HINT_RESOURCE_TYPE, "CurveTexture"), "set_param_texture", "get_param_texture", PARAM_HUE_VARIATION);
	ADD_GROUP("Animation", "anim_");
	ADD_PROPERTYI(PropertyInfo(Variant::REAL, "anim_speed", PROPERTY_HINT_RANGE, "0,128,0.01,or_greater"), "set_param", "get_param", PARAM_ANIM_SPEED);
	ADD_PROPERTYI(PropertyInfo(Variant::REAL, "anim_speed_random", PROPERTY_HINT_RANGE, "0,1,0.01"), "set_param_randomness", "get_param_randomness", PARAM_ANIM_SPEED);
	ADD_PROPERTYI(PropertyInfo(Variant::OBJECT, "anim_speed_curve", PROPERTY_HINT_RESOURCE_TYPE, "CurveTexture"), "set_param_texture", "get_param_texture", PARAM_ANIM_SPEED);
	ADD_PROPERTYI(PropertyInfo(Variant::REAL, "anim_offset", PROPERTY_HINT_RANGE, "0,1,0.01"), "set_param", "get_param", PARAM_ANIM_OFFSET);
	ADD_PROPERTYI(PropertyInfo(Variant::REAL, "anim_offset_random", PROPERTY_HINT_RANGE, "0,1,0.01"), "set_param_randomness", "get_param_randomness", PARAM_ANIM_OFFSET);
	ADD_PROPERTYI(PropertyInfo(Variant::OBJECT, "anim_offset_curve", PROPERTY_HINT_RESOURCE_TYPE, "CurveTexture"), "set_param_texture", "get_param_texture", PARAM_ANIM_OFFSET);
	ADD_PROPERTYI(PropertyInfo(Variant::BOOL, "anim_loop"), "set_flag", "get_flag", FLAG_ANIM_LOOP);

	ADD_GROUP("Noise", "noise_");
	ADD_PROPERTYI(PropertyInfo(Variant::BOOL, "noise_add_noise"), "set_flag", "get_flag", FLAG_NOISE);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "noise_type", PROPERTY_HINT_ENUM, "Random, Simplex, Curl"), "set_noise_type", "get_noise_type");
	ADD_PROPERTYI(PropertyInfo(Variant::REAL, "noise_scale", PROPERTY_HINT_RANGE, "0,1,0.001,or_lesser,or_greater"), "set_param", "get_param", PARAM_NOISE_SCALE);
	ADD_PROPERTYI(PropertyInfo(Variant::REAL, "noise_strength", PROPERTY_HINT_RANGE, "0,1000,1,or_lesser,or_greater"), "set_param", "get_param", PARAM_NOISE_STRENGTH);
	ADD_PROPERTYI(PropertyInfo(Variant::REAL, "noise_timefactor", PROPERTY_HINT_RANGE, "0,1000,1,or_lesser,or_greater"), "set_param", "get_param", PARAM_NOISE_TIMEFACTOR);

	BIND_ENUM_CONSTANT(PARAM_INITIAL_LINEAR_VELOCITY);
	BIND_ENUM_CONSTANT(PARAM_ANGULAR_VELOCITY);
	BIND_ENUM_CONSTANT(PARAM_ORBIT_VELOCITY);
	BIND_ENUM_CONSTANT(PARAM_LINEAR_ACCEL);
	BIND_ENUM_CONSTANT(PARAM_RADIAL_ACCEL);
	BIND_ENUM_CONSTANT(PARAM_TANGENTIAL_ACCEL);
	BIND_ENUM_CONSTANT(PARAM_DAMPING);
	BIND_ENUM_CONSTANT(PARAM_ANGLE);
	BIND_ENUM_CONSTANT(PARAM_SCALE);
	BIND_ENUM_CONSTANT(PARAM_HUE_VARIATION);
	BIND_ENUM_CONSTANT(PARAM_ANIM_SPEED);
	BIND_ENUM_CONSTANT(PARAM_ANIM_OFFSET);
	BIND_ENUM_CONSTANT(PARAM_NOISE_SCALE);
	BIND_ENUM_CONSTANT(PARAM_NOISE_STRENGTH);
	BIND_ENUM_CONSTANT(PARAM_NOISE_TIMEFACTOR);
	BIND_ENUM_CONSTANT(PARAM_MAX);

	BIND_ENUM_CONSTANT(FLAG_ALIGN_Y_TO_VELOCITY);
	BIND_ENUM_CONSTANT(FLAG_ROTATE_Y);
	BIND_ENUM_CONSTANT(FLAG_MAX);

	BIND_ENUM_CONSTANT(EMISSION_SHAPE_POINT);
	BIND_ENUM_CONSTANT(EMISSION_SHAPE_SPHERE);
	BIND_ENUM_CONSTANT(EMISSION_SHAPE_BOX);
	BIND_ENUM_CONSTANT(EMISSION_SHAPE_POINTS);
	BIND_ENUM_CONSTANT(EMISSION_SHAPE_DIRECTED_POINTS);

	BIND_ENUM_CONSTANT(SIMPLEX_NOISE);
	BIND_ENUM_CONSTANT(CURL_NOISE);
}

ParticlesMaterial::ParticlesMaterial() :
		element(this) {

	set_spread(45);
	set_flatness(0);
	set_param(PARAM_INITIAL_LINEAR_VELOCITY, 0);
	set_param(PARAM_ORBIT_VELOCITY, 0);
	set_param(PARAM_LINEAR_ACCEL, 0);
	set_param(PARAM_RADIAL_ACCEL, 0);
	set_param(PARAM_TANGENTIAL_ACCEL, 0);
	set_param(PARAM_DAMPING, 0);
	set_param(PARAM_ANGLE, 0);
	set_param(PARAM_SCALE, 1);
	set_param(PARAM_HUE_VARIATION, 0);
	set_param(PARAM_ANIM_SPEED, 0);
	set_param(PARAM_ANIM_OFFSET, 0);
	set_param(PARAM_NOISE_SCALE, 0.005);
	set_param(PARAM_NOISE_STRENGTH, 3.0);
	set_param(PARAM_NOISE_TIMEFACTOR, 1);
	set_emission_shape(EMISSION_SHAPE_POINT);
	set_emission_sphere_radius(1);
	set_emission_box_extents(Vector3(1, 1, 1));
	set_trail_divisor(1);
	set_gravity(Vector3(0, -9.8, 0));
	emission_point_count = 1;
	set_noise_type(SIMPLEX_NOISE);

	for (int i = 0; i < PARAM_MAX; i++) {
		set_param_randomness(Parameter(i), 0);
	}

	for (int i = 0; i < FLAG_MAX; i++) {
		flags[i] = false;
	}

	set_color(Color(1, 1, 1, 1));

	current_key.key = 0;
	current_key.invalid_key = 1;

	_queue_shader_change();
}

ParticlesMaterial::~ParticlesMaterial() {

	if (material_mutex)
		material_mutex->lock();

	if (shader_map.has(current_key)) {
		shader_map[current_key].users--;
		if (shader_map[current_key].users == 0) {
			//deallocate shader, as it's no longer in use
			VS::get_singleton()->free(shader_map[current_key].shader);
			shader_map.erase(current_key);
		}

		VS::get_singleton()->material_set_shader(_get_material(), RID());
	}

	if (material_mutex)
		material_mutex->unlock();
}
