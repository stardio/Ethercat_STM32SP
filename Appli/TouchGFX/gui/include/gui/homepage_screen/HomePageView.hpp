#ifndef HOMEPAGEVIEW_HPP
#define HOMEPAGEVIEW_HPP

#include <gui/common/NumberFormat.hpp>
#include <gui_generated/homepage_screen/HomePageViewBase.hpp>
#include <gui/homepage_screen/HomePagePresenter.hpp>
#include <touchgfx/widgets/TextAreaWithWildcard.hpp>

class HomePageView : public HomePageViewBase
{
public:
    HomePageView();
    virtual ~HomePageView() {}
    virtual void setupScreen();
    virtual void tearDownScreen();
    virtual void handleTickEvent();
    virtual void function1();
    virtual void function2();

    void updateMotionData(int32_t position, int32_t speed, int16_t torque);
protected:
    touchgfx::TextAreaWithOneWildcard positionText;
    touchgfx::Unicode::UnicodeChar positionBuffer[gui::kNumericBufferSize];
};

#endif // HOMEPAGEVIEW_HPP
