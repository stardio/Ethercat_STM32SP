#include <gui/manualpage_screen/ManualPageView.hpp>
#include <gui/manualpage_screen/ManualPagePresenter.hpp>

ManualPagePresenter::ManualPagePresenter(ManualPageView& v)
    : view(v)
{
}

void ManualPagePresenter::activate()
{
}

void ManualPagePresenter::deactivate()
{
}

void ManualPagePresenter::onMotionDataUpdated(int32_t position, int32_t speed, int16_t torque)
{
    view.updateMotionData(position, speed, torque);
}

void ManualPagePresenter::notifySendPositionDelta(int32_t delta)
{
    model->sendPositionDelta(delta);
}
