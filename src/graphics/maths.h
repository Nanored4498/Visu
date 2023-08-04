// Copyright (C) 2023, Coudert--Osmont Yoann
// SPDX-License-Identifier: AGPL-3.0-or-later
// See <https://www.gnu.org/licenses/>

#pragma once

#include <cmath>

namespace gfx {

template<typename T>
struct get_scalar { using type = typename T::Scalar; };
template<typename T> requires std::is_arithmetic_v<T>
struct get_scalar<T> { using type = T; };
template<typename T>
using get_scalar_t = typename get_scalar<T>::type;

template<int N, typename T, typename V>
struct vec_base {
using Scalar = get_scalar_t<T>;
#define _vecFOR for(std::ptrdiff_t i = 0; i < N; ++i)
private:
	// Casting to V
	inline V& toV() { return *reinterpret_cast<V*>(this); }
	inline const V& toV() const { return *reinterpret_cast<const V*>(this); }

public:
	// Constructor
	vec_base() = default;
	template<typename U, typename W>
	vec_base(const vec_base<N, U, W> &other) { _vecFOR (*this)[i] = other[i]; }

	// Coordinate access
	inline const T& operator[](std::ptrdiff_t i) const { return reinterpret_cast<const T*>(this)[i]; }
	inline T& operator[](std::ptrdiff_t i) { return reinterpret_cast<T*>(this)[i]; }

	// Unary minus
	inline V operator-() const { V v; _vecFOR v[i] = -(*this)[i]; return v; }

	// Operators with scalar
	inline V& operator*=(Scalar x) { _vecFOR (*this)[i] *= x; return toV(); }
	inline V operator*(Scalar x) const { return V(toV()) *= x; }
	inline friend V operator*(Scalar x, const vec_base &v) { return v * x; }
	inline V& operator/=(Scalar x) { _vecFOR (*this)[i] /= x; return toV(); }
	inline V operator/(Scalar x) const { return V(toV()) /= x; }

	// Operators with vectors
	template<typename W>
	inline V& operator+=(const W& other) { _vecFOR (*this)[i] += other[i]; return toV(); }
	template<typename U, typename W>
	inline auto operator+(const vec_base<N, U, W>& other) const {
		using TU = std::common_type_t<T, U>;
		if constexpr (std::is_same_v<TU, T>) return V(toV()) += other;
		if constexpr (std::is_same_v<TU, U>) return W(*this) += other;
	}
	template<typename W>
	inline V& operator-=(const W& other) { _vecFOR (*this)[i] -= other[i]; return toV(); }
	template<typename U, typename W>
	inline auto operator-(const vec_base<N, U, W>& other) const {
		using TU = std::common_type_t<T, U>;
		if constexpr (std::is_same_v<TU, T>) return V(toV()) -= other;
		if constexpr (std::is_same_v<TU, U>) return W(*this) -= other;
	}

	// Dot product
	template<typename U, typename W>
	inline auto operator*(const vec_base<N, U, W>& other) const { decltype((*this)[0]*other[0]) d = 0.; _vecFOR d += (*this)[i]*other[i]; return d; }

	// Equality
	inline bool operator==(const V& other) { _vecFOR if((*this)[i] != other[i]) return false; return true; }
	inline bool operator!=(const V& other) { _vecFOR if((*this)[i] != other[i]) return true; return false; }

	// Norm
	inline Scalar norm2() const { Scalar n2 = 0.; _vecFOR n2 += (*this)[i]*(*this)[i]; return n2; }
	inline Scalar norm() const { return std::sqrt(norm2()); }
	inline V& normalize() { return *this *= 1. / norm(); }

	// Print
	friend std::ostream& operator<<(std::ostream &stream, const vec_base &v) {
		stream << '(' << v[0];
		for(int i = 1; i < N; ++i) stream << ", " << v[i];
		return stream << ')';
	}
#undef _vecFOR
};

template <typename T>
struct vec2T : vec_base<2, T, vec2T<T>> {
	T x, y;
	vec2T() = default;
	vec2T(T x, T y=0): x(x), y(y) {}
	template<typename U, typename W>
	vec2T(const vec_base<2, U, W> &other): vec_base<2, T, vec2T>(other) {}
};
using vec2 = vec2T<double>;
using vec2f = vec2T<float>;
using vec2i = vec2T<int>;

template <typename T>
struct vec3T : vec_base<3, T, vec3T<T>> {
	T x, y, z;
	vec3T() = default;
	vec3T(T x, T y=0, T z=0): x(x), y(y), z(z) {}
	template<typename U, typename W>
	vec3T(const vec_base<3, U, W> &other): vec_base<3, T, vec3T>(other) {}

	inline friend vec3T cross(const vec3T &a, const vec3T &b) {
		return vec3T(a.y*b.z - a.z*b.y, a.z*b.x - a.x*b.z, a.x*b.y - a.y*b.x);
	}
};
using vec3 = vec3T<double>;
using vec3f = vec3T<float>;

template <typename T>
struct vec4T : vec_base<4, T, vec4T<T>> {
	T x, y, z, w;
	vec4T() = default;
	vec4T(T x, T y=0, T z=0, T w=0): x(x), y(y), z(z), w(w) {}
	template<typename U, typename W>
	vec4T(const vec_base<4, U, W> &other): vec_base<4, T, vec4T>(other) {}
};
using vec4 = vec4T<double>;
using vec4f = vec4T<float>;
using vec4i = vec4T<int>;

};