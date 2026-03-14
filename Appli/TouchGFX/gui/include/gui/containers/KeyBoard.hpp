#ifndef KEYBOARD_HPP
#define KEYBOARD_HPP

#include <gui_generated/containers/KeyBoardBase.hpp>

class KeyBoard : public KeyBoardBase
{
public:
    KeyBoard();
    virtual ~KeyBoard() {}

    virtual void initialize();
protected:
};

#endif // KEYBOARD_HPP
