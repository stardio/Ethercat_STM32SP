#ifndef MODELLISTENER_HPP
#define MODELLISTENER_HPP

#include <gui/model/Model.hpp>
#include <stdint.h>

class ModelListener
{
public:
    ModelListener() : model(0) {}

    virtual ~ModelListener() {}

    void bind(Model* m)
    {
        model = m;
    }

    // 서보 상태 (0=off, 1=on)
    virtual void onRunEnableChanged(uint8_t enabled) {}

    // 실시간 수치 업데이트 (position, speed, torque)
    virtual void onMotionDataUpdated(int32_t position, int32_t speed, int16_t torque) {}

protected:
    Model* model;
};

#endif // MODELLISTENER_HPP
