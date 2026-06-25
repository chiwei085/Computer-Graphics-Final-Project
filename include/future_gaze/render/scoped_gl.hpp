#pragma once

#include <GL/freeglut.h>

namespace future_gaze
{

// RAII wrapper for glPushMatrix / glPopMatrix.
class ScopedMatrixPush
{
public:
    ScopedMatrixPush() { glPushMatrix(); }
    ~ScopedMatrixPush() { glPopMatrix(); }
    ScopedMatrixPush(const ScopedMatrixPush&) = delete;
    ScopedMatrixPush& operator=(const ScopedMatrixPush&) = delete;
};

// RAII wrapper for glEnableClientState / glDisableClientState.
class ScopedClientState
{
public:
    explicit ScopedClientState(GLenum state) : state_(state) {
        glEnableClientState(state_);
    }
    ~ScopedClientState() { glDisableClientState(state_); }
    ScopedClientState(const ScopedClientState&) = delete;
    ScopedClientState& operator=(const ScopedClientState&) = delete;

private:
    GLenum state_;
};

}  // namespace future_gaze
