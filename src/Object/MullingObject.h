//
// Created by kamil-hp on 23.10.23.
//

#ifndef MKWMG_MULLINGOBJECT_H
#define MKWMG_MULLINGOBJECT_H
#include "Object.h"
#include "Solids/Solid.h"

#include <map>

namespace bf {
	enum MullingErrorType: uint_fast8_t {
		DeepMulling,
		VerticalMulling,
		DownFlatMulling,
		NonCuttingMulling
	};

	class MullingObject: public bf::Solid {
		unsigned texture=UINT_MAX;
		glm::vec3 size={150.f,150.f,50.f};
		glm::vec<2, int> divisions={600,600};
		std::vector<std::vector<float> > heights;
		static constexpr int stepsPerFrame = 10;
		static constexpr unsigned instantUpdateStep = 10;
		bf::DummySolid lines;
		bf::DummySolid flatEnd, sphericEnd;
		bool isLineShown=true;
		std::map<unsigned, std::string> errors;
		float getHazardDepth(int i, int j);
		//transform.position - position of end
		int endSize=10;
		float endHeight=30.f;
		glm::vec3 endPosition;
		bool isFlat;
		bool isRunning=false;
		float simSpeed=1.f;
		float baseHeight=15.f;
		unsigned id=0;
		float phase=0.0f;
		bool isInstantProcessed=false;
		std::vector<glm::vec3> points;
		void updateVertices(bool isBufferResized);
		void initTexture(bool isResized=false);
		bool checkMullingErrors();
		void updateHeightMap(int beginX, int endX, int beginY, int endY);
		void addError(MullingErrorType type);
		void endAnimation();
	public:
		MullingObject();
		~MullingObject() override = default;
		void draw(const ShaderArray &shader) const override;
		void onMergePoints(int p1, int p2) override;
		[[nodiscard]] ShaderType getShaderType() const override;
		void ObjectGui() override;
		[[nodiscard]] bool isMovable() const override;
		[[nodiscard]] bool isIntersectable() const override;
		void processNextInstantPoint();
		bool loadFromFile(const std::string& path);
		void update(float deltaTime);
		void internalUpdate(float deltaTime);

	};

}// namespace bf

#endif//MKWMG_MULLINGOBJECT_H
