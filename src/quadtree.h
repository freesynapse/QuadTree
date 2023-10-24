#ifndef __QUADTREE_H
#define __QUADTREE_H


#include <vector>
#include <glm/glm.hpp>

#define MAX_DEPTH               12
#define MAX_VERTICES_PER_NODE   8
#define THETA_BH                1.0f    // ratio aabb size and between distance

//
struct AABB2
{
    //
    AABB2() :
        v0({ -1.0f, -1.0f }), v1({ 1.0f, 1.0f })
    {}
    
    AABB2(const glm::vec2 &_v0, const glm::vec2 &_v1) :
        v0(_v0), v1(_v1)
    {}

    AABB2(float _x0, float _x1, float _y0, float _y1) :
        v0(glm::vec2(_x0, _y0)), v1(glm::vec2(_x1, _y1))
    {}

    //
    bool contains(const glm::vec2 &_v)
    {
        return (_v.x >= v0.x && _v.x < v1.x && 
                _v.y >= v0.y && _v.y < v1.y);
    }

    //
    glm::vec2 midpoint() { return glm::vec2(v0 + ((v1 - v0) * 0.5f)); }
    const float size() { return (v1.x - v0.x); }

    //
    void __debug_print(const char *_id) const
    {
        SYN_DEBUG_VECTOR(_id, v0);
        SYN_DEBUG_VECTOR(_id, v1);
    }

    //
    glm::vec2 v0;
    glm::vec2 v1;

};


/* QuadtreeBH used for Barnes-Hut approximation. Inherits the base class. All sub-trees 
 * store the following:
 *  1. number of points in children (i.e. keeps track of all points 'flowing' through this node).
 *  2. a average vector of all points in this and all children of the current node, updated
 *     with every incoming point.
 * This approach is made possible through the assumption that all nodes have the same mass,
 * and thus is not subject to weighting.
 */
class QuadtreeBH
{
public:
    friend class BHRenderer;

public:
    QuadtreeBH(size_t _max_vertices, const AABB2 &_aabb=AABB2(), uint32_t _level=0);
    ~QuadtreeBH() = default;

    void destroy(QuadtreeBH *_qt);
    void insert(QuadtreeBH *_qt, const glm::vec2 &_v);
    uint32_t depth(QuadtreeBH *_qt);


    // Accessors
    const AABB2 &getAABB() { return m_aabb; }
    const std::vector<glm::vec2> &getLocalVertices() { return m_vertices; }
    const uint32_t &getLevel() { return m_level; }

    void getAABBLines(QuadtreeBH *_qt, std::vector<glm::vec2> &_out_vec_lines);
    void getVertices(QuadtreeBH *_qt, std::vector<glm::vec2> &_out_vec_points);


    // Barnes-Hut approximation ---------------------------------------------------------
    //

    // Get vertices (and masses) of the tree based on Barnes-Hut approximation for 
    // distant vertices. A 3-comp vector is used for this (for shader packing) where
    // .xy is the position of the mean vertex and .z is the mass
    void approxBH(QuadtreeBH *_qt, const glm::vec2 &_cmp_vertex, std::vector<glm::vec3> &_out_v_bh);

    // Find the closest vertex to an incoming vector (for interactive debugging)
    void getClosestVertex(QuadtreeBH *_qt, const glm::vec2 &_cmp_vertex, glm::vec2 &_out_closest);

    // Interrogate the tree for incoming vector and return AABB2
    void getSelectedAABB(QuadtreeBH *_qt, const glm::vec2 _v, AABB2 &_out_aabb);
    void getSelectedSubtree(QuadtreeBH *_qt, const glm::vec2 &_v, QuadtreeBH **_out_qt);


    // Overloads for std::shared_ptr<>
    __attribute__((always_inline)) void destroy(std::shared_ptr<QuadtreeBH> _qt) { destroy(_qt.get()); }
    __attribute__((always_inline)) void insert(std::shared_ptr<QuadtreeBH> _qt, const glm::vec2 &_v) { insert(_qt.get(), _v); }
    __attribute__((always_inline)) uint32_t depth(std::shared_ptr<QuadtreeBH> _qt) { return depth(_qt.get());  }
    __attribute__((always_inline)) void getAABBLines(std::shared_ptr<QuadtreeBH> _qt, std::vector<glm::vec2> &_out_vec_lines) { getAABBLines(_qt.get(), _out_vec_lines); }
    __attribute__((always_inline)) void getVertices(std::shared_ptr<QuadtreeBH> _qt, std::vector<glm::vec2> &_out_vec_points) { getVertices(_qt.get(), _out_vec_points); }
    __attribute__((always_inline)) void approxBH(std::shared_ptr<QuadtreeBH> _qt, 
                                                 const glm::vec2 &_cmp_vertex, 
                                                 std::vector<glm::vec3> &_out_v_bh)
    { approxBH(_qt.get(), _cmp_vertex, _out_v_bh); }
    __attribute__((always_inline)) void getClosestVertex(std::shared_ptr<QuadtreeBH> _qt, const glm::vec2 &_cmp_vertex, glm::vec2 &_out_closest)
    {
        getClosestVertex(_qt.get(), _cmp_vertex, _out_closest);
    }
    __attribute__((always_inline)) void getSelectedAABB(std::shared_ptr<QuadtreeBH> _qt, 
                                                        const glm::vec2 _v, 
                                                        AABB2 &_out_aabb) 
    { getSelectedAABB(_qt.get(), _v, _out_aabb); }
        __attribute__((always_inline)) void getSelectedSubtree(std::shared_ptr<QuadtreeBH> _qt, 
                                                           const glm::vec2 &_v, 
                                                           QuadtreeBH **_out_qt)
    { getSelectedSubtree(_qt.get(), _v, _out_qt); }



protected:
    void split(QuadtreeBH *_qt);
    uint8_t getChildIndex(QuadtreeBH *_qt, const glm::vec2 &_v);


protected:
    QuadtreeBH *m_children[4];
    AABB2 m_aabb;
    uint32_t m_level;
    std::vector<glm::vec2> m_vertices;
    size_t m_maxVertices;

    // Barnes-Hut variables
    glm::vec2 m_mean = glm::vec2(0.0f); // mean of all vertices at this node or below
    glm::vec2 m_total = glm::vec2(0.0f); // adds per incoming point
    uint32_t m_vertexCount = 0; // corresponding to the mass

};




#endif // __QUADTREE_H