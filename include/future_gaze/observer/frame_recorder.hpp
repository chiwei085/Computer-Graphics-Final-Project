#pragma once

namespace future_gaze::observer
{

float ObserverDeltaSeconds(float fallback_delta_seconds);
void RecordFramebuffer(int width, int height, float app_time_seconds);

}  // namespace future_gaze::observer
