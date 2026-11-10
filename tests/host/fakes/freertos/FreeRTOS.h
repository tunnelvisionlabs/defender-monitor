#pragma once
typedef int portMUX_TYPE;
#define portMUX_INITIALIZER_UNLOCKED 0
void fake_enter_critical(portMUX_TYPE *lock);
void fake_exit_critical(portMUX_TYPE *lock);
#define portENTER_CRITICAL(lock) fake_enter_critical(lock)
#define portEXIT_CRITICAL(lock) fake_exit_critical(lock)
