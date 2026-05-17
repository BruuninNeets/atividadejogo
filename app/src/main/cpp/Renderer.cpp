#include "Renderer.h"
#include <game-activity/native_app_glue/android_native_app_glue.h>
#include <GLES3/gl3.h>
#include <memory>
#include <vector>
#include <chrono>
#include <cmath>
#include <algorithm>
#include <string>
#include "AndroidOut.h"
#include "Shader.h"
#include "Utility.h"
#include "TextureAsset.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif

// Cores de backup
#define COLOR_CEILING 0.03f, 0.03f, 0.04f, 1.0f

static const char *vertex = R"vertex(#version 300 es
in vec3 inPosition;
in vec2 inUV;
out vec2 fragUV;
uniform mat4 uProjection;
void main() {
    fragUV = inUV;
    gl_Position = uProjection * vec4(inPosition, 1.0);
}
)vertex";

static const char *fragment = R"fragment(#version 300 es
precision mediump float;
in vec2 fragUV;
uniform sampler2D uTexture;
out vec4 outColor;
void main() {
    vec4 tex = texture(uTexture, fragUV);
    if(tex.a < 0.4) discard;
    outColor = tex;
}
)fragment";

Renderer::Renderer(android_app *pApp) :
        app_(pApp), display_(EGL_NO_DISPLAY), surface_(EGL_NO_SURFACE), context_(EGL_NO_CONTEXT),
        width_(0), height_(0), shaderNeedsNewProjectionMatrix_(true), lastTime_(0),
        isBtnUp_(false), isBtnDown_(false), isBtnLeft_(false), isBtnRight_(false), isBtnShoot_(false) {
    game_ = std::make_unique<Game>(pApp);
    initRenderer();
}

Renderer::~Renderer() {
    if (display_ != EGL_NO_DISPLAY) {
        eglMakeCurrent(display_, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        if (context_ != EGL_NO_CONTEXT) eglDestroyContext(display_, context_);
        if (surface_ != EGL_NO_SURFACE) eglDestroySurface(display_, surface_);
        eglTerminate(display_);
    }
}

void Renderer::drawButton(float x, float y, float size, const std::shared_ptr<TextureAsset>& tex) {
    if (!tex) return;
    float sx = size;
    float sy = size * ((float)width_ / (float)height_);
    std::vector<Vertex> v = {
        Vertex({x - sx, y + sy, 0.0f}, {0, 0}),
        Vertex({x + sx, y + sy, 0.0f}, {1, 0}),
        Vertex({x + sx, y - sy, 0.0f}, {1, 1}),
        Vertex({x - sx, y - sy, 0.0f}, {0, 1})
    };
    shader_->drawModel(Model(v, {0, 1, 2, 0, 2, 3}, tex));
}

void Renderer::render() {
    updateRenderArea();
    auto currentTime = std::chrono::high_resolution_clock::now();
    double time = std::chrono::duration<double>(currentTime.time_since_epoch()).count();
    float deltaTime = (lastTime_ == 0) ? 0 : (float)(time - lastTime_);
    lastTime_ = time;

    // Atualiza estados dos botões
    if (isBtnUp_) game_->handleInput(0.08f, 0.0f);
    if (isBtnDown_) game_->handleInput(-0.08f, 0.0f);
    if (isBtnLeft_) game_->handleInput(0.0f, -0.05f);
    if (isBtnRight_) game_->handleInput(0.0f, 0.05f);

    game_->update(deltaTime);

    if (shaderNeedsNewProjectionMatrix_) {
        float projectionMatrix[16];
        Utility::buildOrthographicMatrix(projectionMatrix, 1.0f, (float)width_ / height_, -1.0f, 1.0f);
        shader_->setProjectionMatrix(projectionMatrix);
        shaderNeedsNewProjectionMatrix_ = false;
    }

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // 1. Renderizar Teto e Piso (Tiling)
    if (ceilingTex_) {
        std::vector<Vertex> ceilV = {
            Vertex({-3.0f, 1.0f, 0.98f}, {0, 0}), Vertex({3.0f, 1.0f, 0.98f}, {12, 0}),
            Vertex({3.0f, 0.0f, 0.98f}, {12, 12}), Vertex({-3.0f, 0.0f, 0.98f}, {0, 12})
        };
        shader_->drawModel(Model(ceilV, {0, 1, 2, 0, 2, 3}, ceilingTex_));
    }
    if (floorTex_) {
        std::vector<Vertex> floorV = {
            Vertex({-3.0f, 0.0f, 0.98f}, {0, 0}), Vertex({3.0f, 0.0f, 0.98f}, {12, 0}),
            Vertex({3.0f, -1.0f, 0.98f}, {12, 12}), Vertex({-3.0f, -1.0f, 0.98f}, {0, 12})
        };
        shader_->drawModel(Model(floorV, {0, 1, 2, 0, 2, 3}, floorTex_));
    }

    const auto& map = game_->getMap();
    float pX = game_->getPlayerX(), pY = game_->getPlayerY(), pDir = game_->getPlayerDir();
    float fov = 1.0f;
    int numRays = 80;

    std::shared_ptr<TextureAsset> wall = (game_->getCurrentLevel() == 1) ? wallTex1_ : (game_->getCurrentLevel() == 2 ? wallTex2_ : wallTex3_);

    // 2. Raycasting Paredes
    for (int i = 0; i < numRays; i++) {
        float rayAngle = (pDir - fov / 2.0f) + (float)i / (float)numRays * fov;
        float rx = std::cos(rayAngle), ry = std::sin(rayAngle);
        float dist = 0.01f; bool hit = false;
        while (!hit && dist < 15.0f) {
            dist += 0.08f;
            int tx = (int)(pX + rx * dist), ty = (int)(pY + ry * dist);
            if (ty < 0 || ty >= (int)map.size() || tx < 0 || tx >= (int)map[0].size() || map[ty][tx] == '#') hit = true;
        }
        float h = 1.0f / (dist * std::cos(rayAngle - pDir) + 0.1f);
        float x1 = -1.0f + 2.0f * (float)i / numRays, x2 = -1.0f + 2.0f * (float)(i+1) / numRays;
        std::vector<Vertex> v = {Vertex({x1,h,0.5f},{0,0}), Vertex({x2,h,0.5f},{1,0}), Vertex({x2,-h,0.5f},{1,1}), Vertex({x1,-h,0.5f},{0,1})};
        if (wall) shader_->drawModel(Model(v, {0, 1, 2, 0, 2, 3}, wall));
    }

    // 3. Inimigos e Boss Animados
    for (const auto& e : game_->getEntities()) {
        if (!e.active) continue;
        float dx = e.x - pX, dy = e.y - pY, d = std::sqrt(dx*dx + dy*dy);
        float angle = std::atan2(dy, dx) - pDir;
        while (angle < -M_PI) angle += 2.0f * M_PI; while (angle > M_PI) angle -= 2.0f * M_PI;
        if (std::abs(angle) < fov) {
            float sh = (e.isBoss ? 2.8f : 1.2f) / (d + 0.1f), sx = angle / (fov / 2.0f), sz = (e.isBoss ? 1.0f : 0.4f) / (d + 0.1f);
            std::shared_ptr<TextureAsset> frame = nullptr;
            if (e.isBoss && !bossFrames_.empty()) {
                int fIdx = (int)(time * 12.0f) % (int)bossFrames_.size();
                frame = bossFrames_[fIdx];
            } else if (!enemyFrames_.empty()) {
                int fIdx = (int)(time * 6.0f) % (int)enemyFrames_.size();
                frame = enemyFrames_[fIdx];
            } else { frame = enemyTex_; }
            if (frame) {
                std::vector<Vertex> v = {Vertex({sx-sz,sh,0.1f},{0,0}), Vertex({sx+sz,sh,0.1f},{1,0}), Vertex({sx+sz,-sh,0.1f},{1,1}), Vertex({sx-sz,-sh,0.1f},{0,1})};
                shader_->drawModel(Model(v, {0, 1, 2, 0, 2, 3}, frame));
            }
        }
    }

    // 4. Arma HUD
    std::shared_ptr<TextureAsset> gun = nullptr;
    Weapon w = game_->getWeapon();
    if (w == Weapon::PISTOL && !pistolFrames_.empty()) gun = pistolFrames_[std::min((int)(game_->getAnimTimer() * 12.0f), (int)pistolFrames_.size()-1)];
    else if (w == Weapon::SHOTGUN && !shotgunFrames_.empty()) gun = shotgunFrames_[std::min((int)(game_->getAnimTimer() * 8.0f), (int)shotgunFrames_.size()-1)];
    else if (!rpgFrames_.empty()) gun = rpgFrames_[std::min((int)(game_->getAnimTimer() * 20.0f), (int)rpgFrames_.size()-1)];

    if (gun) {
        float bX = game_->isFiring() ? 0 : std::sin(game_->getWalkTimer() * 8.0f) * 0.05f;
        float bY = game_->isFiring() ? 0 : std::abs(std::cos(game_->getWalkTimer() * 8.0f)) * 0.03f;
        float ws = (w == Weapon::BAZOOKA) ? 1.1f : 0.7f;
        std::vector<Vertex> wv = {Vertex({-ws+bX,-0.25f+bY,0.05f},{0,0}), Vertex({ws+bX,-0.25f+bY,0.05f},{1,0}), Vertex({ws+bX,-1.35f+bY,0.05f},{1,1}), Vertex({-ws+bX,-1.35f+bY,0.05f},{0,1})};
        shader_->drawModel(Model(wv, {0, 1, 2, 0, 2, 3}, gun));
    }

    // 5. Botões HUD Reduzidos (0.06f) e Afastados
    float bS = 0.06f;
    drawButton(-1.8f, -0.5f, bS, isBtnUp_ ? btnUpP_ : btnUp_);
    drawButton(-1.8f, -0.85f, bS, isBtnDown_ ? btnDownP_ : btnDown_);
    drawButton(-1.95f, -0.68f, bS, isBtnLeft_ ? btnLeftP_ : btnLeft_);
    drawButton(-1.65f, -0.68f, bS, isBtnRight_ ? btnRightP_ : btnRight_);
    drawButton(1.6f, -0.65f, bS * 2.2f, isBtnShoot_ ? btnShootP_ : btnShoot_);

    eglSwapBuffers(display_, surface_);
}

void Renderer::initRenderer() {
    auto display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    eglInitialize(display, nullptr, nullptr);
    EGLint numConfigs;
    EGLint attribs[] = {EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT, EGL_SURFACE_TYPE, EGL_WINDOW_BIT, EGL_BLUE_SIZE, 8, EGL_GREEN_SIZE, 8, EGL_RED_SIZE, 8, EGL_DEPTH_SIZE, 24, EGL_NONE};
    EGLConfig config; eglChooseConfig(display, attribs, &config, 1, &numConfigs);
    EGLSurface surface = eglCreateWindowSurface(display, config, app_->window, nullptr);
    EGLint ctxAttribs[] = {EGL_CONTEXT_CLIENT_VERSION, 3, EGL_NONE};
    EGLContext context = eglCreateContext(display, config, nullptr, ctxAttribs);
    eglMakeCurrent(display, surface, surface, context);
    display_ = display; surface_ = surface; context_ = context; width_ = -1; height_ = -1;
    shader_ = std::unique_ptr<Shader>(Shader::loadShader(vertex, fragment, "inPosition", "inUV", "uProjection"));
    shader_->activate();
    glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    createModels();
}

void Renderer::updateRenderArea() {
    EGLint width, height; eglQuerySurface(display_, surface_, EGL_WIDTH, &width); eglQuerySurface(display_, surface_, EGL_HEIGHT, &height);
    if (width != width_ || height != height_) { width_ = width; height_ = height; glViewport(0, 0, width, height); shaderNeedsNewProjectionMatrix_ = true; }
}

void Renderer::createModels() {
    auto am = app_->activity->assetManager;
    wallTex1_ = TextureAsset::loadAsset(am, "textures/wall02_1.png");
    wallTex2_ = TextureAsset::loadAsset(am, "textures/brks_1.png");
    wallTex3_ = TextureAsset::loadAsset(am, "textures/marb1.png");
    floorTex_ = TextureAsset::loadAsset(am, "textures/con_01.png", GL_REPEAT);
    ceilingTex_ = TextureAsset::loadAsset(am, "textures/sky1.png", GL_REPEAT);

    enemyTex_ = TextureAsset::loadAsset(am, "husk_mutant/ZOMBA1.png");
    for (char f : {'A', 'B', 'C', 'D'}) {
        std::string path = "husk_mutant/ZOMB"; path += f; path += "1.png";
        auto p = TextureAsset::loadAsset(am, path); if(p) enemyFrames_.push_back(p);
    }

    // Subset do Boss (Amostra de 50 frames de 0 a 1800 para evitar OOM)
    for (int i = 0; i < 1871; i += 40) {
        char buf[10]; sprintf(buf, "%04d", i);
        std::string path = "boss/03Normalmale/Z"; path += buf; path += ".png";
        auto p = TextureAsset::loadAsset(am, path); if(p) bossFrames_.push_back(p);
    }

    btnUp_ = TextureAsset::loadAsset(am, "mobile_buttons_png/upbutton.png");
    btnDown_ = TextureAsset::loadAsset(am, "mobile_buttons_png/downbutton.png");
    btnLeft_ = TextureAsset::loadAsset(am, "mobile_buttons_png/leftbutton.png");
    btnRight_ = TextureAsset::loadAsset(am, "mobile_buttons_png/rightbutton.png");
    btnShoot_ = TextureAsset::loadAsset(am, "mobile_buttons_png/shootbutton.png");
    btnUpP_ = TextureAsset::loadAsset(am, "mobile_buttons_png/upbuttonpressed.png");
    btnDownP_ = TextureAsset::loadAsset(am, "mobile_buttons_png/downbuttonpressed.png");
    btnLeftP_ = TextureAsset::loadAsset(am, "mobile_buttons_png/leftbuttonpressed.png");
    btnRightP_ = TextureAsset::loadAsset(am, "mobile_buttons_png/rightbuttonpressed.png");
    btnShootP_ = TextureAsset::loadAsset(am, "mobile_buttons_png/shootbuttonpressed.png");

    for (int i = 1; i <= 5; i++) {
        auto p = TextureAsset::loadAsset(am, "frames/pistol" + std::to_string(i) + ".png"); if(p) pistolFrames_.push_back(p);
        auto s = TextureAsset::loadAsset(am, "frames/shotgun" + std::to_string(i) + ".png"); if(s) shotgunFrames_.push_back(s);
    }
    for (int i = 1; i <= 31; i++) {
        if (i == 28) continue;
        auto r = TextureAsset::loadAsset(am, "frames/rpg/" + std::to_string(i) + ".png"); if(r) rpgFrames_.push_back(r);
    }
}

void Renderer::handleInput() {
    auto *ib = android_app_swap_input_buffers(app_);
    if (!ib) return;
    isBtnUp_ = isBtnDown_ = isBtnLeft_ = isBtnRight_ = isBtnShoot_ = false;
    for (auto i = 0; i < ib->motionEventsCount; i++) {
        auto &me = ib->motionEvents[i];
        int action = (me.action & AMOTION_EVENT_ACTION_MASK);
        if (action == AMOTION_EVENT_ACTION_UP || action == AMOTION_EVENT_ACTION_CANCEL) continue;

        float x = GameActivityPointerAxes_getX(&me.pointers[0]);
        float y = GameActivityPointerAxes_getY(&me.pointers[0]);

        // Input detection logic
        if (x > 100 && x < 300 && y > 500 && y < 700) isBtnUp_ = true;
    }
}
