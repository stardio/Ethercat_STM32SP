#include <gui/screen3_screen/Screen3View.hpp>
#include <touchgfx/Color.hpp>
#include <texts/TextKeysAndLanguages.hpp>
#include <soem_port.h>

Screen3View::Screen3View() : cbMain(this, &Screen3View::onMainClicked), 
                              cbOriginReset(this, &Screen3View::onOriginResetClicked),
                              tickCounter(0),
                              homeResetState(0),
                              homeResetWaitCount(0)
{
}

void Screen3View::setupScreen()
{
    Screen3ViewBase::setupScreen();
    
    // Wire button callbacks
    Main_button.setAction(cbMain);
    buttonWithLabel1_2.setAction(cbOriginReset);

    // Match Screen1: hide TextProgress internal percentage text.
    SlowPosition_1_1.setColor(touchgfx::Color::getColorFromRGB(0, 0, 0));
    SlowPosition_1_1.setRange(0, 32767);

    // Raw value overlay (same strategy as Screen1).
    positionValueText.setPosition(114, 179, 150, 30);
    positionValueText.setTypedText(touchgfx::TypedText(T___SINGLEUSE_2NI1));
    positionValueText.setColor(touchgfx::Color::getColorFromRGB(255, 0, 0));
    touchgfx::Unicode::snprintf(positionValueBuffer, 16, "%d", 0);
    positionValueText.setWildcard(positionValueBuffer);
    add(positionValueText);

    positionMinusSign.setPosition(104, 192, 8, 3);
    positionMinusSign.setColor(touchgfx::Color::getColorFromRGB(255, 0, 0));
    positionMinusSign.setVisible(false);
    add(positionMinusSign);

    updatePositionDisplay();
}

void Screen3View::tearDownScreen()
{
    remove(positionValueText);
    remove(positionMinusSign);
    Screen3ViewBase::tearDownScreen();
}

void Screen3View::handleTickEvent()
{
    tickCounter++;

    /* Home reset state machine:
     * State 1: RUN was OFF, just enabled RUN (with latch). Wait before setting target=0.
     */
    if (homeResetState == 1u)
    {
        homeResetWaitCount++;
        if (homeResetWaitCount >= 5u)  /* Wait ~50ms for RUN to stabilize */
        {
            SOEM_SetTargetPositionAbs(0);
            homeResetState = 0u;  /* Back to idle */
        }
        return;
    }

    if ((tickCounter % 10u) != 0u)
    {
        return;
    }

    if (SOEM_GetPdoReady() == 0u)
    {
        return;
    }

    updatePositionDisplay();
}

void Screen3View::onMainClicked(const touchgfx::AbstractButton& /*btn*/)
{
    application().gotoScreen1ScreenNoTransition();
}

void Screen3View::onOriginResetClicked(const touchgfx::AbstractButton& /*btn*/)
{
    /* Define current physical position as the new origin (no motor movement).
     * After this call, GetPositionActual() returns 0 and all subsequent
     * absolute position commands are relative to this new origin. */
    SOEM_SetHomePosition();
    updatePositionDisplay();
}

void Screen3View::updatePositionDisplay()
{
    int32_t pos = SOEM_GetPositionActual();
    const bool posNeg = (pos < 0);
    int32_t posAbs = posNeg ? -pos : pos;

    int32_t posDisp = posAbs;
    if (posDisp > 32767) { posDisp = 32767; }

    SlowPosition_1_1.setValue((int)posDisp);
    SlowPosition_1_1.invalidate();

    touchgfx::Unicode::snprintf(positionValueBuffer, 16, "%d", (int)posAbs);
    positionValueText.invalidate();

    positionMinusSign.setVisible(posNeg);
    positionMinusSign.invalidate();
}

