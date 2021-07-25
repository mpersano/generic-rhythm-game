#pragma once

#include <gx/noncopyable.h>

#include <GL/glew.h>
#include <glm/glm.hpp>

#include <memory>
#include <vector>

class Mesh : private GX::NonCopyable
{
public:
    using IndexType = unsigned;

    explicit Mesh(GLenum primitive = GL_TRIANGLES);
    ~Mesh();

    void setVertexCount(unsigned count);
    void setVertexSize(unsigned size);
    void setIndexCount(unsigned count);
    struct VertexAttribute {
        unsigned componentCount;
        GLenum type;
        unsigned offset;
    };
    void setVertexAttributes(const std::vector<VertexAttribute> &attributes);

    void initialize();
    void setVertexData(const void *data); // is this polymorphism?
    void setIndexData(const void *data);

    void render() const;

private:
    GLenum m_primitive;
    unsigned m_vertexCount = 0;
    unsigned m_vertexSize = 0;
    unsigned m_indexCount = 0;
    std::vector<VertexAttribute> m_attributes;
    GLuint m_vertexBuffer = 0;
    GLuint m_indexBuffer = 0;
    GLuint m_vertexArray = 0;
};
