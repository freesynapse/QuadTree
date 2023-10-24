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

    void viewportResizeCallback(Event *_e);
    void initializeGeometry();
    void updateGeometry();  // called after QuadtreeBH->insert():s    
    void render(const Ref<OrthographicCamera> &_camera);

    // geometry update functions called from main
    void highlightAABB(const AABB2 &_aabb);
    void highlightVertex(const glm::vec2 &_v);
    void highlightBH(std::vector<glm::vec3> &_bh_vertices);

    // accessors
    void toggleAABB() { m_renderAABB = !m_renderAABB; }
    void toggleRenderBH() { m_renderBH = !m_renderBH; }
    void toggleHighlightAABB() { m_renderHighlightAABB = !m_renderHighlightAABB; }
    void toggleHighlightVertex() { m_renderHighlightVertex = !m_renderHighlightVertex; }

    bool getRenderAABB() { return m_renderAABB; }
    bool getRenderBH() { return m_renderBH; }
    bool getRenderHighlightAABB() { return m_renderHighlightAABB; }
    bool getRenderHighlightVertex() { return m_renderHighlightVertex; }

    size_t getTotalVertexCount() { return m_vertexCount; }
    size_t getBHVertexCount() { return m_BH_vertexCount; }




private:
    //
    Ref<QuadtreeBH> m_qt = nullptr;
    Ref<Shader> m_shader = nullptr;
    Ref<Shader> m_BHshader = nullptr;

    glm::ivec2 m_viewportSz = { 0, 0 };

    bool m_buffersInitialized = false;
    float m_defaultPointSize = 4.0f;
    
    // 2d vertices (ie the data)
    size_t m_vertexCount = 0;
    Ref<VertexBuffer> m_verticesVBO;
    Ref<VertexArray> m_verticesVAO;
    
    // tree AABBs
    bool m_renderAABB = true;
    size_t m_maxAABBCount;
    size_t m_aabbCount = 0;
    Ref<VertexBuffer> m_aabbVBO;
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
    size_t __debug_prev_bh_count = 0;
    Ref<VertexBuffer> m_BH_verticesVBO = nullptr;
    Ref<VertexArray> m_BH_verticesVAO = nullptr;

};



#endif // __BH_RENDERER_H
