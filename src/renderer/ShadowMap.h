#pragma once

#include <glad/glad.h>

namespace Renderer {

// ShadowMap owns a framebuffer with a single depth texture attachment.
// Usage each frame:
//
//   1. shadowMap.bindForWriting();          // shadow pass: render scene depth
//      glViewport(0, 0, SHADOW_W, SHADOW_H);
//      // ... draw all shadow-casting geometry with shadow shader ...
//
//   2. shadowMap.bindTextureToUnit(5);       // main pass: sample the depth texture
//      shader.setInt("u_shadowMap", 5);
//
// Resolution: higher = sharper shadows, more VRAM.
// 2048x2048 is a good default for medium-range scenes.

class ShadowMap {
public:
    static constexpr int WIDTH  = 2048;
    static constexpr int HEIGHT = 2048;

    ShadowMap();
    ~ShadowMap();

    ShadowMap(const ShadowMap&)            = delete;
    ShadowMap& operator=(const ShadowMap&) = delete;

    // Bind the shadow FBO for the depth-write pass.
    // Call glViewport(0,0,WIDTH,HEIGHT) after this.
    void bindForWriting() const;

    // Restore the default framebuffer after the shadow pass.
    // Call glViewport(0,0,screenW,screenH) after this.
    void unbindWriting() const;

    // Bind the depth texture to a specific texture unit for sampling
    // in the main lighting shader.
    void bindTextureToUnit(GLuint unit) const;

    GLuint getDepthTexture() const { return m_depthTexture; }

private:
    GLuint m_fbo          = 0;
    GLuint m_depthTexture = 0;
};

} // namespace Renderer