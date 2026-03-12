#ifndef KEYBOARD_HPP
#define KEYBOARD_HPP

#include <gui_generated/containers/KeyBoardBase.hpp>
#include <touchgfx/Callback.hpp>
#include <touchgfx/containers/buttons/AbstractButtonContainer.hpp>

class KeyBoard : public KeyBoardBase
{
public:
    KeyBoard();
    virtual ~KeyBoard() {}

    virtual void initialize();

    /**
     * Set the callback invoked whenever a key is pressed.
     * Key values:
     *   0..9  - digit
     *   10    - minus toggle
     *   11    - enter / confirm
     */
    void setKeyCallback(touchgfx::GenericCallback<int16_t>* cb)
    {
        keyPressedCallback = cb;
    }

protected:
    touchgfx::GenericCallback<int16_t>* keyPressedCallback;

private:
    void onButtonPressed(const touchgfx::AbstractButtonContainer& btn);

    touchgfx::Callback<KeyBoard, const touchgfx::AbstractButtonContainer&> btnCallback;
};

#endif // KEYBOARD_HPP
