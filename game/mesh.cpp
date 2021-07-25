#include "mesh.h"

namespace {

struct VAOBinder : GX::NonCopyable {
    VAOBinder(GLuint vao)
    {
        glBindVertexArray(vao);
    }

    ~VAOBinder()
    {
        glBindVertexArray(0);
    }
};

} // namespace

Mesh::Mesh(GLenum primitive)
    : m_primitive(primitive)
{
}

Mesh::~Mesh()
{
    glDeleteBuffers(1, &m_vertexBuffer);
    glDeleteBuffers(1, &m_indexBuffer);
    glDeleteVertexArrays(1, &m_vertexArray);
}

void Mesh::setVertexCount(unsigned count)
{
    m_vertexCount = count;
}

void Mesh::setVertexSize(unsigned size)
{
    m_vertexSize = size;
}

void Mesh::setIndexCount(unsigned count)
{
    m_indexCount = count;
}

void Mesh::setVertexAttributes(const std::vector<VertexAttribute> &attributes)
{
    m_attributes = attributes;
}

void Mesh::initialize()
{
    assert(m_vertexCount > 0);
    assert(m_vertexSize > 0);
    assert(!m_attributes.empty());

    glGenBuffers(1, &m_vertexBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, m_vertexBuffer);
    glBufferData(GL_ARRAY_BUFFER, m_vertexSize * m_vertexCount, nullptr, GL_STATIC_DRAW);

    if (m_indexCount > 0) {
        glGenBuffers(1, &m_indexBuffer);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_indexBuffer);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(IndexType) * m_indexCount, nullptr, GL_STATIC_DRAW);
    }

    glGenVertexArrays(1, &m_vertexArray);

    VAOBinder vaoBinder(m_vertexArray);
    glBindBuffer(GL_ARRAY_BUFFER, m_vertexBuffer);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_indexBuffer);

    int index = 0;
    for (const auto &attribute : m_attributes) {
        glEnableVertexAttribArray(index);
        glVertexAttribPointer(index, attribute.componentCount, attribute.type, GL_FALSE, m_vertexSize, reinterpret_cast<GLvoid *>(attribute.offset));
        ++index;
    }
}

void Mesh::setVertexData(const void *data)
{
    assert(m_vertexBuffer != 0);
    glBindBuffer(GL_ARRAY_BUFFER, m_vertexBuffer);
    glBufferSubData(GL_ARRAY_BUFFER, 0, m_vertexSize * m_vertexCount, data);
}

void Mesh::setIndexData(const void *data)
{
    assert(m_indexBuffer != 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_indexBuffer);
    glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, sizeof(IndexType) * m_indexCount, data);
}

void Mesh::render() const
{
    VAOBinder vaoBinder(m_vertexArray);
    if (m_indexBuffer != 0)
        glDrawElements(m_primitive, m_indexCount, GL_UNSIGNED_INT, nullptr);
    else
        glDrawArrays(m_primitive, 0, m_vertexCount);
}
