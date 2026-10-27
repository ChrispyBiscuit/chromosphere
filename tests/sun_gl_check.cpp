// Opens a real window with the two sun views using OpenGL, runs the message loop briefly, and
// reports whether the GL context came up and the shader compiled. Exit code 1 if not.
#include "PluginEditor.h"
#include "Sun.h"
#include <cstdio>
#include <cmath>
#include <X11/Xlib.h>

// Not a JUCEApplication, so JUCE leaves Xlib's default error handler in place, which exits on harmless
// WM-hint failures under a bare Xvfb. A host would ignore those; so do we.
static int ignoreXErrors(Display*, XErrorEvent*) { return 0; }

int main(int argc, char** argv) {
    const bool useGl = !(argc > 1 && juce::String(argv[1]) == "--no-gl");
    XSetErrorHandler(ignoreXErrors);
    juce::ScopedJuceInitialiser_GUI init;
    ChromosphereProcessor proc;
    proc.setPlayConfigDetails(2, 2, 48000, 256);
    proc.prepareToPlay(48000, 256);
    auto* ed = new ChromosphereEditor(proc, useGl);
    std::unique_ptr<juce::Component> owner(ed);
    ed->setSize(1080, 700);
    ed->setOpaque(true);
    ed->addToDesktop(0);
    ed->setVisible(true);
    const int hold = argc > 2 ? juce::String(argv[2]).getIntValue() : 60;
    const bool idle = argc > 3 && juce::String(argv[3]) == "idle";   // no audio: measure the editor's own cost
    juce::AudioBuffer<float> buf(2, 256); juce::MidiBuffer midi; double ph = 0;
    for (int i = 0; i < hold; ++i) {
        for (int k = 0; k < (idle ? 0 : 8); ++k) {       // ~43 ms of a 220 Hz tone per 50 ms of wall time: the suns should light up
            for (int n = 0; n < 256; ++n) { const float v = 0.35f * std::sin((float)ph) + 0.35f * std::sin((float)ph * 0.25f); /* 220 Hz plus 55 Hz: level and bass */ ph += 2.0 * 3.14159265 * 220.0 / 48000.0; buf.setSample(0, n, v); buf.setSample(1, n, v); }
            proc.processBlock(buf, midi);
        }
        juce::MessageManager::getInstance()->runDispatchLoopUntil(50);
    }
    // Reach into the editor: both suns must be on the GL path with a working shader.
    bool ok = true;
    for (auto* c : ed->getChildren()) if (auto* sv = dynamic_cast<sun::SunView*>(c)) { std::printf("emblem: openGL %d\n", (int)sv->usingOpenGL()); ok = ok && sv->usingOpenGL(); }
    for (auto* c : ed->getChildren()) for (auto* cc : c->getChildren()) if (auto* sv = dynamic_cast<sun::SunView*>(cc)) { std::printf("scope: openGL %d, energy %.2f\n", (int)sv->usingOpenGL(), sv->energy()); ok = ok && sv->usingOpenGL(); }
    ed->removeFromDesktop();
    std::printf(ok ? "sun GL check passed\n" : "sun GL check FAILED (CPU fallback would be used)\n");
    return ok ? 0 : 1;
}
