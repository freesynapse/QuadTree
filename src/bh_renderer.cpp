
#include <synapse/Synapse>

#include "bh_renderer.h"
#include "quadtree.h"


//
BHRenderer::BHRenderer(std::shared_ptr<QuadtreeBH> &_qt)
{
    //
    m_shader = ShaderLibrary::load("../assets/shaders/quadtree.glsl");
    m_BHshader = ShaderLibrary::load("../assets/shaders/BH_node_shader.glsl");
    
    //
    m_qt = _qt;

    // glPointSize(m_defaultPointSize);

    Renderer::get().enableGLenum(GL_POINT_SMOOTH);
    // Renderer::get().disableGLenum(GL_PROGRAM_POINT_SIZE);

    EventHandler::register_callback(EventType::VIEWPORT_RESIZE, 
                                    SYN_EVENT_MEMBER_FNC(BHRenderer::viewportResizeCallback));

    initializeGeometry();
}

//---------------------------------------------------------------------------------------
void BHRenderer::viewportResizeCallback(Event *_e)
{
    ViewportResizeEvent *e = dynamic_cast<ViewportResizeEvent *>(_e);
    m_viewportSz = e->getViewport();

    //
    // initializeGeometry();
    updateGeometry();
}

//---------------------------------------------------------------------------------------
void BHRenderer::initializeGeometry()
{
    if (!m_qt)
    {
        SYN_WARNING("invalid QuadtreeBH");
        return;
    }

    BufferLayout default_layout({{ VERTEX_ATTRIB_LOCATION_POSITION, 
                                   ShaderDataType::Float2, 
                                   "a_position" }});

    //
    // std::vector<glm::vec2> vertices;
    // m_qt->getVertices(m_qt, vertices);
    // m_vertexCount = vertices.size();

    // vertices of tree data
    m_verticesVBO = API::newVertexBuffer(GL_DYNAMIC_DRAW);
    m_verticesVBO->setData(/*vertices.data()*/NULL, sizeof(glm::vec2) * m_qt->m_maxVertices);
    m_verticesVBO->setBufferLayout(default_layout);
    m_verticesVAO = API::newVertexArray(m_verticesVBO);
    
    //
    // std::vector<glm::vec2> aabbs;
    // m_qt->getAABBLines(m_qt, aabbs);
    // m_aabbCount = aabbs.size();
    m_maxAABBCount = 8 * m_qt->m_maxVertices;

    // lines for AABBs
    m_aabbVBO = API::newVertexBuffer(GL_DYNAMIC_DRAW);
    m_aabbVBO->setData(/*aabbs.data()*/ NULL, sizeof(glm::vec2) * m_maxAABBCount);
    m_aabbVBO->setBufferLayout(default_layout);
    m_aabbVAO = API::newVertexArray(m_aabbVBO);

    // prepare AABB highlight
    m_highlightAABB_VBO = API::newVertexBuffer(GL_DYNAMIC_DRAW);
    m_highlightAABB_VBO->setData(NULL, sizeof(glm::vec2) * 6);
    m_highlightAABB_VBO->setBufferLayout(default_layout);
    m_highlightAABB_VAO = API::newVertexArray(m_highlightAABB_VBO);

    // prepare vertex highlight
    m_highlightVertex_VBO = API::newVertexBuffer(GL_DYNAMIC_DRAW);
    m_highlightVertex_VBO->setData(NULL, sizeof(glm::vec2));
    m_highlightVertex_VBO->setBufferLayout(default_layout);
    m_highlightVertex_VAO = API::newVertexArray(m_highlightVertex_VBO);

    // prepare for BH vertices, dimensioning VBO size
    m_BH_verticesVBO = API::newVertexBuffer(GL_DYNAMIC_DRAW);
    m_BH_verticesVBO->setData(NULL, sizeof(glm::vec3) * m_qt->m_maxVertices);
    m_BH_verticesVBO->setBufferLayout({
        { VERTEX_ATTRIB_LOCATION_POSITION, ShaderDataType::Float3, "a_position" },
    });
    m_BH_verticesVAO = API::newVertexArray(m_BH_verticesVBO);

    //
    m_buffersInitialized = true;
}

//---------------------------------------------------------------------------------------
void BHRenderer::updateGeometry()
{
    // vertices (data)
    if (m_verticesVAO != nullptr)
    {
        // get vertices from tree
        std::vector<glm::vec2> vertices;
        m_qt->getVertices(m_qt, vertices);
        m_vertexCount = vertices.size();
        
        m_verticesVBO->updateBufferData(vertices.data(), sizeof(glm::vec2) * m_vertexCount, 0);
    }

    // AABB
    if (m_aabbVAO != nullptr)
    {
        // get aabbs from tree
        std::vector<glm::vec2> aabbs;
        m_qt->getAABBLines(m_qt, aabbs);
        m_aabbCount = aabbs.size();

        m_aabbVBO->updateBufferData(aabbs.data(), sizeof(glm::vec2) * m_aabbCount, 0);
    }

}

//---------------------------------------------------------------------------------------
void BHRenderer::render(const Ref<OrthographicCamera> &_camera)
{
    if (!m_buffersInitialized)
        return;

    static auto &renderer = Renderer::get();

    m_shader->enable();
    m_shader->setMatrix4fv("u_view_projection_matrix", _camera->getViewProjectionMatrix());

    renderer.enableBlending();

    // AABB rendering
    //
    if (m_highlightAABB_VAO != nullptr && m_renderHighlightAABB)
    {
        m_shader->setUniform4fv("u_color", { 1.0f, 0.0f, 0.0f, 0.2f });
        renderer.drawArrays(m_highlightAABB_VAO, 8, 0, false, GL_TRIANGLES);
    }

    // all AABBs
    if (m_renderAABB)
    {
        if (m_aabbCount > m_maxAABBCount)
        {
            SYN_WARNING("m_aabbCount > m_maxAABBCount");
        }
        else
        {
            m_shader->setUniform4fv("u_color", { 0.7f, 0.7f, 0.7f, 1.0f });
            renderer.drawArrays(m_aabbVAO, m_aabbCount, 0, false, GL_LINES);
        }
    }


    // Vertices rendering
    //
    glm::vec4 vcolor = { 0.7f, 0.55f, 0.15f, 1.0f };
    // if (m_renderBH) vcolor.a = 0.5f;

    // All vertices
    m_shader->setUniform1f("u_point_size", m_defaultPointSize);
    m_shader->setUniform1f("u_zoom_level", _camera->getZoomLevel());
    m_shader->setUniform4fv("u_color", vcolor);
    renderer.drawArrays(m_verticesVAO, m_vertexCount, 0, false, GL_POINTS);

    // Highlight closest vertex
    if (m_renderHighlightVertex && m_highlightVertex_VAO != nullptr)
    {
        // glPointSize(m_defaultPointSize + 10.0f);
        m_shader->setUniform4fv("u_color", { 0.9f, 0.9f, 0.9f, 0.6f });
        renderer.drawArrays(m_highlightVertex_VAO, 1, 0, true, GL_POINTS);
        // glPointSize(m_defaultPointSize);
    }

    // m_shader->disable();

    // Render according to Barnes-Hut
    if (m_renderBH && m_BH_verticesVAO != nullptr)
    {
        m_BHshader->enable();
        m_BHshader->setMatrix4fv("u_view_projection_matrix", _camera->getViewProjectionMatrix());
        m_BHshader->setUniform1f("u_point_scale", m_defaultPointSize);
        m_BHshader->setUniform1f("u_zoom_level", _camera->getZoomLevel());
        
        // renderer.enableGLenum(GL_PROGRAM_POINT_SIZE);
        m_BHshader->setUniform4fv("u_color", { 1.0f, 1.0f, 1.0f, 0.3f });
        renderer.drawArrays(m_BH_verticesVAO, m_BH_vertexCount, 0, true, GL_POINTS);
        // renderer.disableGLenum(GL_PROGRAM_POINT_SIZE);

    }


}

//---------------------------------------------------------------------------------------
void BHRenderer::highlightAABB(const AABB2 &_aabb)
{
    //
    if (m_highlightAABB_VAO != nullptr)
    {
        glm::vec2 v0(_aabb.v0), v1(_aabb.v1);
        glm::vec2 vs[] = {
            { v0.x, v0.y }, // 0
            { v1.x, v0.y }, // 1
            { v1.x, v1.y }, // 2
            { v1.x, v1.y }, // 2
            { v0.x, v1.y }, // 3
            { v0.x, v0.y }, // 0
        };

        m_highlightAABB_VBO->updateBufferData(&(vs[0]), sizeof(glm::vec2) * 6, 0);
    }
}

//---------------------------------------------------------------------------------------
void BHRenderer::highlightVertex(const glm::vec2 &_v)
{
    if (m_highlightVertex_VAO != nullptr)
    {
        float v[2] = { _v.x, _v.y };
        m_highlightVertex_VBO->updateBufferData(&v, sizeof(glm::vec2), 0);
    }
}

//---------------------------------------------------------------------------------------
void BHRenderer::highlightBH(std::vector<glm::vec3> &_bh_vertices)
{
    if (m_BH_verticesVAO != nullptr)
    {
        m_BH_vertexCount = _bh_vertices.size();
        // if (__debug_prev_bh_count != m_BH_vertexCount)
        // {
            // SYN_TRACE("BH vertex count: ", m_BH_vertexCount);
            // for (auto &v : _bh_vertices)
            // {
                // SYN_TRACE("bh v: ", v.x, ", ", v.y, ", mass=", v.z);
            // }
            // __debug_prev_bh_count = m_BH_vertexCount;
        // }
        m_BH_verticesVBO->updateBufferData(&(_bh_vertices)[0], 
                                           sizeof(glm::vec3) * m_BH_vertexCount,
                                           0);
    }
}






