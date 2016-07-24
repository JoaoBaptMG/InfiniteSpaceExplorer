//
//  CustomGLPrograms.cpp
//  SpaceExplorer
//
//  Created by João Baptista on 09/07/15.
//
//

#include "CustomGLPrograms.h"
#include "cocos2d.h"

using namespace cocos2d;

#define STRINGIFY(x) #x

static const GLchar PlayerShapeProgram_frag[] = "\n#ifdef GL_ES\nvarying lowp vec4 v_fragmentColor; varying mediump vec2 v_texCoord;"
"\n#else\n varying vec4 v_fragmentColor; varying vec2 v_texCoord; \n#endif\n"
"uniform vec3 tintColor; uniform float tintAmount;"
"void main() { gl_FragColor = v_fragmentColor * texture2D(CC_Texture0, v_texCoord); "
"gl_FragColor.rgb = mix(gl_FragColor.rgb, tintColor * gl_FragColor.a, tintAmount); }";

static const GLchar LifeMarkerProgram_frag[] = "\n#ifdef GL_ES\nvarying lowp vec4 v_fragmentColor; varying mediump vec2 v_texCoord;"
"\n#else\n varying vec4 v_fragmentColor; varying vec2 v_texCoord; \n#endif\n"
"void main() { gl_FragColor = texture2D(CC_Texture0, v_texCoord); "
"gl_FragColor *= 0.5 + 0.25*step(v_texCoord.x, v_fragmentColor.r) + 0.25*step(v_texCoord.x, v_fragmentColor.g); }";

static const GLchar BezierProgram_frag[] =
"\n#ifdef GL_ES\nvarying lowp vec4 v_fragmentColor; varying mediump vec2 v_texCoord;"
"\n#else\nvarying vec4 v_fragmentColor; varying vec2 v_texCoord;\n#endif\n"
STRINGIFY(
uniform vec2 dTdx;
uniform vec2 dTdy;
uniform float curveWidth;

void main()
{
    vec2 p = v_texCoord;
    
    float fx = (2.0*p.x)*dTdx.x - dTdx.y;
    float fy = (2.0*p.y)*dTdy.x - dTdy.y;
    
    float dist = (p.x*p.x - p.y)/sqrt(fx*fx + fy*fy);
    
    float weight = 0.5 + curveWidth - abs(dist);
    
    gl_FragColor = v_fragmentColor;
    gl_FragColor.a *= clamp(weight, 0.0, 1.0);
}
);

static const GLchar ShooterProgram_frag[] =
"\n#ifdef GL_ES\nvarying lowp vec4 v_fragmentColor;varying mediump vec2 v_texCoord;"
"\n#else\nvarying vec4 v_fragmentColor;varying vec2 v_texCoord;\n#endif\n"
"void main() { gl_FragColor = texture2D(CC_Texture0, v_texCoord); "
"vec3 color = gl_FragColor.r * mix(v_fragmentColor.rgb, vec3(1.0, 1.0, 1.0), gl_FragColor.g); "
"gl_FragColor.rgb = color; }";

static const GLchar PowerupIconProgram_frag[] =
"uniform float pixelSize, tolerance;"
STRINGIFY(
\n#ifdef GL_ES\n
varying lowp vec4 v_fragmentColor;
varying mediump vec2 v_texCoord;
varying mediump vec2 v_vertCoord;
\n#else\n
varying vec4 v_fragmentColor;
varying vec2 v_texCoord;
varying vec2 v_vertCoord;
\n#endif\n

void main()
{
    vec2 pos = (v_texCoord - vec2(0.5, 0.5))*2.0;
    
    float angle = 6.28318530718 * dot(v_fragmentColor.rg, vec2(1.0, 0.00390625)) / v_fragmentColor.a;
    float distance = length(pos)/pixelSize;
    
    float distAngle = mod(1.57079632679 + atan(pos.y, pos.x), 6.28318530718);
    float angleCont = distance == 0.0 ? 1.0 : 0.25 + 0.75 * smoothstep(angle*distance - 0.5*tolerance, angle*distance + 0.5*tolerance, distAngle*distance);
    
    gl_FragColor = texture2D(CC_Texture0, v_texCoord) * angleCont * v_fragmentColor.a;
}
);

static const GLchar BackgroundProgram_frag[] = STRINGIFY(
\n#ifdef GL_ES\n
precision lowp float;
\n#endif\n
                                                         
varying vec4 v_fragmentColor;
varying vec2 v_texCoord;
                                                         
void main()
{
    vec4 tex = texture2D(CC_Texture0, v_texCoord);
    gl_FragColor = v_fragmentColor * tex;
    gl_FragColor.a = tex.r;
}
);

static const GLchar ScoreWidgetProgram_frag[] = "\n#ifdef GL_ES\nvarying lowp vec4 v_fragmentColor; varying mediump vec2 v_texCoord;"
"\n#else\n varying vec4 v_fragmentColor; varying vec2 v_texCoord; \n#endif\nuniform mediump float size; "
"void main() { gl_FragColor = v_fragmentColor * texture2D(CC_Texture0, v_texCoord); "
"float dist = distance(vec2(0.5,0.5), v_texCoord); "
"gl_FragColor.a *= clamp(size*(0.5 - dist), 0.0, 1.0); }";

void loadCustomProgram(const GLchar *vertexShader, const GLchar *fragmentShader, const std::string &key)
{
    GLProgram *program = GLProgramCache::getInstance()->getGLProgram(key);
    
    if (program != nullptr)
    {
        program->reset();
        program->initWithByteArrays(vertexShader, fragmentShader);
        program->link();
        program->updateUniforms();
    }
    else GLProgramCache::getInstance()->addGLProgram(GLProgram::createWithByteArrays(vertexShader, fragmentShader), key);
}

void loadCustomGLPrograms()
{
    log("Loading custom programs!");
    
    loadCustomProgram(ccPositionTextureColor_noMVP_vert, PlayerShapeProgram_frag, "PlayerShapeProgram");
    loadCustomProgram(ccPositionTextureColor_noMVP_vert, LifeMarkerProgram_frag, "LifeMarkerProgram");
    loadCustomProgram(ccPositionTextureColor_noMVP_vert, BezierProgram_frag, "BezierProgram");
    loadCustomProgram(ccPositionTextureColor_noMVP_vert, ShooterProgram_frag, "ShooterProgram");
    loadCustomProgram(ccPositionTextureColor_noMVP_vert, PowerupIconProgram_frag, "PowerupIconProgram");
    loadCustomProgram(ccPositionTextureColor_noMVP_vert, BackgroundProgram_frag, "BackgroundProgram");
    loadCustomProgram(ccPositionTextureColor_noMVP_vert, ScoreWidgetProgram_frag, "ScoreWidgetProgram");
}
