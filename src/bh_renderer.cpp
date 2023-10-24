
#include <synapse/Synapse>

#include "bh_renderer.h"
#include "quadtree.h"


//
BHRenderer::BHRenderer(std::shared_ptr<QuadtreeBH> &_qt)
{
    //
    m_shader = ShaderLibrary::load("../assets/shaders/debug.glsl");
    m_qt = _qt;

    m_lim = m_qt->getAABB();
    m_inv_range = 1.0f / (m_lim.v1 - m_lim.v0);

    Renderer::get().enableGLenum(GL_POINT_SMOOTH);
    Renderer::get().disableGLenum(GL_PROGRAM_POINT_SIZE);

    EventHandler::register_callback(EventType::VIEWPORT_RESIZE, 
                                    SYN_EVENT_MEMBER_FNC(BHRenderer::initializeGeometry));
}

//---------------------------------------------------------------------------------------
void BHRenderer::initializeGeometry(Event *_e)
{
    if (!m_qt)
    {
        SYN_WARNING("invalid QuadtreeBH");
        return;
    }

    ViewportResizeEvent *e = dynamic_cast<ViewportResizeEvent *>(_e);

    //
    std::vector<glm::vec2> vertices;
    m_qt->getVertices(m_qt, vertices);
    m_vertexCount = vertices.size();

    // vertices of tree data
    Ref<VertexBuffer> vbo0 = API::newVertexBuffer(GL_DYNAMIC_DRAW);
    vbo0->setData(vertices.data(), sizeof(glm::vec2) * m_qt->m_maxVertices);
    vbo0->setBufferLayout({
        { VERTEX_ATTRIB_LOCATION_POSITION, ShaderDataType::Float2, "a_position" },
    });
    m_verticesVAO = API::newVertexArray(vbo0);
    
    //
    std::vector<glm::vec2> aabbs;
    m_qt->getAABBLines(m_qt, aabbs);
    m_aabbCount = aabbs.size();

    // lines for AABBs
    Ref<VertexBuffer> vbo1 = API::newVertexBuffer(GL_DYNAMIC_DRAW);
    vbo1->setData(aabbs.data(), sizeof(glm::vec2) * pow(2, MAX_DEPTH) * pow(2, MAX_DEPTH) * 2);
    vbo1->setBufferLayout({
        { VERTEX_ATTRIB_LOCATION_POSITION, ShaderDataType::Float2, "a_position" },
    });
    m_aabbVAO = API::newVertexArray(vbo1);

    // prepare AABB highlight
    m_highlightAABB_VBO = API::newVertexBuffer(GL_DYNAMIC_DRAW);
    m_highlightAABB_VBO->setData(NULL, sizeof(glm::vec2) * 6);
    m_highlightAABB_VBO->setBufferLayout({
        { VERTEX_ATTRIB_LOCATION_POSITION, ShaderDataType::Float2, "a_position" },
    });
    m_highlightAABB_VAO = API::newVertexArray(m_highlightAABB_VBO);

    // prepare vertex highlight
    m_highlightVertex_VBO = API::newVertexBuffer(GL_DYNAMIC_DRAW);
    m_highlightVertex_VBO->setData(NULL, sizeof(glm::vec2));
    m_highlightVertex_VBO->setBufferLayout({
        { VERTEX_ATTRIB_LOCATION_POSITION, ShaderDataType::Float2, "a_position" },
    });
    m_highlightVertex_VAO = API::newVertexArray(m_highlightVertex_VBO);

    // prepare for BH vertices, dimensioning VBO size
    m_BH_verticesVBO = API::newVertexBuffer(GL_DYNAMIC_DRAW);
    m_BH_verticesVBO->setData(NULL, sizeof(glm::vec2) * m_qt->m_maxVertices);
    m_BH_verticesVBO->setBufferLayout({
        { VERTEX_ATTRIB_LOCATION_POSITION, ShaderDataType::Float2, "a_position" },
    });
    m_BH_verticesVAO = API::newVertexArray(m_BH_verticesVBO);

    //
    m_geometry_set = true;
}

//---------------------------------------------------------------------------------------
void BHRenderer::render(const Ref<OrthographicCamera> &_camera)
{
    if (!m_geometry_set)
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
        m_shader->setUniform4fv("u_color", { 0.7f, 0.7f, 0.7f, 1.0f });
        renderer.drawArrays(m_aabbVAO, m_aabbCount, 0, false, GL_LINES);
    }


    // Vertices rendering
    //
    glm::vec4 vcolor = { 0.7f, 0.55f, 0.15f, 1.0f };
    if (m_renderBH) vcolor.a = 0.5f;

    // All vertices
    m_shader->setUniform4fv("u_color", vcolor);
    renderer.drawArrays(m_verticesVAO, m_vertexCount, 0, false, GL_POINTS);

    // Highlight closest vertex
    if (m_renderHighlightVertex && m_highlightVertex_VAO != nullptr)
    {
        glPointSize(m_pointSize + 10.0f);
        m_shader->setUniform4fv("u_color", { 0.9f, 0.9f, 0.9f, 0.6f });
        renderer.drawArrays(m_highlightVertex_VAO, 1, 0, true, GL_POINTS);
        glPointSize(m_pointSize);
    }

    // Render according to Barnes-Hut
    if (m_BH_verticesVAO != nullptr && m_renderBH)
    {
        glPointSize(m_pointSize + 10.0f);
        m_shader->setUniform4fv("u_color", { 1.0f, 1.0f, 1.0f, 1.0f });
        renderer.drawArrays(m_BH_verticesVAO, m_BH_vertexCount, 0, true, GL_POINTS);
        glPointSize(m_pointSize);
    }

    m_shader->disable();

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

        m_highlightAABB_VAO->bind();
        m_highlightAABB_VBO->updateBufferData(&(vs[0]), sizeof(glm::vec2) * 6, 0);
        m_highlightAABB_VAO->unbind();
    }
}

//---------------------------------------------------------------------------------------
void BHRenderer::highlightVertex(const glm::vec2 &_v)
{
    if (m_highlightVertex_VAO != nullptr)
    {
        float v[2] = { _v.x, _v.y };
        m_highlightVertex_VAO->bind();
        m_highlightVertex_VBO->updateBufferData(&v, sizeof(glm::vec2), 0);
        m_highlightVertex_VAO->unbind();
    }
}

//---------------------------------------------------------------------------------------
void BHRenderer::highlightBH(std::vector<VertexBH> &_bh_vertices)
{
    if (m_BH_verticesVAO != nullptr)
    {
        m_BH_vertexCount = _bh_vertices.size();
        m_BH_verticesVAO->bind();
        m_BH_verticesVBO->updateBufferData(&(_bh_vertices)[0], 
                                           sizeof(glm::vec2) * m_BH_vertexCount,
                                           0);
        m_BH_verticesVAO->unbind();
    }
}


