#ifndef SCREEN1VIEW_HPP
#define SCREEN1VIEW_HPP

#include <gui_generated/screen1_screen/Screen1ViewBase.hpp>
#include <gui/screen1_screen/Screen1Presenter.hpp>
#include <touchgfx/Callback.hpp>
#include <touchgfx/Unicode.hpp>
#include <touchgfx/widgets/AbstractButton.hpp>
#include <touchgfx/widgets/Box.hpp>
#include <touchgfx/widgets/TextAreaWithWildcard.hpp>
#include <touchgfx/containers/Slider.hpp>

class Screen1View : public Screen1ViewBase
{
public:
    Screen1View();
    virtual ~Screen1View() {}
    virtual void setupScreen();
    virtual void tearDownScreen();
    virtual void handleTickEvent();

private:
    /* Button action handlers */
    void onBtnHomeModeClicked(const touchgfx::AbstractButton& btn);
    void onBtnManualOPClicked(const touchgfx::AbstractButton& btn);
    void onBtnParameterClicked(const touchgfx::AbstractButton& btn);
    void onBtnProgModeClicked(const touchgfx::AbstractButton& btn);
    void onToggleRunClicked(const touchgfx::AbstractButton& btn);
    void onBtnJogNegClicked(const touchgfx::AbstractButton& btn);
    void onBtnJogPosClicked(const touchgfx::AbstractButton& btn);
    void onSliderValueChanged(const touchgfx::Slider& slider, int value);

    /* Callbacks */
    touchgfx::Callback<Screen1View, const touchgfx::AbstractButton&> cbBtnHomeMode;
    touchgfx::Callback<Screen1View, const touchgfx::AbstractButton&> cbBtnManualOP;
    touchgfx::Callback<Screen1View, const touchgfx::AbstractButton&> cbBtnParameter;
    touchgfx::Callback<Screen1View, const touchgfx::AbstractButton&> cbBtnProgMode;
    touchgfx::Callback<Screen1View, const touchgfx::AbstractButton&> cbToggleRun;
    touchgfx::Callback<Screen1View, const touchgfx::AbstractButton&> cbBtnJogNeg;
    touchgfx::Callback<Screen1View, const touchgfx::AbstractButton&> cbBtnJogPos;
    touchgfx::Callback<Screen1View, const touchgfx::Slider&, int>    cbSlider;

    /* Raw value overlays (TextProgress itself displays % progress, not raw values). */
    touchgfx::TextAreaWithOneWildcard positionValueText;
    touchgfx::TextAreaWithOneWildcard speedValueText;
    touchgfx::TextAreaWithOneWildcard torqueValueText;
    touchgfx::Box positionMinusSign;
    touchgfx::Box speedMinusSign;
    touchgfx::Box torqueMinusSign;
    touchgfx::Unicode::UnicodeChar positionValueBuffer[16];
    touchgfx::Unicode::UnicodeChar speedValueBuffer[16];
    touchgfx::Unicode::UnicodeChar torqueValueBuffer[16];

    uint32_t tickCounter;
    int32_t jogDeltaPerTick;
    uint8_t runUiState;
    uint8_t runAppliedState;
    uint8_t runDebounceCount;
};

#endif // SCREEN1VIEW_HPP
