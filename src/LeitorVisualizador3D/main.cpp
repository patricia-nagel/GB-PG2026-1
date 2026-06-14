/*
 * =============================================================================
 * main.cpp — Visualizador 3D | Grau B
 * Computação Gráfica — Unisinos 2026/1
 * =============================================================================
 * Baseado no Hello3D/ExercicioOBJ da Profa. Rossana Baptista Queiroz.
 *
 * Layout VBO: pos(3) + cor(3) + normal(3) + texcoord(2) = 11 floats/vértice
 * locations:   0        1        2            3
 * (idêntico ao loadSimpleOBJ() da professora)
 *
 * ADIÇÕES DO GRAU B em relação ao Grau A [marcadas com +GB]:
 *   + Arquivo de cena scene.txt  (SceneConfig.h)
 *   + Texturas via stb_image     (loadTexture() em LoadOBJ.h)
 *   + Material lido do .mtl      (parseMTL() em LoadOBJ.h)
 *   + Animação Catmull-Rom       (catmullRomPoint() aqui)
 *   + Câmera/luz configuráveis   (pelo scene.txt)
 *
 * MANTIDO DO GRAU A:
 *   Câmera FPS (WASD+mouse), seleção TAB, rotação X/Y/Z,
 *   translação setas, escala R/F, wireframe B, ortho O, material M
 * =============================================================================
 */

// +GB: stb_image — define a implementação uma única vez aqui
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <iostream>
#include <iomanip>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <cmath>
#include <assert.h>

using namespace std;

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "Camera.h"
#include "Mesh.h"
#include "LoadOBJ.h"
#include "SceneConfig.h"  // +GB

// =============================================================================
// SHADERS — baseados nos da Profa. Rossana (Hello3D / ExercicioOBJ)
// Vertex shader: idêntico ao dela (locations 0-3, 11 floats)
// Fragment shader: adicionado suporte a textura com fallback para cor plana
// =============================================================================

const GLchar* vertexShaderSource = R"glsl(
#version 450
layout (location = 0) in vec3 position;
layout (location = 1) in vec3 color;
layout (location = 2) in vec3 normal;
layout (location = 3) in vec2 texc;

uniform mat4 model;
uniform mat4 projection;
uniform mat4 view;

out vec4 finalColor;
out vec3 fragPos;
out vec3 scaledNormal;
out vec2 texcoord;

void main()
{
    gl_Position  = projection * view * model * vec4(position, 1.0);
    finalColor   = vec4(color, 1.0);
    fragPos      = vec3(model * vec4(position, 1.0));
    scaledNormal = vec3(model * vec4(normal,   0.0));
    texcoord     = vec2(texc.s, 1.0 - texc.t); // flip Y igual à professora
}
)glsl";

// Fragment shader: igual ao da professora + especular + fallback sem textura
const GLchar* fragmentShaderSource = R"glsl(
#version 450
in vec4 finalColor;
in vec3 fragPos;
in vec3 scaledNormal;
in vec2 texcoord;

uniform sampler2D texBuffer;
uniform int hasTexture;     // +GB: 1 = usa textura, 0 = usa finalColor

// Propriedades da superfície/material
uniform float ka;
uniform float kd;
uniform float ks, q;

// Propriedades da fonte de luz
uniform vec3 lightPos;
uniform vec3 lightColor;

// Posição da câmera
uniform vec3 cameraPos;

out vec4 color;

void main()
{
    // Parcela da luz ambiente
    vec3 ambient = ka * lightColor;

    // Parcela da reflexão difusa
    vec3 N    = normalize(scaledNormal);
    vec3 L    = normalize(lightPos - fragPos);
    float diff = max(dot(N, L), 0.0);
    vec3 diffuse = kd * diff * lightColor;

    // Parcela da reflexão especular (Phong)
    vec3 V    = normalize(cameraPos - fragPos);
    vec3 R    = reflect(-L, N);
    float spec = pow(max(dot(V, R), 0.0), q);
    vec3 specular = ks * spec * lightColor;

    // Cor base: textura ou cor do vértice (finalColor)
    vec4 baseColor = (hasTexture == 1)
                   ? texture(texBuffer, texcoord)
                   : finalColor;

    color = (vec4(ambient,1.0) + vec4(diffuse,1.0)) * baseColor
            + vec4(specular, 0.0);
}
)glsl";

// =============================================================================
// GLOBAIS
// =============================================================================
const GLuint WIDTH = 1200, HEIGHT = 800;

Camera camera(glm::vec3(0.0f, 0.0f, 5.0f),
              glm::vec3(0.0f, 1.0f, 0.0f), 90.0f, 0.0f);

float deltaTime  = 0.0f;
float lastFrame  = 0.0f;
float lastX      = WIDTH  / 2.0f;
float lastY      = HEIGHT / 2.0f;
bool  firstMouse = true;

int  selectedObject = 0;
bool wireframe      = false;
bool usePerspective = true;

int  editComponent = 1;
bool materialMode  = false;

vector<Mesh> meshes;

// =============================================================================
// +GB: catmullRomPoint() — posição na curva Catmull-Rom para parâmetro t
// =============================================================================
glm::vec3 catmullRomPoint(const vector<glm::vec3>& pts, float t)
{
    int n = (int)pts.size();
    if (n == 0) return glm::vec3(0.0f);
    if (n == 1) return pts[0];

    t = fmod(t, (float)n);
    if (t < 0.0f) t += (float)n;

    int   i0 = (int)t;
    float u  = t - i0;
    float u2 = u*u, u3 = u2*u;

    auto idx = [&](int i){ return ((i % n) + n) % n; };
    glm::vec3 p0 = pts[idx(i0-1)];
    glm::vec3 p1 = pts[idx(i0)];
    glm::vec3 p2 = pts[idx(i0+1)];
    glm::vec3 p3 = pts[idx(i0+2)];

    return 0.5f * (
        (-p0 + 3.0f*p1 - 3.0f*p2 + p3) * u3 +
        ( 2.0f*p0 - 5.0f*p1 + 4.0f*p2 - p3) * u2 +
        (-p0 + p2) * u +
        2.0f * p1
    );
}

// =============================================================================
// PROTÓTIPOS
// =============================================================================
int  setupShader(const GLchar* vs, const GLchar* fs);
void printMaterial(int idx);
void key_callback(GLFWwindow*, int, int, int, int);
void mouse_callback(GLFWwindow*, double, double);
void scroll_callback(GLFWwindow*, double, double);

// =============================================================================
// MAIN
// =============================================================================
int main(int argc, char* argv[])
{
    // +GB: arquivo de cena como argumento ou padrão
    string sceneFile = (argc > 1) ? argv[1] : "scene.txt";

    // ---- GLFW ----
    glfwInit();
    GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT,
        "Visualizador 3D GB | TAB=Obj | WASD=Cam | M=Mat | B=Wire | O=Proj",
        nullptr, nullptr);
    glfwMakeContextCurrent(window);
    glfwSetKeyCallback(window, key_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // ---- GLAD ----
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    { cerr << "Falha ao inicializar GLAD\n"; return -1; }

    const GLubyte* renderer = glGetString(GL_RENDERER);
    const GLubyte* version  = glGetString(GL_VERSION);
    cout << "Renderer: " << renderer << "\nOpenGL: " << version << "\n\n";

    int fbW, fbH;
    glfwGetFramebufferSize(window, &fbW, &fbH);
    glViewport(0, 0, fbW, fbH);

    // ---- Shader ----
    GLuint shaderID = setupShader(vertexShaderSource, fragmentShaderSource);
    glUseProgram(shaderID);
    glEnable(GL_DEPTH_TEST);

    // Textura na unit 0
    glUniform1i(glGetUniformLocation(shaderID, "texBuffer"), 0);

    // ==========================================================================
    // +GB: Carrega cena do arquivo
    // ==========================================================================
    SceneConfig scene;
    float fov = 45.0f;

    if (scene.loadFromFile(sceneFile))
    {
        // Aplica câmera do arquivo
        camera = Camera(scene.camera.position,
                        glm::vec3(0,1,0),
                        scene.camera.yaw,
                        scene.camera.pitch);
        fov = scene.camera.fov;

        for (auto& oc : scene.objects)
        {
            Mesh m;
            loadSimpleOBJ(oc.filePath, m);
            m.name      = oc.name;
            m.position  = oc.position;
            m.rotation  = oc.rotation;
            m.scale     = oc.scale;
            if (oc.color.r >= 0.0f) m.color = oc.color; // sobrescreve cor do MTL
            m.animPath  = oc.animPoints;
            m.animSpeed = oc.animSpeed;
            meshes.push_back(m);
        }
    }
    else
    {
        // Fallback: cena padrão igual ao Grau A (3 Suzannes)
        cout << "[AVISO] scene.txt nao encontrado — cena padrao\n\n";
        struct ObjCfg { string path; glm::vec3 pos, color, ka, kd, ks; float shi; };
        vector<ObjCfg> cfgs = {
            {"../assets/Modelos3D/SuzanneSubdiv1.obj", glm::vec3(-2.5f,0,0),
             glm::vec3(0.85f,0.20f,0.20f), glm::vec3(0.1f), glm::vec3(0.8f), glm::vec3(0.5f), 32.f},
            {"../assets/Modelos3D/SuzanneSubdiv1.obj", glm::vec3(0.f,0,0),
             glm::vec3(0.20f,0.80f,0.25f), glm::vec3(0.1f), glm::vec3(0.8f), glm::vec3(0.3f), 16.f},
            {"../assets/Modelos3D/SuzanneSubdiv1.obj", glm::vec3(2.5f,0,0),
             glm::vec3(0.20f,0.40f,0.90f), glm::vec3(0.2f), glm::vec3(0.7f), glm::vec3(0.9f), 128.f},
        };
        for (auto& c : cfgs)
        {
            Mesh m;
            loadSimpleOBJ(c.path, m);
            m.position = c.pos; m.color = c.color;
            m.ka = c.ka; m.kd = c.kd; m.ks = c.ks; m.shininess = c.shi;
            meshes.push_back(m);
        }
    }

    if (meshes.empty()) { cerr << "Nenhum objeto carregado!\n"; return -1; }

    // ---- Luz ----
    glm::vec3 lightPos, lightColor;
    if (!scene.lights.empty()) {
        lightPos   = scene.lights[0].position;
        lightColor = scene.lights[0].color * scene.lights[0].intensity;
    } else {
        lightPos   = glm::vec3(-0.5f, 5.0f, 0.0f); // igual à professora
        lightColor = glm::vec3(1.0f);
    }
    glUniform3f(glGetUniformLocation(shaderID,"lightPos"),
                lightPos.x, lightPos.y, lightPos.z);
    glUniform3f(glGetUniformLocation(shaderID,"lightColor"),
                lightColor.x, lightColor.y, lightColor.z);

    cout << "\n=== " << meshes.size() << " objeto(s) na cena ===\n";
    cout << "TAB=selecionar | M=material | B=wireframe | O=projecao\n";
    cout << "X/Y/Z=rotacao  | Setas+PgUp/Dn=translacao | R/F=escala\n\n";
    printMaterial(selectedObject);

    // ==========================================================================
    // GAME LOOP
    // ==========================================================================
    while (!glfwWindowShouldClose(window))
    {
        float now = (float)glfwGetTime();
        deltaTime = now - lastFrame;
        lastFrame = now;

        glfwPollEvents();

        // ---- Câmera WASD ----
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) camera.processKeyboard("FORWARD",  deltaTime);
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) camera.processKeyboard("BACKWARD", deltaTime);
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) camera.processKeyboard("LEFT",     deltaTime);
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) camera.processKeyboard("RIGHT",    deltaTime);
        if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) camera.processKeyboard("UP",       deltaTime);
        if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) camera.processKeyboard("DOWN",     deltaTime);

        // ---- Modo material ----
        if (materialMode && !meshes.empty())
        {
            Mesh& sel  = meshes[selectedObject];
            float step = 0.5f * deltaTime;
            bool up   = (glfwGetKey(window, GLFW_KEY_KP_ADD)      == GLFW_PRESS) ||
                        (glfwGetKey(window, GLFW_KEY_EQUAL)        == GLFW_PRESS);
            bool down = (glfwGetKey(window, GLFW_KEY_KP_SUBTRACT) == GLFW_PRESS) ||
                        (glfwGetKey(window, GLFW_KEY_MINUS)        == GLFW_PRESS);
            if (up || down)
            {
                float dir = up ? 1.0f : -1.0f;
                switch (editComponent)
                {
                    case 0: sel.ka.r = sel.ka.g = sel.ka.b =
                                glm::clamp(sel.ka.r + dir*step, 0.0f, 1.0f); break;
                    case 1: sel.kd.r = sel.kd.g = sel.kd.b =
                                glm::clamp(sel.kd.r + dir*step, 0.0f, 1.0f); break;
                    case 2: sel.ks.r = sel.ks.g = sel.ks.b =
                                glm::clamp(sel.ks.r + dir*step, 0.0f, 1.0f); break;
                    case 3: sel.shininess =
                                glm::clamp(sel.shininess + dir*25.f*deltaTime, 1.f, 256.f); break;
                }
                static float lastPrint = 0.f;
                if (now - lastPrint > 0.25f) { printMaterial(selectedObject); lastPrint = now; }
            }
        }

        // ---- Translação contínua ----
        if (!meshes.empty())
        {
            Mesh& sel = meshes[selectedObject];
            float ts  = 2.0f * deltaTime;
            if (glfwGetKey(window, GLFW_KEY_LEFT)      == GLFW_PRESS) sel.position.x -= ts;
            if (glfwGetKey(window, GLFW_KEY_RIGHT)     == GLFW_PRESS) sel.position.x += ts;
            if (glfwGetKey(window, GLFW_KEY_UP)        == GLFW_PRESS) sel.position.y += ts;
            if (glfwGetKey(window, GLFW_KEY_DOWN)      == GLFW_PRESS) sel.position.y -= ts;
            if (glfwGetKey(window, GLFW_KEY_PAGE_UP)   == GLFW_PRESS) sel.position.z += ts;
            if (glfwGetKey(window, GLFW_KEY_PAGE_DOWN) == GLFW_PRESS) sel.position.z -= ts;
        }

        // +GB: Animação Catmull-Rom
        for (auto& m : meshes)
            if (m.animSpeed > 0.0f && m.animPath.size() >= 2)
            {
                m.animT   += m.animSpeed * deltaTime;
                m.position = catmullRomPoint(m.animPath, m.animT);
            }

        // ---- Clear ----
        glClearColor(0.12f, 0.12f, 0.15f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // ---- View / Projection ----
        glm::mat4 view = camera.getViewMatrix();
        glUniformMatrix4fv(glGetUniformLocation(shaderID,"view"),1,GL_FALSE,glm::value_ptr(view));
        glUniform3f(glGetUniformLocation(shaderID,"cameraPos"),
                    camera.position.x, camera.position.y, camera.position.z);

        glm::mat4 projection = usePerspective
            ? glm::perspective(glm::radians(fov), (float)WIDTH/HEIGHT, 0.1f, 300.0f)
            : glm::ortho(-5.0f, 5.0f, -5.0f*(float)HEIGHT/WIDTH, 5.0f*(float)HEIGHT/WIDTH, 0.1f, 300.0f);
        glUniformMatrix4fv(glGetUniformLocation(shaderID,"projection"),1,GL_FALSE,glm::value_ptr(projection));

        // ---- Draw loop ----
        for (int i = 0; i < (int)meshes.size(); i++)
        {
            if (meshes[i].VAO == 0) continue;

            // Model matrix
            glm::mat4 model = meshes[i].getModelMatrix();
            glUniformMatrix4fv(glGetUniformLocation(shaderID,"model"),1,GL_FALSE,glm::value_ptr(model));

            // Material (ka/kd/ks como escalares — igual à professora)
            float kaV = (meshes[i].ka.r + meshes[i].ka.g + meshes[i].ka.b) / 3.0f;
            float kdV = (meshes[i].kd.r + meshes[i].kd.g + meshes[i].kd.b) / 3.0f;
            float ksV = (meshes[i].ks.r + meshes[i].ks.g + meshes[i].ks.b) / 3.0f;
            glUniform1f(glGetUniformLocation(shaderID,"ka"), kaV);
            glUniform1f(glGetUniformLocation(shaderID,"kd"), kdV);
            glUniform1f(glGetUniformLocation(shaderID,"ks"), ksV);
            glUniform1f(glGetUniformLocation(shaderID,"q"),  meshes[i].shininess);

            // +GB: textura
            if (meshes[i].texID > 0)
            {
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, meshes[i].texID);
                glUniform1i(glGetUniformLocation(shaderID,"hasTexture"), 1);
            }
            else
            {
                glUniform1i(glGetUniformLocation(shaderID,"hasTexture"), 0);
            }

            glBindVertexArray(meshes[i].VAO);

            // Sólido
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
            glDrawArrays(GL_TRIANGLES, 0, meshes[i].nVertices);

            // Wireframe sobreposto
            if (wireframe)
            {
                glUniform1i(glGetUniformLocation(shaderID,"hasTexture"), 0);
                glUniform1f(glGetUniformLocation(shaderID,"ka"), 1.0f);
                glUniform1f(glGetUniformLocation(shaderID,"kd"), 0.0f);
                glUniform1f(glGetUniformLocation(shaderID,"ks"), 0.0f);
                glEnable(GL_POLYGON_OFFSET_LINE);
                glPolygonOffset(-1.0f, -1.0f);
                glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
                glDrawArrays(GL_TRIANGLES, 0, meshes[i].nVertices);
                glDisable(GL_POLYGON_OFFSET_LINE);
            }

            glBindVertexArray(0);
            glBindTexture(GL_TEXTURE_2D, 0);
        }

        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        glfwSwapBuffers(window);
    }

    for (auto& m : meshes) if (m.VAO) glDeleteVertexArrays(1, &m.VAO);
    glDeleteProgram(shaderID);
    glfwTerminate();
    return 0;
}

// =============================================================================
// setupShader() — igual ao da professora, aceita vs e fs como parâmetros
// =============================================================================
int setupShader(const GLchar* vshader, const GLchar* fshader)
{
    GLuint vs = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vs, 1, &vshader, NULL);
    glCompileShader(vs);
    GLint ok; GLchar log[512];
    glGetShaderiv(vs, GL_COMPILE_STATUS, &ok);
    if (!ok) { glGetShaderInfoLog(vs,512,NULL,log); cerr<<"VS error:\n"<<log<<"\n"; }

    GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fs, 1, &fshader, NULL);
    glCompileShader(fs);
    glGetShaderiv(fs, GL_COMPILE_STATUS, &ok);
    if (!ok) { glGetShaderInfoLog(fs,512,NULL,log); cerr<<"FS error:\n"<<log<<"\n"; }

    GLuint p = glCreateProgram();
    glAttachShader(p, vs); glAttachShader(p, fs);
    glLinkProgram(p);
    glGetProgramiv(p, GL_LINK_STATUS, &ok);
    if (!ok) { glGetProgramInfoLog(p,512,NULL,log); cerr<<"Link error:\n"<<log<<"\n"; }

    glDeleteShader(vs); glDeleteShader(fs);
    return p;
}

// =============================================================================
// printMaterial()
// =============================================================================
void printMaterial(int idx)
{
    if (idx < 0 || idx >= (int)meshes.size()) return;
    const Mesh& m = meshes[idx];
    const char* comp[] = {"ka","kd","ks","shininess"};
    cout << fixed << setprecision(3);
    cout << "--- [" << idx << "] " << m.name
         << " | ka=" << m.ka.r
         << " kd=" << m.kd.r
         << " ks=" << m.ks.r
         << " q=" << m.shininess
         << " | edit: " << comp[editComponent] << "\n";
}

// =============================================================================
// key_callback()
// =============================================================================
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GL_TRUE);

    if (action != GLFW_PRESS) return;
    if (meshes.empty()) return;

    Mesh& sel = meshes[selectedObject];

    if (key == GLFW_KEY_TAB)
    {
        selectedObject = (selectedObject + 1) % (int)meshes.size();
        cout << "\n>> [" << selectedObject << "] " << meshes[selectedObject].name << "\n";
        printMaterial(selectedObject);
        return;
    }

    if (key == GLFW_KEY_M)
    {
        materialMode = !materialMode;
        cout << "\n[Modo material: " << (materialMode ? "ON" : "OFF") << "]\n";
        if (materialMode) cout << "  1=ka  2=kd  3=ks  4=shininess  +/-=ajustar\n";
        return;
    }

    if (materialMode)
    {
        if (key == GLFW_KEY_1) { editComponent = 0; cout << "[ka]\n"; }
        if (key == GLFW_KEY_2) { editComponent = 1; cout << "[kd]\n"; }
        if (key == GLFW_KEY_3) { editComponent = 2; cout << "[ks]\n"; }
        if (key == GLFW_KEY_4) { editComponent = 3; cout << "[shininess]\n"; }
        return;
    }

    // Rotação
    if (key == GLFW_KEY_X) sel.rotation.x += 5.0f;
    if (key == GLFW_KEY_Y) sel.rotation.y += 5.0f;
    if (key == GLFW_KEY_Z) sel.rotation.z += 5.0f;

    // Escala uniforme
    if (key == GLFW_KEY_R) sel.scale += glm::vec3(0.05f);
    if (key == GLFW_KEY_F) sel.scale  = glm::max(sel.scale - glm::vec3(0.05f), glm::vec3(0.05f));

    // Projeção
    if (key == GLFW_KEY_O)
    {
        usePerspective = !usePerspective;
        cout << "[Projecao: " << (usePerspective ? "Perspectiva" : "Ortografica") << "]\n";
    }

    // Wireframe
    if (key == GLFW_KEY_B)
    {
        wireframe = !wireframe;
        cout << "[Wireframe: " << (wireframe ? "ON" : "OFF") << "]\n";
    }

    // Reset
    if (key == GLFW_KEY_BACKSPACE)
    {
        sel.rotation = glm::vec3(0.0f);
        sel.scale    = glm::vec3(1.0f);
        cout << "[Objeto " << selectedObject << " resetado]\n";
    }

    // +GB: Ctrl+A pausa/retoma animação
    if (key == GLFW_KEY_A && (mode & GLFW_MOD_CONTROL))
    {
        if (!sel.animPath.empty())
        {
            if (sel.animSpeed > 0.0f)
            {
                sel.rotation.z = sel.animSpeed; // guarda velocidade
                sel.animSpeed  = 0.0f;
                cout << "[Animacao PAUSADA]\n";
            }
            else
            {
                sel.animSpeed  = sel.rotation.z > 0.0f ? sel.rotation.z : 0.5f;
                sel.rotation.z = 0.0f;
                cout << "[Animacao RETOMADA]\n";
            }
        }
    }
}

// =============================================================================
// mouse_callback() / scroll_callback()
// =============================================================================
void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
    if (firstMouse) { lastX = (float)xpos; lastY = (float)ypos; firstMouse = false; }
    float xoff = (float)xpos - lastX;
    float yoff = lastY - (float)ypos;
    lastX = (float)xpos; lastY = (float)ypos;
    camera.processMouseMovement(xoff, yoff);
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    camera.movementSpeed = glm::clamp(camera.movementSpeed + (float)yoffset * 0.5f, 1.0f, 20.0f);
}
