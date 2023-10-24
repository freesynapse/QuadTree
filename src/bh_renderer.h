#ifndef __BH_RENDERER_H
#define __BH_RENDERER_H

#include <synapse/Renderer>

#include "quadtree.h"

using namespace Syn;


class BHRenderer
{
public:
    BHRenderer(Ref<QuadtreeBH> &_qt);
    ~BHRenderer() = default;

    void initializeGeometry(Event *_e);
    void render(const Ref<OrthographicCamera> &_camera);

    void toggleAABB() { m_renderAABB = !m_renderAABB; }
    void toggleRenderBH() { m_renderBH = !m_renderBH; }
    void toggleHighlightAABB() { m_renderHighlightAABB = !m_renderHighlightAABB; }
    void toggleHighlightVertex() { m_renderHighlightVertex = !m_renderHighlightVertex; }

    bool getRenderAABB() { return m_renderAABB; }
    bool getRenderBH() { return m_renderBH; }
    bool getRenderHighlightAABB() { return m_renderHighlightAABB; }
    bool getRenderHighlightVertex() { return m_renderHighlightVertex; }

    void highlightAABB(const AABB2 &_aabb);
    void highlightVertex(const glm::vec2 &_v);
    void highlightBH(std::vector<VertexBH> &_bh_vertices);


private:
    //
    Ref<QuadtreeBH> m_qt = nullptr;
    Ref<Shader> m_shader;

    bool m_geometry_set = false;
    float m_pointSize = 5.0f;
    
    // geometry
    AABB2 m_lim;
    glm::vec2 m_inv_range;

    // points
    size_t m_vertexCount = 0;
    Ref<VertexArray> m_verticesVAO;
    
    // AABBs
    bool m_renderAABB = true;
    size_t m_aabbCount = 0;
    Ref<VertexArray> m_aabbVAO;

    // selected AABB
    bool m_renderHighlightAABB = false;
    AABB2 m_highlightAABB;
    Ref<VertexBuffer> m_highlightAABB_VBO = nullptr;
    Ref<VertexArray> m_highlightAABB_VAO = nullptr;

    // selected vertex
    bool m_renderHighlightVertex = true;
    Ref<VertexBuffer> m_highlightVertex_VBO = nullptr;
    Ref<VertexArray> m_highlightVertex_VAO = nullptr;

    // BH vertices
    bool m_renderBH = false;
    size_t m_BH_vertexCount = 0;
    Ref<VertexBuffer> m_BH_verticesVBO = nullptr;
    Ref<VertexArray> m_BH_verticesVAO = nullptr;

};



#endif // __BH_RENDERER_H
