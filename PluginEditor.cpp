#include "PluginEditor.h"
#include <BinaryData.h>

using namespace juce;

namespace
{
    Image loadImage (const void* data, size_t size)
    {
        return ImageFileFormat::loadFrom (data, size);
    }

    const Colour kPointer { 0xfff4efe3 };

    // centres des boutons sur la façade (grille 840 x 392)
    constexpr Point<float> kInputC  { 191.f, 112.f };
    constexpr Point<float> kCompC   { 420.f, 266.f };
    constexpr Point<float> kOutputC { 602.f, 186.f };
    constexpr Point<float> kMixC    { 742.f, 186.f };

    constexpr float kSmallR = 35.0f, kBigR = 48.0f;
}

// ───────────────────────── Look & feel ─────────────────────────

AmberLookAndFeel::AmberLookAndFeel()
{
    skirtSmall = loadImage (BinaryData::skirtS_png, (size_t) BinaryData::skirtS_pngSize);
    topSmall   = loadImage (BinaryData::topS_png,   (size_t) BinaryData::topS_pngSize);
    skirtBig   = loadImage (BinaryData::skirtB_png, (size_t) BinaryData::skirtB_pngSize);
    topBig     = loadImage (BinaryData::topB_png,   (size_t) BinaryData::topB_pngSize);
}

void AmberLookAndFeel::drawRotarySlider (Graphics& g, int x, int y, int w, int h,
                                         float pos, float a0, float a1, Slider& s)
{
    const bool big = (bool) s.getProperties().getWithDefault ("big", false);
    const auto c   = Rectangle<float> ((float) x, (float) y, (float) w, (float) h).getCentre();

    const float angle = a0 + pos * (a1 - a0);               // 0 = midi (haut), sens horaire
    const float Rk    = big ? kBigR : kSmallR;
    const float half  = big ? 54.0f : 40.0f;                // demi-côté de l'image
    const auto  dest  = Rectangle<float> (c.x - half, c.y - half, half * 2.0f, half * 2.0f);

    const float dx = std::sin (angle), dy = -std::cos (angle);

    auto line = [&] (float r0, float r1, float ox, float oy, float thick, Colour col)
    {
        Path p;
        p.startNewSubPath (c.x + dx * r0 + ox, c.y + dy * r0 + oy);
        p.lineTo          (c.x + dx * r1 + ox, c.y + dy * r1 + oy);
        g.setColour (col);
        g.strokePath (p, PathStrokeType (thick, PathStrokeType::curved, PathStrokeType::rounded));
    };

    // gros bouton : le trait de repère se prolonge jusqu'à la graduation du cadran
    if (big)
    {
        line (44.0f, 80.0f, 0.9f, 1.3f, 2.6f, Colours::black.withAlpha (0.5f));
        line (44.0f, 80.0f, 0.0f, 0.0f, 1.8f, Colour (0xfffbf8ee));
    }

    g.setImageResamplingQuality (Graphics::highResamplingQuality);

    {   // corps cannelé : seul élément qui tourne
        Graphics::ScopedSaveState ss (g);
        g.addTransform (AffineTransform::rotation (angle, c.x, c.y));
        g.drawImage (big ? skirtBig : skirtSmall, dest);
    }

    g.drawImage (big ? topBig : topSmall, dest);   // plateau, reflets : fixes (la lumière ne tourne pas)

    // repère clair sur le plateau
    line (Rk * 0.46f, Rk * 0.98f, 0.8f, 1.1f, 3.4f, Colours::black.withAlpha (0.45f));
    line (Rk * 0.46f, Rk * 0.98f, 0.0f, 0.0f, 2.6f, kPointer);
}

// ───────────────────────── Interrupteur ─────────────────────────

PowerSwitch::PowerSwitch (Image up, Image down)
    : Button ("power"), leverUp (std::move (up)), leverDown (std::move (down))
{
    setClickingTogglesState (true);
    setMouseCursor (MouseCursor::PointingHandCursor);
    setTooltip ("Power (bypass)");
}

void PowerSwitch::paintButton (Graphics& g, bool, bool)
{
    g.setImageResamplingQuality (Graphics::highResamplingQuality);
    g.drawImage (getToggleState() ? leverUp : leverDown, getLocalBounds().toFloat());
}

bool PowerSwitch::hitTest (int x, int y)
{
    return Rectangle<int> (22, 8, 56, 84).contains (x, y);   // écrou + levier, pas les coins transparents
}

// ───────────────────────── Éditeur ─────────────────────────

AmberCompEditor::AmberCompEditor (AmberCompProcessor& p)
    : AudioProcessorEditor (&p), processor (p),
      powerSwitch (loadImage (BinaryData::tog_up_png,   (size_t) BinaryData::tog_up_pngSize),
                   loadImage (BinaryData::tog_down_png, (size_t) BinaryData::tog_down_pngSize))
{
    background = loadImage (BinaryData::bg_png,      (size_t) BinaryData::bg_pngSize);
    meterGlass = loadImage (BinaryData::glass_png,   (size_t) BinaryData::glass_pngSize);
    ledOn      = loadImage (BinaryData::led_on_png,  (size_t) BinaryData::led_on_pngSize);
    ledOff     = loadImage (BinaryData::led_off_png, (size_t) BinaryData::led_off_pngSize);

    setLookAndFeel (&lnf);

    setupKnob (input.slider,       false);
    setupKnob (compression.slider, true);
    setupKnob (output.slider,      false);
    setupKnob (mix.slider,         false);

    input.slider.setTextValueSuffix (" dB");
    output.slider.setTextValueSuffix (" dB");
    mix.slider.setTextValueSuffix (" %");
    input.slider.setNumDecimalPlacesToDisplay (1);
    output.slider.setNumDecimalPlacesToDisplay (1);
    mix.slider.setNumDecimalPlacesToDisplay (0);

    // info-bulle pendant le réglage : seuil et makeup automatique correspondants
    compression.slider.textFromValueFunction = [] (double v)
    {
        return String (v, 1) + "  (seuil " + String (-v, 1) + " dB, makeup +"
               + String (arp::autoMakeupDb ((float) -v), 1) + " dB)";
    };

    input.attachment       = std::make_unique<AudioProcessorValueTreeState::SliderAttachment> (processor.apvts, "input",       input.slider);
    compression.attachment = std::make_unique<AudioProcessorValueTreeState::SliderAttachment> (processor.apvts, "compression", compression.slider);
    output.attachment      = std::make_unique<AudioProcessorValueTreeState::SliderAttachment> (processor.apvts, "output",      output.slider);
    mix.attachment         = std::make_unique<AudioProcessorValueTreeState::SliderAttachment> (processor.apvts, "mix",         mix.slider);

    addAndMakeVisible (powerSwitch);
    powerAttachment = std::make_unique<AudioProcessorValueTreeState::ButtonAttachment> (processor.apvts, "power", powerSwitch);
    powerSwitch.onStateChange = [this] { repaint (ledRect.getSmallestIntegerContainer()); };

    setSize (W, H);
    startTimerHz (30);
}

AmberCompEditor::~AmberCompEditor()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

void AmberCompEditor::setupKnob (Slider& s, bool big)
{
    s.setSliderStyle (Slider::RotaryHorizontalVerticalDrag);
    s.setTextBoxStyle (Slider::NoTextBox, false, 0, 0);
    // 270° de course, de -135° à +135° (identique aux graduations de la façade)
    s.setRotaryParameters (MathConstants<float>::pi * 1.25f, MathConstants<float>::pi * 2.75f, true);
    s.setPopupDisplayEnabled (true, false, this);
    if (big)
        s.getProperties().set ("big", true);
    addAndMakeVisible (s);
}

void AmberCompEditor::resized()
{
    auto place = [] (Slider& s, Point<float> c, int size)
    {
        s.setBounds (Rectangle<int> (size, size).withCentre (c.toInt()));
    };
    place (input.slider,       kInputC,  88);
    place (compression.slider, kCompC,  124);
    place (output.slider,      kOutputC, 88);
    place (mix.slider,         kMixC,    88);

    powerSwitch.setBounds (switchRect);
}

void AmberCompEditor::timerCallback()
{
    shownGR += (processor.grMeter.load() - shownGR) * 0.35f;
    repaint (glassRect.getSmallestIntegerContainer());
}

void AmberCompEditor::drawNeedle (Graphics& g) const
{
    Path clip;
    clip.addRoundedRectangle (meterWindow, 12.0f);
    Graphics::ScopedSaveState ss (g);
    g.reduceClipRegion (clip);

    // pivot sous la fenêtre ; l'aiguille balaie de -38° (20 dB) à +38° (0 dB), comme la graduation du cadran
    const float px = meterWindow.getCentreX(), py = meterWindow.getBottom() + 14.0f;
    const float len = 92.0f, aMax = 38.0f;
    const float a = (-aMax + (20.0f - jlimit (0.0f, 20.0f, shownGR)) / 20.0f * 2.0f * aMax) * MathConstants<float>::pi / 180.0f;
    const float tx = px + std::sin (a) * len, ty = py - std::cos (a) * len;

    auto seg = [&] (float ox, float oy, float thick, Colour col)
    {
        g.setColour (col);
        g.drawLine (px + ox, py + oy, tx + ox, ty + oy, thick);
    };

    const Colour sh (0xff1c1408);
    seg (7.2f, 7.5f, 5.0f, sh.withAlpha (0.10f));   // ombre portée floue (la lame est loin du cadran)
    seg (7.2f, 7.5f, 3.6f, sh.withAlpha (0.14f));
    seg (7.2f, 7.5f, 2.4f, sh.withAlpha (0.18f));
    seg (3.3f, 3.5f, 2.0f, sh.withAlpha (0.12f));
    seg (3.3f, 3.5f, 1.2f, sh.withAlpha (0.30f));
    seg (0.0f, 0.0f, 1.5f, Colour (0xff0f0b06));
    seg (-0.4f, 0.0f, 0.5f, Colours::white.withAlpha (0.18f));
}

void AmberCompEditor::paint (Graphics& g)
{
    g.setImageResamplingQuality (Graphics::highResamplingQuality);
    g.drawImage (background, getLocalBounds().toFloat());

    g.drawImage (powerSwitch.getToggleState() ? ledOn : ledOff, ledRect);

    drawNeedle (g);
    g.drawImage (meterGlass, glassRect);   // vitre et reflets par-dessus l'aiguille
}
