#ifndef MANUALPAGEVIEW_HPP
#define MANUALPAGEVIEW_HPP

#include <stdint.h>
#include <gui/common/NumberFormat.hpp>
#include <gui_generated/manualpage_screen/ManualPageViewBase.hpp>
#include <gui/manualpage_screen/ManualPagePresenter.hpp>
#include <touchgfx/widgets/TextAreaWithWildcard.hpp>

class ManualPageView : public ManualPageViewBase
{
public:
    ManualPageView();
    virtual ~ManualPageView() {}
    virtual void setupScreen();
    virtual void tearDownScreen();
    virtual void handleTickEvent();

    virtual void function1();
    virtual void function2();
    virtual void function3();
    virtual void function4();
    virtual void function5();

    void updateMotionData(int32_t position, int32_t speed, int16_t torque);
protected:
    int jogStepCounts;
    touchgfx::TextAreaWithOneWildcard numericTexts[3];
    touchgfx::Unicode::UnicodeChar numericBuffers[3][gui::kNumericBufferSize];
};

#endif // MANUALPAGEVIEW_HPP
