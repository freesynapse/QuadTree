
#include <string.h>
#include <synapse/Debug>

#include "quadtree.h"


//
QuadtreeBH::QuadtreeBH(size_t _max_vertices, const AABB2 &_aabb, uint32_t _level)
{
    for (int i = 0; i < 4; i++)
        m_children[i] = NULL;
    m_level = _level;
    m_maxVertices = _max_vertices;
    m_vertexCount = 0;
    m_aabb = _aabb;
    m_vertices.clear();
}

//---------------------------------------------------------------------------------------
void QuadtreeBH::destroy(QuadtreeBH *_qt)
{
    if (_qt == NULL)
        return;

    for (int i = 0; i < 4; i++)
        _qt->destroy(_qt->m_children[i]);
    
    delete _qt;
    _qt = NULL;
}

//---------------------------------------------------------------------------------------
void QuadtreeBH::insert(QuadtreeBH *_qt, const glm::vec2 &_v)
{
    if (_qt->m_level > MAX_DEPTH)
        return;

    if (_qt->m_vertexCount > _qt->m_maxVertices)
    {
        SYN_WARNING("QuadtreeBH full, discarding new vertex: ", _qt->m_vertexCount, " > ", _qt->m_maxVertices);
        return;
    }

    // add to count and update mean node
    _qt->m_total += _v;
    _qt->m_vertexCount++;
    _qt->m_mean = _qt->m_total / (float)_qt->m_vertexCount;

    // tree is not split
    if (_qt->m_children[0] == NULL)
    {
        // number of vertices here is not yet at max capacity
        if (_qt->m_vertices.size() < MAX_VERTICES_PER_NODE)
            _qt->m_vertices.push_back(_v);

        // this node is full, split tree and distribute vertices accordingly
        else
        {
            _qt->m_vertices.push_back(_v);

            if (_qt->m_level != MAX_DEPTH)
            {
                _qt->split(_qt);

                //
                for (auto &v : _qt->m_vertices)
                {
                    uint8_t idx = _qt->getChildIndex(_qt, v);
                    _qt->insert(_qt->m_children[idx], v);
                }
                _qt->m_vertices.clear();
            }
        }
    }
    // tree is already split at this level, put point in correct child quandrant
    else if (_qt->m_children[0] != NULL)
    {
        uint8_t idx = _qt->getChildIndex(_qt, _v);
        _qt->insert(_qt->m_children[idx], _v);
    }

}

//---------------------------------------------------------------------------------------
uint8_t QuadtreeBH::getChildIndex(QuadtreeBH *_qt, const glm::vec2 &_v)
{
    glm::vec2 h = _qt->m_aabb.midpoint();
    return ((_v.x > h.x) + ((_v.y > h.y) << 1));
}

//---------------------------------------------------------------------------------------
void QuadtreeBH::split(QuadtreeBH *_qt)
{
    AABB2 aabb = _qt->m_aabb;
    glm::vec2 h = aabb.midpoint();
    _qt->m_children[0] = new QuadtreeBH(_qt->m_maxVertices, AABB2(aabb.v0.x, h.x, aabb.v0.y, h.y), _qt->m_level + 1);
    _qt->m_children[1] = new QuadtreeBH(_qt->m_maxVertices, AABB2(h.x, aabb.v1.x, aabb.v0.y, h.y), _qt->m_level + 1);
    _qt->m_children[2] = new QuadtreeBH(_qt->m_maxVertices, AABB2(aabb.v0.x, h.x, h.y, aabb.v1.y), _qt->m_level + 1);
    _qt->m_children[3] = new QuadtreeBH(_qt->m_maxVertices, AABB2(h.x, aabb.v1.x, h.y, aabb.v1.y), _qt->m_level + 1);
}

//---------------------------------------------------------------------------------------
void QuadtreeBH::getAABBLines(QuadtreeBH *_qt, std::vector<glm::vec2> &_out_vec_lines)
{
    // add aabb for this level
    AABB2 aabb = _qt->m_aabb;

    // no children, add bounding box
    if (_qt->m_children[0] == NULL)
    {
        // if (_qt->m_vertices.size() > 0)
        // {
            _out_vec_lines.push_back({ aabb.v0.x, aabb.v0.y });
            _out_vec_lines.push_back({ aabb.v1.x, aabb.v0.y });
            _out_vec_lines.push_back({ aabb.v0.x, aabb.v1.y });
            _out_vec_lines.push_back({ aabb.v1.x, aabb.v1.y });
            _out_vec_lines.push_back({ aabb.v0.x, aabb.v0.y });
            _out_vec_lines.push_back({ aabb.v0.x, aabb.v1.y });
            _out_vec_lines.push_back({ aabb.v1.x, aabb.v0.y });
            _out_vec_lines.push_back({ aabb.v1.x, aabb.v1.y });
        // }
    }
    else
    {
        for (int i = 0; i < 4; i++)
            _qt->getAABBLines(_qt->m_children[i], _out_vec_lines);
    }
}

//---------------------------------------------------------------------------------------
void QuadtreeBH::getVertices(QuadtreeBH *_qt, std::vector<glm::vec2> &_out_vec_points)
{
    if (_qt->m_children[0] == NULL)
        _out_vec_points.insert(_out_vec_points.end(), _qt->m_vertices.begin(), _qt->m_vertices.end());
    else
    {
        for (int i = 0; i < 4; i++)
            _qt->getVertices(_qt->m_children[i], _out_vec_points);
    }
}

//---------------------------------------------------------------------------------------
void QuadtreeBH::approxBH(QuadtreeBH *_qt, 
                          const glm::vec2 &_cmp_vertex, 
                          std::vector<glm::vec3> &_out_v_bh)
{
    // skip empty trees
    if (!_qt->m_vertexCount)
        return;

    // TODO : trivial case for when the node is in this tree?
    // TODO : check that we don't compare to itself

    float s = _qt->m_aabb.size();
    float d = glm::distance(_qt->m_mean, _cmp_vertex);
    bool is_close = s / d >= THETA_BH;

    // close with children
    if (is_close && _qt->m_children[0] != NULL)
    {
        for (int i = 0; i < 4; i++)
            _qt->m_children[i]->approxBH(_qt->m_children[i], _cmp_vertex, _out_v_bh);
    }
    
    // close but without children (leaf node) -- all vertices are relevant
    else if (is_close && _qt->m_children[0] == NULL)
    {
        for (auto &v : _qt->m_vertices)
            _out_v_bh.push_back(glm::vec3(v.x, v.y, 1.0f));
    }
    
    // sufficiently far away
    else if (!is_close)
        _out_v_bh.push_back(glm::vec3(_qt->m_mean.x, _qt->m_mean.y, (float)_qt->m_vertexCount));

}

//---------------------------------------------------------------------------------------
void QuadtreeBH::getClosestVertex(QuadtreeBH *_qt, 
                                  const glm::vec2 &_cmp_vertex, 
                                  glm::vec2 &_out_closest)
{
    float min_dist = 1e14;
    for (auto &v : _qt->m_vertices)
    {
        float dist = glm::distance(_cmp_vertex, v);
        if (dist < min_dist)
        {
            min_dist = dist;
            _out_closest = v;
        }
    }
}

//---------------------------------------------------------------------------------------
void QuadtreeBH::getSelectedAABB(QuadtreeBH *_qt, const glm::vec2 _v, AABB2 &_out_aabb)
{
    if (!_qt->m_aabb.contains(_v))
        return;

    // no children and contained here
    else if (_qt->m_children[0] == NULL)
        _out_aabb = _qt->m_aabb;

    // interrogate children
    else
    {
        for (int i = 0; i < 4; i++)
            _qt->getSelectedAABB(_qt->m_children[i], _v, _out_aabb);
    }
    
}

//---------------------------------------------------------------------------------------
void QuadtreeBH::getSelectedSubtree(QuadtreeBH *_qt, 
                                    const glm::vec2& _v, 
                                    QuadtreeBH **_out_qt)
{
    if (!_qt->m_aabb.contains(_v))
        return;

    // no children and contained here
    else if (_qt->m_children[0] == NULL)
        *_out_qt = _qt;

    // interrogate children
    else
    {
        for (int i = 0; i < 4; i++)
            _qt->getSelectedSubtree(_qt->m_children[i], _v, _out_qt);
    }
}

//---------------------------------------------------------------------------------------
uint32_t QuadtreeBH::depth(QuadtreeBH *_qt)
{
    if (_qt->m_children[0] != NULL)
    {
        uint32_t d = 0;
        for (int i = 0; i < 4; i++)
            d = std::max(d, _qt->depth(_qt->m_children[i]));
        return std::max(d, _qt->m_level);
    }
    // leaf node
    else
        return _qt->m_level;

}



