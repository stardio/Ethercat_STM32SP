#ifndef PROGRAMPAGEVIEW_HPP
#define PROGRAMPAGEVIEW_HPP

#include <gui/common/NumberFormat.hpp>
#include <gui_generated/programpage_screen/ProgramPageViewBase.hpp>
#include <gui/programpage_screen/ProgramPagePresenter.hpp>
#include <touchgfx/widgets/TextAreaWithWildcard.hpp>

class ProgramPageView : public ProgramPageViewBase
{
public:
    ProgramPageView();
    virtual ~ProgramPageView() {}
    virtual void setupScreen();
    virtual void tearDownScreen();
    virtual void handleTickEvent();
    void updateMotionData(int32_t position, int32_t speed, int16_t torque);
protected:
    touchgfx::TextAreaWithOneWildcard numericTexts[8];
    touchgfx::Unicode::UnicodeChar numericBuffers[8][gui::kNumericBufferSize];
    touchgfx::TextAreaWithOneWildcard delayText;
    touchgfx::Unicode::UnicodeChar delayBuffer[gui::kNumericBufferSize];
};

#endif // PROGRAMPAGEVIEW_HPP
