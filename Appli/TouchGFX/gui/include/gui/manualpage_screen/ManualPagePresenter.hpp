#ifndef MANUALPAGEPRESENTER_HPP
#define MANUALPAGEPRESENTER_HPP

#include <gui/model/ModelListener.hpp>
#include <mvp/Presenter.hpp>

using namespace touchgfx;

class ManualPageView;

class ManualPagePresenter : public touchgfx::Presenter, public ModelListener
{
public:
    ManualPagePresenter(ManualPageView& v);

    virtual void activate();
    virtual void deactivate();
    virtual ~ManualPagePresenter() {}

    virtual void onMotionDataUpdated(int32_t position, int32_t speed, int16_t torque);

    void notifySendPositionDelta(int32_t delta);

private:
    ManualPagePresenter();
    ManualPageView& view;
};

#endif // MANUALPAGEPRESENTER_HPP
