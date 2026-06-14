// =============================================================================
// SceneConfig.h — Leitura do arquivo de configuração de cena (NOVO no Grau B)
// =============================================================================
// Formato do arquivo de cena (texto simples, comentários com #):
//
//   camera
//       position  x y z
//       yaw       val        # graus
//       pitch     val        # graus
//       fov       val        # graus
//       near      val
//       far       val
//   end
//
//   light
//       position  x y z
//       color     r g b
//       intensity val
//   end
//
//   object
//       name      identificador
//       file      caminho/modelo.obj
//       position  x y z
//       rotation  xdeg ydeg zdeg
//       scale     x y z
//       color     r g b       # opcional (sobrescreve o MTL)
//       # Animação Catmull-Rom (opcional):
//       anim_speed  val
//       anim_point  x y z     # repita quantos pontos quiser (min 2)
//   end
//
// DESIGN:
//   SceneConfig só guarda dados (POD). Quem chama loadSimpleOBJ() é o main.cpp,
//   exatamente como no Grau A — evita dependência circular.
// =============================================================================

#ifndef SCENE_CONFIG_H
#define SCENE_CONFIG_H

#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <iostream>
#include <glm/glm.hpp>

// -----------------------------------------------------------------------------
// Configuração de um objeto a ser carregado
// -----------------------------------------------------------------------------
struct ObjectConfig
{
    std::string name;
    std::string filePath;

    glm::vec3 position = glm::vec3(0.0f);
    glm::vec3 rotation = glm::vec3(0.0f);
    glm::vec3 scale    = glm::vec3(1.0f);
    glm::vec3 color    = glm::vec3(-1.0f); // -1 = "usar cor do MTL"

    float             animSpeed = 0.0f;
    std::vector<glm::vec3> animPoints; // se vazio, sem animação
};

// -----------------------------------------------------------------------------
// Configuração da fonte de luz
// -----------------------------------------------------------------------------
struct LightConfig
{
    glm::vec3 position  = glm::vec3(3.0f, 5.0f, 3.0f);
    glm::vec3 color     = glm::vec3(1.0f);
    float     intensity = 1.0f;
};

// -----------------------------------------------------------------------------
// Configuração da câmera inicial
// -----------------------------------------------------------------------------
struct CameraConfig
{
    glm::vec3 position = glm::vec3(0.0f, 0.0f, 5.0f);
    float yaw      = -90.0f; // compatível com default do Grau A
    float pitch    =   0.0f;
    float fov      =  45.0f;
    float nearPlane = 0.1f;
    float farPlane  = 100.0f;
};

// -----------------------------------------------------------------------------
// SceneConfig — agrega tudo e faz o parse do arquivo
// -----------------------------------------------------------------------------
struct SceneConfig
{
    std::vector<ObjectConfig> objects;
    std::vector<LightConfig>  lights;
    CameraConfig              camera;

    // Retorna true se carregou com sucesso.
    bool loadFromFile(const std::string& path)
    {
        std::ifstream f(path);
        if (!f.is_open())
        {
            std::cerr << "[Scene] Nao encontrado: " << path << "\n";
            return false;
        }

        auto trim = [](std::string s) -> std::string {
            size_t a = s.find_first_not_of(" \t\r\n");
            size_t b = s.find_last_not_of(" \t\r\n");
            return (a == std::string::npos) ? "" : s.substr(a, b-a+1);
        };

        std::string line;
        while (std::getline(f, line))
        {
            line = trim(line);
            if (line.empty() || line[0] == '#') continue;

            // ---- Bloco: camera ----
            if (line == "camera")
            {
                while (std::getline(f, line))
                {
                    line = trim(line);
                    if (line == "end") break;
                    if (line.empty() || line[0] == '#') continue;
                    std::istringstream ss(line); std::string tok; ss >> tok;
                    if      (tok=="position") ss>>camera.position.x>>camera.position.y>>camera.position.z;
                    else if (tok=="yaw")      ss>>camera.yaw;
                    else if (tok=="pitch")    ss>>camera.pitch;
                    else if (tok=="fov")      ss>>camera.fov;
                    else if (tok=="near")     ss>>camera.nearPlane;
                    else if (tok=="far")      ss>>camera.farPlane;
                }
            }
            // ---- Bloco: light ----
            else if (line == "light")
            {
                LightConfig lc;
                while (std::getline(f, line))
                {
                    line = trim(line);
                    if (line == "end") break;
                    if (line.empty() || line[0] == '#') continue;
                    std::istringstream ss(line); std::string tok; ss >> tok;
                    if      (tok=="position")  ss>>lc.position.x>>lc.position.y>>lc.position.z;
                    else if (tok=="color")     ss>>lc.color.r>>lc.color.g>>lc.color.b;
                    else if (tok=="intensity") ss>>lc.intensity;
                }
                lights.push_back(lc);
            }
            // ---- Bloco: object ----
            else if (line == "object")
            {
                ObjectConfig oc;
                while (std::getline(f, line))
                {
                    line = trim(line);
                    if (line == "end") break;
                    if (line.empty() || line[0] == '#') continue;
                    std::istringstream ss(line); std::string tok; ss >> tok;
                    if      (tok=="name")       ss>>oc.name;
                    else if (tok=="file")       ss>>oc.filePath;
                    else if (tok=="position")   ss>>oc.position.x>>oc.position.y>>oc.position.z;
                    else if (tok=="rotation")   ss>>oc.rotation.x>>oc.rotation.y>>oc.rotation.z;
                    else if (tok=="scale")      ss>>oc.scale.x>>oc.scale.y>>oc.scale.z;
                    else if (tok=="color")      ss>>oc.color.r>>oc.color.g>>oc.color.b;
                    else if (tok=="anim_speed") ss>>oc.animSpeed;
                    else if (tok=="anim_point")
                    {
                        glm::vec3 p; ss>>p.x>>p.y>>p.z;
                        oc.animPoints.push_back(p);
                    }
                }
                objects.push_back(oc);
            }
        }

        std::cout << "[Scene] " << objects.size() << " objeto(s), "
                  << lights.size() << " luz(es), arquivo: " << path << "\n";
        return true;
    }
};

#endif // SCENE_CONFIG_H
