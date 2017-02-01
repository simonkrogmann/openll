#pragma once

#include <vector>

#include <openll/layout/layoutbase.h>
#include <openll/layout/algorithm.h>
#include <openll/layout/RelativeLabelPosition.h>

int labelOverlaps(const std::vector<gloperate_text::Label> & labels, const glm::vec2 & relativePadding = {0.f, 0.f});
float labelOverlapArea(const std::vector<gloperate_text::Label> & labels, const glm::vec2 & relativePadding = {0.f, 0.f});
int labelsHidden(const std::vector<gloperate_text::Label> & labels);
float labelPenalty(const std::vector<gloperate_text::Label> & labels, gloperate_text::layout::PenaltyFunction penaltyFunction);
std::map<gloperate_text::RelativeLabelPosition, unsigned int>  labelPositionDesirability(const std::vector<gloperate_text::Label> & labels);
