#ifndef ANDROIDGLINVESTIGATIONS_RENDERER_H
#define ANDROIDGLINVESTIGATIONS_RENDERER_H

#include <EGL/egl.h>
#include <memory>
#include <vector>
#include <map>
#include "Model.h"
#include "Shader.h"
#include "Game.h"

struct android_app;

class Renderer {
public:
    Renderer(android_app *pApp);
    virtual ~Renderer();
    void handleInput();
    void render();

private:
    void initRenderer();
    void updateRenderArea();
    void createModels();
    void drawButton(float x, float y, float size, const std::shared_ptr<TextureAsset>& tex);

    android_app *app_;
    EGLDisplay display_;
    EGLSurface surface_;
    EGLContext context_;
    EGLint width_;
    EGLint height_;

    bool shaderNeedsNewProjectionMatrix_;
    std::unique_ptr<Shader> shader_;
    std::vector<Model> models_;

    // Texturas de Ambiente e Piso
    std::shared_ptr<TextureAsset> wallTex1_, wallTex2_, wallTex3_;
    std::shared_ptr<TextureAsset> floorTex_;
    std::shared_ptr<TextureAsset> ceilingTex_;

    // Inimigos e Boss (Sprites animados)
    std::vector<std::shared_ptr<TextureAsset>> enemyFrames_;
    std::vector<std::shared_ptr<TextureAsset>> bossFrames_;
    std::shared_ptr<TextureAsset> enemyTex_;
    std::shared_ptr<TextureAsset> bossTex_;

    // Frames das Armas
    std::vector<std::shared_ptr<TextureAsset>> pistolFrames_;
    std::vector<std::shared_ptr<TextureAsset>> shotgunFrames_;
    std::vector<std::shared_ptr<TextureAsset>> rpgFrames_;

    // Botões
    std::shared_ptr<TextureAsset> btnUp_, btnDown_, btnLeft_, btnRight_, btnShoot_;
    std::shared_ptr<TextureAsset> btnUpP_, btnDownP_, btnLeftP_, btnRightP_, btnShootP_;
    bool isBtnUp_, isBtnDown_, isBtnLeft_, isBtnRight_, isBtnShoot_;

    std::unique_ptr<Game> game_;
    double lastTime_;
};

#endif
