# Leitor e Visualizador de Cenas 3D — Grau B
**Computação Gráfica — Unisinos 2026/1**

---

## Integrantes
- Cássio Braga
- Gabriel Walber
- Patrícia Nagel

---

## O que foi adicionado em relação ao Grau A

| # | Requisito GB | Como foi implementado |
|---|---|---|
| 1 | Múltiplos OBJs com grupos | `loadSimpleOBJ()` lê qualquer .obj; a cena define N objetos no `scene.txt` |
| 2 | Materiais MTL (Ka/Kd/Ks/Ns/map_Kd) | `parseMTL()` em `LoadOBJ.h`; valores aplicados automaticamente ao `Mesh` |
| 3 | Iluminação de Phong (ambiente + difusa + especular) | Fragment shader herdado do Grau A; coeficientes do MTL ou ajustáveis via `M` |
| 4 | Texturas | `stb_image` + `uniform sampler2D` + `hasTexture` no fragment shader |
| 5 | Câmera FPS teclado + mouse | Herdada do Grau A; posição/yaw/pitch/fov configuráveis no `scene.txt` |
| 6 | Seleção + transformações (rot/trans/escala) | Herdado do Grau A (`TAB`, `X/Y/Z`, setas, `R/F`) |
| 7 | Animação Catmull-Rom | `catmullRomPoint()` em `main.cpp`; objetos com `anim_point` no `scene.txt` |
| 8 | Arquivo de configuração de cena | `SceneConfig.h` — parser do `scene.txt` (câmera, luzes, objetos, animação) |

---

## Estrutura de pastas esperada

```
GB-PG2026-1/
├── assets/
│   └── Modelos3D/
│       ├── SuzanneSubdiv1.obj   ← modelos 3D aqui
│       ├── cube.obj
│       └── ...
├── build/                       ← executável gerado aqui
│   └── LeitorVisualizador3D.exe
├── src/
│   └── LeitorVisualizador3D/
│       ├── main.cpp
│       ├── Mesh.h
│       ├── LoadOBJ.h
│       ├── SceneConfig.h
│       ├── Camera.h
│       └── Camera.cpp
├── common/
│   └── glad.c
├── include/
│   ├── glad/
│   │   └── glad.h
│   └── ...
├── CMakeLists.txt
└── README.md
```

---

## Compilação

### Dependências
- GCC/MinGW com C++17
- [GLFW3](https://www.glfw.org/) — baixado automaticamente pelo CMake via FetchContent
- [GLM](https://glm.g-truc.net/) — baixado automaticamente pelo CMake via FetchContent
- [GLAD](https://glad.dav1d.de/) — OpenGL 3.3 Core; coloque `glad.c` em `common/` e `glad.h` em `include/glad/`
- [stb_image.h](https://github.com/nothings/stb/blob/master/stb_image.h)

### Build com CMake
Utilize o VS Code com a extensão **CMake Tools** (Microsoft):
`Ctrl+Shift+P` → `CMake: Configure` → `CMake: Build`

---

## Como executar

**1.** Copie o arquivo `scene.txt` para dentro da pasta `build/`:
```
build/
├── LeitorVisualizador3D.exe
└── scene.txt                ← precisa estar aqui!
```

**2.** Execute pelo terminal a partir da pasta `build/`:
```bash
cd build
./LeitorVisualizador3D.exe
```

Ou passe um arquivo de cena diferente como argumento:
```bash
./LeitorVisualizador3D.exe minha_cena.txt
```

> **Sem `scene.txt`:** o programa cai em modo fallback e tenta carregar 3 Suzannes de `../assets/Modelos3D/SuzanneSubdiv1.obj`. Se esse arquivo não existir, o programa encerra com mensagem de erro no terminal.

---

## Formato do `scene.txt`

```
# comentário

camera
    position  x y z
    yaw       graus       # rotação horizontal (90 = olha para +Z)
    pitch     graus       # rotação vertical (negativo = olha para baixo)
    fov       graus       # field of view (ex: 60)
    near      val         # plano near do frustum (ex: 0.1)
    far       val         # plano far do frustum (ex: 300)
end

light
    position  x y z
    color     r g b       # valores de 0.0 a 1.0
    intensity val         # multiplicador de brilho
end

object
    name      identificador
    file      ../assets/Modelos3D/modelo.obj   # caminho relativo ao executável
    position  x y z
    rotation  xdeg ydeg zdeg
    scale     x y z
    color     r g b           # opcional; sobrescreve a cor do MTL
    # Animação Catmull-Rom (opcional):
    anim_speed  0.5           # segmentos percorridos por segundo
    anim_point  x y z         # ponto de controle (mínimo 2 para animar)
    anim_point  x y z
end
```

> Os caminhos dos arquivos `.obj` são **relativos ao executável** (que fica em `build/`).
> Exemplo: `../assets/Modelos3D/SuzanneSubdiv1.obj` sobe um nível da `build/` e entra em `assets/`.

---

## Controles

### Câmera
| Tecla | Ação |
|---|---|
| `W` `A` `S` `D` | Move câmera (frente/esquerda/trás/direita) |
| `Q` / `E` | Move câmera para baixo / cima |
| Mouse | Rotaciona a câmera (look) |
| Scroll | Ajusta velocidade de movimento da câmera |

### Seleção e transformação de objetos
| Tecla | Ação |
|---|---|
| `TAB` | Seleciona o próximo objeto (nome aparece no terminal) |
| `X` / `Y` / `Z` | Rotaciona o objeto selecionado +5° no eixo |
| `←` `→` `↑` `↓` | Translada em X e Y |
| `Page Up` / `Page Down` | Translada em Z |
| `R` / `F` | Aumenta / diminui escala uniforme |
| `Backspace` | Reseta rotação e escala do objeto selecionado |

### Material e visualização
| Tecla | Ação |
|---|---|
| `M` | Entra/sai do modo edição de material |
| `1` `2` `3` `4` | (no modo M) Seleciona ka / kd / ks / shininess |
| `+` / `-` | (no modo M) Aumenta / diminui o coeficiente selecionado |
| `B` | Liga/desliga wireframe sobreposto |
| `O` | Alterna entre projeção perspectiva e ortográfica |

### Animação
| Tecla | Ação |
|---|---|
| `Ctrl+A` | Pausa / retoma animação do objeto selecionado |

### Geral
| Tecla | Ação |
|---|---|
| `ESC` | Sair |

---

## Arquitetura do código

```
main.cpp        — game loop, shaders, Catmull-Rom, callbacks de input
Mesh.h          — struct Mesh: VAO, transformações, material Phong, animação
LoadOBJ.h       — parser OBJ + MTL + texturas via stb_image
SceneConfig.h   — parser do scene.txt (câmera, luzes, objetos)
Camera.h/.cpp   — câmera FPS com ângulos de Euler (herdada do Grau A)
```

### Fluxo de dados CPU → GPU
1. `loadSimpleOBJ()` lê o `.obj` e monta um `vector<GLfloat>` com **11 floats por vértice**: `pos(3) + cor(3) + normal(3) + texcoord(2)`
2. Esses dados são enviados à GPU via `glBufferData` e ficam no VBO
3. O VAO registra como interpretar esses 11 floats (locations 0-3)
4. A cada frame, `getModelMatrix()` combina posição/rotação/escala em uma matriz 4×4 enviada via `glUniform`
5. O vertex shader transforma os vértices pelas matrizes Model × View × Projection
6. O fragment shader calcula Phong (ambiente + difusa + especular) e multiplica pela cor base (textura ou cor do vértice)

---

## Referências
- LearnOpenGL: https://learnopengl.com
- Rossana Baptista Queiroz — exemplos da disciplina (Unisinos 2026/1)
- Anton Gerdelan: https://antongerdelan.net/opengl/
- stb_image: https://github.com/nothings/stb