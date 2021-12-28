#include <map>
#include "TextSample.h"
#include "../util/GLUtils.h"
#include "../util/LogUtil.h"
#include <ft2build.h>
#include <freetype/freetype.h>
#include <ext.hpp>
#include FT_FREETYPE_H

const std::string ASSETS_DIR = "/sdcard/Android/data/com.chenxf.opengles/files/Download";

TextSample::TextSample() {
    m_pShader = nullptr;
}

TextSample::~TextSample() {
}



void TextSample::Init() {
    LOGCATE("TextSample::Init");
    if (m_pShader != nullptr)
        return;

    char vShaderStr[] =
            "#version 300 es                       \n"
            "layout (location = 0) in vec4 vertex; \n"
            "out vec2 TexCoords;                   \n"
            "uniform mat4 projection;              \n"
            "void main()                           \n"
            "{                                     \n"
            "    gl_Position = projection * vec4(vertex.xy, 0.0, 1.0);\n"
            "    TexCoords = vertex.zw;            \n"
            "}";

    char fShaderStr[] =
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

    m_pShader = new Shader(vShaderStr, fShaderStr);
    initFreeType();
    LOGCATE("TextSample::Init done");
}

int TextSample::initFreeType() {
    LOGCATE("initFreeType start");

    FT_Library ft;
    // All functions return a value different than 0 whenever an error occurred
    if (FT_Init_FreeType(&ft)) {
        LOGCATE("ERROR::FREETYPE: Could not init FreeType Library");
        return -1;
    }

    // find path to fonts
    std::string font_name = ASSETS_DIR + "/fonts/Antonio-Bold.ttf";
    if (font_name.empty()) {
        LOGCATE("ERROR::FREETYPE: Failed to load font_name");
        return -1;
    }

    // load fonts as face
    FT_Face face;
    if (FT_New_Face(ft, font_name.c_str(), 0, &face)) {
        LOGCATE("ERROR::FREETYPE: Failed to load fonts");
        return -1;
    } else {
        // set size to load glyphs as
        FT_Set_Pixel_Sizes(face, 0, 48);

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
            LOGCATE("initFreeType, add one texture: width %d, height %d", face->glyph->bitmap.width, face->glyph->bitmap.rows);
            unsigned int texture;
            glGenTextures(1, &texture);
            glBindTexture(GL_TEXTURE_2D, texture);
            glTexImage2D(
                    GL_TEXTURE_2D,
                    0,
                    GL_R8,//TODO very important, use GL_RED will fail for GLES 3.0. see https://stackoverflow.com/questions/70285879/android12-opengles3-0-glteximage2d-0x502-error
                    face->glyph->bitmap.width,
                    face->glyph->bitmap.rows,
                    0,
                    GL_RED,
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
            Characters.insert(std::pair<char, Character>(c, character));
        }
        glBindTexture(GL_TEXTURE_2D, 0);
    }
    // destroy FreeType once we're finished
    FT_Done_Face(face);
    FT_Done_FreeType(ft);

    LOGCATE("FT_Done_FreeType done ");


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

    LOGCATE("initFreeType done ");
    return 0;
}

// render line of text
// -------------------
void RenderText(Shader *shader, std::string text, float x, float y, float scale, glm::vec3 color) {
    LOGCATE("RenderText start ");

    // activate corresponding render state
    //shader->use();
    shader->setVec3("textColor", color);
    //glUniform3f(glGetUniformLocation(shader->ID, "textColor"), color.x, color.y, color.z);
    glActiveTexture(GL_TEXTURE0);
    glBindVertexArray(VAO);

    // iterate through all characters
    std::string::const_iterator c;
    for (c = text.begin(); c != text.end(); c++) {
        Character ch = Characters[*c];

        float xpos = x + ch.Bearing.x * scale;
        float ypos = y - (ch.Size.y - ch.Bearing.y) * scale;

        float w = ch.Size.x * scale;
        float h = ch.Size.y * scale;
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

void TextSample::Draw(int screenW, int screenH) {
    LOGCATE("Draw2, screenW  %d screenH %d ", screenW, screenH);

    if(m_pShader == nullptr)
        return;

    DEBUG_LOGCATE();
    glClear(GL_STENCIL_BUFFER_BIT | GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glClearColor(1.0, 1.0, 1.0, 1.0);
    // enable alpha blending

    glEnable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    m_pShader->use();

    glm::mat4 projection = glm::ortho(0.0f, static_cast<float>(screenW), 0.0f, static_cast<float>(screenH));
    m_pShader->setMat4("projection", projection);
    RenderText(m_pShader, "This is sample text DDDDDDDDDDDD", 25.0f, 25.0f, 1.0f, glm::vec3(0.5f, 0.8f, 0.2f));
    RenderText(m_pShader, "(C) LearnOpenGL.com", 540.0f, 570.0f, 0.5f, glm::vec3(1.0f, 0.7f, 0.9f));
}

void TextSample::Destroy() {
    LOGCATE("TextSample::Destroy");
    if (m_pShader != nullptr) {
        m_pShader->Destroy();
        delete m_pShader;
        m_pShader = nullptr;
    }
    LOGCATE("TextSample::Destroy done");
}

void TextSample::checkGLError(const char *msg) {
    int error;
    if ((error = glGetError()) != GL_NO_ERROR) {
        // TODO 抛出异常
        LOGCATE("%s: GL error: %x", msg, error);
    }
}
