#ifndef MODEL_HPP
#define MODEL_HPP

#include <stdint.h>

class ModelListener;

class Model
{
public:
    Model();

    void bind(ModelListener* listener)
    {
        modelListener = listener;
    }

    void tick();

    // View→Model 커맨드 (서보 ON/OFF)
    void setRunEnable(uint8_t enable);

    // View→Model 커맨드 (조그)
    void sendPositionDelta(int32_t delta);

    // View→Model 커맨드 (홈 설정)
    void setHomePosition();

protected:
    ModelListener* modelListener;
};

#endif // MODEL_HPP
