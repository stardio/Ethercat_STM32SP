#include <gui/model/Model.hpp>
#include <gui/model/ModelListener.hpp>
#include <stdint.h>

#if !defined(SIMULATOR)
extern "C"
{
uint8_t  SOEM_GetPdoReady(void);
uint8_t  SOEM_GetRunEnable(void);
int32_t  SOEM_GetPositionActual(void);
int32_t  SOEM_GetVelocityActual(void);
int16_t  SOEM_GetTorqueActual(void);
void     SOEM_SetRunEnable(uint8_t enable);
void     SOEM_SetTargetPositionDelta(int32_t delta);
void     SOEM_SetHomePosition(void);
}
#endif

namespace
{
#if defined(SIMULATOR)
uint8_t simRunEnable = 0U;
int32_t simPositionActual = 0;
int32_t simVelocityActual = 0;
int16_t simTorqueActual = 0;

uint8_t modelGetPdoReady()
{
    return 1U;
}

uint8_t modelGetRunEnable()
{
    return simRunEnable;
}

int32_t modelGetPositionActual()
{
    return simPositionActual;
}

int32_t modelGetVelocityActual()
{
    return simVelocityActual;
}

int16_t modelGetTorqueActual()
{
    return simTorqueActual;
}

void modelSetRunEnable(uint8_t enable)
{
    simRunEnable = (enable != 0U) ? 1U : 0U;
}

void modelSetTargetPositionDelta(int32_t delta)
{
    simPositionActual += delta;
    simVelocityActual = (delta < 0) ? -delta : delta;

    // Keep torque in a reasonable display range for simulator-only UI checks.
    int32_t torqueCandidate = simVelocityActual / 10;
    if (torqueCandidate > 200)
    {
        torqueCandidate = 200;
    }
    simTorqueActual = static_cast<int16_t>(torqueCandidate);
}

void modelSetHomePosition()
{
    simPositionActual = 0;
}
#else
uint8_t modelGetPdoReady()
{
    return SOEM_GetPdoReady();
}

uint8_t modelGetRunEnable()
{
    return SOEM_GetRunEnable();
}

int32_t modelGetPositionActual()
{
    return SOEM_GetPositionActual();
}

int32_t modelGetVelocityActual()
{
    return SOEM_GetVelocityActual();
}

int16_t modelGetTorqueActual()
{
    return SOEM_GetTorqueActual();
}

void modelSetRunEnable(uint8_t enable)
{
    SOEM_SetRunEnable(enable);
}

void modelSetTargetPositionDelta(int32_t delta)
{
    SOEM_SetTargetPositionDelta(delta);
}

void modelSetHomePosition()
{
    SOEM_SetHomePosition();
}
#endif
}

Model::Model() : modelListener(0)
{
}

void Model::tick()
{
    if (modelListener == 0)
    {
        return;
    }

    if (modelGetPdoReady() != 0U)
    {
        modelListener->onMotionDataUpdated(
            modelGetPositionActual(),
            modelGetVelocityActual(),
            modelGetTorqueActual());

        modelListener->onRunEnableChanged(modelGetRunEnable());
    }
}

void Model::setRunEnable(uint8_t enable)
{
    modelSetRunEnable(enable);
}

void Model::sendPositionDelta(int32_t delta)
{
    modelSetTargetPositionDelta(delta);
}

void Model::setHomePosition()
{
    modelSetHomePosition();
}
