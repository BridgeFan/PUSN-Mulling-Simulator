//
// Created by kamil-hp on 06.05.23.
//
#include "JsonUtil.h"
#include "Object/Transform.h"
#include <fstream>
#include "Object/Point.h"
#include "Solids/Torus.h"
void bf::load(const Json::Value &value, std::string &a, const std::string &name) {
	if(!value.isMember(name) || !value[name].isString()) return;
	a = value[name].asString();
}

bool bf::loadFromFile(Json::Value& value, const std::string &path) {
	std::ifstream file(path);
	if(!file.good())
		return false;
	try {
		file >> value;
	}
	catch(...) {
		value.clear();
		return false;
	}
	return true;
}

void bf::load(const Json::Value &value, bool &a, const std::string &name) {
	if (!value.isMember(name) || !value[name].isBool()) return;
	a = value[name].asBool();
}

std::pair<unsigned, bf::Point *> bf::loadPoint(Json::Value &pointValue) {
	unsigned id = pointValue["id"].asUInt();
	glm::vec3 pos = loadVec3(pointValue["position"]);
	bf::Transform t(pos);
	if(pointValue.isMember("name"))
		return {id, new bf::Point(t, pointValue["name"].asString())};
	else
		return {id, new bf::Point(t)};
}

std::pair<unsigned, bf::Torus *> bf::loadTorus(Json::Value &torusValue) {
	unsigned id = torusValue["id"].asUInt();
	glm::vec3 pos = loadVec3(torusValue["position"]);
	glm::vec3 rot = loadVec3(torusValue["rotation"]);
	glm::vec3 scale = loadVec3(torusValue["scale"]);
	bf::Transform t(pos,rot,scale);
	bf::Torus* torus;
	if(torusValue.isMember("name"))
		torus = new bf::Torus(t, torusValue["name"].asString());
	else
		torus = new bf::Torus(t);
	torus->smallRadius=torusValue["smallRadius"].asFloat();
	torus->bigRadius=torusValue["largeRadius"].asFloat();
	auto v = loadVec2i(torusValue["samples"]);
	torus->bigFragments=v.x;
	torus->smallFragments=v.y;
	return {id, torus};
}

glm::vec3 bf::loadVec3(Json::Value &tValue) {
	return {tValue["x"].asFloat(), tValue["y"].asFloat(), tValue["z"].asFloat()};
}

glm::vec<2, int> bf::loadVec2i(Json::Value &tValue) {
	return {tValue["x"].asInt(), tValue["y"].asInt()};
}

Json::Value bf::saveValue(const bf::Transform &t, bool isPoint) {
	Json::Value value;
	value["position"]=saveValue(t.position);
	if(!isPoint) {
		value["rotation"]=saveValue(t.rotation);
		value["scale"]=saveValue(t.scale);
	}
	return value;
}

Json::Value bf::saveValue(const glm::vec3& v) {
	Json::Value value;
	value["x"]=v.x;
	value["y"]=v.y;
	value["z"]=v.z;
	return value;
}

Json::Value bf::saveValue(const glm::vec<2,int>& v) {
	Json::Value value;
	value["x"]=v.x;
	value["y"]=v.y;
	return value;
}

Json::Value bf::saveValue(const bf::Point &point, unsigned int id) {
	Json::Value value=saveValue(point.getTransform(),true);
	value["name"]=point.name;
	value["id"]=id;
	return value;
}

Json::Value bf::saveValue(const bf::Torus &torus, unsigned int id) {
	Json::Value value=saveValue(torus.getTransform(),false);
	value["name"]=torus.name;
	value["smallRadius"]=torus.smallRadius;
	value["largeRadius"]=torus.bigRadius;
	value["samples"]=saveValue({torus.bigFragments,torus.smallFragments});
	value["id"]=id;
	value["objectType"]="torus";
	return value;
}
