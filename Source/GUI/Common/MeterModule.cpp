#include "MeterModule.h"
#include "../../Core/Constants.h"
#include "../LookAndFeel/FF360LabsLookAndFeel.h"

MeterModule::MeterModule(const juce::String& name, MeterModuleType type)
    : moduleName(name), moduleType(type)
{
    addAndMakeVisible(menuButton);
    menuButton.onClick = [this] { showModuleMenu(); };
}

void MeterModule::showModuleMenu()
{
    juce::PopupMenu menu;
    menu.addItem(1, "Resize");
    menu.addItem(2, "Detach to Window", onDetach != nullptr);
    menu.addSeparator();
    menu.addItem(3, "Close / Remove", onClose != nullptr);

    auto options = juce::PopupMenu::Options()
                       .withTargetComponent(&menuButton)
                       .withMaximumNumColumns(1);

    menu.showMenuAsync(options, [this](int result)
    {
        switch (result)
        {
            case 1: if (onMaximize) onMaximize(this); break;
            case 2: if (onDetach)   onDetach(this);   break;
            case 3: if (onClose)    onClose(this);    break;
            default: break;
        }
    });
}

void MeterModule::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    
    // Glass Card Treatment (Frosted Glass, Sheen, Hairline Border)
    FF360LabsLookAndFeel::drawGlassPanel(g, bounds, 8.0f);

    // Slim Module Header
    auto headerRect = bounds.removeFromTop((float)headerHeight);
    
    // Hairline Separator under Header
    g.setColour(ff360_labs::HairlineBorder);
    g.drawHorizontalLine((int)headerRect.getBottom(), headerRect.getX() + 6.0f, headerRect.getRight() - 6.0f);

    // Header Text — quiet sans-serif chrome (mockup's .m-title is muted/dim, not
    // bright bold mono; brightness and mono type are reserved for live data).
    g.setColour(ff360_labs::TextMuted);
    g.setFont(FF360LabsLookAndFeel::getUiFont(10.5f, juce::Font::plain));
    g.drawText(moduleName.toUpperCase(),
               headerRect.toNearestInt().withTrimmedLeft(padding * 2 + 2),
               juce::Justification::centredLeft,
               true);

    // Give derived class a chance to paint in the remaining area
    juce::Graphics::ScopedSaveState state(g);
    g.reduceClipRegion(getModuleBounds());
    paintModule(g);
}

void MeterModule::resized()
{
    auto headerRect = getLocalBounds().removeFromTop(headerHeight);
    
    menuButton.setBounds(headerRect.removeFromRight(headerHeight + 4).reduced(3, 4));
    
    // Delegate to derived class
    resizedModule();
}

juce::Rectangle<int> MeterModule::getModuleBounds() const
{
    return getLocalBounds().withTrimmedTop(headerHeight).reduced(padding);
}
