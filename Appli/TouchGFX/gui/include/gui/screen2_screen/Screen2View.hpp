#ifndef SCREEN2VIEW_HPP
#define SCREEN2VIEW_HPP

#include <gui_generated/screen2_screen/Screen2ViewBase.hpp>
#include <gui/screen2_screen/Screen2Presenter.hpp>
#include <gui/containers/KeyBoard.hpp>
#include <touchgfx/Callback.hpp>
#include <touchgfx/Unicode.hpp>
#include <touchgfx/widgets/AbstractButton.hpp>
#include <touchgfx/widgets/Box.hpp>
#include <touchgfx/widgets/TextAreaWithWildcard.hpp>
#include <touchgfx/events/ClickEvent.hpp>

class Screen2View : public Screen2ViewBase
{
public:
    Screen2View();
    virtual ~Screen2View() {}
    virtual void setupScreen();
    virtual void tearDownScreen();
    virtual void handleTickEvent();
    virtual void handleClickEvent(const touchgfx::ClickEvent& evt);

    /* --- Field enum (public for static array declarations) --- */
    enum ActiveField
    {
        FIELD_NONE = -1,
        FIELD_SLOW_POS = 0,
        FIELD_TGT_POS,
        FIELD_DELAY,
        FIELD_SEG1_POS,
        FIELD_SEG2_POS,
        FIELD_SEG1_TRQ,
        FIELD_SEG2_TRQ,
        FIELD_COUNT
    };

private:
    /* --- Keyboard key handler --- */
    void onKeyPressed(int16_t key);
    touchgfx::Callback<Screen2View, int16_t> cbKeyPressed;

    /* --- Main navigation handler --- */
    void onMainClicked(const touchgfx::AbstractButton& btn);
    touchgfx::Callback<Screen2View, const touchgfx::AbstractButton&> cbMain;

    /* --- Mode-button handlers --- */
    void onStartClicked(const touchgfx::AbstractButton& btn);
    void onStopClicked(const touchgfx::AbstractButton& btn);
    void onHomeClicked(const touchgfx::AbstractButton& btn);
    touchgfx::Callback<Screen2View, const touchgfx::AbstractButton&> cbStart;
    touchgfx::Callback<Screen2View, const touchgfx::AbstractButton&> cbStop;
    touchgfx::Callback<Screen2View, const touchgfx::AbstractButton&> cbHome;

    /* --- Virtual keyboard container --- */
    KeyBoard keyboard;

    /* --- Input field selections ---
     * FIELD_NONE     : no field active, keyboard hidden
     * FIELD_SLOW_POS : SlowPosition  (y=144)  - slow start position [pls]
     * FIELD_TGT_POS  : TargetPosition (y=207) - target position [pls]
     * FIELD_DELAY    : DelyTime       (y=267) - delay time [msec]
     * FIELD_SEG1_POS : SlowPosition_1 (332,144) - seg1 speed/position [pls]
     * FIELD_SEG2_POS : SlowPosition_2 (332,207) - seg2 speed/position [pls]
     * FIELD_SEG1_TRQ : SlowPosition_3 (519,144) - seg1 torque limit [%*10]
     * FIELD_SEG2_TRQ : SlowPosition_4 (519,207) - seg2 torque limit [%*10]
     */
    ActiveField activeField;

    /* --- Stored parameter values --- */
    int32_t paramValues[FIELD_COUNT];

    /* --- Keyboard input state --- */
    bool     inputNegative;       /* minus toggled */
    int32_t  inputAccumulator;    /* digits accumulated so far */
    bool     inputActive;         /* at least one digit entered */

    /* --- Overlay text areas for input fields --- */
    /* Each one sits on top of its TextProgress widget */
    touchgfx::TextAreaWithOneWildcard ovSlowPos;
    touchgfx::TextAreaWithOneWildcard ovTgtPos;
    touchgfx::TextAreaWithOneWildcard ovDelay;
    touchgfx::TextAreaWithOneWildcard ovSeg1Pos;
    touchgfx::TextAreaWithOneWildcard ovSeg2Pos;
    touchgfx::TextAreaWithOneWildcard ovSeg1Trq;
    touchgfx::TextAreaWithOneWildcard ovSeg2Trq;
    touchgfx::Unicode::UnicodeChar    ovBuf[FIELD_COUNT][16];

    /* --- Highlight box showing which field is selected --- */
    touchgfx::Box fieldHighlight;

    /* --- Overlay text areas for current pos / torque display --- */
    touchgfx::TextAreaWithOneWildcard ovCurPos;
    touchgfx::TextAreaWithOneWildcard ovCurTrq;
    touchgfx::Box                     curPosMinusSign;
    touchgfx::Box                     curTrqMinusSign;
    touchgfx::Unicode::UnicodeChar    curPosBuf[16];
    touchgfx::Unicode::UnicodeChar    curTrqBuf[16];

    uint32_t tickCounter;

    /* --- Helpers --- */
    void activateField(ActiveField f);
    void refreshOverlay(ActiveField f);
    void refreshAllOverlays();
    void updateKeyboardDisplay();
    void updateCurPosDisplay();
    void updateCurTrqDisplay();
};

#endif // SCREEN2VIEW_HPP
