#include <map>
#include "EnglishTextSample.h"
#include "../util/GLUtils.h"
#include "../util/LogUtil.h"
#include <ft2build.h>
#include <freetype/freetype.h>
#include <freetype/ftglyph.h>
#include <ext.hpp>
#include FT_FREETYPE_H

const std::string ASSETS_DIR = "/sdcard/Android/data/com.chenxf.opengles/files/Download";

const char vShaderStr[] =
        "#version 300 es                       \n"
        "layout (location = 0) in vec4 vertex; \n"
        "out vec2 TexCoords;                   \n"
        "uniform mat4 u_MVPMatrix;              \n"
        "void main()                           \n"
        "{                                     \n"
        "    gl_Position = u_MVPMatrix * vec4(vertex.xy, 0.0, 1.0);\n"
        "    TexCoords = vertex.zw;            \n"
        "}";

const char fShaderStr[] =
        "#version 300 es            \n"
        "in vec2 TexCoords;         \n"
        "out vec4 color;            \n"
        "uniform sampler2D text;    \n"
        "uniform vec3 textColor;    \n"
        "void main()                \n"
        "{                          \n"
        "    vec4 sampled = vec4(1.0, 1.0, 1.0, texture(text, TexCoords).r);\n"
        "    color = vec4(textColor, 1.0) * sampled;                        \n"
        "}";

EnglishTextSample::EnglishTextSample() {
    m_pShader = nullptr;

    m_ScaleX = 1.0f;
    m_ScaleY = 1.0f;
    m_AngleX = 0;
    m_AngleY = 0;
}

EnglishTextSample::~EnglishTextSample() {
}


void EnglishTextSample::Init() {
    LOGCATE("EnglishTextSample::Init");
    if (m_pShader != nullptr)
        return;
    m_pShader = new Shader(vShaderStr, fShaderStr);
    initFreeTypeASCII();
    initGLES();
    LOGCATE("EnglishTextSample::Init done");
}

int EnglishTextSample::initFreeTypeASCII() {
    LOGCATE("initFreeTypeASCII start");

    FT_Library ft;
    // All functions return a value different than 0 whenever an error occurred
    if (FT_Init_FreeType(&ft)) {
        LOGCATE("ERROR::FREETYPE: Could not init FreeType Library");
        return -1;
    }

    // find path to fonts
    std::string font_name = ASSETS_DIR + "/fonts/Antonio-Regular.ttf";
    if (font_name.empty()) {
        LOGCATE("ERROR::FREETYPE: Failed to load font_name");
        return -1;
    }

    // load fonts as face
    FT_Face face;
    if (FT_New_Face(ft, font_name.c_str(), 0, &face)) {
        LOGCATE("ERROR::FREETYPE: Failed to load fonts");
        return -1;
    }
    // set size to load glyphs as
    FT_Set_Pixel_Sizes(face, 0, 96);

    // disable byte-alignment restriction
    //禁用字节对齐限制
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    // load first 128 characters of ASCII set
    for (unsigned char c = 0; c < 128; c++) {
        // Load character glyph
        if (FT_Load_Char(face, c, FT_LOAD_RENDER)) {
            LOGCATE("ERROR::FREETYTPE: Failed to load Glyph");
            continue;
        }
        // generate texture
        LOGCATE("initFreeTypeASCII, add one texture: width %d, height %d",
                face->glyph->bitmap.width, face->glyph->bitmap.rows);
        unsigned int texture;
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexImage2D(
                GL_TEXTURE_2D,
                0,
                GL_LUMINANCE,//TODO very important, use GL_RED will fail for GLES 3.0. see https://stackoverflow.com/questions/70285879/android12-opengles3-0-glteximage2d-0x502-error
                face->glyph->bitmap.width,
                face->glyph->bitmap.rows,
                0,
                GL_LUMINANCE,
                GL_UNSIGNED_BYTE,
                face->glyph->bitmap.buffer
        );
        checkGLError("glTexImage2D");
        // set texture options
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        // now store character for later use
        Character character = {
                texture,
                glm::ivec2(face->glyph->bitmap.width, face->glyph->bitmap.rows),
                glm::ivec2(face->glyph->bitmap_left, face->glyph->bitmap_top),
                static_cast<unsigned int>(face->glyph->advance.x)
        };
        mCharacters.insert(std::pair<GLint, Character>(c, character));
    }
    glBindTexture(GL_TEXTURE_2D, 0);
    // destroy FreeType once we're finished
    FT_Done_Face(face);
    FT_Done_FreeType(ft);

    LOGCATE("FT_Done_FreeType done ");

    LOGCATE("initFreeTypeASCII done ");
    return 0;
}


void EnglishTextSample::initGLES() {
    // configure VAO/VBO for texture quads
    // -----------------------------------
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6 * 4, NULL, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}


// render line of text
// -------------------
void EnglishTextSample::RenderText(Shader *shader, std::string text, float x, float y, float scale,
                                   glm::vec3 color, glm::vec2 viewport) {
    LOGCATE("RenderText start %s", text.c_str());

    // activate corresponding render state
    //shader->use();
    shader->setVec3("textColor", color);
    //glUniform3f(glGetUniformLocation(shader->ID, "textColor"), color.x, color.y, color.z);
    glActiveTexture(GL_TEXTURE0);
    glBindVertexArray(VAO);

    x *= viewport.x;
    y *= viewport.y;
    // iterate through all characters
    std::string::const_iterator c;
    for (c = text.begin(); c != text.end(); c++) {
        Character ch = mCharacters[*c];

        float xpos = x + ch.Bearing.x * scale;
        float ypos = y - (ch.Size.y - ch.Bearing.y) * scale;

        xpos /= viewport.x;
        ypos /= viewport.y;
        float w = ch.Size.x * scale;
        float h = ch.Size.y * scale;

        w /= viewport.x;
        h /= viewport.y;
        // update VBO for each character
        float vertices[6][4] = {
                {xpos,     ypos + h, 0.0f, 0.0f},
                {xpos,     ypos,     0.0f, 1.0f},
                {xpos + w, ypos,     1.0f, 1.0f},

                {xpos,     ypos + h, 0.0f, 0.0f},
                {xpos + w, ypos,     1.0f, 1.0f},
                {xpos + w, ypos + h, 1.0f, 0.0f}
        };
        // render glyph texture over quad
        glBindTexture(GL_TEXTURE_2D, ch.TextureID);
        LOGCATD("glBindTexture %d", ch.TextureID);
        // update content of VBO memory
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices),
                        vertices); // be sure to use glBufferSubData and not glBufferData

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        // render quad
        glDrawArrays(GL_TRIANGLES, 0, 6);
        // now advance cursors for next glyph (note that advance is number of 1/64 pixels)
        x += (ch.Advance >> 6) *
             scale; // bitshift by 6 to get value in pixels (2^6 = 64 (divide amount of 1/64th pixels by 64 to get amount of pixels))
    }
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
    LOGCATE("RenderText done ");

}

void EnglishTextSample::Draw(int screenW, int screenH) {
    LOGCATE("Draw2, screenW  %d screenH %d ", screenW, screenH);

    if (m_pShader == nullptr)
        return;

    DEBUG_LOGCATE();
    glClear(GL_STENCIL_BUFFER_BIT | GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glClearColor(1.0, 1.0, 1.0, 1.0);
    // enable alpha blending

    glEnable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    m_pShader->use();

    glm::vec2 viewport(screenW, screenH);
    UpdateMVPMatrix(m_MVPMatrix, m_AngleX, m_AngleY, viewport.x / viewport.y);

    m_pShader->setMat4("u_MVPMatrix", m_MVPMatrix);
    RenderText(m_pShader, "Hello World", 0.0f, 0.0f, 2.0f,
               glm::vec3(0.2, 0.4f, 0.7f), viewport);

}

void
EnglishTextSample::UpdateTransformMatrix(float rotateX, float rotateY, float scaleX, float scaleY) {
    GLSampleBase::UpdateTransformMatrix(rotateX, rotateY, scaleX, scaleY);
    m_AngleX = static_cast<int>(rotateX);
    m_AngleY = static_cast<int>(rotateY);
    m_ScaleX = scaleX;
    m_ScaleY = scaleY;
}

/**
 * @param angleX 绕X轴旋转度数
 * @param angleY 绕Y轴旋转度数
 * @param ratio 宽高比
 * */
void EnglishTextSample::UpdateMVPMatrix(glm::mat4 &mvpMatrix, int angleX, int angleY, float ratio) {
    LOGCATE("TextRenderSample::UpdateMVPMatrix angleX = %d, angleY = %d, ratio = %f", angleX,
            angleY, ratio);
    angleX = angleX % 360;
    angleY = angleY % 360;

    //转化为弧度角
    float radiansX = static_cast<float>(MATH_PI / 180.0f * angleX);
    float radiansY = static_cast<float>(MATH_PI / 180.0f * angleY);


    // Projection matrix
    glm::mat4 Projection = glm::ortho(-1.0f, 1.0f, -1.0f, 1.0f, 0.1f, 100.0f);
    //glm::mat4 Projection = glm::frustum(-ratio, ratio, -1.0f, 1.0f, 4.0f, 100.0f);
    //glm::mat4 Projection = glm::perspective(45.0f,ratio, 0.1f,100.f);

    // View matrix
    glm::mat4 View = glm::lookAt(
            glm::vec3(0, 0, 4), // Camera is at (0,0,1), in World Space
            glm::vec3(0, 0, 0), // and looks at the origin
            glm::vec3(0, 1, 0)  // Head is up (set to 0,-1,0 to look upside-down)
    );

    // Model matrix
    glm::mat4 Model = glm::mat4(1.0f);
    Model = glm::scale(Model, glm::vec3(m_ScaleX, m_ScaleY, 1.0f));
    Model = glm::rotate(Model, radiansX, glm::vec3(1.0f, 0.0f, 0.0f));
    Model = glm::rotate(Model, radiansY, glm::vec3(0.0f, 1.0f, 0.0f));
    Model = glm::translate(Model, glm::vec3(0.0f, 0.0f, 0.0f));

    mvpMatrix = Projection * View * Model;

}

void EnglishTextSample::Destroy() {
    LOGCATE("EnglishTextSample::Destroy");
    if (m_pShader != nullptr) {
        m_pShader->Destroy();
        delete m_pShader;
        m_pShader = nullptr;
    }
    LOGCATE("EnglishTextSample::Destroy done");
}

void EnglishTextSample::checkGLError(const char *msg) {
    int error;
    if ((error = glGetError()) != GL_NO_ERROR) {
        // TODO 抛出异常
        LOGCATE("%s: GL error: %x", msg, error);
    }
}
