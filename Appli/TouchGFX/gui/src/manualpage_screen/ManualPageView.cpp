#include <gui/manualpage_screen/ManualPageView.hpp>
#include <gui/manualpage_screen/ManualPagePresenter.hpp>
#include <gui/common/NumberFormat.hpp>

ManualPageView::ManualPageView()
    : jogStepCounts(1000)
{
}

void ManualPageView::setupScreen()
{
    ManualPageViewBase::setupScreen();
    jogStepCounts = 1000;

    gui::configureNumericOverlay(numericTexts[0], CurrentPosition, numericBuffers[0]);
    gui::configureNumericOverlay(numericTexts[1], CurrentPosition_1, numericBuffers[1]);
    gui::configureNumericOverlay(numericTexts[2], CurrentPosition_2, numericBuffers[2]);
    add(numericTexts[0]);
    add(numericTexts[1]);
    add(numericTexts[2]);
    gui::formatSignedWithCommas(0, numericBuffers[0], gui::kNumericBufferSize);
    gui::formatUnsignedWithCommas(0, numericBuffers[1], gui::kNumericBufferSize);
    gui::formatUnsignedWithCommas(0, numericBuffers[2], gui::kNumericBufferSize);
}

void ManualPageView::tearDownScreen()
{
    ManualPageViewBase::tearDownScreen();
}

void ManualPageView::handleTickEvent()
{
    ManualPageViewBase::handleTickEvent();
}

void ManualPageView::function1() {}
void ManualPageView::function2() {}
void ManualPageView::function3() {}

void ManualPageView::function4()
{
    presenter->notifySendPositionDelta(jogStepCounts);
}

void ManualPageView::function5()
{
    presenter->notifySendPositionDelta(-jogStepCounts);
}

void ManualPageView::updateMotionData(int32_t position, int32_t speed, int16_t torque)
{
    gui::formatSignedWithCommas(position, numericBuffers[0], gui::kNumericBufferSize);
    gui::formatAbsoluteWithCommas(speed, numericBuffers[1], gui::kNumericBufferSize);
    gui::formatTorquePercent(torque, numericBuffers[2], gui::kNumericBufferSize);
    numericTexts[0].invalidate();
    numericTexts[1].invalidate();
    numericTexts[2].invalidate();
}

