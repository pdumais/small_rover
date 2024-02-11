#pragma once

typedef void (*state_processor_callback_t)(uint8_t type, void *data, uint32_t old_state, uint32_t new_state);

typedef struct
{
    state_processor_callback_t cb;
} fsm_state_t;