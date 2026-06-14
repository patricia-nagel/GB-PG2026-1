// =============================================================================
// Mesh.h — Evolução do Grau A para o Grau B
// =============================================================================
// MUDANÇAS EM RELAÇÃO AO GRAU A:
//   + texID:      ID da textura OpenGL carregada do MTL (0 = sem textura)
//   + animPath:   pontos de controle para animação Catmull-Rom
//   + animT:      parâmetro acumulado da curva (avança no game loop)
//   + animSpeed:  velocidade em unidades-de-segmento por segundo
//   Tudo mais permanece IDÊNTICO ao Grau A.
// =============================================================================

#ifndef MESH_H
#define MESH_H

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <string>
#include <vector>

struct Mesh
{
    // ---- Dados na GPU (igual ao Grau A) ----
    GLuint VAO       = 0;
    int    nVertices = 0;
    std::string name;

    // ---- Transformações (igual ao Grau A) ----
    glm::vec3 position = glm::vec3(0.0f);
    glm::vec3 rotation = glm::vec3(0.0f); // graus X/Y/Z
    glm::vec3 scale    = glm::vec3(1.0f);

    // ---- Material Phong (igual ao Grau A) ----
    glm::vec3 ka        = glm::vec3(0.1f);
    glm::vec3 kd        = glm::vec3(0.8f);
    glm::vec3 ks        = glm::vec3(0.5f);
    float     shininess = 32.0f;
    glm::vec3 color     = glm::vec3(1.0f, 0.5f, 0.2f);

    // +GB: ID da textura (0 = sem textura, usa só a cor acima)
    GLuint texID = 0;

    // +GB: Animação Catmull-Rom
    // Se animPath tiver >= 2 pontos, o objeto se move pela curva no game loop.
    std::vector<glm::vec3> animPath;
    float animT     = 0.0f;  // parâmetro acumulado [0, N)
    float animSpeed = 0.0f;  // segmentos por segundo (0 = estático)

    // ---- getModelMatrix() — IDÊNTICO ao Grau A ----
    glm::mat4 getModelMatrix() const
    {
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, position);
        model = glm::rotate(model, glm::radians(rotation.x), glm::vec3(1,0,0));
        model = glm::rotate(model, glm::radians(rotation.y), glm::vec3(0,1,0));
        model = glm::rotate(model, glm::radians(rotation.z), glm::vec3(0,0,1));
        model = glm::scale(model, scale);
        return model;
    }
};

#endif // MESH_H
