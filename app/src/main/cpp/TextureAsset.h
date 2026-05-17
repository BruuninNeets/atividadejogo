#ifndef ANDROIDGLINVESTIGATIONS_TEXTUREASSET_H
#define ANDROIDGLINVESTIGATIONS_TEXTUREASSET_H

#include <memory>
#include <android/asset_manager.h>
#include <GLES3/gl3.h>
#include <string>
#include <vector>

class TextureAsset : public std::enable_shared_from_this<TextureAsset> {
public:
    static std::shared_ptr<TextureAsset>
    loadAsset(AAssetManager *assetManager, const std::string &assetPath, GLint wrapMode = GL_CLAMP_TO_EDGE);

    ~TextureAsset();

    constexpr GLuint getTextureID() const { return textureID_; }

private:
    inline TextureAsset(GLuint textureId) : textureID_(textureId) {}

    GLuint textureID_;
};

#endif
