#include <cassert>
#include <iostream>
#include <map>

#include <GLFW/glfw3.h>

#include <glbinding/Binding.h>
#include <glbinding/callbacks.h>
#include <glbinding/gl/gl.h>
#include <globjects/globjects.h>

#include <glm/gtc/matrix_transform.hpp>

#include <openll/GlyphRenderer.h>
#include <openll/FontLoader.h>
#include <openll/stages/GlyphPreparationStage.h>

#include <openll/FontFace.h>
#include <openll/GlyphSequence.h>
#include <openll/Alignment.h>
#include <openll/LineAnchor.h>

#include <cpplocate/cpplocate.h>
#include <cpplocate/ModuleInfo.h>

using namespace gl;

const auto lorem =
R"(Lorem ipsum dolor sit amet, consetetur sadipscing elitr, sed diam nonumy eirmod tempor invidunt ut labore et dolore magna aliquyam erat, sed diam voluptua. At vero eos et accusam et justo duo dolores et ea rebum. Stet clita kasd gubergren, no sea takimata sanctus est Lorem ipsum dolor sit amet. Lorem ipsum dolor sit amet, consetetur sadipscing elitr, sed diam nonumy eirmod tempor invidunt ut labore et dolore magna aliquyam erat, sed diam voluptua. At vero eos et accusam et justo duo dolores et ea rebum. Stet clita kasd gubergren, no sea takimata sanctus est Lorem ipsum dolor sit amet.)";

glm::uvec2 g_viewport{640, 480};
bool g_viewport_changed = true;

void onResize(GLFWwindow*, int width, int height)
{
    g_viewport = {width, height};
    g_viewport_changed = true;
}

void onKeyPress(GLFWwindow* window, int key, int, int action, int mods)
{
    if (key == 'Q' && action == GLFW_PRESS && mods == GLFW_MOD_CONTROL)
    {
        glfwSetWindowShouldClose(window, 1);
    }
}

void glInitialize()
{
    glbinding::Binding::initialize(false);
    glbinding::setCallbackMaskExcept(
        glbinding::CallbackMask::After | glbinding::CallbackMask::Parameters,
        {"glGetError"});
    glbinding::setAfterCallback([](const glbinding::FunctionCall& call) {
        const auto error = glGetError();
        if (error != GL_NO_ERROR)
        {
            std::cout << error << " in " << call.function->name()
                      << " with parameters:" << std::endl;
            for (const auto& parameter : call.parameters)
            {
                std::cout << "    " << parameter->asString() << std::endl;
            }
        }
    });
    globjects::init();
    std::cout << "OpenGL version: " << glGetString(GL_VERSION) << std::endl;
}

gloperate_text::GlyphVertexCloud prepareGlyphSequences(std::string string, gloperate_text::FontFace * font, glm::uvec2 viewport)
{
    std::vector<gloperate_text::GlyphSequence> sequences;

    // font->setLinespace(1.25f);
    gloperate_text::GlyphSequence sequence;
    std::u32string unicode_string(string.begin(), string.end());
    sequence.setString(unicode_string);
    sequence.setWordWrap(true);
    sequence.setLineWidth(500.f);
    sequence.setAlignment(gloperate_text::Alignment::Centered);
    sequence.setLineAnchor(gloperate_text::LineAnchor::Baseline);
    sequence.setFontSize(16.f);
    sequence.setFontFace(font);

    const glm::vec4 margins {0.f, 0.f, 0.f, 0.f};
    const float ppiScale = 1.f;
    const glm::vec2 origin {0.f, .5f};

    // compute  transform matrix
    glm::mat4 transform;
    // translate to lower left in NDC
    transform = glm::translate(transform, glm::vec3(-1.f, -1.f, 0.f));
    // scale glyphs to NDC size
    transform = glm::scale(transform, 2.f / glm::vec3(viewport.x, viewport.y, 1.f));
    // scale glyphs to pixel size with respect to the displays ppi
    transform = glm::scale(transform, glm::vec3(ppiScale));
    // translate to origin in point space - scale origin within
    // margined extend (i.e., viewport with margined areas removed)
    const auto marginedExtent = glm::vec2(viewport.x, viewport.y) / ppiScale
       - glm::vec2(margins[3] + margins[1], margins[2] + margins[0]);
    transform = glm::translate(transform
       , glm::vec3((0.5f * origin + 0.5f) * marginedExtent, 0.f) + glm::vec3(margins[3], margins[2], 0.f));

    sequence.setAdditionalTransform(transform);
    sequences.push_back(sequence);

    return gloperate_text::prepareGlyphs(sequences, true);
}



int main()
{
    glfwInit();
    glfwWindowHint(GLFW_AUTO_ICONIFY, 0);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);

    auto window = glfwCreateWindow(g_viewport.x, g_viewport.y, "Text-Demo", 0, nullptr);

    if (!window)
    {
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);

    glInitialize();

    glfwSetWindowSizeCallback(window, onResize);
    glfwSetKeyCallback(window, onKeyPress);

    cpplocate::ModuleInfo moduleInfo = cpplocate::findModule("openll");
    std::string dataPath = moduleInfo.value("dataPath");

    gloperate_text::FontLoader loader;
    auto font = loader.load(dataPath + "/fonts/opensansr36/opensansr36.fnt");
    gloperate_text::GlyphRenderer renderer;
    gloperate_text::GlyphVertexCloud cloud;
    glClearColor(1.f, 1.f, 1.f, 1.f);

    while (!glfwWindowShouldClose(window))
    {
        if (g_viewport_changed)
        {
            std::cout << "updated viewport (" << g_viewport.x << ", " << g_viewport.y << ")" << std::endl;
            glViewport(0, 0, g_viewport.x, g_viewport.y);
            cloud = prepareGlyphSequences(lorem, font, g_viewport);
        }

        gl::glClear(gl::GL_COLOR_BUFFER_BIT | gl::GL_DEPTH_BUFFER_BIT);

        gl::glDepthMask(gl::GL_FALSE);
        gl::glEnable(gl::GL_CULL_FACE);
        gl::glEnable(gl::GL_BLEND);
        gl::glBlendFunc(gl::GL_SRC_ALPHA, gl::GL_ONE_MINUS_SRC_ALPHA);

        renderer.render(cloud);

        gl::glDepthMask(gl::GL_TRUE);
        gl::glDisable(gl::GL_CULL_FACE);
        gl::glDisable(gl::GL_BLEND);

        g_viewport_changed = false;

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
}
