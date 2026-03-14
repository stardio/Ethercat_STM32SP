#include <gui/homepage_screen/HomePageView.hpp>
#include <gui/homepage_screen/HomePagePresenter.hpp>
#include <gui/common/NumberFormat.hpp>

HomePageView::HomePageView()
{
}

void HomePageView::setupScreen()
{
    HomePageViewBase::setupScreen();

    gui::configureNumericOverlay(positionText, CurrentPosition, positionBuffer);
    add(positionText);
    gui::formatSignedWithCommas(0, positionBuffer, gui::kNumericBufferSize);
}

void HomePageView::tearDownScreen()
{
    HomePageViewBase::tearDownScreen();
}

void HomePageView::handleTickEvent()
{
    HomePageViewBase::handleTickEvent();
}

void HomePageView::function1()
{
    /* CP ORG Rset button: latch current position as home origin */
    presenter->notifySetHomePosition();
}

void HomePageView::function2() {}

void HomePageView::updateMotionData(int32_t position, int32_t /*speed*/, int16_t /*torque*/)
{
    gui::formatSignedWithCommas(position, positionBuffer, gui::kNumericBufferSize);
    positionText.invalidate();
}

