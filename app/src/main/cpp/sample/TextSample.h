#ifndef _NDK_OPENGLES_3_0_TEXT_H
#define _NDK_OPENGLES_3_0_TEXT_H


#include "GLSampleBase.h"
#include <shader.h>

class TextSample : public GLSampleBase {
public:


    /// Holds all state information relevant to a character as loaded using FreeType
    struct Character {
        unsigned int TextureID; // ID handle of the glyph texture
        glm::ivec2 Size;      // Size of glyph
        glm::ivec2 Bearing;   // Offset from baseline to left/top of glyph
        unsigned int Advance;   // Horizontal offset to advance to next glyph
    };

    TextSample();

    virtual ~TextSample();

    virtual void Init();

    virtual void Draw(int screenW, int screenH);

    virtual void Destroy();

private:
    void checkGLError(const char *msg);

    int initFreeType();

    Shader *m_pShader;
    std::map<GLchar, Character> Characters;
    unsigned int VAO, VBO;

};


#endif //NDK_OPENGLES_3_0_TRIANGLESAMPLE_H
