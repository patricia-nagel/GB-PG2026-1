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
| 3 | Iluminação de Phong | Fragment shader (herdado do Grau A); coeficientes do MTL ou ajustáveis via M |
| 4 | Texturas | stb_image + `uniform sampler2D` + `hasTexture` no fragment shader |
| 5 | Câmera FPS teclado+mouse | Herdada do Grau A; posição/yaw/pitch/fov configuráveis no `scene.txt` |
| 6 | Seleção + transformações (rot/trans/escala) | Herdado do Grau A (TAB, X/Y/Z, Setas, R/F) |
| 7 | Animação Catmull-Rom | `catmullRomPoint()` em `main.cpp`; objetos com `anim_point` no `scene.txt` |
| 8 | Arquivo de configuração de cena | `SceneConfig.h` — parser de `scene.txt` (câmera, luzes, objetos, animação) |

---

## Compilação

### Dependências
- GCC/G++ com C++17
- [GLFW3](https://www.glfw.org/)  — `sudo apt install libglfw3-dev`
- [GLAD](https://glad.dav1d.de/) — OpenGL 3.3 Core (inclua `glad.c` na compilação)
- [GLM](https://glm.g-truc.net/) — `sudo apt install libglm-dev`
- [stb_image.h](https://github.com/nothings/stb) — copie para o diretório de includes

### Compilação manual
```bash
g++ -std=c++17 -O2 \
    main.cpp Camera.cpp \
    ../common/glad.c \
    -I../common/include -I../include \
    -lglfw -lGL -ldl \
    -o visualizador3d
```
> Ajuste os caminhos de acordo com a estrutura do repositório da disciplina.

---

## Uso

```bash
./visualizador3d [arquivo_de_cena.txt]
```
Sem argumento, usa `scene.txt` no diretório atual.

---

## Formato do `scene.txt`

```
# comentário

camera
    position  x y z
    yaw       graus       # rotação horizontal
    pitch     graus       # rotação vertical
    fov       graus
    near      val
    far       val
end

light
    position  x y z
    color     r g b
    intensity val
end

object
    name      identificador
    file      caminho/modelo.obj
    position  x y z
    rotation  xdeg ydeg zdeg
    scale     x y z
    color     r g b           # opcional; sobrescreve a cor do MTL
    # Animação Catmull-Rom (opcional):
    anim_speed  0.5           # segmentos por segundo
    anim_point  x y z         # repita para cada ponto de controle (mínimo 2)
end
```

---

## Controles

| Tecla | Ação |
|---|---|
| `W A S D` | Move câmera (frente/trás/esquerda/direita) |
| `Q` / `E` | Move câmera para baixo/cima |
| Mouse | Rotaciona câmera (look) |
| Scroll | Ajusta velocidade da câmera |
| `TAB` | Seleciona próximo objeto |
| `X` / `Y` / `Z` | Rotaciona objeto selecionado (+5°) |
| Setas / `PgUp` `PgDn` | Translada objeto selecionado |
| `R` / `F` | Aumenta/diminui escala uniforme |
| `M` | Modo edição de material (+`1/2/3/4` selecionam componente, `+/-` ajustam) |
| `B` | Toggle wireframe sobreposto |
| `O` | Toggle perspectiva/ortográfica |
| `Ctrl+A` | Pausa/retoma animação do objeto selecionado |
| `Backspace` | Reseta rotação e escala do objeto selecionado |
| `ESC` | Sair |

---

## Estrutura dos arquivos

```
grauB/
├── main.cpp        # loop principal, shaders, Catmull-Rom, game loop
├── Mesh.h          # struct Mesh (+texID, +animPath — evolução do Grau A)
├── LoadOBJ.h       # parser OBJ+MTL+stb_image (retrocompatível com Grau A)
├── SceneConfig.h   # parser do scene.txt (novo no Grau B)
├── Camera.h        # declaração da câmera (inalterada do Grau A)
├── Camera.cpp      # implementação da câmera (inalterada do Grau A)
├── scene.txt       # exemplo de cena com 17 objetos + 1 animado
└── README.md
```

---

## Referências
- LearnOpenGL: https://learnopengl.com
- Rossana Baptista Queiroz — exemplos da disciplina (Unisinos 2026/1)
- Anton Gerdelan: https://antongerdelan.net/opengl/
- stb_image: https://github.com/nothings/stb
