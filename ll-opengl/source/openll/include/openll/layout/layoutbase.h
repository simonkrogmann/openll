#pragma once

#include <glm/vec2.hpp>

#include <openll/LineAnchor.h>
#include <openll/Alignment.h>
#include <openll/GlyphSequence.h>

#include <openll/openll_api.h>

namespace gloperate_text
{

class GlyphSequence;

struct OPENLL_API LabelPlacement
{
    glm::vec2 offset;
    gloperate_text::Alignment alignment;
    gloperate_text::LineAnchor lineAnchor;
    bool display;
};

struct OPENLL_API Label
{
    GlyphSequence sequence;
    glm::vec2 pointLocation;
    unsigned int priority;
    LabelPlacement placement;
};

GlyphSequence OPENLL_API applyPlacement(const Label & label);

} // namespace gloperate_text
