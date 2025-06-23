#pragma once

#include <GL/glew.h>
#include <vector>
#include <cmath>

class Mesh {
public:
    GLuint vao = 0;
    GLuint vbo = 0;
    size_t vertexCount = 0;

    ~Mesh() {
        if (vbo) glDeleteBuffers(1, &vbo);
        if (vao) glDeleteVertexArrays(1, &vao);
    }

    void generateSphere(float radius, int segments) {
        std::vector<float> vertices;

        for (int y = 0; y <= segments; ++y) {
            float theta = y * glm::pi<float>() / segments;
            float sinTheta = sin(theta);
            float cosTheta = cos(theta);

            for (int x = 0; x <= segments; ++x) {
                float phi = x * 2.0f * glm::pi<float>() / segments;
                float sinPhi = sin(phi);
                float cosPhi = cos(phi);

                float px = radius * sinTheta * cosPhi;
                float py = radius * cosTheta;
                float pz = radius * sinTheta * sinPhi;

                vertices.push_back(px);
                vertices.push_back(py);
                vertices.push_back(pz);
            }
        }

        vertexCount = vertices.size() / 3;

        glGenVertexArrays(1, &vao);
        glBindVertexArray(vao);

        glGenBuffers(1, &vbo);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);

        glBindVertexArray(0);
    }

    void draw() const {
        glBindVertexArray(vao);
        glDrawArrays(GL_POINTS, 0, static_cast<GLsizei>(vertexCount));  // use GL_TRIANGLES if triangulated
        glBindVertexArray(0);
    }
};