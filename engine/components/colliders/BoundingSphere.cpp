#pragma once

#include "BoundingSphere.h"

bool BoundingSphere::isCollidingWith(const BoundingSphere &otherSphere) const  {
    float distance = glm::distance(m_center, otherSphere.m_center);
    float sumRadii = m_radius + otherSphere.m_radius;
    return distance <= sumRadii;
}

glm::vec3 BoundingSphere::getClosestPoint(const glm::vec3 &point) const {
    glm::vec3 direction = point - m_center;
    float distance = glm::length(direction);

    // If the point is inside the m_sphere, return the m_center
    if (distance < m_radius) {
        return m_center;
    }

    // Otherwise, project the point onto the m_sphere's surface
    return m_center + direction * (m_radius / distance);
}

bool BoundingSphere::isCollidingWith(const TerrainMesh &terrain) const {

    /// subdivide the terrain mesh using octree
    /// actually - all meshes can use octree to partition the faces and vertices


    // Check if the m_sphere is completely outside the bounding box of the mesh
//    if (m_center.x - m_radius > terrain.max_x ||
//        m_center.x + m_radius < terrain.min_x ||
//        m_center.y - m_radius > terrain.max_y ||
//        m_center.y + m_radius < terrain.min_y ||
//        m_center.z - m_radius > terrain.max_z ||
//        m_center.z + m_radius < terrain.min_z) {
//        return false;
//    }

    // Check if the m_sphere intersects any of the triangles in the mesh
//    for (const auto &face : terrain.faces) {
//        // Calculate the normal vector of the face
//        glm::vec3 v1 = terrain.vertices[face[0]];
//        glm::vec3 v2 = terrain.vertices[face[1]];
//        glm::vec3 v3 = terrain.vertices[face[2]];
//        glm::vec3 normal = glm::normalize(glm::cross(v2 - v1, v3 - v1));
//
//        // Calculate the distance between the m_center of the m_sphere and the plane of the face
//        float distance = glm::dot(normal, m_center - v1);
//
//        // If the m_sphere is on the opposite side of the plane from the normal vector, it cannot intersect the face
//        if (distance > m_radius) {
//            continue;
//        }
//
//        // Project the m_sphere's m_center onto the plane of the face
//        glm::vec3 projected_center = m_center - distance * normal;
//
//        // Calculate the barycentric coordinates of the projected m_center in relation to the face
//        glm::vec3 barycentric = glm::inverse(glm::mat3(v1, v2, v3)) * projected_center;
//
//        // If the projected m_center is inside the triangle and closer than the m_sphere m_radius, there is a collision
//        if (barycentric.x >= 0 && barycentric.x <= 1 &&
//            barycentric.y >= 0 && barycentric.y <= 1 &&
//            barycentric.z >= 0 && barycentric.z <= 1 &&.
//            glm::distance(projected_center, m_center) <= m_radius) {
//            return true;
//        }
//    }

    return false;
}

float BoundingSphere::getRadius() const {
    return m_radius;
}

void BoundingSphere::setRadius(float radius) {
    BoundingSphere::m_radius = radius;
}

const glm::vec3 &BoundingSphere::getCenter() const {
    return m_center;
}

void BoundingSphere::setCenter(const glm::vec3 &center) {
    BoundingSphere::m_center = center;
}

void BoundingSphere::findRadiusAndCenter(const std::vector<glm::vec3> &vertices) {
    if (vertices.empty()) {
        m_center = glm::vec3(0.0f);
        m_radius = 0.0f;
        return;
    }

    glm::vec3 sum(0.0f);
    for (const auto& vertex : vertices) {
        sum += vertex;
    }

    m_center = sum / static_cast<float>(vertices.size());

    float maxDistanceSq = 0.0f;
    for (const auto& vertex : vertices) {
        float distanceSq = glm::distance(vertex, m_center);
        distanceSq = distanceSq * distanceSq;
        if (distanceSq > maxDistanceSq) {
            maxDistanceSq = distanceSq;
        }
    }

    m_radius = std::sqrt(maxDistanceSq);
}

void BoundingSphere::moveCenter(const glm::vec3 &offset) {
    this->m_center+=offset;
}
