// The minimum duty cycle to stop the motor
#define DUTY_STOP 0
// The initial duty cycle to kick-start the motor
#define DUTY_KICK 10
// The maximum duty cycle of the motor
#define DUTY_MAX 45
// RPM max
#define RPM_MAX 7200

// 1 second in ticks
#define NUM_TICKS_PER_SEC 1000
// the number to hall sensor interrupts per revolution
#define HALL_COUNT_PER_REV 18
// the number of seconds in a min
#define SECS_IN_MIN 60
// the reference voltage of the motor
#define MOTOR_REFERENCE_VOLTAGE 3.3
// the resistance of the current sensors
#define R_SENSE 0.007
// the gain of the current sensors
#define G_CSA 10
