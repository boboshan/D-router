#include "Persistence/AtomicXmlWrite.h"
#include <juce_core/juce_core.h>

struct AtomicXmlWriteTests : juce::UnitTest
{
    AtomicXmlWriteTests() : juce::UnitTest ("AtomicXmlWrite") {}

    void runTest() override
    {
        beginTest ("writes parseable content to a fresh path");
        {
            juce::TemporaryFile scratch; // owns + deletes its path on scope exit
            const juce::File target = scratch.getFile();

            juce::XmlElement xml ("root");
            xml.setAttribute ("k", "v1");
            expect (dcr::writeXmlAtomically (xml, target));
            expect (target.existsAsFile());

            auto back = juce::parseXML (target);
            expect (back != nullptr);
            expectEquals (back->getStringAttribute ("k"), juce::String ("v1"));
        }

        beginTest ("atomically replaces existing content");
        {
            juce::TemporaryFile scratch;
            const juce::File target = scratch.getFile();

            juce::XmlElement a ("root");
            a.setAttribute ("k", "v1");
            expect (dcr::writeXmlAtomically (a, target));

            juce::XmlElement b ("root");
            b.setAttribute ("k", "v2");
            expect (dcr::writeXmlAtomically (b, target));

            auto back = juce::parseXML (target);
            expect (back != nullptr);
            expectEquals (back->getStringAttribute ("k"), juce::String ("v2"));
        }

        beginTest ("leaves no temp sibling for the target");
        {
            juce::TemporaryFile scratch;
            const juce::File target = scratch.getFile();

            juce::XmlElement xml ("root");
            expect (dcr::writeXmlAtomically (xml, target));

            auto leftovers = target.getParentDirectory()
                                 .findChildFiles (juce::File::findFiles, false, "*.temp");
            for (auto& f : leftovers)
                expect (!f.getFileName().startsWith (target.getFileName()));
        }
    }
};

static AtomicXmlWriteTests atomicXmlWriteTests;
