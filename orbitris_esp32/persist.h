#pragma once

#include "highscore.h"
#include "stats.h"

bool load_highscores(HighscoreTable& out);

bool persist_highscores(const HighscoreTable& highscores);

bool load_stats(Stats &out);

bool persist_stats(const Stats &stats);
