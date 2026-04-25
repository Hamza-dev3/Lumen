// ============================================================
//  LUMEN
//  A textured ball rolling across three abandoned orbital
//  platforms.  Collect the blue orbs, reach the teleporter
//  ring at the end of each relay.
//
//  Controls:  WASD / arrows = roll,  Space = jump,
//             Mouse X = orbit camera,  Esc = quit
//
//  Assets (in assets/):
//    platform.jpg  – hull plating (static platforms)
//    ball.jpg      – ball texture
//    moving.jpg    – energy conduit pattern (moving platforms)
//    sky.jpg       – nebula skybox
//
//  NOTE: This version uses NO LIGHTING — all surfaces show
//  their raw texture/colour, fully unlit.
// ============================================================

#include <iostream>
#include <vector>
#include <cmath>

#define GLEW_STATIC
#include <GLEW/include/GL/glew.h>
#include <GLFW/include/GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#pragma warning(push)
#pragma warning(disable: 6262)
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#pragma warning(pop)

// --- window + physics constants ---
const unsigned int SCR_W = 1280;
const unsigned int SCR_H = 960;
const float GRAVITY = 28.0f;
const float BALL_R = 0.9f;

// --- ball physics state ---
glm::vec3 ballPos = glm::vec3(0.f, 0.5f, 0.f);
glm::vec3 ballVel = glm::vec3(0.f);
float     ballRoll = 0.f;
glm::vec3 ballRollAxis = glm::vec3(1.f, 0.f, 0.f);

// --- timing ---
float deltaTime = 0.f, lastFrame = 0.f;

// --- mouse / camera ---
bool  firstMouse = true;
float camYaw = 0.f, lastX = SCR_W / 2.f, lastY = SCR_H / 2.f;

// --- game flow ---
enum State { MENU, PLAYING, DEAD, STAGE_CLEAR, WIN };
State state = MENU;
int   curStage = 0;
int   orbsTotal = 0;
int   orbsGot = 0;
float stageTimer = 0.f;
float deadTimer = 0.f;
float clearTimer = 0.f;
bool  wasOnGround = false;

// --- level data ---
struct Plat { glm::vec3 pos, size; bool moving; float amp, speed, phase; };
struct Orb { glm::vec3 pos; bool got; };
struct Stage { glm::vec3 spawn, goal; std::vector<Plat> plats; std::vector<Orb> orbs; };
Stage stages[3];

void buildStages()
{
    // Stage 1 — straight relay, one moving section
    stages[0].spawn = { 0.0f, 0.5f,   0.0f };
    stages[0].goal = { 4.0f, 0.5f, -34.0f };
    stages[0].plats = {
        {{ 0.0f,0.f,  0.0f},{3.0f,0.3f,3.0f}, false, 0,0,0},
        {{ 0.0f,0.f, -7.0f},{1.2f,0.3f,5.0f}, false, 0,0,0},
        {{ 0.0f,0.f,-15.0f},{2.5f,0.3f,2.5f}, false, 0,0,0},
        {{ 0.0f,0.f,-20.0f},{1.5f,0.3f,1.5f}, true,  3.5f, 0.9f, 0.0f},
        {{ 0.0f,0.f,-25.0f},{1.2f,0.3f,3.5f}, false, 0,0,0},
        {{ 4.0f,0.f,-30.0f},{2.0f,0.3f,2.0f}, false, 0,0,0},
        {{ 4.0f,0.f,-34.0f},{3.0f,0.3f,3.0f}, false, 0,0,0},
        {{ 5.0f,-0.5f, -9.0f},{1.2f,0.3f,1.2f}, false, 0,0,0},
        {{-5.0f,-0.5f,-17.0f},{1.2f,0.3f,1.2f}, false, 0,0,0},
        {{ 5.0f,-0.5f,-27.0f},{1.2f,0.3f,1.2f}, false, 0,0,0},
    };
    stages[0].orbs = {
        {{ 5.0f,0.4f, -9.0f}, false},
        {{-5.0f,0.4f,-17.0f}, false},
        {{ 5.0f,0.4f,-27.0f}, false},
    };

    // Stage 2 — zigzag, two energy conduits
    stages[1].spawn = { 0.0f, 0.5f,   0.0f };
    stages[1].goal = { -6.0f, 0.5f, -40.0f };
    stages[1].plats = {
        {{  0.0f,0.f,  0.0f},{2.5f,0.3f,2.5f}, false, 0,0,0},
        {{  0.0f,0.f, -7.0f},{1.0f,0.3f,5.0f}, false, 0,0,0},
        {{  0.0f,0.f,-14.0f},{1.5f,0.3f,1.5f}, true, 4.0f,1.0f,0.0f},
        {{  5.0f,0.f,-18.0f},{3.5f,0.3f,1.0f}, false, 0,0,0},
        {{ 10.0f,0.f,-18.0f},{2.0f,0.3f,2.0f}, false, 0,0,0},
        {{ 10.0f,0.f,-24.0f},{1.0f,0.3f,5.0f}, false, 0,0,0},
        {{ 10.0f,0.f,-31.0f},{1.5f,0.3f,1.5f}, true, 5.0f,1.3f,0.8f},
        {{  3.0f,0.f,-35.0f},{5.0f,0.3f,1.0f}, false, 0,0,0},
        {{ -4.0f,0.f,-35.0f},{2.0f,0.3f,2.0f}, false, 0,0,0},
        {{ -6.0f,0.f,-40.0f},{3.0f,0.3f,3.0f}, false, 0,0,0},
        {{ -5.0f,-0.5f, -9.0f},{1.2f,0.3f,1.2f}, false, 0,0,0},
        {{ 14.5f,-0.5f,-20.0f},{1.2f,0.3f,1.2f}, false, 0,0,0},
        {{ 14.5f,-0.5f,-27.0f},{1.2f,0.3f,1.2f}, false, 0,0,0},
        {{  3.0f,-0.5f,-38.0f},{1.2f,0.3f,1.2f}, false, 0,0,0},
    };
    stages[1].orbs = {
        {{ -5.0f,0.4f, -9.0f}, false},
        {{ 14.5f,0.4f,-20.0f}, false},
        {{ 14.5f,0.4f,-27.0f}, false},
        {{  3.0f,0.4f,-38.0f}, false},
    };

    // Stage 3 — narrow spiral, three energy conduits
    stages[2].spawn = { 0.0f, 0.5f,   0.0f };
    stages[2].goal = { 0.0f, 0.5f, -50.0f };
    stages[2].plats = {
        {{  0.0f,0.f,  0.0f},{2.5f,0.3f,2.5f}, false, 0,0,0},
        {{  0.0f,0.f, -7.0f},{0.9f,0.3f,5.5f}, false, 0,0,0},
        {{  0.0f,0.f,-15.0f},{1.5f,0.3f,1.5f}, true, 4.5f,1.1f,0.0f},
        {{  0.0f,0.f,-20.0f},{0.9f,0.3f,4.0f}, false, 0,0,0},
        {{ -5.0f,0.f,-25.0f},{2.0f,0.3f,1.0f}, false, 0,0,0},
        {{ -9.0f,0.f,-25.0f},{2.0f,0.3f,2.0f}, false, 0,0,0},
        {{ -9.0f,0.f,-31.0f},{0.9f,0.3f,5.0f}, false, 0,0,0},
        {{ -9.0f,0.f,-38.0f},{1.5f,0.3f,1.5f}, true, 5.5f,1.5f,1.2f},
        {{ -3.0f,0.f,-42.0f},{5.5f,0.3f,1.0f}, false, 0,0,0},
        {{  3.0f,0.f,-42.0f},{2.0f,0.3f,2.0f}, false, 0,0,0},
        {{  0.0f,0.f,-46.0f},{1.5f,0.3f,1.5f}, true, 4.0f,1.4f,0.5f},
        {{  0.0f,0.f,-50.0f},{3.0f,0.3f,3.0f}, false, 0,0,0},
        {{  5.5f,-0.5f, -9.0f},{1.2f,0.3f,1.2f}, false, 0,0,0},
        {{  5.5f,-0.5f,-22.0f},{1.2f,0.3f,1.2f}, false, 0,0,0},
        {{-14.0f,-0.5f,-28.0f},{1.2f,0.3f,1.2f}, false, 0,0,0},
        {{ -3.0f,-0.5f,-44.0f},{1.2f,0.3f,1.2f}, false, 0,0,0},
    };
    stages[2].orbs = {
        {{  5.5f,0.4f, -9.0f}, false},
        {{  5.5f,0.4f,-22.0f}, false},
        {{-14.0f,0.4f,-28.0f}, false},
        {{ -3.0f,0.4f,-44.0f}, false},
    };

    orbsTotal = 0;
    for (auto& s : stages) orbsTotal += (int)s.orbs.size();
}

// ============================================================
//  SHADERS  —  NO LIGHTING, PURE TEXTURING
// ============================================================

// 3D vertex shader — transforms position and passes UV through.
const char* vertSrc =
"#version 330 core\n"
"layout(location=0) in vec3 aPos;\n"
"layout(location=1) in vec3 aCol;\n"
"layout(location=2) in vec3 aNormal;\n"
"layout(location=3) in vec2 aUV;\n"
"out vec2 vUV;\n"
"uniform mat4 model, view, projection;\n"
"void main(){\n"
"    vUV = aUV;\n"
"    gl_Position = projection * view * model * vec4(aPos, 1.0);\n"
"}\n";

// 3D fragment shader — objMode picks a texture or flat colour, NO LIGHTING.
//   0 hull plating (texture)  1 ball (texture)   2 orb (flat blue)
//   3 ring (flat cyan)        4 conduit (texture) 5 skybox (texture)
const char* fragSrc =
"#version 330 core\n"
"out vec4 FragColor;\n"
"in vec2 vUV;\n"
"uniform int   objMode;\n"
"uniform sampler2D tPlat;\n"  // platform.jpg   (unit 0)
"uniform sampler2D tBall;\n"  // ball.jpg       (unit 1)
"uniform sampler2D tMove;\n"  // moving.jpg     (unit 2)
"uniform sampler2D tSky;\n"   // sky.jpg        (unit 3)
"\n"
"void main(){\n"
"    if(objMode == 0){\n"
"        // Hull plating — raw texture\n"
"        FragColor = vec4(texture(tPlat, vUV).rgb, 1.0);\n"
"    }\n"
"    else if(objMode == 1){\n"
"        // Ball — raw texture, no lighting\n"
"        FragColor = vec4(texture(tBall, vUV).rgb, 1.0);\n"
"    }\n"
"    else if(objMode == 2){\n"
"        // Orb — flat blue\n"
"        FragColor = vec4(0.4, 0.8, 1.0, 1.0);\n"
"    }\n"
"    else if(objMode == 3){\n"
"        // Teleporter ring — flat cyan-white\n"
"        FragColor = vec4(0.5, 1.0, 1.0, 1.0);\n"
"    }\n"
"    else if(objMode == 4){\n"
"        // Energy conduit — raw texture\n"
"        FragColor = vec4(texture(tMove, vUV).rgb, 1.0);\n"
"    }\n"
"    else {\n"
"        // Skybox — raw texture\n"
"        FragColor = vec4(texture(tSky, vUV).rgb, 1.0);\n"
"    }\n"
"}\n";

// HUD shader — single-colour screen-space quads.  No textures.
const char* hudVert =
"#version 330 core\n"
"layout(location=0) in vec2 aPos;\n"
"uniform mat4 mvp;\n"
"void main(){ gl_Position = mvp * vec4(aPos, 0.0, 1.0); }\n";

const char* hudFrag =
"#version 330 core\n"
"out vec4 FragColor;\n"
"uniform vec4 color;\n"
"void main(){ FragColor = color; }\n";

// ============================================================
//  SHADER + GEOMETRY HELPERS
// ============================================================

unsigned int compileShader(const char* vs, const char* fs)
{
    int ok; char log[512];
    unsigned int v = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(v, 1, &vs, NULL); glCompileShader(v);
    glGetShaderiv(v, GL_COMPILE_STATUS, &ok);
    if (!ok) { glGetShaderInfoLog(v, 512, NULL, log); std::cout << "VS: " << log << "\n"; }

    unsigned int f = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(f, 1, &fs, NULL); glCompileShader(f);
    glGetShaderiv(f, GL_COMPILE_STATUS, &ok);
    if (!ok) { glGetShaderInfoLog(f, 512, NULL, log); std::cout << "FS: " << log << "\n"; }

    unsigned int p = glCreateProgram();
    glAttachShader(p, v); glAttachShader(p, f); glLinkProgram(p);
    glGetProgramiv(p, GL_LINK_STATUS, &ok);
    if (!ok) { glGetProgramInfoLog(p, 512, NULL, log); std::cout << "LINK: " << log << "\n"; }
    glDeleteShader(v); glDeleteShader(f);
    return p;
}

// UV sphere — vertex layout: pos(3) col(3) normal(3) uv(2) = 11 floats
unsigned int buildSphere(int& idxCount)
{
    const int STACKS = 24, SLICES = 24;
    std::vector<float> verts;
    std::vector<unsigned int> idx;
    for (int i = 0; i <= STACKS; i++) {
        float phi = glm::pi<float>() * i / STACKS;
        for (int j = 0; j <= SLICES; j++) {
            float th = 2.f * glm::pi<float>() * j / SLICES;
            float x = sinf(phi) * cosf(th), y = cosf(phi), z = sinf(phi) * sinf(th);
            verts.insert(verts.end(), { x * BALL_R, y * BALL_R, z * BALL_R,
                                         0.9f, 0.9f, 1.0f,
                                         x, y, z,
                                         (float)j / SLICES, (float)i / STACKS });
        }
    }
    for (int i = 0; i < STACKS; i++) for (int j = 0; j < SLICES; j++) {
        unsigned int a = i * (SLICES + 1) + j, b = a + SLICES + 1;
        idx.insert(idx.end(), { a,b,a + 1, b,b + 1,a + 1 });
    }
    idxCount = (int)idx.size();

    unsigned int VAO, VBO, EBO;
    glGenVertexArrays(1, &VAO); glGenBuffers(1, &VBO); glGenBuffers(1, &EBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, verts.size() * 4, verts.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, idx.size() * 4, idx.data(), GL_STATIC_DRAW);
    const int st = 11 * sizeof(float);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, st, (void*)0);               glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, st, (void*)(3 * sizeof(float))); glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, st, (void*)(6 * sizeof(float))); glEnableVertexAttribArray(2);
    glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, st, (void*)(9 * sizeof(float))); glEnableVertexAttribArray(3);
    return VAO;
}

// Unit cube with per-face UVs and normals, 36 verts.
unsigned int buildBox()
{
    float d[] = {
        // back
        -1,-1,-1, 1,1,1,  0,0,-1, 0,0,  1,-1,-1, 1,1,1, 0,0,-1, 1,0,  1,1,-1, 1,1,1, 0,0,-1, 1,1,
         1, 1,-1, 1,1,1,  0,0,-1, 1,1, -1,1,-1, 1,1,1, 0,0,-1, 0,1, -1,-1,-1,1,1,1, 0,0,-1, 0,0,
         // front
         -1,-1, 1, 1,1,1,  0,0, 1, 0,0,  1,-1, 1, 1,1,1, 0,0, 1, 1,0,  1,1, 1, 1,1,1, 0,0, 1, 1,1,
          1, 1, 1, 1,1,1,  0,0, 1, 1,1, -1,1, 1, 1,1,1, 0,0, 1, 0,1, -1,-1, 1,1,1,1, 0,0, 1, 0,0,
          // left
          -1, 1, 1, 1,1,1, -1,0, 0, 1,1, -1, 1,-1, 1,1,1,-1,0, 0, 0,1, -1,-1,-1,1,1,1,-1,0, 0, 0,0,
          -1,-1,-1, 1,1,1, -1,0, 0, 0,0, -1,-1, 1, 1,1,1,-1,0, 0, 1,0, -1, 1, 1,1,1,1,-1,0, 0, 1,1,
          // right
           1, 1, 1, 1,1,1,  1,0, 0, 1,1,  1, 1,-1, 1,1,1, 1,0, 0, 0,1,  1,-1,-1,1,1,1, 1,0, 0, 0,0,
           1,-1,-1, 1,1,1,  1,0, 0, 0,0,  1,-1, 1, 1,1,1, 1,0, 0, 1,0,  1, 1, 1,1,1,1, 1,0, 0, 1,1,
           // bottom
           -1,-1,-1, 1,1,1,  0,-1,0, 0,0,  1,-1,-1, 1,1,1, 0,-1,0, 1,0,  1,-1, 1, 1,1,1, 0,-1,0, 1,1,
            1,-1, 1, 1,1,1,  0,-1,0, 1,1, -1,-1, 1, 1,1,1, 0,-1,0, 0,1, -1,-1,-1,1,1,1, 0,-1,0, 0,0,
            // top
            -1, 1,-1, 1,1,1,  0, 1,0, 0,0,  1, 1,-1, 1,1,1, 0, 1,0, 1,0,  1, 1, 1, 1,1,1, 0, 1,0, 1,1,
             1, 1, 1, 1,1,1,  0, 1,0, 1,1, -1, 1, 1, 1,1,1, 0, 1,0, 0,1, -1, 1,-1,1,1,1, 0, 1,0, 0,0,
    };
    unsigned int VAO, VBO;
    glGenVertexArrays(1, &VAO); glGenBuffers(1, &VBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(d), d, GL_STATIC_DRAW);
    const int st = 11 * sizeof(float);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, st, (void*)0);                  glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, st, (void*)(3 * sizeof(float)));   glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, st, (void*)(6 * sizeof(float)));   glEnableVertexAttribArray(2);
    glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, st, (void*)(9 * sizeof(float)));   glEnableVertexAttribArray(3);
    return VAO;
}

// A unit [0,1]x[0,1] quad in XY for HUD overlays.
unsigned int buildHudQuad()
{
    float v[] = { 0,0, 1,0, 1,1,  0,0, 1,1, 0,1 };
    unsigned int VAO, VBO;
    glGenVertexArrays(1, &VAO); glGenBuffers(1, &VBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(v), v, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    return VAO;
}

// Triangle fan for a filled circle pip.
unsigned int buildCircle(int& triCount)
{
    const int SEG = 20;
    std::vector<float> v;
    v.push_back(0.f); v.push_back(0.f);
    for (int i = 0; i <= SEG; i++) {
        float a = 2.f * glm::pi<float>() * i / SEG;
        v.push_back(cosf(a));
        v.push_back(sinf(a));
    }
    triCount = SEG + 2;
    unsigned int VAO, VBO;
    glGenVertexArrays(1, &VAO); glGenBuffers(1, &VBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, v.size() * 4, v.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    return VAO;
}

// stb_image texture loader with a grey fallback if the file is missing.
unsigned int loadTexture(const char* path)
{
    unsigned int t;
    glGenTextures(1, &t);
    glBindTexture(GL_TEXTURE_2D, t);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    stbi_set_flip_vertically_on_load(true);
    int w, h, ch;
    unsigned char* data = stbi_load(path, &w, &h, &ch, 0);
    if (data) {
        GLenum fmt = (ch == 4) ? GL_RGBA : GL_RGB;
        glTexImage2D(GL_TEXTURE_2D, 0, fmt, w, h, 0, fmt, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
        std::cout << "Loaded: " << path << "\n";
    }
    else {
        unsigned char px[] = { 80,80,80,255 };
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, px);
        std::cout << "MISSING: " << path << " — using grey fallback\n";
    }
    stbi_image_free(data);
    return t;
}

// ============================================================
//  COLLISION + SPAWN
// ============================================================

bool checkPlatform(const glm::vec3& pos, const glm::vec3& half)
{
    glm::vec3 cl = glm::clamp(ballPos, pos - half, pos + half);
    if (glm::length(ballPos - cl) >= BALL_R) return false;
    if (ballPos.y >= pos.y) {
        ballPos.y = pos.y + half.y + BALL_R;
        if (ballVel.y < 0.f) ballVel.y = 0.f;
    }
    return true;
}

void spawnAtStage(int s)
{
    ballPos = stages[s].spawn;
    ballVel = glm::vec3(0.f);
    ballRoll = 0.f;
    stageTimer = 0.f;
    wasOnGround = false;
    state = PLAYING;
}

// ============================================================
//  CALLBACKS
// ============================================================

void framebuffer_size_callback(GLFWwindow*, int w, int h) { glViewport(0, 0, w, h); }

void mouse_callback(GLFWwindow*, double xi, double yi)
{
    float x = (float)xi, y = (float)yi;
    if (firstMouse) { lastX = x; lastY = y; firstMouse = false; }
    camYaw -= (x - lastX) * 0.3f;
    lastX = x; lastY = y;
}

void key_callback(GLFWwindow* win, int key, int, int action, int)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(win, true);

    if (action == GLFW_PRESS) {
        if (state == MENU) {
            curStage = 0; orbsGot = 0;
            for (auto& s : stages) for (auto& o : s.orbs) o.got = false;
            spawnAtStage(0);
        }
        else if (state == WIN) { state = MENU; }
    }
}

// ============================================================
//  PHYSICS / GAMEPLAY UPDATE
// ============================================================

void updateBall(GLFWwindow* win)
{
    if (state != PLAYING) return;
    stageTimer += deltaTime;

    // Camera-relative movement axes
    glm::vec3 fwd(sinf(glm::radians(camYaw)), 0.f, cosf(glm::radians(camYaw)));
    glm::vec3 rgt = glm::normalize(glm::cross(fwd, glm::vec3(0, 1, 0)));
    glm::vec3 dir(0.f);
    if (glfwGetKey(win, GLFW_KEY_W) == GLFW_PRESS || glfwGetKey(win, GLFW_KEY_UP) == GLFW_PRESS)    dir -= fwd;
    if (glfwGetKey(win, GLFW_KEY_S) == GLFW_PRESS || glfwGetKey(win, GLFW_KEY_DOWN) == GLFW_PRESS)  dir += fwd;
    if (glfwGetKey(win, GLFW_KEY_A) == GLFW_PRESS || glfwGetKey(win, GLFW_KEY_LEFT) == GLFW_PRESS)  dir += rgt;
    if (glfwGetKey(win, GLFW_KEY_D) == GLFW_PRESS || glfwGetKey(win, GLFW_KEY_RIGHT) == GLFW_PRESS) dir -= rgt;
    if (glm::length(dir) > 0.01f) dir = glm::normalize(dir);

    // --- collision against every platform in the current stage ---
    float t = (float)glfwGetTime();
    Stage& sd = stages[curStage];
    bool onGround = false;
    for (auto& p : sd.plats) {
        glm::vec3 wp = p.pos;
        if (p.moving) wp.x += p.amp * sinf(t * p.speed + p.phase);
        if (checkPlatform(wp, p.size)) onGround = true;
    }

    // --- ground logic: jump, steer, friction ---
    const float ACCEL = 8.f, MAX_SPD = 8.f, FRIC = 0.995f;
    if (onGround) {
        bool spaceDown = (glfwGetKey(win, GLFW_KEY_SPACE) == GLFW_PRESS);
        if (spaceDown && !wasOnGround) {
            ballVel.y = 9.0f;
        }
        wasOnGround = spaceDown;
        if (glm::length(dir) > 0.01f) {
            ballVel.x += dir.x * ACCEL * deltaTime;
            ballVel.z += dir.z * ACCEL * deltaTime;
            float h = sqrtf(ballVel.x * ballVel.x + ballVel.z * ballVel.z);
            if (h > MAX_SPD) { ballVel.x *= MAX_SPD / h; ballVel.z *= MAX_SPD / h; }
        }
        ballVel.x *= FRIC; ballVel.z *= FRIC;
    }
    else {
        wasOnGround = false;
    }

    // --- rolling animation ---
    float hSpd = sqrtf(ballVel.x * ballVel.x + ballVel.z * ballVel.z);
    if (hSpd > 0.05f) {
        glm::vec3 vd = glm::normalize(glm::vec3(ballVel.x, 0.f, ballVel.z));
        ballRollAxis = glm::cross(vd, glm::vec3(0, 1, 0));
        ballRoll -= hSpd * deltaTime * (180.f / (glm::pi<float>() * BALL_R));
    }

    // --- gravity + integrate ---
    ballVel.y -= GRAVITY * deltaTime;
    ballPos += ballVel * deltaTime;

    // --- orb collection ---
    for (auto& o : sd.orbs) {
        if (!o.got && glm::length(ballPos - o.pos) < (BALL_R + 0.55f)) {
            o.got = true;
            orbsGot++;
        }
    }

    // --- fall-off ---
    if (ballPos.y < -6.0f) { state = DEAD; deadTimer = 0.f; }

    // --- goal check ---
    float dx = ballPos.x - sd.goal.x, dz = ballPos.z - sd.goal.z;
    if (sqrtf(dx * dx + dz * dz) < 2.0f && ballPos.y > sd.goal.y - 1.0f) {
        if (curStage < 2) { state = STAGE_CLEAR; clearTimer = 0.f; }
        else { state = WIN; }
    }
}

// ============================================================
//  DRAW HELPERS
// ============================================================

void setMVP(unsigned int prog, const glm::mat4& m, const glm::mat4& v, const glm::mat4& p)
{
    glUniformMatrix4fv(glGetUniformLocation(prog, "model"), 1, GL_FALSE, glm::value_ptr(m));
    glUniformMatrix4fv(glGetUniformLocation(prog, "view"), 1, GL_FALSE, glm::value_ptr(v));
    glUniformMatrix4fv(glGetUniformLocation(prog, "projection"), 1, GL_FALSE, glm::value_ptr(p));
}

void drawBox(unsigned int prog, unsigned int VAO,
    const glm::vec3& pos, const glm::vec3& sc,
    const glm::mat4& view, const glm::mat4& proj)
{
    glm::mat4 m = glm::scale(glm::translate(glm::mat4(1), pos), sc);
    setMVP(prog, m, view, proj);
    glBindVertexArray(VAO);
    glDrawArrays(GL_TRIANGLES, 0, 36);
}

void drawHudRect(unsigned int prog, unsigned int VAO, const glm::mat4& ortho,
    float x, float y, float w, float h,
    float r, float g, float b, float a)
{
    glm::mat4 m = glm::scale(glm::translate(glm::mat4(1), { x, y, 0 }), { w, h, 1 });
    glUniformMatrix4fv(glGetUniformLocation(prog, "mvp"), 1, GL_FALSE, glm::value_ptr(ortho * m));
    glUniform4f(glGetUniformLocation(prog, "color"), r, g, b, a);
    glBindVertexArray(VAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
}

void drawHudCircle(unsigned int prog, unsigned int VAO, int triCount,
    const glm::mat4& ortho,
    float cx, float cy, float radius,
    float r, float g, float b, float a)
{
    glm::mat4 m = glm::scale(glm::translate(glm::mat4(1), { cx, cy, 0 }), { radius, radius, 1 });
    glUniformMatrix4fv(glGetUniformLocation(prog, "mvp"), 1, GL_FALSE, glm::value_ptr(ortho * m));
    glUniform4f(glGetUniformLocation(prog, "color"), r, g, b, a);
    glBindVertexArray(VAO);
    glDrawArrays(GL_TRIANGLE_FAN, 0, triCount);
}

// ============================================================
//  MAIN
// ============================================================

int main()
{
    // --- GLFW ---
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
    GLFWwindow* window = glfwCreateWindow(SCR_W, SCR_H, "LUMEN", NULL, NULL);
    if (!window) { glfwTerminate(); return -1; }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetKeyCallback(window, key_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) return -1;

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    buildStages();

    // --- compile shaders, build geometry, load textures ---
    unsigned int mainProg = compileShader(vertSrc, fragSrc);
    unsigned int hudProg = compileShader(hudVert, hudFrag);

    int sphereCount;
    unsigned int sphereVAO = buildSphere(sphereCount);
    unsigned int boxVAO = buildBox();
    unsigned int hudVAO = buildHudQuad();
    int circleCount;
    unsigned int circleVAO = buildCircle(circleCount);

    unsigned int tPlat = loadTexture("assets/platform.jpg");
    unsigned int tBall = loadTexture("assets/ball.jpg");
    unsigned int tMove = loadTexture("assets/moving.jpg");
    unsigned int tSky = loadTexture("assets/sky.jpg");

    glUseProgram(mainProg);
    glUniform1i(glGetUniformLocation(mainProg, "tPlat"), 0);
    glUniform1i(glGetUniformLocation(mainProg, "tBall"), 1);
    glUniform1i(glGetUniformLocation(mainProg, "tMove"), 2);
    glUniform1i(glGetUniformLocation(mainProg, "tSky"), 3);

    glm::mat4 ortho = glm::ortho(0.f, (float)SCR_W, 0.f, (float)SCR_H);

    // ============================================================
    //  RENDER LOOP
    // ============================================================
    while (!glfwWindowShouldClose(window))
    {
        float now = (float)glfwGetTime();
        deltaTime = glm::clamp(now - lastFrame, 0.f, 0.05f);
        lastFrame = now;
        glfwPollEvents();

        // Auto-respawn and auto-advance timers
        if (state == DEAD) {
            deadTimer += deltaTime;
            if (deadTimer > 1.5f) spawnAtStage(curStage);
        }
        if (state == STAGE_CLEAR) {
            clearTimer += deltaTime;
            if (clearTimer > 2.0f) { curStage++; spawnAtStage(curStage); }
        }

        updateBall(window);

        // Camera orbits around the ball
        glm::vec3 camOff(sinf(glm::radians(camYaw)) * 14.f, 8.f,
            cosf(glm::radians(camYaw)) * 14.f);
        glm::vec3 camPos = ballPos + camOff;
        glm::mat4 view = glm::lookAt(camPos, ballPos, { 0, 1, 0 });
        glm::mat4 proj = glm::perspective(glm::radians(45.f),
            (float)SCR_W / (float)SCR_H, 0.1f, 300.f);

        glClearColor(0.03f, 0.02f, 0.05f, 1.f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // ================ SKYBOX ================
        glDepthMask(GL_FALSE);
        glDisable(GL_DEPTH_TEST);
        glUseProgram(mainProg);
        glActiveTexture(GL_TEXTURE3);
        glBindTexture(GL_TEXTURE_2D, tSky);
        glUniform1i(glGetUniformLocation(mainProg, "objMode"), 5);
        glm::mat4 skyView = glm::mat4(glm::mat3(view));
        glm::mat4 skyModel = glm::scale(glm::mat4(1.f), glm::vec3(100.f));
        setMVP(mainProg, skyModel, skyView, proj);
        glBindVertexArray(boxVAO);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        glDepthMask(GL_TRUE);
        glEnable(GL_DEPTH_TEST);

        // ================ SCENE ================
        Stage& sd = stages[curStage < 3 ? curStage : 2];

        // --- platforms ---
        glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D, tPlat);
        glActiveTexture(GL_TEXTURE2); glBindTexture(GL_TEXTURE_2D, tMove);
        for (auto& p : sd.plats) {
            glm::vec3 wp = p.pos;
            if (p.moving) wp.x += p.amp * sinf(now * p.speed + p.phase);
            glUniform1i(glGetUniformLocation(mainProg, "objMode"), p.moving ? 4 : 0);
            drawBox(mainProg, boxVAO, wp, p.size, view, proj);
        }

        // --- orbs (skip collected ones) ---
        glUniform1i(glGetUniformLocation(mainProg, "objMode"), 2);
        for (auto& o : sd.orbs) {
            if (o.got) continue;
            float bob = o.pos.y + 0.2f * sinf(now * 2.f + o.pos.x);
            glm::mat4 m = glm::scale(
                glm::rotate(glm::translate(glm::mat4(1.f), { o.pos.x, bob, o.pos.z }),
                    now * 3.f, { 0, 1, 0 }),
                glm::vec3(0.14f));
            setMVP(mainProg, m, view, proj);
            glBindVertexArray(sphereVAO);
            glDrawElements(GL_TRIANGLES, sphereCount, GL_UNSIGNED_INT, 0);
        }

        // --- teleporter ring: 24 small cubes in a circle ---
        {
            glUniform1i(glGetUniformLocation(mainProg, "objMode"), 3);
            const int  RING_SEGS = 24;
            const float RING_RAD = 1.6f;
            glm::vec3   ringCentre = sd.goal + glm::vec3(0.f, 1.5f, 0.f);
            for (int i = 0; i < RING_SEGS; i++) {
                float a = 2.f * glm::pi<float>() * i / RING_SEGS + now * 1.5f;
                glm::vec3 segPos = ringCentre + glm::vec3(cosf(a) * RING_RAD,
                    sinf(a) * RING_RAD,
                    0.f);
                glm::mat4 m = glm::translate(glm::mat4(1.f), segPos);
                m = glm::rotate(m, a, { 0, 0, 1 });
                m = glm::scale(m, glm::vec3(0.18f, 0.18f, 0.12f));
                setMVP(mainProg, m, view, proj);
                glBindVertexArray(boxVAO);
                glDrawArrays(GL_TRIANGLES, 0, 36);
            }
        }

        // --- ball: textured, no lighting ---
        if (state == PLAYING || state == STAGE_CLEAR) {
            glActiveTexture(GL_TEXTURE1); glBindTexture(GL_TEXTURE_2D, tBall);
            glUniform1i(glGetUniformLocation(mainProg, "objMode"), 1);
            glm::mat4 m = glm::translate(glm::mat4(1.f), ballPos);
            if (glm::length(ballRollAxis) > 0.01f)
                m = glm::rotate(m, glm::radians(ballRoll), glm::normalize(ballRollAxis));
            setMVP(mainProg, m, view, proj);
            glBindVertexArray(sphereVAO);
            glDrawElements(GL_TRIANGLES, sphereCount, GL_UNSIGNED_INT, 0);
        }

        // ============================================================
        //  HUD
        // ============================================================
        glDisable(GL_DEPTH_TEST);
        glUseProgram(hudProg);
        float W = (float)SCR_W, H = (float)SCR_H;

        // --- gameplay HUD: orb pips + timer bar ---
        if (state == PLAYING || state == DEAD || state == STAGE_CLEAR) {
            int stageOrbs = (int)sd.orbs.size();
            int stageGot = 0;
            for (auto& o : sd.orbs) if (o.got) stageGot++;
            for (int i = 0; i < stageOrbs; i++) {
                float cx = 30.f + i * 28.f;
                float cy = H - 30.f;
                bool got = (i < stageGot);
                drawHudCircle(hudProg, circleVAO, circleCount, ortho,
                    cx, cy, 11.f, 0.2f, 0.3f, 0.6f, 0.9f);
                if (got) {
                    float p = 0.8f + 0.2f * sinf(now * 5.f + i);
                    drawHudCircle(hudProg, circleVAO, circleCount, ortho,
                        cx, cy, 8.f, 0.6f * p, 0.95f * p, 1.0f * p, 1.f);
                }
            }

            // Stage progress pips
            for (int i = 0; i < 3; i++) {
                float cx = W / 2 - 30 + i * 30.f;
                float cy = H - 30.f;
                drawHudCircle(hudProg, circleVAO, circleCount, ortho,
                    cx, cy, 10.f, 0.15f, 0.15f, 0.25f, 0.9f);
                if (i < curStage) {
                    drawHudCircle(hudProg, circleVAO, circleCount, ortho,
                        cx, cy, 7.f, 0.2f, 0.9f, 0.4f, 1.f);
                }
                else if (i == curStage) {
                    float p = 0.75f + 0.25f * sinf(now * 4.f);
                    drawHudCircle(hudProg, circleVAO, circleCount, ortho,
                        cx, cy, 7.f, 0.8f * p, 0.3f * p, 1.f * p, 1.f);
                }
            }

            // Timer bar
            float tw = 160.f, th = 6.f;
            float tx = W - tw - 24.f, ty = H - 28.f;
            drawHudRect(hudProg, hudVAO, ortho, tx, ty, tw, th,
                0.08f, 0.10f, 0.18f, 0.9f);
            float frac = glm::clamp(stageTimer / 60.f, 0.f, 1.f);
            drawHudRect(hudProg, hudVAO, ortho, tx, ty, tw * frac, th,
                0.4f, 0.8f, 1.f, 1.f);
        }

        // --- MENU ---
        if (state == MENU) {
            drawHudRect(hudProg, hudVAO, ortho, 0, 0, W, H, 0.0f, 0.0f, 0.05f, 0.85f);
            float pulse = 0.85f + 0.15f * sinf(now * 2.f);
            drawHudCircle(hudProg, circleVAO, circleCount, ortho,
                W / 2, H / 2, 120.f * pulse, 0.3f, 0.1f, 0.7f, 0.35f);
            drawHudCircle(hudProg, circleVAO, circleCount, ortho,
                W / 2, H / 2, 80.f, 0.5f, 0.2f, 0.9f, 0.6f);
            drawHudCircle(hudProg, circleVAO, circleCount, ortho,
                W / 2, H / 2, 45.f * pulse, 0.85f, 0.55f, 1.f, 1.f);
            drawHudCircle(hudProg, circleVAO, circleCount, ortho,
                W / 2 - 8, H / 2 + 8, 12.f, 1.f, 0.9f, 1.f, 0.8f);
            if (sinf(now * 4.f) > 0.f) {
                drawHudRect(hudProg, hudVAO, ortho,
                    W / 2 - 80, H / 2 - 140, 160, 4,
                    0.8f, 0.4f, 1.f, 0.9f);
            }
        }

        // --- DEAD ---
        if (state == DEAD) {
            float fade = glm::clamp(deadTimer / 0.4f, 0.f, 1.f);
            drawHudRect(hudProg, hudVAO, ortho, 0, 0, W, H,
                0.6f, 0.0f, 0.0f, 0.45f * fade);
            float bw = 12.f;
            drawHudRect(hudProg, hudVAO, ortho, 0, 0, W, bw, 1.f, 0.1f, 0.1f, fade);
            drawHudRect(hudProg, hudVAO, ortho, 0, H - bw, W, bw, 1.f, 0.1f, 0.1f, fade);
            drawHudRect(hudProg, hudVAO, ortho, 0, 0, bw, H, 1.f, 0.1f, 0.1f, fade);
            drawHudRect(hudProg, hudVAO, ortho, W - bw, 0, bw, H, 1.f, 0.1f, 0.1f, fade);
            float prog = glm::clamp(deadTimer / 1.5f, 0.f, 1.f);
            float barW = 260.f;
            drawHudRect(hudProg, hudVAO, ortho, W / 2 - barW / 2, 60,
                barW, 6, 0.2f, 0.f, 0.f, 1.f);
            drawHudRect(hudProg, hudVAO, ortho, W / 2 - barW / 2, 60,
                barW * prog, 6, 1.f, 0.3f, 0.2f, 1.f);
        }

        // --- STAGE_CLEAR ---
        if (state == STAGE_CLEAR) {
            float fade = glm::clamp(clearTimer / 0.3f, 0.f, 1.f);
            drawHudRect(hudProg, hudVAO, ortho, 0, 0, W, H,
                0.0f, 0.4f, 0.1f, 0.4f * fade);
            for (int i = 0; i < 3; i++) {
                float cx = W / 2 - 80 + i * 80.f;
                float cy = H / 2;
                drawHudCircle(hudProg, circleVAO, circleCount, ortho,
                    cx, cy, 36.f, 0.05f, 0.2f, 0.1f, 0.95f);
                if (i <= curStage) {
                    float p = (i == curStage)
                        ? (0.7f + 0.3f * sinf(now * 6.f))
                        : 1.f;
                    drawHudCircle(hudProg, circleVAO, circleCount, ortho,
                        cx, cy, 28.f, 0.3f * p, 1.f * p, 0.5f * p, 1.f);
                }
            }
            float prog = glm::clamp(clearTimer / 2.f, 0.f, 1.f);
            float barW = 300.f;
            drawHudRect(hudProg, hudVAO, ortho, W / 2 - barW / 2, H / 2 - 80,
                barW, 4, 0.05f, 0.2f, 0.1f, 1.f);
            drawHudRect(hudProg, hudVAO, ortho, W / 2 - barW / 2, H / 2 - 80,
                barW * prog, 4, 0.3f, 1.f, 0.5f, 1.f);
        }

        // --- WIN ---
        if (state == WIN) {
            drawHudRect(hudProg, hudVAO, ortho, 0, 0, W, H,
                0.3f, 0.22f, 0.0f, 0.75f);
            const int RAYS = 12;
            float beamLen = 180.f + 20.f * sinf(now * 2.f);
            for (int i = 0; i < RAYS; i++) {
                float a = 2.f * glm::pi<float>() * i / RAYS + now * 0.3f;
                glm::mat4 m(1.f);
                m = glm::translate(m, { W / 2, H / 2, 0 });
                m = glm::rotate(m, a, { 0, 0, 1 });
                m = glm::translate(m, { 30.f, -4.f, 0.f });
                m = glm::scale(m, { beamLen, 8.f, 1.f });
                glUniformMatrix4fv(glGetUniformLocation(hudProg, "mvp"),
                    1, GL_FALSE, glm::value_ptr(ortho * m));
                glUniform4f(glGetUniformLocation(hudProg, "color"),
                    1.f, 0.85f, 0.2f, 0.9f);
                glBindVertexArray(hudVAO);
                glDrawArrays(GL_TRIANGLES, 0, 6);
            }
            float pulse = 0.9f + 0.1f * sinf(now * 3.f);
            drawHudCircle(hudProg, circleVAO, circleCount, ortho,
                W / 2, H / 2, 60.f * pulse,
                1.f, 0.9f, 0.3f, 1.f);
            drawHudCircle(hudProg, circleVAO, circleCount, ortho,
                W / 2, H / 2, 35.f,
                1.f, 1.f, 0.8f, 1.f);
        }

        glEnable(GL_DEPTH_TEST);
        glfwSwapBuffers(window);
    }

    // --- cleanup ---
    unsigned int vaos[] = { sphereVAO, boxVAO, hudVAO, circleVAO };
    for (auto v : vaos) glDeleteVertexArrays(1, &v);
    unsigned int texs[] = { tPlat, tBall, tMove, tSky };
    for (auto t : texs) glDeleteTextures(1, &t);
    glDeleteProgram(mainProg);
    glDeleteProgram(hudProg);
    glfwTerminate();
    return 0;
}
