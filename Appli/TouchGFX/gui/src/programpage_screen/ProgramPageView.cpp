#include <gui/programpage_screen/ProgramPageView.hpp>
#include <gui/common/NumberFormat.hpp>

ProgramPageView::ProgramPageView()
{
}

void ProgramPageView::setupScreen()
{
    ProgramPageViewBase::setupScreen();

    gui::configureNumericOverlay(numericTexts[0], CurrentPosition, numericBuffers[0]);
    gui::configureNumericOverlay(numericTexts[1], CurrentPosition_1, numericBuffers[1]);
    gui::configureNumericOverlay(numericTexts[2], CurrentPosition_2, numericBuffers[2]);
    gui::configureNumericOverlay(numericTexts[3], CurrentPosition_3, numericBuffers[3]);
    gui::configureNumericOverlay(numericTexts[4], CurrentPosition_4, numericBuffers[4]);
    gui::configureNumericOverlay(numericTexts[5], CurrentPosition_5, numericBuffers[5]);
    gui::configureNumericOverlay(numericTexts[6], CurrentPosition_6, numericBuffers[6]);
    gui::configureNumericOverlay(numericTexts[7], CurrentPosition_7, numericBuffers[7]);

    add(numericTexts[0]);
    add(numericTexts[1]);
    add(numericTexts[2]);
    add(numericTexts[3]);
    add(numericTexts[4]);
    add(numericTexts[5]);
    add(numericTexts[6]);
    add(numericTexts[7]);

    for (uint8_t index = 0U; index < 8U; index++)
    {
        gui::formatUnsignedWithCommas(0U, numericBuffers[index], gui::kNumericBufferSize);
    }
}

void ProgramPageView::tearDownScreen()
{
    ProgramPageViewBase::tearDownScreen();
}

void ProgramPageView::handleTickEvent()
{
    ProgramPageViewBase::handleTickEvent();
}

void ProgramPageView::updateMotionData(int32_t position, int32_t speed, int16_t torque)
{
    gui::formatSignedWithCommas(position, numericBuffers[6], gui::kNumericBufferSize);
    gui::formatTorquePercent(torque, numericBuffers[7], gui::kNumericBufferSize);
    gui::formatAbsoluteWithCommas(speed, numericBuffers[1], gui::kNumericBufferSize);
    numericTexts[1].invalidate();
    numericTexts[6].invalidate();
    numericTexts[7].invalidate();
}

