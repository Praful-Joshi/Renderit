#include "ShadowMap.h"

#include <stdexcept>

namespace Renderer {

ShadowMap::ShadowMap() {

    // ── Depth texture ─────────────────────────────────────────────────────────
    // GL_DEPTH_COMPONENT24: 24-bit depth precision — matches typical hardware.
    // We store only depth, no colour.
    glGenTextures(1, &m_depthTexture);
    glBindTexture(GL_TEXTURE_2D, m_depthTexture);

    glTexImage2D(GL_TEXTURE_2D,
                 0,
                 GL_DEPTH_COMPONENT24,  // internal format: 24-bit depth
                 WIDTH, HEIGHT,
                 0,
                 GL_DEPTH_COMPONENT,    // data format
                 GL_FLOAT,              // data type
                 nullptr);              // no CPU data — GPU will fill it

    // Nearest filtering: for depth comparison we want exact texel values,
    // not interpolated ones. Linear filtering on depth makes PCF look odd.
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    // Clamp to border with depth 1.0 (maximum depth = fully lit).
    // Without this, UVs outside [0,1] — i.e. fragments outside the light
    // frustum — would repeat the shadow map edge and appear incorrectly shadowed.
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

    glBindTexture(GL_TEXTURE_2D, 0);

    // ── Framebuffer ───────────────────────────────────────────────────────────
    glGenFramebuffers(1, &m_fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);

    // Attach our depth texture as the depth buffer.
    // No colour attachment — this is a depth-only FBO.
    glFramebufferTexture2D(GL_FRAMEBUFFER,
                           GL_DEPTH_ATTACHMENT,
                           GL_TEXTURE_2D,
                           m_depthTexture,
                           0); // mip level 0

    // Tell OpenGL explicitly: no colour reads or writes.
    // Required — without this the FBO is incomplete on some drivers.
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        throw std::runtime_error("[ShadowMap] Framebuffer incomplete");

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

ShadowMap::~ShadowMap() {
    if (m_fbo)          glDeleteFramebuffers(1, &m_fbo);
    if (m_depthTexture) glDeleteTextures(1, &m_depthTexture);
}

void ShadowMap::bindForWriting() const {
    glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
    glClear(GL_DEPTH_BUFFER_BIT);  // clear last frame's depth data
}

void ShadowMap::unbindWriting() const {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void ShadowMap::bindTextureToUnit(GLuint unit) const {
    glActiveTexture(GL_TEXTURE0 + unit);
    glBindTexture(GL_TEXTURE_2D, m_depthTexture);
}

} // namespace Renderer