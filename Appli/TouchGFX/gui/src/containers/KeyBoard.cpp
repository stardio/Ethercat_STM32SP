#include <gui/containers/KeyBoard.hpp>
#include <touchgfx/containers/buttons/AbstractButtonContainer.hpp>

KeyBoard::KeyBoard() :
    keyPressedCallback(nullptr),
    btnCallback(this, &KeyBoard::onButtonPressed)
{
}

void KeyBoard::initialize()
{
    KeyBoardBase::initialize();

    N0.setAction(btnCallback);
    N1.setAction(btnCallback);
    N2.setAction(btnCallback);
    N3.setAction(btnCallback);
    N4.setAction(btnCallback);
    N5.setAction(btnCallback);
    N6.setAction(btnCallback);
    N7.setAction(btnCallback);
    N8.setAction(btnCallback);
    N9.setAction(btnCallback);
    Minus.setAction(btnCallback);
    ent.setAction(btnCallback);
}

void KeyBoard::onButtonPressed(const touchgfx::AbstractButtonContainer& btn)
{
    if (keyPressedCallback == nullptr || !keyPressedCallback->isValid())
    {
        return;
    }

    int16_t key = -1;

    /* Compare button addresses directly via const_cast to identify which button was pressed */
    const AbstractButtonContainer* pBtn = &btn;
    
    if      (pBtn == static_cast<const touchgfx::AbstractButtonContainer*>(&N0))    { key = 0;  }
    else if (pBtn == static_cast<const touchgfx::AbstractButtonContainer*>(&N1))    { key = 1;  }
    else if (pBtn == static_cast<const touchgfx::AbstractButtonContainer*>(&N2))    { key = 2;  }
    else if (pBtn == static_cast<const touchgfx::AbstractButtonContainer*>(&N3))    { key = 3;  }
    else if (pBtn == static_cast<const touchgfx::AbstractButtonContainer*>(&N4))    { key = 4;  }
    else if (pBtn == static_cast<const touchgfx::AbstractButtonContainer*>(&N5))    { key = 5;  }
    else if (pBtn == static_cast<const touchgfx::AbstractButtonContainer*>(&N6))    { key = 6;  }
    else if (pBtn == static_cast<const touchgfx::AbstractButtonContainer*>(&N7))    { key = 7;  }
    else if (pBtn == static_cast<const touchgfx::AbstractButtonContainer*>(&N8))    { key = 8;  }
    else if (pBtn == static_cast<const touchgfx::AbstractButtonContainer*>(&N9))    { key = 9;  }
    else if (pBtn == static_cast<const touchgfx::AbstractButtonContainer*>(&Minus)) { key = 10; }  /* minus toggle */
    else if (pBtn == static_cast<const touchgfx::AbstractButtonContainer*>(&ent))   { key = 11; }  /* enter/confirm */

    if (key >= 0)
    {
        keyPressedCallback->execute(key);
    }
}
