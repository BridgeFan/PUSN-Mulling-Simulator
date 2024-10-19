//
// Created by kamil-hp on 16.03.2022.
//

#include "Solid.h"
#include <GL/glew.h>
#include "src/Shader/ShaderArray.h"
#include <OpenGLUtil.h>
int bf::Solid::sindex = 1;
unsigned oldVerticesSize = 0;

bf::Solid::~Solid() {
	glDeleteVertexArrays(1, &VAO);
	glDeleteBuffers(1, &VBO);
	glDeleteBuffers(1, &IBO);
}

void bf::Solid::setBuffers() {
    oldVerticesSize = vertices.size();
	//remove old ones
    if(VAO==UINT_MAX) {
        if(indices.empty() || vertices.empty()) {
            return;
        }
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
		// position attribute
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), reinterpret_cast<void*>(0));
		glEnableVertexAttribArray(0);
		// texture coord attribute
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), reinterpret_cast<void*>(3 * sizeof(float)));
		glEnableVertexAttribArray(1);
		// normal coord attribute
		glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), reinterpret_cast<void*>(5 * sizeof(float)));
		glEnableVertexAttribArray(2);
        int usage = isDynamic ? GL_DYNAMIC_DRAW : GL_STATIC_DRAW;
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), usage);
        glGenBuffers(1, &IBO);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned), &indices[0], usage);
    }
    else {
        bf::gl::namedBufferData(VBO, vertices, isDynamic);
        bf::gl::namedBufferData(IBO, indices, isDynamic);
    }
}

void bf::Solid::draw(const bf::ShaderArray& shaderArray) const {
    if(shaderArray.getActiveIndex()!=bf::ShaderType::BasicShader)
        return;
    anyDraw(shaderArray);
}

void bf::Solid::anyDraw(const bf::ShaderArray& shaderArray/*, const bf::Transform& relativeTo*/) const {
    if(indices.empty() || vertices.empty())
        return;
    //function assumes set projection and view matrices
    glBindVertexArray(VAO);
    shaderArray.getActiveShader().setMat4("model", getModelMatrix(/*relativeTo*/));
    glDrawElements(GL_LINES, indices.size(), GL_UNSIGNED_INT,   // type
                   reinterpret_cast<void*>(0)           // element array buffer offset
    );
}

void bf::Solid::ObjectGui() {
	Object::ObjectGui();
}

void bf::Solid::addVertex(const glm::vec3 &p) {
	vertices.emplace_back(p);
}

bf::ShaderType bf::Solid::getShaderType() const {
    return BasicShader;
}

bf::Solid::Solid(bf::Solid &&solid) noexcept {
    swapSolids(*this, solid);
}

void bf::Solid::swapSolids(bf::Solid &a, bf::Solid &b) {
    if(&a==&b)
        return;
    std::swap(a.VAO, b.VAO);
    std::swap(a.VBO, b.VBO);
    std::swap(a.IBO, b.IBO);
    std::swap(a.indices, b.indices);
    std::swap(a.vertices, b.vertices);
    std::swap(a.isDynamic, b.isDynamic);
    std::swap(a.indestructibilityIndex, b.indestructibilityIndex);
    std::swap(a.transform, b.transform);
    std::swap(a.name, b.name);
}

void bf::Solid::glUpdateVertices() const {
    if(oldVerticesSize!=vertices.size()) {
        bf::gl::namedBufferData(VBO, vertices, isDynamic);
        oldVerticesSize=vertices.size();
    }
    else {
        bf::gl::namedBufferSubData(VBO, vertices, 0, vertices.size());
    }
}

bf::Vertex::Vertex(float _x, float _y, float _z, float _tX, float _tY, float _nX, float _nY, float _nZ) noexcept : x(_x), y(_y), z(_z), tX(_tX), tY(_tY),
			nX(_nX), nY(_nY), nZ(_nZ) {}

bf::Vertex::Vertex(const glm::vec3 &p, const glm::vec2 &t, const glm::vec3 &n) noexcept : Vertex(p.x,p.y,p.z,t.x,t.y, n.x, n.y, n.z) {

}

void bf::Vertex::setPosition(const glm::vec3 &p) noexcept {
    x = p.x; y = p.y; z = p.z;
}

void bf::Vertex::setTexturePosition(const glm::vec2 &t) noexcept {
    tX = t.x; tY = t.y;
}

bf::DummySolid::DummySolid(const std::string &solidName, bool dynamic) : Solid(solidName, dynamic) {}

bf::DummySolid &bf::DummySolid::operator=(bf::DummySolid &&solid) noexcept {
    swapSolids(*this, solid);
    return *this;
}
void bf::DummySolid::drawTriangles(const ShaderArray& shaderArray) const {
	if(shaderArray.getActiveIndex()!=bf::ShaderType::BasicShader || indices.empty() || vertices.empty())
		return;
	//function assumes set projection and view matrices
	glBindVertexArray(VAO);
	shaderArray.getActiveShader().setMat4("model", getModelMatrix(/*relativeTo*/));
	glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, reinterpret_cast<void*>(0));
}
