#include "PluginProcessor.h"
#include "PluginEditor.h"

using namespace juce;

AmberCompProcessor::AmberCompProcessor()
    : AudioProcessor (BusesProperties()
                        .withInput  ("Input",  AudioChannelSet::stereo(), true)
                        .withOutput ("Output", AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "STATE", createLayout())
{
}

AudioProcessorValueTreeState::ParameterLayout AmberCompProcessor::createLayout()
{
    using P = AudioParameterFloat;
    using R = NormalisableRange<float>;

    // Seuls 4 réglages + l'interrupteur sont exposés.
    // Compression : 0 .. 40 = seuil 0 .. -40 dB (tourner vers la droite = seuil plus bas).
    // Le makeup est automatique (0 dB à 0, +8 dB à 40) ; ratio 2:1, attack 30 ms et release 30 ms sont fixes (aucune saturation tube).
    std::vector<std::unique_ptr<RangedAudioParameter>> params;
    params.push_back (std::make_unique<P> (ParameterID { "input",       1 }, "Input",       R (-12.f, 12.f, 0.1f), 0.f));
    params.push_back (std::make_unique<P> (ParameterID { "compression", 1 }, "Compression", R (0.f, 40.f, 0.1f),   12.f));
    params.push_back (std::make_unique<P> (ParameterID { "output",      1 }, "Output",      R (-6.f, 6.f, 0.1f),   0.f));
    params.push_back (std::make_unique<P> (ParameterID { "mix",         1 }, "Mix",         R (0.f, 100.f, 1.f),   100.f));
    params.push_back (std::make_unique<AudioParameterBool> (ParameterID { "power", 1 }, "Power", true));
    return { params.begin(), params.end() };
}

void AmberCompProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    core.prepare (sampleRate);
    dryBuffer.setSize (jmax (2, getTotalNumInputChannels()), jmax (1, samplesPerBlock));
    bypassGain = apvts.getRawParameterValue ("power")->load() > 0.5f ? 1.0f : 0.0f;
}

bool AmberCompProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto& out = layouts.getMainOutputChannelSet();
    if (out != AudioChannelSet::mono() && out != AudioChannelSet::stereo())
        return false;
    return out == layouts.getMainInputChannelSet();
}

void AmberCompProcessor::processBlock (AudioBuffer<float>& buffer, MidiBuffer&)
{
    ScopedNoDenormals noDenormals;

    const int n   = buffer.getNumSamples();
    const int nCh = jmin (buffer.getNumChannels(), arp::kMaxChannels);

    for (int c = getTotalNumInputChannels(); c < getTotalNumOutputChannels(); ++c)
        buffer.clear (c, 0, n);

    auto p = [this] (const char* id) { return apvts.getRawParameterValue (id)->load(); };

    arp::CompParams cp;                      // ratio, attack, release : valeurs fixes (voir CompCore.h)
    cp.inputDb     = p ("input");
    cp.thresholdDb = -p ("compression");     // tourner vers la droite abaisse le seuil
    cp.makeupDb    = arp::autoMakeupDb (cp.thresholdDb);   // le makeup suit automatiquement le seuil
    cp.outputDb    = p ("output");
    cp.mix         = p ("mix") * 0.01f;

    const float target = p ("power") > 0.5f ? 1.0f : 0.0f;
    const bool  fade   = (bypassGain != 1.0f) || (target != 1.0f);

    if (fade)   // on garde le signal d'entrée pour le fondu enchaîné actif <-> contourné
    {
        dryBuffer.setSize (jmax (dryBuffer.getNumChannels(), nCh), jmax (dryBuffer.getNumSamples(), n), true, false, true);
        for (int c = 0; c < nCh; ++c)
            dryBuffer.copyFrom (c, 0, buffer, c, 0, n);
    }

    // tout le traitement (dont passe-bas 10 kHz, sidechain 50 Hz et EQ fixe de sortie) est dans CompCore.h
    core.process (buffer.getArrayOfWritePointers(), nCh, n, cp);

    if (fade)
    {
        for (int c = 0; c < nCh; ++c)
        {
            auto* wet = buffer.getWritePointer (c);
            const auto* dry = dryBuffer.getReadPointer (c);
            for (int i = 0; i < n; ++i)
            {
                const float b = bypassGain + (target - bypassGain) * (float) (i + 1) / (float) n;
                wet[i] = dry[i] * (1.0f - b) + wet[i] * b;
            }
        }
    }
    bypassGain = target;

    grMeter.store (target > 0.5f ? core.getGainReductionDb() : 0.0f);
}

AudioProcessorEditor* AmberCompProcessor::createEditor()
{
    return new AmberCompEditor (*this);
}

void AmberCompProcessor::getStateInformation (MemoryBlock& destData)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void AmberCompProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (ValueTree::fromXml (*xml));
}

AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new AmberCompProcessor();
}
