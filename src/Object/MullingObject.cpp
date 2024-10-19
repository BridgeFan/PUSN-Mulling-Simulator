//
// Created by kamil-hp on 23.10.23.
//

#include "MullingObject.h"
#include "ConfigState.h"
#include "ImGui/ImGuiUtil.h"
#include "ImGui/imgui_include.h"
#include "OpenGLUtil.h"
#include "Shader/ShaderArray.h"
#include "Util.h"
#include <GL/glew.h>
#include <fstream>
#include <iostream>
#include <algorithm>
#include <cstdint>
#include <glm/gtx/norm.hpp>

namespace bf {
	void MullingObject::draw(const ShaderArray &shaderArray) const {
		if(!points.empty() && shaderArray.getActiveIndex()==BasicShader) {
			const auto& shader = shaderArray.getActiveShader();
			if(isLineShown) {
				shaderArray.setColor(0u,255u,0u);
				lines.draw(shaderArray);
			}
			shaderArray.setColor(255u,127u,127u);
			shader.setBool("isNormalUsed", true);
			shader.setVec3("light.direction", -.2f,-1.f,-.3f);
			shader.setVec3("light.ambient", 0.2f, 0.2f, 0.2f);
			shader.setVec3("light.diffuse", 0.5f, 0.5f, 0.5f);
			shader.setVec3("light.specular", 1.0f, 1.0f, 1.0f);
			shader.setFloat("shininess", 32.f);
			flatEnd.drawTriangles(shaderArray);
			if(!isFlat/* && shaderArray.getActiveIndex()!=BasicShader*/) {
				sphericEnd.drawTriangles(shaderArray);
			}
			shaderArray.getActiveShader().setBool("isNormalUsed", false);
		}
		if(shaderArray.getActiveIndex()!=SolidShader || indices.empty() || vertices.empty())
			return;
		if(texture<UINT_MAX) {
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, texture);
		}
		const auto& shader = shaderArray.getActiveShader();
		shader.setVec3("color", 1.f,1.f,1.f);
		shader.setVec3("light.direction", -.2f,-1.f,-.3f);
		shader.setVec3("light.ambient", 0.2f, 0.2f, 0.2f);
		shader.setVec3("light.diffuse", 0.5f, 0.5f, 0.5f);
		shader.setVec3("light.specular", 1.0f, 1.0f, 1.0f);
		shader.setVec2("divisions", divisions);
		shader.setVec3("size", size);
		shader.setFloat("shininess", 32.f);
		shader.setInt("heightMap", 0);
		shader.setInt("heightMapFrag", 0);
		//function assumes set projection and view matrices
		glBindVertexArray(VAO);
		glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT,   // type
                   reinterpret_cast<void*>(0));          // element array buffer offset
	}
	
	void MullingObject::updateHeightMap(int beginX, int endX, int beginY, int endY) {
		if (beginX > endX || beginY > endY)
			return;
		std::vector<float> data;
		data.reserve((endX - beginX + 1) * (endY - beginY + 1));
		for (int i = beginY; i <= endY; i++) {
			std::copy(heights[i].begin() + beginX, heights[i].begin() + endX + 1, std::back_inserter(data));
		}
		glTextureSubImage2D(texture, 0, beginX, beginY, endX - beginX + 1, endY - beginY + 1, GL_RED, GL_FLOAT, data.data());
	}
	void MullingObject::addError(MullingErrorType type) {
		if (errors.contains(id) && type == NonCuttingMulling)
			return;
		std::string errorString;
		switch (type) {
			case DeepMulling:
				errorString = "Too deep mulling";
				break;
			case VerticalMulling:
				errorString = "Vertical mulling";
				break;
			case DownFlatMulling:
				errorString = "Down flat-end mulling";
				break;
			case NonCuttingMulling:
				errorString = "Non-cutting part of edge culling";
				break;
		}
		if (errors.contains(id)) {
			errors[id] += ", " + errorString;
		} else {
			errors[id] = errorString;
		}
		std::cerr << id + 1 << ": " << errorString << "\n";
	}
	void MullingObject::endAnimation() {
		isInstantProcessed=false;
		isRunning=false;
		points.clear();
		lines.vertices.clear();
		lines.indices.clear();
		lines.setBuffers();
		if (!errors.empty())
			ImGui::OpenPopup("Error menu");
	}

	void MullingObject::update(float deltaTime) {
		if(!isRunning || points.size()<2) return;
		for(int i=0;i<stepsPerFrame;i++) {
			internalUpdate(deltaTime/static_cast<float>(stepsPerFrame));
		}
	}

	void MullingObject::internalUpdate(float deltaTime) {
		if(!isRunning || points.size()<2) return;
		float dist = glm::distance(points[id], points[id+1]);
		//update heights
		float endSize2_25 = static_cast<float>(endSize*endSize)*0.25f;
		auto pos = lerp(points[id], points[id+1], phase);
		float dX = divisions.x/size.x; //div/mm
		float dY = divisions.y/size.y; //div/mm
		glm::vec2 divisionSize = {dX, dY};
		glm::vec2 pointIndices={(pos.x+size.x*.5f)*divisionSize.x, (pos.y+size.y*.5f)*divisionSize.y};
		glm::vec2 endDivSize={endSize*divisionSize.x*.5f, endSize*divisionSize.y*.5f}; //div
		int beginX = static_cast<int>(std::max(pointIndices.x-endDivSize.x,0.f));
		int beginY = static_cast<int>(std::max(pointIndices.y-endDivSize.y,0.f));
		int endX = std::min(static_cast<int>(pointIndices.x+endDivSize.x),static_cast<int>(divisions.x)-1);
		int endY = std::min(static_cast<int>(pointIndices.y+endDivSize.y),static_cast<int>(divisions.y)-1);
		for(int j=beginY;j<=endY; j++) {
			for(int i=beginX;i<=endX; i++) {
				auto hazardDepth = getHazardDepth(i,j);
				float difX = std::abs(pointIndices.x-i); //div
				float difY = std::abs(pointIndices.y-j); //div
				float sqrDist = difX*difX/(dX*dX)+difY*difY/(dY*dY); //mm^2
				if(sqrDist>endSize2_25) continue;
				if(isFlat) {
					if(pos.z < hazardDepth && !errors.contains(id)) {
						addError(NonCuttingMulling);
					}
					if(heights[j][i]*size.z>pos.z)
						heights[j][i]=pos.z/size.z;
				}
				else {
					float s=pos.z+endSize*0.5f-std::sqrt(endSize2_25-sqrDist);
					if(s < hazardDepth && !errors.contains(id)) {
						addError(NonCuttingMulling);
					}
					if(heights[j][i]*size.z>s)
						heights[j][i]=s/size.z;
				}
			}
		}
		//update texture
		updateHeightMap(beginX, endX, beginY, endY);
		//update phase
		phase+=simSpeed*deltaTime/dist;
		if(phase>1) {
			id++;
			if(id+1>=points.size()) {
				endAnimation();
			}
			else {
				checkMullingErrors();
				phase-=1.f;
			}
		}
		//update visible position
		if(isFlat)
			flatEnd.setPosition(lerp(points[id], points[id+1], phase));
		else {
			auto addVec = glm::vec3(0.f,0.f,static_cast<float>(endSize)*.5f);
			flatEnd.setPosition(lerp(points[id], points[id+1], phase)+addVec);
		}
		sphericEnd.setPosition(lerp(points[id], points[id+1], phase));
	}


	void MullingObject::onMergePoints(int, int) {	}

	ShaderType MullingObject::getShaderType() const {
		return bf::ShaderType::SolidShader;
	}
	void MullingObject::ObjectGui() {
		if(isInstantProcessed) {
			std::string buf = std::to_string(id)+"/"+std::to_string(points.size());
			float part = static_cast<float>(id)/static_cast<float>(points.size());
			ImGui::ProgressBar(part, ImVec2(0.f, 0.f), buf.c_str());
			auto endId=std::min<unsigned>(id+instantUpdateStep, points.size()-1);
			while(id<endId) {
				checkMullingErrors();
				processNextInstantPoint();
			}
			if(id>=points.size()-1) { //end of instant
				updateHeightMap(0, heights[0].size()-1, 0, heights.size()-1);
				endAnimation();
			}
		}
		if(isInstantProcessed)
			return;
		if (ImGui::BeginPopupModal("Error menu", nullptr, ImGuiWindowFlags_MenuBar)) {
			ImGui::Text("Found %ld errors", errors.size());
			for(auto const& [ID, val]: errors) {
				ImGui::Text("Part %d: %s", ID, val.c_str());
			}
			if(ImGui::Button("OK")) {
				ImGui::CloseCurrentPopup();
				errors.clear();
			}
            ImGui::EndPopup();
		}
		if(!points.empty())
			ImGui::BeginDisabled();
		auto oldZ = size.z;
		if(bf::imgui::checkChanged("Part size", size)) {
			if(!almostEqual(oldZ, size.z)) {
				for (auto &v: heights) {
					for (auto &i: v) {
						//i = size.z;
						i = 1.f;
					}
				}
			}
			updateVertices(false);
		}
		if(bf::imgui::checkChanged("Divisions", divisions)) {
			heights.resize(divisions.y);
			for(auto& v: heights) {
				v.resize(divisions.x, 1.f);
			}
			updateVertices(true);
		}
		if(bf::imgui::checkChanged("Size of end", endSize)) {
			auto myScale = glm::vec3(static_cast<float>(endSize));
			flatEnd.setScale(myScale);
			sphericEnd.setScale(myScale);
		}
		ImGui::Checkbox("Is flat", &isFlat);
		if(!points.empty())
			ImGui::EndDisabled();
		bf::imgui::checkChanged("Height of end", endHeight);
		ImGui::Checkbox("Is line shown", &isLineShown);
		ImGui::SameLine();
		if(ImGui::Button("Reset")) {
			endAnimation();
		}
		bf::imgui::checkChanged("Height of basic", baseHeight);
		bf::imgui::checkChanged("Movement speed", simSpeed);
		bool areEmpty = points.empty();
		if(areEmpty)
			ImGui::BeginDisabled();
		if (ImGui::Button(isRunning ? "Pause" : "Start")) {
			isRunning=!isRunning;
		}
		if (ImGui::Button("Instant")) {
			isInstantProcessed=true;
		}
		if(areEmpty)
			ImGui::EndDisabled();

	}
	bool MullingObject::isMovable() const {
		return false;
	}
	bool MullingObject::isIntersectable() const {
		return false;
	}
	MullingObject::MullingObject() : Solid(bf::Transform::Default, true), lines("", false), flatEnd("", false), sphericEnd("", false) {

		//init heights and texture
		heights.resize(divisions.y);
		for (auto &v: heights) {
			v.resize(divisions.x, 1.f);
		}
		indices = {0, 0};
		vertices.emplace_back(.0f, .0f, .0f);
		setBuffers();
		updateVertices(true);
		lines.setTransform(bf::Transform::Default);
		lines.setBuffers();
		//init flatEnd
		//TODO: init sphericEnd
		constexpr int vertSize = 32 * 4 + 2;
		flatEnd.vertices.reserve(vertSize);
		flatEnd.indices.reserve(12 * 32);
		flatEnd.vertices.emplace_back(bf::ZPlus*1000.f, glm::vec2(.0f), bf::ZPlus);
		flatEnd.vertices.emplace_back(glm::vec3(.0f), glm::vec2(.0f), bf::ZMinus);
		for (int i = 0; i < 32; i++) {
			float u = i * std::numbers::pi / 15.f; //[0,2*pi]
			glm::vec3 pos1 = {std::cos(u)*.5f, std::sin(u)*.5f, 0.f};
			glm::vec3 pos2 = pos1+1000.f*ZPlus;
			flatEnd.vertices.emplace_back(pos1, glm::vec2(.0f), bf::ZMinus);
			flatEnd.vertices.emplace_back(pos1, glm::vec2(.0f), pos1);
			flatEnd.vertices.emplace_back(pos2, glm::vec2(.0f), bf::ZPlus);
			flatEnd.vertices.emplace_back(pos2, glm::vec2(.0f), pos1);
			std::vector<unsigned> v = {0, 4*i, (4*i+4)%vertSize,
							1, (4*i+6)%vertSize, 4*i+2,
							4*i+1, (4*i+5)%vertSize, (4*i+7)%vertSize,
							4*i+1, (4*i+7)%vertSize, 4*i+3
							};
			flatEnd.indices.insert(flatEnd.indices.end(), v.begin(), v.end());
		}
		flatEnd.setBuffers();
		for (int i = 0; i < 16; i++) {
			for (int j = 0; j < 16; j++) {
				float u = i * std::numbers::pi / 15.f;//half, so [0,pi]
				float v = j * std::numbers::pi / 15.f;//[0,pi]
				glm::vec3 pos = {cos(u) * sin(v) * 0.5f, cos(v) * 0.5f, (1.f - sin(u) * sin(v)) * 0.5f};
				glm::vec3 du = {-sin(u)*sin(v),0.f,-cos(u)*sin(v)};
				glm::vec3 dv = {cos(u)*cos(v),-sin(v),-sin(u)*cos(v)};
				sphericEnd.vertices.emplace_back(pos, glm::vec2(.0f), -glm::cross(du,dv));
			}
		}
		for (int i = 0; i < 16 - 1; i++) {
			for (int j = 0; j < 16; j++) {
				sphericEnd.indices.push_back(i * 16 + j);
				sphericEnd.indices.push_back(i * 16 + (j + 1) % 16);
				sphericEnd.indices.push_back((i + 1) * 16 + j);
				sphericEnd.indices.push_back(i * 16 + (j + 1) % 16);
				sphericEnd.indices.push_back((i + 1) * 16 + (j + 1) % 16);
				sphericEnd.indices.push_back((i + 1) * 16 + j);
			}
		}
		sphericEnd.setBuffers();
	}
	float MullingObject::getHazardDepth(int x, int y) {
		float ret=std::numeric_limits<float>::min();
		for(int i=std::max(x-1,0);i<=std::min(x+1,divisions.x-1);i++)
			for(int j=std::max(y-1,0);j<=std::min(y+1,divisions.y-1);j++)
				ret=std::max(heights[j][i],ret);
		return ret-endHeight/size.z;
	}

	void MullingObject::updateVertices(bool isBufferResized) {
		//vertices.reserve(4)
		vertices.clear();
		if(divisions.x<2 || divisions.y<2) {
			indices.clear();
			bf::gl::namedBufferData(VBO, vertices, true);
			bf::gl::namedBufferData(IBO, indices, true);
			return;
		}
		vertices.reserve(divisions.x*divisions.y);
		vertices.reserve(divisions.x*divisions.y);
		for(int i = 0; i < divisions.x; i++) {
			for (int j = 0; j < divisions.y; j++) {
				glm::vec3 n = {0.f,0.f,1.f};
				float pX = static_cast<float>(i)/static_cast<float>(divisions.x-1);
				float pY = static_cast<float>(j)/static_cast<float>(divisions.y-1);
				glm::vec3 fPos = {size.x * (-.5f + pX), size.y * (-.5f + pY), size.z};
				vertices.emplace_back(fPos, glm::vec2(pX, pY), n);
				//vertices.emplace_back(glm::vec3(size.x, -size.y, size.z), glm::vec2(1.f, 0.f), n);
				//vertices.emplace_back(glm::vec3(size.x, size.y, size.z), glm::vec2(1.f, 1.f), n);
				//vertices.emplace_back(glm::vec3(-size.x, size.y, size.z), glm::vec2(0.f, 1.f), n);
			}
		}
		//indices
		if(isBufferResized) {
			indices.clear();
			for(int i = 0; i < divisions.x-1; i++)       // for each row a.k.a. each strip
			{
				for(int j = 0; j < divisions.y-1; j++)      // for each column
				{
					indices.push_back(j + divisions.y * i);
					indices.push_back(j + divisions.y * (i+1));
					indices.push_back(j+1 + divisions.y * i);
					indices.push_back(j+1 + divisions.y * i);
					indices.push_back(j + divisions.y * (i+1));
					indices.push_back(j+1 + divisions.y * (i+1));
				}
			}
			bf::gl::namedBufferData(VBO, vertices, true);
			bf::gl::namedBufferData(IBO, indices, true);
			initTexture(isBufferResized);
		}
		else
			bf::gl::namedBufferSubData(VBO, vertices, 0, vertices.size());
	}
	float getPartClosestPointFromLine(const glm::vec2& p, const glm::vec2& P1, const glm::vec2& P2) {
		//based on https://paulbourke.net/geometry/pointlineplane/
		if(almostEqual(P1.x,P2.x) && almostEqual(P1.y,P2.y))
			return 0.0f;
		float t = ((p.x-P1.x)*(P2.x-P1.x)+(p.y-P1.y)*(P2.y-P1.y))/glm::dot(P2-P1,P2-P1);
		return std::clamp(t,0.f,1.f);
	}


	glm::vec2 getClosestPointFromLine(const glm::vec2& p, const glm::vec2& P1, const glm::vec2& P2) {
		return lerp(P1,P2,getPartClosestPointFromLine(p,P1,P2));
	}

	void MullingObject::processNextInstantPoint() {
		if(almostEqual(points[id].x, points[id+1].x) && almostEqual(points[id].y, points[id+1].y)) {
			//vertical line
			//TODO: vertical line down is last case
			id++;
			return;
		}
		//update heights
		//auto pos = lerp(points[id], points[id+1], phase);
		float dX = divisions.x/size.x;
		float dY = divisions.y/size.y;
		float endSize2_25 = static_cast<float>(endSize*endSize)*0.25f;
		glm::vec2 divisionSize = {dX, dY};
		glm::vec<2,int> pointIndices0={(points[id].x+size.x*.5f)*divisionSize.x, (points[id].y+size.y*.5f)*divisionSize.y};
		glm::vec<2,int> pointIndices1={(points[id+1].x+size.x*.5f)*divisionSize.x, (points[id+1].y+size.y*.5f)*divisionSize.y};
		glm::vec<2,int> endDivSize={endSize*divisionSize.x*.5f, endSize*divisionSize.y*.5f};
		int beginX = std::max(std::min(pointIndices0.x,pointIndices1.x)-endDivSize.x,0);
		int beginY = std::max(std::min(pointIndices0.y,pointIndices1.y)-endDivSize.y,0);
		int endX = std::min(std::max(pointIndices0.x,pointIndices1.x)+endDivSize.x,divisions.x-1);
		int endY = std::min(std::max(pointIndices0.y,pointIndices1.y)+endDivSize.y,divisions.y-1);
		auto pathLength = glm::distance(glm::vec2(points[id+1]), glm::vec2(points[id]));
		auto v = glm::vec2(points[id+1]-points[id])/pathLength;
		float H = (points[id+1].z-points[id].z)/pathLength;
		//TODO - check, vertical line
		for(int j=beginY;j<=endY; j++) {
			for(int i=beginX;i<=endX; i++) {
				float px = i/dX - size.x*.5f;
				float py = j/dY - size.y*.5f;
				glm::vec2 p = {px, py};
				auto t = getPartClosestPointFromLine(p, points[id], points[id+1]);
				auto closest = lerp<glm::vec2>(points[id],points[id+1],t);
				//auto dist = glm::distance(closest, p);
				auto L2 = glm::length2(closest-p);
				if(L2>=endSize2_25) {
					continue;
				}
				auto hazardDepth = getHazardDepth(i,j);
				float z;
				if(isFlat) {
					float diff = std::sqrt(endSize2_25-L2);
					float t0 = std::max(0.f, t-diff/pathLength);
					float t1 = std::min(1.f, t+diff/pathLength);
					z = std::min(lerp(points[id], points[id+1], t0).z, lerp(points[id], points[id+1], t1).z);
				}
				else {
					float x = std::sqrt(H*H / (1+H*H) * (endSize2_25-L2)); //move on x axis
					float t0 = std::max(0.f, t-x/pathLength);
					float t1 = std::min(1.f, t+x/pathLength);
					z = std::min(lerp(points[id], points[id+1], t0).z, lerp(points[id], points[id+1], t1).z);
					z += endSize*.5f-std::sqrt(endSize2_25-L2-x*x);
				}
				if(z < hazardDepth && !errors.contains(id)) {
					addError(NonCuttingMulling);
				}
				if(heights[j][i]*size.z>z)
					heights[j][i] = z/size.z;
			}
		}
		id++;
	}

	bool MullingObject::loadFromFile(const std::string& path) {
		if(points.size()>1) return true;
		auto fExt = path.find_last_of('.');
		auto ext = path.substr(fExt+1);
		if(ext.size()!=3) return true;
		if(tolower(ext[0])=='k') isFlat=false;
		else if(tolower(ext[0])=='f') isFlat=true;
		else return true;
		endSize = (ext[1]-'0')*10+(ext[2]-'0');

		auto myScale = glm::vec3(static_cast<float>(endSize));
		flatEnd.setScale(myScale);
		sphericEnd.setScale(myScale);

		endHeight = std::max(endHeight, static_cast<float>(endSize));
		std::ifstream file(path);
		if(file.bad())
			return true;
		std::vector<glm::vec3> _positions;
		for(std::string line; std::getline(file, line); ) {
			if(line.size()<=2 || line[0]!='N') continue;
			std::vector<char> limiters = {'G', 'X', 'Y', 'Z'};
			glm::vec3 pos;
			for(char limiter: limiters) {
				auto f = line.find(limiter);
				if(f==std::string::npos) continue;
				switch(limiter) {
					case 'G':
					case 'X':
						break;
					case 'Y':
						pos.x = std::stof(line.substr(0,f));
						break;
					case 'Z':
						pos.y = std::stof(line.substr(0,f));
						break;
					default:
						break;
				}
				line.erase(0, f+1);
			}
			pos.z = std::stof(line);
			_positions.push_back(pos);
		}
		if(_positions.size()<2)
			return true;
		points = std::move(_positions);
		lines.vertices.clear();
		lines.indices.clear();
		for(std::size_t i=0;i<points.size();i++) {
			lines.vertices.emplace_back(points[i]);
			if(i!=0 && i<points.size()-1)
				lines.indices.push_back(i);
			lines.indices.push_back(i);
		}
		lines.setBuffers();
		id=0;
		phase=0.0f;

		//update visible position
		if(isFlat)
			flatEnd.setPosition(lerp(points[id], points[id+1], phase));
		else {
			auto addVec = glm::vec3(0.f,0.f,static_cast<float>(endSize)*.5f);
			flatEnd.setPosition(lerp(points[id], points[id+1], phase)+addVec);
		}
		sphericEnd.setPosition(lerp(points[id], points[id+1], phase));
		return false;
	}
	void MullingObject::initTexture(bool isResized) {
		if(texture==UINT_MAX) {
			glGenTextures(1, &texture);
			glBindTexture(GL_TEXTURE_2D, texture);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		}
		std::vector data(divisions.x * divisions.y, 1.f);
		if(!isResized)
			glTextureSubImage2D(texture, 0, 0, 0, divisions.x, divisions.y, GL_RED, GL_FLOAT, data.data());
		else {
			glBindTexture(GL_TEXTURE_2D, texture);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_R16, divisions.x, divisions.y, 0, GL_RED, GL_FLOAT, data.data());
		}
	}
	bool MullingObject::checkMullingErrors() {
		if(points.size()<2) return false;
		//out of range - no errors
		if (std::min(std::abs(points[id].x), std::abs(points[id + 1].x)) > (size.x+endSize)*0.5f ||
			std::min(std::abs(points[id].y), std::abs(points[id + 1].y)) > (size.y+endSize)*0.5f)
			return false;
		bool ret=false;
		//check other errors
		if(std::min(points[id + 1].z,points[id].z)<baseHeight) {
			addError(DeepMulling);
			ret=true;
		}
		if(isFlat) {
			if(points[id + 1].z<points[id].z) {
				addError(DownFlatMulling);
				ret=true;
			}
		}
		else {
			if(almostEqual(points[id].x, points[id+1].x) && almostEqual(points[id].y, points[id+1].y) &&
			points[id+1].z<points[id].z) {
				addError(VerticalMulling);
				ret=true;
			}
		}
		return ret;
	}
}// namespace bf
