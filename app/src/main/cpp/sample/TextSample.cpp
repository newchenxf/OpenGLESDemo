#include <map>
#include "TextSample.h"
#include "../util/GLUtils.h"
#include "../util/LogUtil.h"

#define WSTRING_SOURCE(src) L""#src

const std::string ASSETS_DIR = "/sdcard/Android/data/com.chenxf.opengles/files/Download";
static const int MAX_SHORT_VALUE = 65536;
static const wchar_t CHINESE_COMMON[] = WSTRING_SOURCE(
                                                ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghigklmnopqrstuvwxyz12367890的一是了我不人在他有这个上们来到时大地为子中你说生国年着就那和要她出也得里后自以会家可下而过天去能对小多然于心学么之都好看起发当没成只如事把还用第样道想作种开美总从无情己面最女但现前些所同日手又行意动方期它头经长儿回位分爱老因很给名法间斯知世什两次使身者被高已亲其进此话常与活正感见明问力理尔点文几定本公特做外孩相西果走将月十实向声车全信重三机工物气每并别真打太新比才便夫再书部水像眼等体却加电主界门利海受听表德少克代员许稜先口由死安写性马光白或住难望教命花结乐色更拉东神记处让母父应直字场平报友关放至张认接告入笑内英军候民岁往何度山觉路带万男边风解叫任金快原吃妈变通师立象数四失满战远格士音轻目条呢病始达深完今提求清王化空业思切怎非找片罗钱紶吗语元喜曾离飞科言干流欢约各即指合反题必该论交终林请医晚制球决窢传画保读运及则房早院量苦火布品近坐产答星精视五连司巴奇管类未朋且婚台夜青北队久乎越观落尽形影红爸百令周吧识步希亚术留市半热送兴造谈容极随演收首根讲整式取照办强石古华諣拿计您装似足双妻尼转诉米称丽客南领节衣站黑刻统断福城故历惊脸选包紧争另建维绝树系伤示愿持千史谁准联妇纪基买志静阿诗独复痛消社算算义竟确酒需单治卡幸兰念举仅钟怕共毛句息功官待究跟穿室易游程号居考突皮哪费倒价图具刚脑永歌响商礼细专黄块脚味灵改据般破引食仍存众注笔甚某沉血备习校默务土微娘须试怀料调广蜖苏显赛查密议底列富梦错座参八除跑亮假印设线温虽掉京初养香停际致阳纸李纳验助激够严证帝饭忘趣支春集丈木研班普导顿睡展跳获艺六波察群皇段急庭创区奥器谢弟店否害草排背止组州朝封睛板角况曲馆育忙质河续哥呼若推境遇雨标姐充围案伦护冷警贝著雪索剧啊船险烟依斗值帮汉慢佛肯闻唱沙局伯族低玩资屋击速顾泪洲团圣旁堂兵七露园牛哭旅街劳型烈姑陈莫鱼异抱宝权鲁简态级票怪寻杀律胜份汽右洋范床舞秘午登楼贵吸责例追较职属渐左录丝牙党继托赶章智冲叶胡吉卖坚喝肉遗救修松临藏担戏善卫药悲敢靠伊村戴词森耳差短祖云规窗散迷油旧适乡架恩投弹铁博雷府压超负勒杂醒洗采毫嘴毕九冰既状乱景席珍童顶派素脱农疑练野按犯拍征坏骨余承置臓彩灯巨琴免环姆暗换技翻束增忍餐洛塞缺忆判欧层付阵玛批岛项狗休懂武革良恶恋委拥娜妙探呀营退摇弄桌熟诺宣银势奖宫忽套康供优课鸟喊降夏困刘罪亡鞋健模败伴守挥鲜财孤枪禁恐伙杰迹妹藸遍盖副坦牌江顺秋萨菜划授归浪听凡预奶雄升碃编典袋莱含盛济蒙棋端腿招释介烧误);
static const wchar_t CHINESE_TEST[] = WSTRING_SOURCE(Love小爱心HAHA);
#define ENABLE_ROTATE 0

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

TextSample::TextSample() {
    m_pShader = nullptr;
    m_ScaleX = 1.0f;
    m_ScaleY = 1.0f;
    m_AngleX = 0;
    m_AngleY = 0;
    ft = nullptr;
}

TextSample::~TextSample() {
    LOGCATE("TextSample:: release");
    // Destroy FreeType
    if (ft != nullptr) {
        FT_Done_FreeType(ft);
    }
}


void TextSample::Init() {
    LOGCATD("TextSample::Init");
    if (m_pShader != nullptr)
        return;
    m_pShader = new Shader(vShaderStr, fShaderStr);
    //add common used text
    hasFTLoaded = initFreeType();
    if (hasFTLoaded) {
        makeTextAsGLTexture(CHINESE_COMMON, sizeof(CHINESE_COMMON) / sizeof(CHINESE_COMMON[0]) - 1);
    }
    initGLES();
    LOGCATD("TextSample::Init done");
}


int TextSample::makeTextAsGLTexture(const wchar_t *text, int size) {
    // find path to fonts
    std::string font_name = ASSETS_DIR + "/fonts/chinese_lvshu.ttf";

    // load fonts as face
    FT_Face face;
    if (FT_New_Face(ft, font_name.c_str(), 0, &face)) {
        LOGCATE("ERROR::FREETYPE: Failed to load fonts");
        return false;
    }

    // Set size to load glyphs as
    FT_Set_Pixel_Sizes(face, 0, 96);
    FT_Select_Charmap(face, ft_encoding_unicode);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    for (int i = 0; i < size; ++i) {
        //int index =  FT_Get_Char_Index(face,unicodeArr[i]);
        if (FT_Load_Glyph(face, FT_Get_Char_Index(face, text[i]), FT_LOAD_DEFAULT)) {
            LOGCATE("Failed to load Glyph");
            continue;
        }

        FT_Glyph glyph;
        FT_Get_Glyph(face->glyph, &glyph);

        //Convert the glyph to a bitmap.
        FT_Glyph_To_Bitmap(&glyph, ft_render_mode_normal, 0, 1);
        FT_BitmapGlyph bitmap_glyph = (FT_BitmapGlyph) glyph;

        //This reference will make accessing the bitmap easier
        FT_Bitmap &bitmap = bitmap_glyph->bitmap;

        // Generate texture
        GLuint texture;
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexImage2D(
                GL_TEXTURE_2D,
                0,
                GL_LUMINANCE,
                bitmap.width,
                bitmap.rows,
                0,
                GL_LUMINANCE,
                GL_UNSIGNED_BYTE,
                bitmap.buffer
        );

        LOGCATE("initFreeType textureId %d, text[i]=%d [w,h,buffer]=[%d, %d, %p], advance.x=%ld",
                texture, text[i], bitmap.width, bitmap.rows, bitmap.buffer,
                glyph->advance.x / MAX_SHORT_VALUE);
        // Set texture options
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        // Now store character for later use
        Character character = {
                texture,
                glm::ivec2(bitmap.width, bitmap.rows),
                glm::ivec2(face->glyph->bitmap_left, face->glyph->bitmap_top),
                static_cast<GLuint>((glyph->advance.x / MAX_SHORT_VALUE) << 6)
        };
        LOGCATE("initFreeType, add to slot[%d], size (%d,%d), bearing(%d, %d)", text[i],
                bitmap.width, bitmap.rows, face->glyph->bitmap_left, face->glyph->bitmap_top);
        mCharacters.insert(std::pair<GLint, Character>(text[i], character));
    }
    glBindTexture(GL_TEXTURE_2D, 0);
    FT_Done_Face(face);
    return 0;
}

bool TextSample::initFreeType() {
    LOGCATE("initFreeType start");

    // All functions return a value different than 0 whenever an error occurred
    if (FT_Init_FreeType(&ft)) {
        LOGCATE("ERROR::FREETYPE: Could not init FreeType Library");
        return false;
    }

    // find path to fonts
    std::string font_name = ASSETS_DIR + "/fonts/Antonio-Bold.ttf";
    if (font_name.empty()) {
        LOGCATE("ERROR::FREETYPE: Failed to load font_name");
        return false;
    }
    // load fonts as face
    FT_Face face;
    if (FT_New_Face(ft, font_name.c_str(), 0, &face)) {
        LOGCATE("ERROR::FREETYPE: Failed to load fonts");
        return false;
    }

    LOGCATD("initFreeType done");
    return true;

}

void TextSample::initGLES() {
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

void TextSample::Draw(int screenW, int screenH) {
    LOGCATD("Draw2, screenW  %d screenH %d ", screenW, screenH);

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

    RenderTextChinese(m_pShader, CHINESE_TEST,
                      sizeof(CHINESE_TEST) / sizeof(CHINESE_TEST[0]) - 1, -0.5f, 0.0f, 1.0f,
                      glm::vec3(0.5f, 0.8f, 0.2f), viewport);
}

void TextSample::UpdateTransformMatrix(float rotateX, float rotateY, float scaleX, float scaleY) {
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
void TextSample::UpdateMVPMatrix(glm::mat4 &mvpMatrix, int angleX, int angleY, float ratio) {
    LOGCATD("TextRenderSample::UpdateMVPMatrix angleX = %d, angleY = %d, ratio = %f", angleX,
            angleY, ratio);
    angleX = angleX % 360;
    angleY = angleY % 360;

    //转化为弧度角
    float radiansX = static_cast<float>(MATH_PI / 180.0f * angleX);
    float radiansY = static_cast<float>(MATH_PI / 180.0f * angleY);


    // Projection matrix
    glm::mat4 Projection = glm::ortho(-1.0f, 1.0f, -1.0f, 1.0f, 0.1f, 100.0f);

    // View matrix
    glm::mat4 View = glm::lookAt(
            glm::vec3(0, 0, 4), // Camera is at (0,0,1), in World Space
            glm::vec3(0, 0, 0), // and looks at the origin
            glm::vec3(0, 1, 0)  // Head is up (set to 0,-1,0 to look upside-down)
    );

    // Model matrix
    glm::mat4 Model = glm::mat4(1.0f);
    Model = glm::scale(Model, glm::vec3(m_ScaleX, m_ScaleY, 1.0f));
#if ENABLE_ROTATE
    Model = glm::rotate(Model, radiansX, glm::vec3(1.0f, 0.0f, 0.0f));
    Model = glm::rotate(Model, radiansY, glm::vec3(0.0f, 1.0f, 0.0f));
#endif
    Model = glm::translate(Model, glm::vec3(0.0f, 0.0f, 0.0f));

    mvpMatrix = Projection * View * Model;
}

/**
 * t character, if not exist, will make a new one
 * @param oneText
 * @param ch
 */
void TextSample::getCharacter(const wchar_t oneText, Character &ch) {
    if (mCharacters.find(oneText) != mCharacters.end()) {
        ch = mCharacters[oneText];
    } else {
        LOGCATD("getCharacter, make a new text");
        const wchar_t temp[] = {oneText};
        makeTextAsGLTexture(temp, 1);
        ch = mCharacters[oneText];
    }
}

void TextSample::RenderTextChinese(Shader *shader, const wchar_t *text, int textLen, GLfloat x,
                                   GLfloat y, GLfloat scale,
                                   glm::vec3 color, glm::vec2 viewport) {
    // 激活合适的渲染状态
    shader->setVec3("textColor", color);
    glBindVertexArray(VAO);
    checkGLError("RenderTextChinese");
    x *= viewport.x;
    y *= viewport.y;
    for (int i = 0; i < textLen; ++i) {
        Character ch;
        getCharacter(text[i], ch);
        LOGCATD("RenderTextChinese, slot[%d], textureId %d", text[i], ch.TextureID);

        GLfloat xpos = x + ch.Bearing.x * scale;
        GLfloat ypos = y - (ch.Size.y - ch.Bearing.y) * scale;

        xpos /= viewport.x;
        ypos /= viewport.y;

        GLfloat w = ch.Size.x * scale;
        GLfloat h = ch.Size.y * scale;

        w /= viewport.x;
        h /= viewport.y;

        LOGCATD("RenderTextChinese [xpos,ypos,w,h]=[%f, %f, %f, %f]", xpos, ypos, w, h);

        // 当前字符的VBO
        GLfloat vertices[6][4] = {
                {xpos,     ypos + h, 0.0, 0.0},
                {xpos,     ypos,     0.0, 1.0},
                {xpos + w, ypos,     1.0, 1.0},

                {xpos,     ypos + h, 0.0, 0.0},
                {xpos + w, ypos,     1.0, 1.0},
                {xpos + w, ypos + h, 1.0, 0.0}
        };

        // 在方块上绘制字形纹理
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, ch.TextureID);
        //glUniform1i(m_SamplerLoc, 0);
        checkGLError("RenderTextChinese 2");
        // 更新当前字符的VBO
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
        checkGLError("RenderTextChinese 3");
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        // 绘制方块
        glDrawArrays(GL_TRIANGLES, 0, 6);
        checkGLError("RenderTextChinese 4");
        // 更新位置到下一个字形的原点，注意单位是1/64像素
        x += (ch.Advance >> 6) * scale; //(2^6 = 64)
    }
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void TextSample::Destroy() {
    LOGCATW("TextSample::Destroy");
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
