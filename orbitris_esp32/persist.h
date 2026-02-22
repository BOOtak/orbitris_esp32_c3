#pragma once

#include "stats.h"

bool load_stats(Stats &out);

bool persist_stats(const Stats &stats);
