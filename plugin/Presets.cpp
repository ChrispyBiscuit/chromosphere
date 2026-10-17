#include "Presets.h"
#include "PluginProcessor.h"
#include <BinaryData.h>

PresetManager::PresetManager(FancyReverbProcessor& p) : proc_(p) { reload(); }

juce::UndoManager& PresetManager::undo() { return proc_.undoManager; }

juce::File PresetManager::userFolder() const {
    return juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory).getChildFile("FancyReverb").getChildFile("Presets");
}

void PresetManager::reload() {
    const juce::String keep = currentName();
    presets_.clear();
    // Factory presets from the embedded JSON.
    const juce::var root = juce::JSON::parse(juce::String::fromUTF8(BinaryData::factory_json, BinaryData::factory_jsonSize));
    if (auto* list = root["presets"].getArray())
        for (const auto& item : *list) {
            Preset p;
            p.name = item["name"].toString(); p.category = item["category"].toString(); p.ir = item["ir"].toString();
            if (auto* t = item["tags"].getArray()) for (const auto& s : *t) p.tags.add(s.toString());
            if (auto* o = item["params"].getDynamicObject()) p.params = o->getProperties();
            presets_.push_back(std::move(p));
        }
    // User presets.
    for (const auto& f : userFolder().findChildFiles(juce::File::findFiles, false, "*.frpreset")) {
        if (auto xml = juce::parseXML(f)) {
            if (!xml->hasTagName("FRPreset")) continue;
            Preset p;
            p.factory = false; p.file = f;
            p.name = xml->getStringAttribute("name", f.getFileNameWithoutExtension());
            p.category = xml->getStringAttribute("category", "User"); p.ir = xml->getStringAttribute("ir");
            p.tags.addTokens(xml->getStringAttribute("tags"), ",", "");
            p.tags.trim(); p.tags.removeEmptyStrings();
            for (auto* e : xml->getChildWithTagNameIterator("PARAM")) p.params.set(e->getStringAttribute("id"), e->getDoubleAttribute("value"));
            presets_.push_back(std::move(p));
        }
    }
    current_ = indexOf(keep);
}

int PresetManager::indexOf(const juce::String& name) const {
    for (size_t i = 0; i < presets_.size(); ++i) if (presets_[i].name == name) return (int)i;
    return -1;
}

void PresetManager::applyOne(juce::RangedAudioParameter& prm, const juce::var& v) {
    float plain;
    if (auto* choice = dynamic_cast<juce::AudioParameterChoice*>(&prm)) {
        plain = v.isString() ? (float)choice->choices.indexOf(v.toString()) : (float)(double)v;
        if (plain < 0) plain = prm.convertFrom0to1(prm.getDefaultValue());
    } else if (v.isBool()) plain = (bool)v ? 1.f : 0.f;
    else plain = (float)(double)v;
    prm.setValueNotifyingHost(prm.convertTo0to1(plain));
}

void PresetManager::setParameters(const juce::NamedValueSet& values) {
    for (auto* prm : proc_.getParameters()) {
        auto* r = dynamic_cast<juce::RangedAudioParameter*>(prm);
        if (!r) continue;
        const juce::String id = r->getParameterID();
        if (id == "freeze") continue;                      // a preset never starts frozen (the input would be muted)
        if (values.contains(id)) applyOne(*r, values[id]);
        else r->setValueNotifyingHost(r->getDefaultValue());
    }
}

bool PresetManager::apply(int index) {
    if (index < 0 || index >= (int)presets_.size()) return false;
    const Preset& p = presets_[(size_t)index];
    undo().beginNewTransaction("Load preset " + p.name);
    setParameters(p.params);
    if (p.ir.isNotEmpty()) proc_.loadFactoryImpulseResponse(p.ir);
    current_ = index;
    ab_[(size_t)slot_] = proc_.apvts.copyState();
    return true;
}

bool PresetManager::saveUser(const juce::String& name, const juce::String& category, const juce::StringArray& tags) {
    if (name.trim().isEmpty()) return false;
    juce::XmlElement xml("FRPreset");
    xml.setAttribute("name", name.trim()); xml.setAttribute("category", category.isEmpty() ? juce::String("User") : category);
    xml.setAttribute("tags", tags.joinIntoString(","));
    xml.setAttribute("ir", proc_.getImpulseResponsePath().startsWith("factory:") ? proc_.getImpulseResponsePath().fromFirstOccurrenceOf("factory:", false, false) : juce::String());
    for (auto* prm : proc_.getParameters())
        if (auto* r = dynamic_cast<juce::RangedAudioParameter*>(prm)) {
            auto* e = xml.createNewChildElement("PARAM");
            e->setAttribute("id", r->getParameterID());
            e->setAttribute("value", (double)r->convertFrom0to1(r->getValue()));
        }
    auto folder = userFolder();
    if (!folder.createDirectory()) return false;
    juce::String safe = juce::File::createLegalFileName(name.trim());
    if (!xml.writeTo(folder.getChildFile(safe + ".frpreset"))) return false;
    reload();
    current_ = indexOf(name.trim());
    return true;
}

bool PresetManager::deleteUser(int index) {
    if (index < 0 || index >= (int)presets_.size() || presets_[(size_t)index].factory) return false;
    const bool ok = presets_[(size_t)index].file.deleteFile();
    reload();
    return ok;
}

void PresetManager::toggleAB() {
    ab_[(size_t)slot_] = proc_.apvts.copyState();
    slot_ = 1 - slot_;
    if (ab_[(size_t)slot_].isValid()) {
        undo().beginNewTransaction("A/B");
        const juce::String path = ab_[(size_t)slot_].getProperty("irPath").toString();
        proc_.apvts.replaceState(ab_[(size_t)slot_].createCopy());
        if (path != proc_.getImpulseResponsePath()) {
            if (path.startsWith("factory:")) proc_.loadFactoryImpulseResponse(path.fromFirstOccurrenceOf("factory:", false, false));
            else if (path.isNotEmpty() && juce::File(path).existsAsFile()) proc_.loadImpulseResponse(juce::File(path));
        }
    } else ab_[(size_t)slot_] = proc_.apvts.copyState();
}

void PresetManager::copyToOtherSlot() { ab_[(size_t)(1 - slot_)] = proc_.apvts.copyState(); }
