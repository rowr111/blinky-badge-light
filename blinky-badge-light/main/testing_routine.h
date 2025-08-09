#ifndef TESTING_ROUTINE_H
#define TESTING_ROUTINE_H


const char *reset_reason_str(esp_reset_reason_t reason);
void testing_routine();
extern volatile bool show_testing_routine;

#endif // TESTING_ROUTINE_H