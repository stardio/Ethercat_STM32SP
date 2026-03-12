#ifndef SCREEN3VIEW_HPP
#define SCREEN3VIEW_HPP

#include <gui_generated/screen3_screen/Screen3ViewBase.hpp>
#include <gui/screen3_screen/Screen3Presenter.hpp>
#include <touchgfx/Callback.hpp>
#include <touchgfx/Unicode.hpp>
#include <touchgfx/widgets/AbstractButton.hpp>
#include <touchgfx/widgets/Box.hpp>
#include <touchgfx/widgets/TextAreaWithWildcard.hpp>

class Screen3View : public Screen3ViewBase
{
public:
    Screen3View();
    virtual ~Screen3View() {}
    virtual void setupScreen();
    virtual void tearDownScreen();
    virtual void handleTickEvent();
    
protected:
    void onMainClicked(const touchgfx::AbstractButton& btn);
    void onOriginResetClicked(const touchgfx::AbstractButton& btn);
    void updatePositionDisplay();
    
private:
    touchgfx::Callback<Screen3View, const touchgfx::AbstractButton&> cbMain;
    touchgfx::Callback<Screen3View, const touchgfx::AbstractButton&> cbOriginReset;
    touchgfx::TextAreaWithOneWildcard positionValueText;
    touchgfx::Box positionMinusSign;
    touchgfx::Unicode::UnicodeChar positionValueBuffer[16];
    uint32_t tickCounter;
    uint8_t homeResetState;
    uint32_t homeResetWaitCount;
};

#endif // SCREEN3VIEW_HPP
