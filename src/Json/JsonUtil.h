#pragma once
//
// Created by kamil-hp on 06.05.23.
//

#ifndef MG1_ZAD2_JSONUTIL_H
#define MG1_ZAD2_JSONUTIL_H
#define JSON
#include "library_include.h"
#include <glm/vec3.hpp>

namespace bf {
	class Point;
	class Torus;
	struct Transform;
	template<std::integral T>
	void load(const Json::Value &value, T &a, const std::string &name) {
		if (!value.isMember(name) || !value[name].isIntegral()) return;
		a = static_cast<T>(value[name].asLargestInt());
	}
	template<std::floating_point T>
	void load(const Json::Value &value, T &a, const std::string &name) {
		if (!value.isMember(name) || !value[name].isNumeric()) return;
		a = static_cast<T>(value[name].asDouble());
	}
	void load(const Json::Value &value, bool &a, const std::string &name);
	[[maybe_unused]] void load(const Json::Value &value, std::string &a, const std::string &name);
	bool loadFromFile(Json::Value &root, const std::string &path);
	std::pair<unsigned, bf::Point*> loadPoint(Json::Value& pointValue);
	std::pair<unsigned, bf::Torus*> loadTorus(Json::Value& torusValue);
	Json::Value saveValue(const bf::Point& point, unsigned id);
	Json::Value saveValue(const bf::Torus& torus, unsigned id);
	Json::Value saveValue(const glm::vec3& vec);
	Json::Value saveValue(const glm::vec<2, int>& vec);
	Json::Value saveValue(const bf::Transform& torus, bool isPoint);
	glm::vec3 loadVec3(Json::Value& tValue);
	glm::vec<2,int> loadVec2i(Json::Value& tValue);
}

#endif //MG1_ZAD2_JSONUTIL_H
