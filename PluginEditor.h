#pragma once
#include "PluginProcessor.h"

// Bouton rotatif : corps cannelé pré-rendu (qui tourne) + plateau fixe + repère dessiné en code.
// Deux tailles : petit bouton (Input / Output / Mix) et gros bouton Compression (propriété "big").
class AmberLookAndFeel : public juce::LookAndFeel_V4
{
public:
    AmberLookAndFeel();

    void drawRotarySlider (juce::Graphics&, int x, int y, int width, int height,
                           float sliderPosProportional, float rotaryStartAngle,
                           float rotaryEndAngle, juce::Slider&) override;

private:
    juce::Image skirtSmall, topSmall;   // 80 x 80 px (rendu @2x)
    juce::Image skirtBig,   topBig;     // 108 x 108 px (rendu @2x)

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AmberLookAndFeel)
};

// Interrupteur Power : levier chromé en haut = actif, en bas = contourné
class PowerSwitch : public juce::Button
{
public:
    PowerSwitch (juce::Image up, juce::Image down);
    void paintButton (juce::Graphics&, bool, bool) override;
    bool hitTest (int x, int y) override;

private:
    juce::Image leverUp, leverDown;
};

class AmberCompEditor : public juce::AudioProcessorEditor,
                        private juce::Timer
{
public:
    explicit AmberCompEditor (AmberCompProcessor&);
    ~AmberCompEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void drawNeedle (juce::Graphics&) const;
    void setupKnob (juce::Slider&, bool big);

    struct Knob
    {
        juce::Slider slider;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
    };

    AmberCompProcessor& processor;
    AmberLookAndFeel lnf;
    Knob input, compression, output, mix;
    PowerSwitch powerSwitch;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> powerAttachment;

    juce::Image background;               // façade complète (1680 x 784, rendu @2x)
    juce::Image meterGlass;               // vitre du VU-mètre (reflets), transparente
    juce::Image ledOn, ledOff;

    float shownGR = 0.0f;

    static constexpr int W = 840;
    static constexpr int H = 392;

    // positions (px, grille 840 x 392) : voir la maquette
    const juce::Rectangle<float> meterWindow { 355.f, 41.f, 130.f, 88.f };
    const juce::Rectangle<float> glassRect   { 353.f, 39.f, 134.f, 92.f };
    const juce::Rectangle<float> ledRect     { 17.f, 40.f, 104.f, 104.f };
    const juce::Rectangle<int>   switchRect  { 19, 146, 100, 100 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AmberCompEditor)
};
