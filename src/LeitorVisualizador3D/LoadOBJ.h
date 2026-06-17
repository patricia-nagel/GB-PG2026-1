// =============================================================================
// LoadOBJ.h — Grau B | Layout 11 floats/vértice (igual ao exemplo da Profa.)
// =============================================================================
// Layout do VBO: pos(3) + cor(3) + normal(3) + texcoord(2) = 11 floats
// locations:      0        1        2            3
//
// Igual ao loadSimpleOBJ() do Hello3D/ExercicioOBJ da Rossana, com adições:
//   + parseMTL(): lê Ka, Kd, Ks, Ns, map_Kd do .mtl
//   + loadTexture(): stb_image → texID (função separada, igual à dela)
//   + Preenche outMesh.ka/kd/ks/shininess/color/texID com valores do MTL
// =============================================================================

#ifndef LOADOBJ_H
#define LOADOBJ_H

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <map>

#include <glad/glad.h>
#include <glm/glm.hpp>

#include "Mesh.h"

// =============================================================================
// loadTexture() — igual ao da professora (HelloCubemap)
// =============================================================================
int loadTexture(const std::string& filePath)
{
    GLuint texID;
    glGenTextures(1, &texID);
    glBindTexture(GL_TEXTURE_2D, texID);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,     GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,     GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    int width, height, nrChannels;
    unsigned char* data = stbi_load(filePath.c_str(), &width, &height, &nrChannels, 0);

    if (data)
    {
        if (nrChannels == 3)
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB,  width, height, 0, GL_RGB,  GL_UNSIGNED_BYTE, data);
        else
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);

        glGenerateMipmap(GL_TEXTURE_2D);
        stbi_image_free(data);
        glBindTexture(GL_TEXTURE_2D, 0);
        std::cout << "[TEX] " << filePath << " (" << width << "x" << height << ")\n";
        return texID;
    }
    else
    {
        std::cerr << "[TEX] Falha ao carregar: " << filePath << "\n";
        // Textura 1x1 branca como fallback
        unsigned char white[4] = {255,255,255,255};
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, white);
        stbi_image_free(data);
        glBindTexture(GL_TEXTURE_2D, 0);
        return texID;
    }
}

// =============================================================================
// parseMTL() — lê arquivo .mtl, retorna mapa { nomeMaterial → Mesh }
// =============================================================================
static std::map<std::string, Mesh> parseMTL(const std::string& mtlPath,
                                              const std::string& baseDir)
{
    std::map<std::string, Mesh> mats;
    std::ifstream f(mtlPath);
    if (!f.is_open())
    {
        std::cerr << "[MTL] Nao encontrado: " << mtlPath << "\n";
        return mats;
    }

    std::string line, curName;
    Mesh cur;

    while (std::getline(f, line))
    {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        std::istringstream ss(line);
        std::string tok; ss >> tok;

        if (tok == "newmtl")
        {
            if (!curName.empty()) mats[curName] = cur;
            cur = Mesh(); ss >> curName;
        }
        else if (tok == "Ka") { ss >> cur.ka.r  >> cur.ka.g  >> cur.ka.b; }
        else if (tok == "Kd") { ss >> cur.kd.r  >> cur.kd.g  >> cur.kd.b;
                                  cur.color = cur.kd; }
        else if (tok == "Ks") { ss >> cur.ks.r  >> cur.ks.g  >> cur.ks.b; }
        else if (tok == "Ns") { ss >> cur.shininess; }
        else if (tok == "map_Kd")
        {
            std::string texName; ss >> texName;
            if (!texName.empty() && texName.back() == '\r') texName.pop_back();
            cur.texID = loadTexture(baseDir + texName);
        }
    }
    if (!curName.empty()) mats[curName] = cur;

    std::cout << "[MTL] " << mtlPath << " — " << mats.size() << " material(ais)\n";
    return mats;
}

// =============================================================================
// loadSimpleOBJ() — igual ao da professora + leitura de MTL
// =============================================================================
// Layout VBO: pos(3) + cor(3) + normal(3) + texcoord(2) = 11 floats/vértice
// Retorna VAO (igual à professora). Também preenche outMesh com material/textura.
// =============================================================================
bool loadSimpleOBJ(const std::string& filePath, Mesh& outMesh)
{
    // Diretório base para resolver MTL e texturas com caminhos relativos
    std::string baseDir;
    size_t sl = filePath.find_last_of("/\\");
    if (sl != std::string::npos) baseDir = filePath.substr(0, sl + 1);

    std::vector<glm::vec3> vertices;
    std::vector<glm::vec2> texCoords;
    std::vector<glm::vec3> normals;
    std::vector<GLfloat>   vBuffer;

    // Cor padrão (igual à professora: vermelho como placeholder)
    glm::vec3 color = glm::vec3(1.0f, 0.0f, 0.0f);

    // +GB: materiais do MTL e material ativo
    std::map<std::string, Mesh> materials;
    std::string curMaterial;

    std::ifstream arqEntrada(filePath.c_str());
    if (!arqEntrada.is_open())
    {
        std::cerr << "[OBJ] Erro ao tentar ler o arquivo " << filePath << "\n";
        return false;
    }

    std::string line;
    while (std::getline(arqEntrada, line))
    {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        std::istringstream ssline(line);
        std::string word;
        ssline >> word;

        if (word == "v")
        {
            glm::vec3 vertice;
            ssline >> vertice.x >> vertice.y >> vertice.z;
            vertices.push_back(vertice);
        }
        else if (word == "vt")
        {
            glm::vec2 vt;
            ssline >> vt.s >> vt.t;
            texCoords.push_back(vt);
        }
        else if (word == "vn")
        {
            glm::vec3 normal;
            ssline >> normal.x >> normal.y >> normal.z;
            normals.push_back(normal);
        }
        // carrega o MTL referenciado
        else if (word == "mtllib")
        {
            std::string mtlName; ssline >> mtlName;
            if (!mtlName.empty() && mtlName.back() == '\r') mtlName.pop_back();
            materials = parseMTL(baseDir + mtlName, baseDir);
        }
        // troca o material ativo (atualiza a cor usada no VBO)
        else if (word == "usemtl")
        {
            ssline >> curMaterial;
            if (!curMaterial.empty() && curMaterial.back() == '\r') curMaterial.pop_back();
            if (materials.count(curMaterial))
                color = materials.at(curMaterial).kd; // usa Kd como cor do vértice
        }
        else if (word == "f")
        {
            // Igual à professora, mas com fan triangulation para polígonos
            std::vector<std::string> faceVerts;
            std::string fv;
            while (ssline >> fv) faceVerts.push_back(fv);

            for (int i = 1; i + 1 < (int)faceVerts.size(); i++)
            {
                std::string trio[3] = { faceVerts[0], faceVerts[i], faceVerts[i+1] };
                for (const auto& w : trio)
                {
                    int vi = 0, ti = 0, ni = 0;
                    std::istringstream ss(w);
                    std::string index;

                    if (std::getline(ss, index, '/')) vi = !index.empty() ? std::stoi(index)-1 : 0;
                    if (std::getline(ss, index, '/')) ti = !index.empty() ? std::stoi(index)-1 : 0;
                    if (std::getline(ss, index))      ni = !index.empty() ? std::stoi(index)-1 : 0;

                    // pos (3) — location 0
                    vBuffer.push_back(vertices[vi].x);
                    vBuffer.push_back(vertices[vi].y);
                    vBuffer.push_back(vertices[vi].z);
                    // cor (3) — location 1
                    vBuffer.push_back(color.r);
                    vBuffer.push_back(color.g);
                    vBuffer.push_back(color.b);
                    // normal (3) — location 2
                    vBuffer.push_back(normals[ni].x);
                    vBuffer.push_back(normals[ni].y);
                    vBuffer.push_back(normals[ni].z);
                    // texcoord (2) — location 3
                    vBuffer.push_back(texCoords[ti].s);
                    vBuffer.push_back(texCoords[ti].t);
                }
            }
        }
    }
    arqEntrada.close();

    if (vBuffer.empty())
    {
        std::cerr << "[OBJ] Nenhuma face em: " << filePath << "\n";
        return false;
    }

    // ---- Upload para GPU (igual à professora) ----
    std::cout << "Gerando o buffer de geometria...\n";
    GLuint VBO, VAO;
    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vBuffer.size()*sizeof(GLfloat), vBuffer.data(), GL_STATIC_DRAW);

    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);

    // Atributo 0: posição (x,y,z)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 11*sizeof(GLfloat), (GLvoid*)0);
    glEnableVertexAttribArray(0);
    // Atributo 1: cor (r,g,b)
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 11*sizeof(GLfloat), (GLvoid*)(3*sizeof(GLfloat)));
    glEnableVertexAttribArray(1);
    // Atributo 2: normal (x,y,z)
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 11*sizeof(GLfloat), (GLvoid*)(6*sizeof(GLfloat)));
    glEnableVertexAttribArray(2);
    // Atributo 3: texcoord (s,t)
    glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, 11*sizeof(GLfloat), (GLvoid*)(9*sizeof(GLfloat)));
    glEnableVertexAttribArray(3);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    outMesh.VAO       = VAO;
    outMesh.nVertices = (int)(vBuffer.size() / 11);
    outMesh.name      = filePath;

    // aplica material do MTL na Mesh
    if (!curMaterial.empty() && materials.count(curMaterial))
    {
        const Mesh& mat = materials.at(curMaterial);
        outMesh.ka        = mat.ka;
        outMesh.kd        = mat.kd;
        outMesh.ks        = mat.ks;
        outMesh.shininess = mat.shininess;
        outMesh.color     = mat.color;
        outMesh.texID     = mat.texID;
    }

    std::cout << "[OBJ] " << filePath
              << " | verts=" << outMesh.nVertices
              << " | tex=" << (outMesh.texID ? "sim" : "nao") << "\n";
    return true;
}

#endif // LOADOBJ_H
