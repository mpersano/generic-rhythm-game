#pragma once

#include <gx/shaderprogram.h>

#include <array>
#include <memory>

namespace GX::GL {
class ShaderProgram;
};

class ShaderManager
{
public:
    ~ShaderManager();

    enum Program {
        Debug,
        Decal,
        DecalFog,
        Lighting,
        LightingFog,
        LightingFogClip,
        NumPrograms
    };
    void useProgram(Program program);

    enum Uniform {
        ModelViewProjection,
        ProjectionMatrix,
        ViewMatrix,
        ModelMatrix,
        NormalMatrix,
        ModelViewMatrix,
        BaseColorTexture,
        Eye,
        FogColor,
        FogDistance,
        LightPosition,
        ClipPlane,
        NumUniforms
    };

    template<typename T>
    void setUniform(Uniform uniform, T &&value)
    {
        if (!m_currentProgram)
            return;
        const auto location = uniformLocation(uniform);
        if (location == -1)
            return;
        m_currentProgram->program->setUniform(location, std::forward<T>(value));
    }

    // HACK FIXME: can't rely on m_currentProgram always having the currently bound program,
    // other components bind shaders outside ShaderManager#@!!!
    void clearCurrentProgram()
    {
        m_currentProgram = nullptr;
    }

private:
    int uniformLocation(Uniform uniform);

    struct CachedProgram {
        std::unique_ptr<GX::GL::ShaderProgram> program;
        std::array<int, Uniform::NumUniforms> uniformLocations;
    };
    std::array<std::unique_ptr<CachedProgram>, Program::NumPrograms> m_cachedPrograms;
    CachedProgram *m_currentProgram = nullptr;
};
