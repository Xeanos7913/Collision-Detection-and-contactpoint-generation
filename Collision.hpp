#include <glm.hpp>
#include <gtc/matrix_transform.hpp>
#include <gtx/string_cast.hpp>
#include <iostream>
#include <vector>
#include <cmath>
#include <limits>
#include <string>

struct OBB {
    glm::vec3 center;      // Center of the OBB
    glm::vec3 halfExtents; // Half-sizes along each axis
    glm::vec3 axes[3];     // Local x, y, z axes (unit vectors)

    // Transforms OBB using a model matrix
    void transform(const glm::mat4& model) {
        center = glm::vec3(model * glm::vec4(center, 1.0f));
        axes[0] = glm::normalize(glm::vec3(model * glm::vec4(axes[0], 0.0f)));
        axes[1] = glm::normalize(glm::vec3(model * glm::vec4(axes[1], 0.0f)));
        axes[2] = glm::normalize(glm::vec3(model * glm::vec4(axes[2], 0.0f)));
    }

    // Get all 8 vertices of the OBB
    std::vector<glm::vec3> getVertices() const {
        std::vector<glm::vec3> vertices(8);
        glm::vec3 hX = axes[0] * halfExtents.x;
        glm::vec3 hY = axes[1] * halfExtents.y;
        glm::vec3 hZ = axes[2] * halfExtents.z;

        vertices[0] = center + hX + hY + hZ;
        vertices[1] = center + hX + hY - hZ;
        vertices[2] = center + hX - hY + hZ;
        vertices[3] = center + hX - hY - hZ;
        vertices[4] = center - hX + hY + hZ;
        vertices[5] = center - hX + hY - hZ;
        vertices[6] = center - hX - hY + hZ;
        vertices[7] = center - hX - hY - hZ;

        return vertices;
    }

    // Get edges of the OBB (each edge is a pair of vertices)
    std::vector<std::pair<glm::vec3, glm::vec3>> getEdges() const {
        std::vector<glm::vec3> vertices = getVertices();
        std::vector<std::pair<glm::vec3, glm::vec3>> edges;

        // Edges along x-axis
        edges.emplace_back(vertices[0], vertices[1]);
        edges.emplace_back(vertices[2], vertices[3]);
        edges.emplace_back(vertices[4], vertices[5]);
        edges.emplace_back(vertices[6], vertices[7]);

        // Edges along y-axis
        edges.emplace_back(vertices[0], vertices[2]);
        edges.emplace_back(vertices[1], vertices[3]);
        edges.emplace_back(vertices[4], vertices[6]);
        edges.emplace_back(vertices[5], vertices[7]);

        // Edges along z-axis
        edges.emplace_back(vertices[0], vertices[4]);
        edges.emplace_back(vertices[1], vertices[5]);
        edges.emplace_back(vertices[2], vertices[6]);
        edges.emplace_back(vertices[3], vertices[7]);

        return edges;
    }
};

// Define a struct to store the collision result
struct CollisionResult {
    bool collisionDetected = false;
    glm::vec3 collisionNormal;
    float penetrationDepth;
    glm::vec3 contactPoint;
    std::string collisionType;
};

// Helper function to project a point onto a plane
glm::vec3 projectPointOntoPlane(const glm::vec3& point, const glm::vec3& planeOrigin, const glm::vec3& planeNormal) {
    float dist = glm::dot(point - planeOrigin, planeNormal);
    return point - dist * planeNormal;
}

// Compute the signed distance from a point to a plane
float signedDistanceToPlane(const glm::vec3& point, const glm::vec3& planeOrigin, const glm::vec3& planeNormal) {
    return glm::dot(point - planeOrigin, planeNormal);
}

// Checks if a point is inside the bounds of a face
bool isPointInFaceBounds(const glm::vec3& point, const glm::vec3& faceCenter, const glm::vec3& u, const glm::vec3& v, float uHalf, float vHalf) {
    glm::vec3 rel = point - faceCenter;
    float uCoord = glm::dot(rel, u);
    float vCoord = glm::dot(rel, v);
    return std::abs(uCoord) <= uHalf && std::abs(vCoord) <= vHalf;
}

// Vertex-face collision detection
glm::vec3 vertexFaceCollision(const OBB& obb1, const OBB& obb2) {
    std::vector<glm::vec3> vertices1 = obb1.getVertices();
    std::vector<glm::vec3> vertices2 = obb2.getVertices();

    glm::vec3 closestPoint = glm::vec3(0.0f);
    float closestDistance = std::numeric_limits<float>::max();

    // Test OBB1 vertices against OBB2 faces
    for (const auto& vertex : vertices1) {
        for (int i = 0; i < 3; ++i) {
            glm::vec3 faceNormal = obb2.axes[i];
            glm::vec3 faceCenter = obb2.center;
            glm::vec3 u = obb2.axes[(i + 1) % 3];
            glm::vec3 v = obb2.axes[(i + 2) % 3];

            // Compute the signed distance from the vertex to the face plane
            float distance = signedDistanceToPlane(vertex, faceCenter, faceNormal);

            // If the vertex is behind the face (penetrating the OBB)
            if (distance < 0.0f) {
                // Check if the vertex is inside the bounds of the face
                if (isPointInFaceBounds(vertex, faceCenter, u, v, obb2.halfExtents[(i + 1) % 3], obb2.halfExtents[(i + 2) % 3])) {
                    // If this vertex is closer than the previously recorded one, update
                    if (std::abs(distance) < closestDistance) {
                        closestDistance = std::abs(distance);
                        closestPoint = vertex;
                    }
                }
            }
        }
    }

    // Test OBB2 vertices against OBB1 faces
    for (const auto& vertex : vertices2) {
        for (int i = 0; i < 3; ++i) {
            glm::vec3 faceNormal = obb1.axes[i];
            glm::vec3 faceCenter = obb1.center;
            glm::vec3 u = obb1.axes[(i + 1) % 3];
            glm::vec3 v = obb1.axes[(i + 2) % 3];

            // Compute the signed distance from the vertex to the face plane
            float distance = signedDistanceToPlane(vertex, faceCenter, faceNormal);

            // If the vertex is behind the face (penetrating the OBB)
            if (distance < 0.0f) {
                // Check if the vertex is inside the bounds of the face
                if (isPointInFaceBounds(vertex, faceCenter, u, v, obb1.halfExtents[(i + 1) % 3], obb1.halfExtents[(i + 2) % 3])) {
                    // If this vertex is closer than the previously recorded one, update
                    if (std::abs(distance) < closestDistance) {
                        closestDistance = std::abs(distance);
                        closestPoint = vertex;
                    }
                }
            }
        }
    }

    return closestPoint; // Return the exact vertex-face contact point
}

// Compute the squared distance between two line segments
float squaredDistanceBetweenEdges(const glm::vec3& p1, const glm::vec3& q1, const glm::vec3& p2, const glm::vec3& q2) {
    glm::vec3 d1 = q1 - p1;  // Direction vector of segment S1
    glm::vec3 d2 = q2 - p2;  // Direction vector of segment S2
    glm::vec3 r = p1 - p2;

    float a = glm::dot(d1, d1);  // Squared length of segment S1
    float e = glm::dot(d2, d2);  // Squared length of segment S2
    float f = glm::dot(d2, r);

    float s, t;
    if (a <= 1e-6f && e <= 1e-6f) {
        return glm::dot(r, r);  // Both segments degenerate into points
    }
    if (a <= 1e-6f) {
        s = 0.0f;
        t = glm::clamp(f / e, 0.0f, 1.0f); // Closest point on segment S2 to point p1
    }
    else {
        float c = glm::dot(d1, r);
        if (e <= 1e-6f) {
            t = 0.0f;
            s = glm::clamp(-c / a, 0.0f, 1.0f); // Closest point on segment S1 to point p2
        }
        else {
            float b = glm::dot(d1, d2);
            float denom = a * e - b * b;
            if (denom != 0.0f) {
                s = glm::clamp((b * f - c * e) / denom, 0.0f, 1.0f);
            }
            else {
                s = 0.0f;
            }
            t = glm::clamp((b * s + f) / e, 0.0f, 1.0f);
        }
    }

    glm::vec3 c1 = p1 + s * d1;
    glm::vec3 c2 = p2 + t * d2;
    return glm::length2(c1 - c2); // Squared distance between the closest points on the segments
}

// Find the closest pair of edges between two OBBs
std::pair<std::pair<glm::vec3, glm::vec3>, std::pair<glm::vec3, glm::vec3>> findClosestEdges(const OBB& obb1, const OBB& obb2) {
    std::vector<std::pair<glm::vec3, glm::vec3>> edges1 = obb1.getEdges();
    std::vector<std::pair<glm::vec3, glm::vec3>> edges2 = obb2.getEdges();

    float minDistanceSquared = std::numeric_limits<float>::max();
    std::pair<std::pair<glm::vec3, glm::vec3>, std::pair<glm::vec3, glm::vec3>> closestEdges;

    // Iterate over all edge pairs
    for (const auto& edge1 : edges1) {
        for (const auto& edge2 : edges2) {
            // Compute the squared distance between the two edges
            float distSquared = squaredDistanceBetweenEdges(edge1.first, edge1.second, edge2.first, edge2.second);

            // Track the closest pair of edges
            if (distSquared < minDistanceSquared) {
                minDistanceSquared = distSquared;
                closestEdges = { edge1, edge2 };
            }
        }
    }

    return closestEdges;
}

// Compute closest points between two line segments
glm::vec3 closestPointBetweenLines(const glm::vec3& p1, const glm::vec3& q1, const glm::vec3& p2, const glm::vec3& q2) {
    glm::vec3 d1 = q1 - p1;  // Direction vector of segment S1
    glm::vec3 d2 = q2 - p2;  // Direction vector of segment S2
    glm::vec3 r = p1 - p2;
    float a = glm::dot(d1, d1);  // Squared length of segment S1
    float e = glm::dot(d2, d2);  // Squared length of segment S2
    float f = glm::dot(d2, r);

    float s, t;
    if (a <= 1e-6f && e <= 1e-6f) {
        // Both segments degenerate into points
        return p1;
    }
    if (a <= 1e-6f) {
        // First segment degenerate into a point
        s = 0.0f;
        t = glm::clamp(f / e, 0.0f, 1.0f); // Use the closest point on segment S2 to point p1
    }
    else {
        float c = glm::dot(d1, r);
        if (e <= 1e-6f) {
            // Second segment degenerate into a point
            t = 0.0f;
            s = glm::clamp(-c / a, 0.0f, 1.0f); // Use the closest point on segment S1 to point p2
        }
        else {
            float b = glm::dot(d1, d2);
            float denom = a * e - b * b;

            if (denom != 0.0f) {
                s = glm::clamp((b * f - c * e) / denom, 0.0f, 1.0f);
            }
            else {
                s = 0.0f;
            }

            t = glm::clamp((b * s + f) / e, 0.0f, 1.0f);
        }
    }

    // Compute the closest points on the actual segments
    glm::vec3 c1 = p1 + s * d1; // Closest point on segment S1
    glm::vec3 c2 = p2 + t * d2; // Closest point on segment S2

    // Return the point that is on the closest segment line (S1 or S2)
    return glm::length(c1 - p1) < glm::length(c2 - p2) ? c1 : c2;
}

// Perform edge-edge collision detection on the closest edges
glm::vec3 edgeEdgeCollision(const OBB& obb1, const OBB& obb2) {
    // Find the closest pair of edges between OBB1 and OBB2
    auto closestEdges = findClosestEdges(obb1, obb2);
    const auto& edge1 = closestEdges.first;
    const auto& edge2 = closestEdges.second;

    // Compute the closest point between the two selected edges
    return closestPointBetweenLines(edge1.first, edge1.second, edge2.first, edge2.second);
}

// Helper function to project an OBB onto an axis and compute min/max projections
void projectOBB(const OBB& obb, const glm::vec3& axis, float& min, float& max) {
    float centerProjection = glm::dot(obb.center, axis);
    float extent = obb.halfExtents.x * std::abs(glm::dot(obb.axes[0], axis)) +
        obb.halfExtents.y * std::abs(glm::dot(obb.axes[1], axis)) +
        obb.halfExtents.z * std::abs(glm::dot(obb.axes[2], axis));

    min = centerProjection - extent;
    max = centerProjection + extent;
}

// Checks for overlap along a single axis
bool overlapOnAxis(const OBB& obb1, const OBB& obb2, const glm::vec3& axis, float& penetrationDepth) {
    float min1, max1, min2, max2;
    projectOBB(obb1, axis, min1, max1);
    projectOBB(obb2, axis, min2, max2);

    // Check for overlap
    if (max1 < min2 || max2 < min1) {
        return false; // No overlap
    }

    // Calculate the penetration depth
    float overlap = std::min(max1, max2) - std::max(min1, min2);
    penetrationDepth = overlap;
    return true;
}

// Perform SAT collision test between two OBBs and return contact points in CollisionResult
bool SATCollision(const OBB& obb1, const OBB& obb2, CollisionResult& result) {
    glm::vec3 testAxes[15];

    float minPenetrationDepth = std::numeric_limits<float>::max();
    glm::vec3 smallestAxis;
    int axisType = -1;  // Tracks which axis detected the collision (0-5: vertex-face, 6-14: edge-edge)

    // 3 axes from OBB1 (vertex-face)
    testAxes[0] = obb1.axes[0];
    testAxes[1] = obb1.axes[1];
    testAxes[2] = obb1.axes[2];

    // 3 axes from OBB2 (vertex-face)
    testAxes[3] = obb2.axes[0];
    testAxes[4] = obb2.axes[1];
    testAxes[5] = obb2.axes[2];

    // 9 cross product axes between OBB1 and OBB2 (edge-edge)
    int idx = 6;
    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 3; ++j) {
            glm::vec3 crossAxis = glm::cross(obb1.axes[i], obb2.axes[j]);
            if (glm::length(crossAxis) > 1e-6f) { // Skip degenerate axes
                testAxes[idx++] = glm::normalize(crossAxis);
            }
        }
    }

    // Test for overlap on all axes (skip degenerate axes)
    for (int i = 0; i < idx; ++i) {
        glm::vec3 axis = testAxes[i];
        float penetrationDepth = 0.0f;
        if (!overlapOnAxis(obb1, obb2, axis, penetrationDepth)) {
            return false; // Separating axis found, no collision
        }

        // Track the axis with the smallest penetration depth
        if (penetrationDepth < minPenetrationDepth) {
            minPenetrationDepth = penetrationDepth;
            smallestAxis = axis;
            axisType = i; // Record which axis caused the collision
        }
    }

    // If no separating axis is found, there is a collision
    result.collisionDetected = true;
    result.penetrationDepth = minPenetrationDepth;
    result.collisionNormal = smallestAxis;

    // Check which type of axis caused the collision
    if (axisType >= 0 && axisType <= 5) {
        // Vertex-face collision
        result.collisionType = "vertex-face";
        result.contactPoint = vertexFaceCollision(obb1, obb2);
    }
    else if (axisType >= 6 && axisType <= 14) {
        // Edge-edge collision
        result.collisionType = "edge-edge";
        result.contactPoint = edgeEdgeCollision(obb1, obb2);
    }

    return true; // Collision detected
}
