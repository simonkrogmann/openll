#include <cassert>
#include <iostream>
#include <map>
#include <random>
#include <chrono>


#include <glm/gtc/matrix_transform.hpp>

#include <openll/GlyphRenderer.h>
#include <openll/FontLoader.h>
#include <openll/Typesetter.h>
#include <openll/stages/GlyphPreparationStage.h>

#include <openll/FontFace.h>
#include <openll/GlyphSequence.h>
#include <openll/Alignment.h>
#include <openll/LineAnchor.h>
#include <openll/SuperSampling.h>
#include <openll/layout/layoutbase.h>
#include <openll/layout/algorithm.h>

#include <cpplocate/cpplocate.h>
#include <cpplocate/ModuleInfo.h>

#include "benchmark.h"

using namespace gl;

long int g_seed = 0;

struct Algorithm
{
    std::string name;
    std::function<void(std::vector<gloperate_text::Label>&)> function;
};

using namespace std::placeholders;

std::vector<Algorithm> layoutAlgorithms
{
    //{"constant",                          gloperate_text::layout::constant},
    {"random",                            gloperate_text::layout::random},
    {"greedyArea",                  std::bind(gloperate_text::layout::greedy, _1, gloperate_text::layout::standard)},
    {"discreteGradientDescentArea", std::bind(gloperate_text::layout::discreteGradientDescent, _1, gloperate_text::layout::standard)},
    //{"simulatedAnnealingArea",      std::bind(gloperate_text::layout::simulatedAnnealing, _1, gloperate_text::layout::overlapArea, false, glm::vec2(0.f))},
    //{"simulatedAnnealing",                std::bind(gloperate_text::layout::simulatedAnnealing, _1, gloperate_text::layout::standard, false, glm::vec2(0.f))},
    //{"simulatedAnnealingPadding",   std::bind(gloperate_text::layout::simulatedAnnealing, _1, gloperate_text::layout::standard, false, glm::vec2(0.2f))},
    {"simulatedAnnealingSelection", std::bind(gloperate_text::layout::simulatedAnnealing, _1, gloperate_text::layout::standard, false, glm::vec2(0.f))},
    //{"simulatedAnnealingEverything",   std::bind(gloperate_text::layout::simulatedAnnealing, _1, gloperate_text::layout::standard, true, glm::vec2(0.2f))},
};


std::string random_name(std::default_random_engine engine)
{
    std::uniform_int_distribution<int> upperDistribution(65, 90);
    std::uniform_int_distribution<int> lowerDistribution(97, 122);
    std::uniform_int_distribution<int> lengthDistribution(3, 14);
    const auto length = lengthDistribution(engine);
    std::vector<char> characters;
    characters.push_back(static_cast<char>(upperDistribution(engine)));
    for (int i = 0; i < length; ++i)
    {
        characters.push_back(static_cast<char>(lowerDistribution(engine)));
    }
    return {characters.begin(), characters.end()};
}

std::vector<gloperate_text::Label> prepareLabels(gloperate_text::FontFace * font, glm::uvec2 viewport, int numLabels)
{
    std::vector<gloperate_text::Label> labels;

    std::default_random_engine generator;
    generator.seed(g_seed);
    std::uniform_real_distribution<float> distribution(-1.f, 1.f);
    std::uniform_int_distribution<unsigned int> priorityDistribution(1, 10);

    for (int i = 0; i < numLabels; ++i)
    {
        const auto string = random_name(generator);
        const auto priority = priorityDistribution(generator);
        gloperate_text::GlyphSequence sequence;
        std::u32string unicode_string(string.begin(), string.end());
        sequence.setString(unicode_string);
        sequence.setWordWrap(true);
        sequence.setLineWidth(400.f);
        sequence.setAlignment(gloperate_text::Alignment::LeftAligned);
        sequence.setLineAnchor(gloperate_text::LineAnchor::Ascent);
        sequence.setFontSize(10.f + priority);
        sequence.setFontFace(font);
        sequence.setFontColor(glm::vec4(glm::vec3(0.5f - priority * 0.05f), 1.f));
        sequence.setSuperSampling(gloperate_text::SuperSampling::Quincunx);

        const auto origin = glm::vec2{distribution(generator), distribution(generator)};
        // compute  transform matrix
        glm::mat4 transform;
        transform = glm::translate(transform, glm::vec3(origin, 0.f));
        transform = glm::scale(transform, glm::vec3(1.f,
            static_cast<float>(viewport.x) / viewport.y, 1.f));
        transform = glm::scale(transform, glm::vec3(1 / 300.f));

        const auto placement = gloperate_text::LabelPlacement{ glm::vec2{ 0.f, 0.f }
            , gloperate_text::Alignment::LeftAligned, gloperate_text::LineAnchor::Baseline, true };

        sequence.setAdditionalTransform(transform);
        labels.push_back({sequence, origin, priority, placement});
    }
    return labels;
}

void runAndBenchmark(std::vector<gloperate_text::Label> & labels, Algorithm algorithm)
{
    auto start = std::chrono::steady_clock::now();
    algorithm.function(labels);
    auto end = std::chrono::steady_clock::now();
    std::chrono::duration<double> diff = end - start;
    auto positions = labelPositionDesirability(labels);
    std::cout
        //<< algorithm.name
        <<", "<< diff.count();
        //<<", "<< labelPenalty(labels, gloperate_text::layout::standard);
        /*<<", "<< labelsHidden(labels)
        <<", "<< labelOverlaps(labels)
        <<", "<< labelOverlapArea(labels)
        <<", "<< labelOverlaps(labels, {0.2f, 0.2f})
        <<", "<< labelOverlapArea(labels, {0.2f, 0.2f})
        <<", "<< positions[gloperate_text::RelativeLabelPosition::UpperRight]
        <<", "<< positions[gloperate_text::RelativeLabelPosition::UpperLeft]
        <<", "<< positions[gloperate_text::RelativeLabelPosition::LowerLeft]
        <<", "<< positions[gloperate_text::RelativeLabelPosition::LowerRight]*/
        //<< std::endl;
}


int main()
{
    cpplocate::ModuleInfo moduleInfo = cpplocate::findModule("openll");
    std::string dataPath = moduleInfo.value("dataPath");

    gloperate_text::FontLoader loader;
    auto font = loader.load(dataPath + "/fonts/opensansr36/opensansr36.fnt", true);
    std::vector<gloperate_text::Label> labels;

    for (int i = 10; i < 500; i += 10)
    {
        std::cout << i;
        for (const auto & algo : layoutAlgorithms)
        {
            labels = prepareLabels(font, {640.f, 480.f}, i);
            runAndBenchmark(labels, algo);
        }
        std::cout << std::endl;
    }
}
