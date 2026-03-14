#ifndef HOMEPAGEPRESENTER_HPP
#define HOMEPAGEPRESENTER_HPP

#include <gui/model/ModelListener.hpp>
#include <mvp/Presenter.hpp>

using namespace touchgfx;

class HomePageView;

class HomePagePresenter : public touchgfx::Presenter, public ModelListener
{
public:
    HomePagePresenter(HomePageView& v);

    virtual void activate();
    virtual void deactivate();
    virtual ~HomePagePresenter() {}

    virtual void onMotionDataUpdated(int32_t position, int32_t speed, int16_t torque);

    void notifySetHomePosition();

private:
    HomePagePresenter();
    HomePageView& view;
};

#endif // HOMEPAGEPRESENTER_HPP
