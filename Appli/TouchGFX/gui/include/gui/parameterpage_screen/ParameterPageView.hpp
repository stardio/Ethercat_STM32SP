#ifndef PARAMETERPAGEVIEW_HPP
#define PARAMETERPAGEVIEW_HPP

#include <gui_generated/parameterpage_screen/ParameterPageViewBase.hpp>
#include <gui/parameterpage_screen/ParameterPagePresenter.hpp>

class ParameterPageView : public ParameterPageViewBase
{
public:
    ParameterPageView();
    virtual ~ParameterPageView() {}
    virtual void setupScreen();
    virtual void tearDownScreen();
protected:
};

#endif // PARAMETERPAGEVIEW_HPP
