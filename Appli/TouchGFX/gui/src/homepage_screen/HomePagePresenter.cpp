#include <gui/homepage_screen/HomePageView.hpp>
#include <gui/homepage_screen/HomePagePresenter.hpp>

HomePagePresenter::HomePagePresenter(HomePageView& v)
    : view(v)
{
}

void HomePagePresenter::activate()
{
}

void HomePagePresenter::deactivate()
{
}

void HomePagePresenter::onMotionDataUpdated(int32_t position, int32_t speed, int16_t torque)
{
    view.updateMotionData(position, speed, torque);
}

void HomePagePresenter::notifySetHomePosition()
{
    model->setHomePosition();
}
