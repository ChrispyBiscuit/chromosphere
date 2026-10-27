#pragma once
// INFRA's place in the shared Chromosphere look: the infrared palette (the Theme defaults), its embedded fonts and
// solar images, and its update channel.
#include "Look.h"
#include "Updater.h"
#include <InfraData.h>

#ifndef INFRA_UPDATE_REPO
#define INFRA_UPDATE_REPO "ChrispyBiscuit/chromosphere"
#endif

inline look::Theme infraTheme() {
    look::Theme t;                                   // the defaults are INFRA's infrared
    t.resource = [](const char* name, int& size) -> const char* { return InfraData::getNamedResource(name, size); };
    return t;
}

inline UpdaterProduct infraUpdaterProduct() {
    UpdaterProduct p;                                // the defaults are INFRA's
    p.repo = INFRA_UPDATE_REPO;
    return p;
}
