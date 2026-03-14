#include <gui/mainpage_screen/MainPageView.hpp>
#include <gui/mainpage_screen/MainPagePresenter.hpp>
#include <gui/common/NumberFormat.hpp>
#include <stdint.h>

MainPageView::MainPageView()
    : jogStepCounts(1000),
      runUiState(0U),
      runAppliedState(0U),
      runDebounceCount(0U),
      runUpdatePending(0U),
      runClickGuardTicks(0U),
      runFeedbackLast(0U),
      runFeedbackStableCount(0U)
{

}

void MainPageView::setupScreen()
{
    MainPageViewBase::setupScreen();

    JogSpeedSlider.setValue(10);
    jogStepCounts = 1000;

    gui::configureNumericOverlay(numericTexts[0], CurrentPosition, numericBuffers[0]);
    gui::configureNumericOverlay(numericTexts[1], CurrentPosition_1, numericBuffers[1]);
    gui::configureNumericOverlay(numericTexts[2], CurrentPosition_1_1, numericBuffers[2]);
    add(numericTexts[0]);
    add(numericTexts[1]);
    add(numericTexts[2]);
    gui::formatSignedWithCommas(0, numericBuffers[0], gui::kNumericBufferSize);
    gui::formatUnsignedWithCommas(0, numericBuffers[1], gui::kNumericBufferSize);
    gui::formatUnsignedWithCommas(0, numericBuffers[2], gui::kNumericBufferSize);

    runUiState = 0U;
    runAppliedState = 0U;
    runDebounceCount = 0U;
    runUpdatePending = 0U;
    runClickGuardTicks = 0U;
    runFeedbackLast = 0U;
    runFeedbackStableCount = 0U;
    ServoON.forceState(false);
}

void MainPageView::tearDownScreen()
{
    MainPageViewBase::tearDownScreen();
}

void MainPageView::handleTickEvent()
{
    if (runClickGuardTicks > 0U)
    {
        runClickGuardTicks--;
    }

    if (runUpdatePending != 0U)
    {
        if (runDebounceCount < 20U)
        {
            runDebounceCount++;
        }

        if (runDebounceCount >= 5U)
        {
            if (runAppliedState != runUiState)
            {
                runAppliedState = runUiState;
                presenter->notifySetRunEnable(runAppliedState);
            }
            runUpdatePending = 0U;
            runDebounceCount = 0U;
        }
    }

    /* Hold-to-jog: send delta every tick while button is physically held */
    if (JogREVbutton.getPressedState())
    {
        presenter->notifySendPositionDelta(-jogStepCounts);
    }
    else if (JogFWDbutton.getPressedState())
    {
        presenter->notifySendPositionDelta(jogStepCounts);
    }

    MainPageViewBase::handleTickEvent();
}

void MainPageView::function1()
{
    if (runClickGuardTicks > 0U)
    {
        return;
    }

    runUiState = ServoON.getState() ? 1U : 0U;
    runDebounceCount = 0U;
    runUpdatePending = 1U;
    runClickGuardTicks = 8U;
}

void MainPageView::function2()
{
    /* JogREV: handled by hold-to-jog in handleTickEvent */
}

void MainPageView::function3()
{
    /* JogFWD: handled by hold-to-jog in handleTickEvent */
}

void MainPageView::function4(int value)
{
    if (value < 0)
    {
        value = 0;
    }
    jogStepCounts = value * 100;
    if (jogStepCounts <= 0)
    {
        jogStepCounts = 10;
    }
}

void MainPageView::function5()
{
}

void MainPageView::function6()
{
}

void MainPageView::function7()
{
}

void MainPageView::updateMotionData(int32_t position, int32_t speed, int16_t torque)
{
    gui::formatSignedWithCommas(position, numericBuffers[0], gui::kNumericBufferSize);
    gui::formatAbsoluteWithCommas(speed, numericBuffers[1], gui::kNumericBufferSize);
    gui::formatTorquePercent(torque, numericBuffers[2], gui::kNumericBufferSize);
    numericTexts[0].invalidate();
    numericTexts[1].invalidate();
    numericTexts[2].invalidate();
}

void MainPageView::updateRunEnable(uint8_t enabled)
{
    const uint8_t runFeedback = (enabled != 0U) ? 1U : 0U;
    if (runFeedback != runFeedbackLast)
    {
        runFeedbackLast = runFeedback;
        runFeedbackStableCount = 0U;
    }
    else if (runFeedbackStableCount < 20U)
    {
        runFeedbackStableCount++;
    }

    // Sync UI only when no local toggle transition is in progress and feedback is stable.
    if ((runUpdatePending == 0U) && (runUiState == runAppliedState) && (runFeedbackStableCount >= 5U) && (runAppliedState != runFeedback))
    {
        runUiState = runFeedback;
        runAppliedState = runFeedback;
        ServoON.forceState(runFeedback != 0U);
    }
}
