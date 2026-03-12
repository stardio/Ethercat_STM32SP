#include <gui/screen2_screen/Screen2View.hpp>
#include <touchgfx/Color.hpp>
#include <texts/TextKeysAndLanguages.hpp>
#include "soem_port.h"

/* TextProgress background bitmap size for the ROUNDED_LIGHT style: 174 x 54 */
static const int16_t TP_W = 174;
static const int16_t TP_H = 54;

/* Input field click-detection rectangles (x, y, x2, y2) matching Screen2ViewBase */
struct FieldRect { int16_t x, y, x2, y2; };
static const FieldRect FIELD_RECTS[Screen2View::FIELD_COUNT] =
{
    { 148, 144, 148+TP_W, 144+TP_H },  /* FIELD_SLOW_POS */
    { 148, 207, 148+TP_W, 207+TP_H },  /* FIELD_TGT_POS  */
    { 148, 267, 148+TP_W, 267+TP_H },  /* FIELD_DELAY    */
    { 332, 144, 332+TP_W, 144+TP_H },  /* FIELD_SEG1_POS */
    { 332, 207, 332+TP_W, 207+TP_H },  /* FIELD_SEG2_POS */
    { 519, 144, 519+TP_W, 144+TP_H },  /* FIELD_SEG1_TRQ */
    { 519, 207, 519+TP_W, 207+TP_H },  /* FIELD_SEG2_TRQ */
};

/* Keyboard placed at right side of screen, below header bar */
static const int16_t KB_X = 545;
static const int16_t KB_Y = 195;

/* Text format IDs for each overlay (shows current unit hint) */
static const TEXTS OVERLAY_TEXT_IDS[Screen2View::FIELD_COUNT] =
{
    T___SINGLEUSE_JWVT,   /* "<> pls" for SlowPosition   */
    T___SINGLEUSE_1NSA,   /* "<> pls" for TargetPosition */
    T___SINGLEUSE_GGHG,   /* "<> msec" for DelyTime      */
    T___SINGLEUSE_F282,   /* "<> pls" for SlowPosition_1 */
    T___SINGLEUSE_GI2K,   /* "<> pls" for SlowPosition_2 */
    T___SINGLEUSE_TCSY,   /* "<> %"   for SlowPosition_3 */
    T___SINGLEUSE_SBEZ,   /* "<> %"   for SlowPosition_4 */
};

/* Keep Program mode values while switching screens. */
static int32_t gProgramParams[Screen2View::FIELD_COUNT] = { 0 };

/* ─────────────────────────── constructor ─────────────────────────────────── */

Screen2View::Screen2View() :
    cbKeyPressed(this, &Screen2View::onKeyPressed),
    cbMain(this, &Screen2View::onMainClicked),
    cbStart(this, &Screen2View::onStartClicked),
    cbStop(this,  &Screen2View::onStopClicked),
    cbHome(this,  &Screen2View::onHomeClicked),
    activeField(FIELD_NONE),
    inputNegative(false),
    inputAccumulator(0),
    inputActive(false),
    tickCounter(0)
{
    for (int i = 0; i < FIELD_COUNT; i++)
    {
        paramValues[i] = gProgramParams[i];
    }
}

/* ─────────────────────────── setupScreen ────────────────────────────────── */

void Screen2View::setupScreen()
{
    Screen2ViewBase::setupScreen();

    /* --- Keep static labels readable on dark background --- */
    const touchgfx::colortype WHITE = touchgfx::Color::getColorFromRGB(255, 255, 255);
    textArea1.setColor(WHITE);
    textArea2.setColor(WHITE);
    textArea2_2.setColor(WHITE);
    textArea2_2_1.setColor(WHITE);
    textArea2_2_1_1_2.setColor(WHITE);
    textArea2_2_1_1.setColor(WHITE);
    textArea2_2_1_1_1.setColor(WHITE);
    textArea2_1.setColor(WHITE);
    textArea2_1_1.setColor(WHITE);

    /* --- Hook mode buttons --- */
    Main_button.setAction(cbMain);
    Main_button_1.setAction(cbStart);
    Main_button_1_1.setAction(cbStop);
    Main_button_1_1_1.setAction(cbHome);

    /* --- Mute TextProgress internal text (will be replaced by overlays) --- */
    const touchgfx::colortype HIDE = touchgfx::Color::getColorFromRGB(0, 0, 0);
    SlowPosition.setColor(HIDE);
    SlowPosition_1.setColor(HIDE);
    SlowPosition_1_1.setColor(HIDE);
    SlowPosition_1_2.setColor(HIDE);
    SlowPosition_2.setColor(HIDE);
    SlowPosition_3.setColor(HIDE);
    SlowPosition_4.setColor(HIDE);
    TargetPosition.setColor(HIDE);
    DelyTime.setColor(HIDE);

    /* --- Input field overlays --- */
    const touchgfx::colortype RED  = touchgfx::Color::getColorFromRGB(220, 20, 20);
    const touchgfx::colortype TEXT_COLOR = RED;

    /* Helper macro to set up one overlay */
#define SETUP_OV(ov, buf, fx, fy, textid) \
    ov.setPosition((fx)+12, (fy)+12, TP_W-20, 30); \
    ov.setTypedText(touchgfx::TypedText(textid)); \
    ov.setColor(TEXT_COLOR); \
    touchgfx::Unicode::snprintf(buf, 16, "%d", 0); \
    ov.setWildcard(buf); \
    add(ov)

    SETUP_OV(ovSlowPos,  ovBuf[FIELD_SLOW_POS], 148, 144, T___SINGLEUSE_JWVT);
    SETUP_OV(ovTgtPos,   ovBuf[FIELD_TGT_POS],  148, 207, T___SINGLEUSE_1NSA);
    SETUP_OV(ovDelay,    ovBuf[FIELD_DELAY],     148, 267, T___SINGLEUSE_GGHG);
    SETUP_OV(ovSeg1Pos,  ovBuf[FIELD_SEG1_POS], 332, 144, T___SINGLEUSE_F282);
    SETUP_OV(ovSeg2Pos,  ovBuf[FIELD_SEG2_POS], 332, 207, T___SINGLEUSE_GI2K);
    SETUP_OV(ovSeg1Trq,  ovBuf[FIELD_SEG1_TRQ], 519, 144, T___SINGLEUSE_TCSY);
    SETUP_OV(ovSeg2Trq,  ovBuf[FIELD_SEG2_TRQ], 519, 207, T___SINGLEUSE_SBEZ);
#undef SETUP_OV

    refreshAllOverlays();

    /* --- Field highlight box (shown around active field) --- */
    fieldHighlight.setPosition(0, 0, TP_W, TP_H);
    fieldHighlight.setColor(touchgfx::Color::getColorFromRGB(255, 200, 0));
    fieldHighlight.setAlpha(64);
    fieldHighlight.setVisible(false);
    add(fieldHighlight);

    /* --- Current position / torque display overlays --- */
    /* SlowPosition_1_1 is at (353,304), SlowPosition_1_2 at (544,304) */
    ovCurPos.setPosition(353+12, 304+12, TP_W-20, 30);
    ovCurPos.setTypedText(touchgfx::TypedText(T___SINGLEUSE_FZEC));
    ovCurPos.setColor(RED);
    touchgfx::Unicode::snprintf(curPosBuf, 16, "%d", 0);
    ovCurPos.setWildcard(curPosBuf);
    add(ovCurPos);

    curPosMinusSign.setPosition(344, 322, 8, 3);
    curPosMinusSign.setColor(RED);
    curPosMinusSign.setVisible(false);
    add(curPosMinusSign);

    ovCurTrq.setPosition(544+12, 304+12, TP_W-20, 30);
    ovCurTrq.setTypedText(touchgfx::TypedText(T___SINGLEUSE_2AQY));
    ovCurTrq.setColor(RED);
    touchgfx::Unicode::snprintf(curTrqBuf, 16, "%d", 0);
    ovCurTrq.setWildcard(curTrqBuf);
    add(ovCurTrq);

    curTrqMinusSign.setPosition(535, 322, 8, 3);
    curTrqMinusSign.setColor(RED);
    curTrqMinusSign.setVisible(false);
    add(curTrqMinusSign);

    /* --- Keyboard container --- */
    keyboard.initialize();
    keyboard.setKeyCallback(&cbKeyPressed);
    keyboard.setXY(KB_X, KB_Y);
    keyboard.setVisible(false);
    add(keyboard);
}

void Screen2View::tearDownScreen()
{
    for (int i = 0; i < FIELD_COUNT; i++)
    {
        gProgramParams[i] = paramValues[i];
    }

    Screen2ViewBase::tearDownScreen();
    activeField = FIELD_NONE;
}

/* ─────────────────────────── tick event ─────────────────────────────────── */

void Screen2View::handleTickEvent()
{
    tickCounter++;

    /* Refresh current position/torque at ~6 Hz */
    if ((tickCounter % 10u) != 0u)
    {
        return;
    }

    if (SOEM_GetPdoReady() == 0u)
    {
        return;
    }

    updateCurPosDisplay();
    updateCurTrqDisplay();
}

/* ─────────────────────────── click event (field selection) ──────────────── */

void Screen2View::handleClickEvent(const touchgfx::ClickEvent& evt)
{
    /* Let built-in button logic run first (Main/Start/Stop/Home, keyboard keys) */
    Screen2ViewBase::handleClickEvent(evt);

    if (evt.getType() != touchgfx::ClickEvent::PRESSED)
    {
        return;
    }

    const int16_t cx = static_cast<int16_t>(evt.getX());
    const int16_t cy = static_cast<int16_t>(evt.getY());

    auto inRect = [cx, cy](int16_t x, int16_t y, int16_t w, int16_t h) -> bool
    {
        return (cx >= x && cx < (x + w) && cy >= y && cy < (y + h));
    };

    /* Keep active field while user is interacting with popup keyboard */
    if (keyboard.isVisible() && inRect(keyboard.getX(), keyboard.getY(), keyboard.getWidth(), keyboard.getHeight()))
    {
        return;
    }

    /* Ignore touches on bottom buttons for field-selection logic */
    if (inRect(Main_button.getX(), Main_button.getY(), Main_button.getWidth(), Main_button.getHeight()) ||
        inRect(Main_button_1.getX(), Main_button_1.getY(), Main_button_1.getWidth(), Main_button_1.getHeight()) ||
        inRect(Main_button_1_1.getX(), Main_button_1_1.getY(), Main_button_1_1.getWidth(), Main_button_1_1.getHeight()) ||
        inRect(Main_button_1_1_1.getX(), Main_button_1_1_1.getY(), Main_button_1_1_1.getWidth(), Main_button_1_1_1.getHeight()))
    {
        return;
    }

    for (int i = 0; i < FIELD_COUNT; i++)
    {
        const FieldRect& r = FIELD_RECTS[i];
        if (cx >= r.x && cx < r.x2 && cy >= r.y && cy < r.y2)
        {
            activateField(static_cast<ActiveField>(i));
            return;
        }
    }

    /* Click outside all fields: deactivate */
    activateField(FIELD_NONE);
}

void Screen2View::onMainClicked(const touchgfx::AbstractButton& /*btn*/)
{
    application().gotoScreen1ScreenNoTransition();
}

/* ─────────────────────────── field activation ───────────────────────────── */

void Screen2View::activateField(ActiveField f)
{
    activeField = f;

    /* Reset input accumulator */
    inputNegative   = false;
    inputAccumulator = 0;
    inputActive      = false;

    if (f == FIELD_NONE)
    {
        fieldHighlight.setVisible(false);
        fieldHighlight.invalidate();
        keyboard.setVisible(false);
        keyboard.invalidate();
        return;
    }

    /* Position highlight box */
    const FieldRect& r = FIELD_RECTS[static_cast<int>(f)];
    fieldHighlight.setPosition(r.x - 2, r.y - 2, TP_W + 4, TP_H + 4);
    fieldHighlight.setVisible(true);
    fieldHighlight.invalidate();

    /* Show keyboard and pre-fill with current stored value */
    keyboard.setVisible(true);
    keyboard.invalidate();

    /* Load current stored value into input state for display */
    int32_t val = paramValues[static_cast<int>(f)];
    inputNegative    = (val < 0);
    inputAccumulator = inputNegative ? -val : val;
    inputActive      = true;

    /* Show it in the overlay */
    updateKeyboardDisplay();
}

/* ─────────────────────────── key press handler ──────────────────────────── */

void Screen2View::onKeyPressed(int16_t key)
{
    if (activeField == FIELD_NONE)
    {
        return;
    }

    if (key >= 0 && key <= 9)
    {
        /* Digit: append */
        int32_t newVal = inputAccumulator * 10 + key;
        /* Clamp to reasonable range */
        if (newVal <= 999999)
        {
            inputAccumulator = newVal;
        }
        inputActive = true;
        updateKeyboardDisplay();
    }
    else if (key == 10)
    {
        /* Minus toggle */
        /* Only certain fields can be negative (positions) */
        if (activeField == FIELD_SLOW_POS || activeField == FIELD_TGT_POS ||
            activeField == FIELD_SEG1_POS || activeField == FIELD_SEG2_POS)
        {
            inputNegative = !inputNegative;
            updateKeyboardDisplay();
        }
    }
    else if (key == 11)
    {
        /* Enter: confirm value */
        int32_t newVal = inputNegative ? -inputAccumulator : inputAccumulator;
        paramValues[static_cast<int>(activeField)] = newVal;
        gProgramParams[static_cast<int>(activeField)] = newVal;
        refreshOverlay(activeField);

        /* Deactivate field */
        activateField(FIELD_NONE);
    }
}

/* ─────────────────────────── overlay refresh ────────────────────────────── */

void Screen2View::updateKeyboardDisplay()
{
    if (activeField == FIELD_NONE)
    {
        return;
    }

    /* Show the number being typed in the active field overlay */
    const int fi = static_cast<int>(activeField);
    touchgfx::Unicode::snprintf(ovBuf[fi], 16, "%d", (int)inputAccumulator);

    touchgfx::TextAreaWithOneWildcard* ovs[FIELD_COUNT] =
    {
        &ovSlowPos, &ovTgtPos, &ovDelay,
        &ovSeg1Pos, &ovSeg2Pos, &ovSeg1Trq, &ovSeg2Trq
    };
    ovs[fi]->invalidate();
}

void Screen2View::refreshOverlay(ActiveField f)
{
    if (f == FIELD_NONE)
    {
        return;
    }

    const int fi = static_cast<int>(f);
    int32_t val = paramValues[fi];
    int32_t absVal = (val < 0) ? -val : val;

    touchgfx::Unicode::snprintf(ovBuf[fi], 16, "%d", (int)absVal);

    touchgfx::TextAreaWithOneWildcard* ovs[FIELD_COUNT] =
    {
        &ovSlowPos, &ovTgtPos, &ovDelay,
        &ovSeg1Pos, &ovSeg2Pos, &ovSeg1Trq, &ovSeg2Trq
    };
    ovs[fi]->invalidate();
}

void Screen2View::refreshAllOverlays()
{
    for (int i = 0; i < FIELD_COUNT; i++)
    {
        refreshOverlay(static_cast<ActiveField>(i));
    }
}

/* ─────────────────────────── current POS / TRQ display ──────────────────── */

void Screen2View::updateCurPosDisplay()
{
    int32_t pos = SOEM_GetPositionActual();
    const bool neg = (pos < 0);
    int32_t absPos = neg ? -pos : pos;

    touchgfx::Unicode::snprintf(curPosBuf, 16, "%d", (int)absPos);
    ovCurPos.invalidate();

    curPosMinusSign.setVisible(neg);
    curPosMinusSign.invalidate();

    /* Also update the backing TextProgress value for the background bar */
    int32_t disp = absPos > 32767 ? 32767 : absPos;
    SlowPosition_1_1.setValue((int)disp);
    SlowPosition_1_1.invalidate();
}

void Screen2View::updateCurTrqDisplay()
{
    int16_t trq = SOEM_GetTorqueActual();
    const bool neg = (trq < 0);
    int32_t absTrq = neg ? -(int32_t)trq : (int32_t)trq;

    touchgfx::Unicode::snprintf(curTrqBuf, 16, "%d", (int)absTrq);
    ovCurTrq.invalidate();

    curTrqMinusSign.setVisible(neg);
    curTrqMinusSign.invalidate();

    int32_t disp = absTrq > 32767 ? 32767 : absTrq;
    SlowPosition_1_2.setValue((int)disp);
    SlowPosition_1_2.invalidate();
}

/* ─────────────────────────── START / STOP / HOME ────────────────────────── */

void Screen2View::onStartClicked(const touchgfx::AbstractButton& /*btn*/)
{
    /* Move to target position and enable RUN */
    SOEM_SetTargetPositionAbs(SOEM_GetPositionActual());  /* latch first */
    SOEM_SetRunEnable(1U);

    /* Set target to programmed target position */
    SOEM_SetTargetPositionAbs(paramValues[FIELD_TGT_POS]);
}

void Screen2View::onStopClicked(const touchgfx::AbstractButton& /*btn*/)
{
    SOEM_SetRunEnable(0U);
}

void Screen2View::onHomeClicked(const touchgfx::AbstractButton& /*btn*/)
{
    /* Latch current position then move to absolute zero */
    SOEM_SetTargetPositionAbs(SOEM_GetPositionActual());
    SOEM_SetTargetPositionAbs(0);
}
