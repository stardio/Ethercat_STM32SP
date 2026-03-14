#include <gui/programpage_screen/ProgramPageView.hpp>
#include <gui/programpage_screen/ProgramPagePresenter.hpp>

ProgramPagePresenter::ProgramPagePresenter(ProgramPageView& v)
    : view(v)
{
}

void ProgramPagePresenter::activate()
{
}

void ProgramPagePresenter::deactivate()
{
}

void ProgramPagePresenter::onMotionDataUpdated(int32_t position, int32_t speed, int16_t torque)
{
    view.updateMotionData(position, speed, torque);
}
