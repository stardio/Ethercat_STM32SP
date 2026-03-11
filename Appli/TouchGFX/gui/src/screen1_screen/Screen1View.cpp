#include <gui/screen1_screen/Screen1View.hpp>
#include <touchgfx/Color.hpp>
#include <texts/TextKeysAndLanguages.hpp>
#include "soem_port.h"

/* JOG speed scaling: slider 0..100 -> delta per tick (encoder counts/tick) */
static const int32_t JOG_MIN_DELTA_PER_TICK = 100;
static const int32_t JOG_MAX_DELTA_PER_TICK = 4000;

Screen1View::Screen1View() :
    cbBtnHomeMode(this,  &Screen1View::onBtnHomeModeClicked),
    cbBtnManualOP(this,  &Screen1View::onBtnManualOPClicked),
    cbBtnParameter(this, &Screen1View::onBtnParameterClicked),
    cbBtnProgMode(this,  &Screen1View::onBtnProgModeClicked),
    cbToggleRun(this,    &Screen1View::onToggleRunClicked),
    cbBtnJogNeg(this,    &Screen1View::onBtnJogNegClicked),
    cbBtnJogPos(this,    &Screen1View::onBtnJogPosClicked),
    cbSlider(this,       &Screen1View::onSliderValueChanged),
    tickCounter(0),
    jogDeltaPerTick(0),
    runUiState(0),
    runAppliedState(0),
    runDebounceCount(0)
{
}

void Screen1View::setupScreen()
{
    Screen1ViewBase::setupScreen();

    /* Hook button callbacks */
    buttonWithLabel1.setAction(cbBtnHomeMode);
    buttonWithLabel1_1.setAction(cbBtnManualOP);
    buttonWithLabel1_2.setAction(cbBtnParameter);
    buttonWithLabel1_3.setAction(cbBtnProgMode);
    /* RUN is handled with debounced polling in handleTickEvent(). */
    buttonWithIcon1.setAction(cbBtnJogNeg);
    buttonWithIcon1_1.setAction(cbBtnJogPos);
    slider1.setNewValueCallback(cbSlider);

    /* Hide TextProgress internal percentage text (we overlay raw values below). */
    textProgress1.setColor(touchgfx::Color::getColorFromRGB(0, 0, 0));
    textProgress1_1.setColor(touchgfx::Color::getColorFromRGB(0, 0, 0));
    textProgress1_2.setColor(touchgfx::Color::getColorFromRGB(0, 0, 0));

    /* Raw value text overlays */
    positionValueText.setPosition(106, 37, 150, 30);
    positionValueText.setTypedText(touchgfx::TypedText(T___SINGLEUSE_2NI1));
    positionValueText.setColor(touchgfx::Color::getColorFromRGB(255, 0, 0));
    touchgfx::Unicode::snprintf(positionValueBuffer, 16, "%d", 0);
    positionValueText.setWildcard(positionValueBuffer);
    add(positionValueText);

    positionMinusSign.setPosition(96, 50, 8, 3);
    positionMinusSign.setColor(touchgfx::Color::getColorFromRGB(255, 0, 0));
    positionMinusSign.setVisible(false);
    add(positionMinusSign);

    speedValueText.setPosition(106, 101, 150, 30);
    speedValueText.setTypedText(touchgfx::TypedText(T___SINGLEUSE_2NI1));
    speedValueText.setColor(touchgfx::Color::getColorFromRGB(255, 0, 0));
    touchgfx::Unicode::snprintf(speedValueBuffer, 16, "%d", 0);
    speedValueText.setWildcard(speedValueBuffer);
    add(speedValueText);

    speedMinusSign.setPosition(96, 114, 8, 3);
    speedMinusSign.setColor(touchgfx::Color::getColorFromRGB(255, 0, 0));
    speedMinusSign.setVisible(false);
    add(speedMinusSign);

    torqueValueText.setPosition(106, 161, 150, 30);
    torqueValueText.setTypedText(touchgfx::TypedText(T___SINGLEUSE_2NI1));
    torqueValueText.setColor(touchgfx::Color::getColorFromRGB(255, 0, 0));
    touchgfx::Unicode::snprintf(torqueValueBuffer, 16, "%d", 0);
    torqueValueText.setWildcard(torqueValueBuffer);
    add(torqueValueText);

    torqueMinusSign.setPosition(96, 174, 8, 3);
    torqueMinusSign.setColor(touchgfx::Color::getColorFromRGB(255, 0, 0));
    torqueMinusSign.setVisible(false);
    add(torqueMinusSign);

    /* Initial display values */
    textProgress1.setValue(0);
    textProgress1_1.setValue(0);
    textProgress1_2.setValue(0);
    slider1.setValue(0);
    jogDeltaPerTick = 0;

    /* Safety default: after reset, stay STOP until user turns RUN ON. */
    toggleButton1.forceState(false);
    SOEM_SetRunEnable(0U);
    runUiState = 0U;
    runAppliedState = 0U;
    runDebounceCount = 0U;
}

void Screen1View::tearDownScreen()
{
    Screen1ViewBase::tearDownScreen();
}

void Screen1View::handleTickEvent()
{
    tickCounter++;

    /* Debounced RUN/STOP handling:
     * apply RUN state only after it remains stable for several frames. */
    {
        const uint8_t currentRunUi = toggleButton1.getState() ? 1U : 0U;

        if (currentRunUi != runUiState)
        {
            runUiState = currentRunUi;
            runDebounceCount = 0U;
        }
        else if (runDebounceCount < 20U)
        {
            runDebounceCount++;
        }

        if ((runDebounceCount >= 5U) && (runAppliedState != runUiState))
        {
            runAppliedState = runUiState;

            /* On RUN ON, latch current position as target to avoid sudden jump-to-zero fault. */
            if (runAppliedState != 0U)
            {
                SOEM_SetTargetPositionAbs(SOEM_GetPositionActual());
            }

            SOEM_SetRunEnable(runAppliedState);

            if (runAppliedState == 0U)
            {
                SOEM_SetTargetPositionAbs(0);
            }
        }
    }

    /* JOG hold control: move while the yellow button is physically pressed. */
    if ((SOEM_GetPdoReady() != 0u) && (runAppliedState != 0u) && (jogDeltaPerTick > 0))
    {
        const uint16_t sw = SOEM_GetStatusword();
        const bool opEnabled = ((sw & 0x006FU) == 0x0027U);
        const bool jogNegPressed = buttonWithIcon1.getPressedState();
        const bool jogPosPressed = buttonWithIcon1_1.getPressedState();

        if (opEnabled && jogNegPressed && !jogPosPressed)
        {
            SOEM_SetTargetPositionDelta(-jogDeltaPerTick);
        }
        else if (opEnabled && jogPosPressed && !jogNegPressed)
        {
            SOEM_SetTargetPositionDelta(jogDeltaPerTick);
        }
    }

    /* Update display ~6 times per second (every 10 ticks at 60 Hz) */
    if ((tickCounter % 10u) != 0u)
    {
        return;
    }

    if (SOEM_GetPdoReady() == 0u)
    {
        return;
    }

    /* --- Position (range 0-32767 as raw encoder counts mod 32767) --- */
    int32_t pos = SOEM_GetPositionActual();
    const bool posNeg = (pos < 0);
    int32_t posAbs = posNeg ? -pos : pos;
    /* Show absolute value clamped to 0-32767 for progress display */
    int32_t posDisp = posAbs;
    if (posDisp > 32767) { posDisp = 32767; }
    textProgress1.setValue((int)posDisp);
    textProgress1.invalidate();
    touchgfx::Unicode::snprintf(positionValueBuffer, 16, "%d", (int)posAbs);
    positionValueText.invalidate();
    positionMinusSign.setVisible(posNeg);
    positionMinusSign.invalidate();

    /* --- Velocity (0-32767) --- */
    int32_t vel = SOEM_GetVelocityActual();
    const bool velNeg = (vel < 0);
    int32_t velAbs = velNeg ? -vel : vel;
    int32_t velDisp = velAbs;
    if (velDisp > 32767) { velDisp = 32767; }
    textProgress1_1.setValue((int)velDisp);
    textProgress1_1.invalidate();
    touchgfx::Unicode::snprintf(speedValueBuffer, 16, "%d", (int)velAbs);
    speedValueText.invalidate();
    speedMinusSign.setVisible(velNeg);
    speedMinusSign.invalidate();

    /* --- Torque (0-32767, actual range -32768..+32767) --- */
    int16_t tor = SOEM_GetTorqueActual();
    const bool torNeg = (tor < 0);
    int32_t torAbs = torNeg ? -(int32_t)tor : (int32_t)tor;
    int32_t torDisp = torAbs;
    if (torDisp > 32767) { torDisp = 32767; }
    textProgress1_2.setValue((int)torDisp);
    textProgress1_2.invalidate();
    touchgfx::Unicode::snprintf(torqueValueBuffer, 16, "%d", (int)torAbs);
    torqueValueText.invalidate();
    torqueMinusSign.setVisible(torNeg);
    torqueMinusSign.invalidate();
}

/* ─── Button handlers ─────────────────────────────────────────────────────── */

void Screen1View::onBtnHomeModeClicked(const touchgfx::AbstractButton& /*btn*/)
{
    /* Home Mode: move to absolute position 0 */
    SOEM_SetTargetPositionAbs(0);
    slider1.setValue(0);
    slider1.invalidate();
}

void Screen1View::onBtnManualOPClicked(const touchgfx::AbstractButton& /*btn*/)
{
    /* Manual OP: set target to current actual position (hold position) */
    int32_t pos = SOEM_GetPositionActual();
    SOEM_SetTargetPositionAbs(pos);
}

void Screen1View::onBtnParameterClicked(const touchgfx::AbstractButton& /*btn*/)
{
    /* Parameter: placeholder — no action in this version */
}

void Screen1View::onBtnProgModeClicked(const touchgfx::AbstractButton& /*btn*/)
{
    /* Prog Mode: placeholder — no action in this version */
}

void Screen1View::onToggleRunClicked(const touchgfx::AbstractButton& /*btn*/)
{
    /* Intentionally unused. RUN is applied via debounced polling in handleTickEvent(). */
}

void Screen1View::onBtnJogNegClicked(const touchgfx::AbstractButton& /*btn*/)
{
    /* Hold behavior handled in handleTickEvent using getPressedState(). */
}

void Screen1View::onBtnJogPosClicked(const touchgfx::AbstractButton& /*btn*/)
{
    /* Hold behavior handled in handleTickEvent using getPressedState(). */
}

void Screen1View::onSliderValueChanged(const touchgfx::Slider& /*slider*/, int value)
{
    /* Slider is JOG speed setting only (not target position).
     * value 0 => no jog motion, 1..100 => scaled jog speed. */
    if (value <= 0)
    {
        jogDeltaPerTick = 0;
        return;
    }

    const int32_t span = (JOG_MAX_DELTA_PER_TICK - JOG_MIN_DELTA_PER_TICK);
    jogDeltaPerTick = JOG_MIN_DELTA_PER_TICK + ((span * value) / 100);
}
