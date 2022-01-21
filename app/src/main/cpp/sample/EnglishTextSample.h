#ifndef _TEXT_SAMPLE_ENGLISH_H
#define _TEXT_SAMPLE_ENGLISH_H


#include "GLSampleBase.h"
#include <shader.h>

class EnglishTextSample : public GLSampleBase {
public:

    /// Holds all state information relevant to a character as loaded using FreeType
    struct Character {
        unsigned int TextureID; // ID handle of the glyph texture
        glm::ivec2 Size;      // Size of glyph
        glm::ivec2 Bearing;   // Offset from baseline to left/top of glyph
        unsigned int Advance;   // Horizontal offset to advance to next glyph
    };

    EnglishTextSample();

    virtual ~EnglishTextSample();

    virtual void Init();

    virtual void Draw(int screenW, int screenH);

    virtual void Destroy();

    virtual void UpdateTransformMatrix(float rotateX, float rotateY, float scaleX, float scaleY);

private:
    void checkGLError(const char *msg);

    int initFreeTypeASCII();
    void initGLES();
    void RenderText(Shader *shader, std::string text, float x, float y, float scale, glm::vec3 color, glm::vec2 viewport);
    void UpdateMVPMatrix(glm::mat4 &mvpMatrix, int angleX, int angleY, float ratio);
    Shader *m_pShader;
    std::map<GLint, Character> mCharacters;
    unsigned int VAO, VBO;
    glm::mat4 m_MVPMatrix;

    float m_ScaleX;
    float m_ScaleY;
    int m_AngleX;
    int m_AngleY;

};


#endif
