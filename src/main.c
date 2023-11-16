#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "pico/time.h"
#include "hardware/pwm.h"
#include <math.h>
 
#define BREATHING_PIN 0
// 125MHz...
#define SYSTEM_CLOCK_FREQ 125000000
#define PWM_FREQUENCY 125
#define PWM_WRAP 10000

#define DEBOUNCE_DELAY 400000

#define BUTTON_PIN_A 20
#define BUTTON_PIN_B 21
#define BUTTON_PIN_CALCULATE 22

// NOTE: Anything beyond 63, requires 6 bits (6 LEDs)
#define MAX_SUPPORTED_VALUE 63

// Track when was the last current time tat button was pressed...
volatile uint64_t last_button_press = 0;

struct repeating_timer breathing_timer; 
uint64_t breathing_start_time = 0;
volatile float brightness = 0.0;
volatile bool increasing_brightness = true;
uint led_slice_num = 0;

struct repeating_timer reset_timer;
bool resetting = false;

int variable_a = 0;
int variable_b = 0;


//#region UTILs

void update_pwm_duty_cycle(uint slice_num, float duty_cycle) {
    uint16_t level = (uint16_t)((PWM_WRAP + 1) * (duty_cycle / 100.0));
    pwm_set_chan_level(slice_num, pwm_gpio_to_channel(BREATHING_PIN), level);
}

void reset_led_states() {
    for (int i = 1; i <= 15; i++) {
        gpio_put(i, 0);
    }
    update_pwm_duty_cycle(led_slice_num, 0);
}

// NOTE: First index should be LSB
void int_to_binary_array(int n, int binaryArray[], int size) {
    for (int i = 0; i < size; ++i) {
        if (i < sizeof(n) * 8) {
            binaryArray[i] = (n >> i) & 1;
        } else {
            binaryArray[i] = 0;
        }
    }
}

//#endregion

bool breathing_timer_callback(struct repeating_timer *t) {
    // NOTE: Breathing status is updated every 0.1 second.
    float time_elapsed = (time_us_64() - breathing_start_time) / 1000000.0;
    
    // Calculate and set PWM duty cycle based on time elapsed,
    // increasing duty cycle linearly over 1 seconds.
    float duty_cycle = time_elapsed * 100.0;
    pwm_set_chan_level(pwm_gpio_to_slice_num(BREATHING_PIN), pwm_gpio_to_channel(BREATHING_PIN), (uint16_t)((PWM_WRAP / 100.0) * duty_cycle));

    if (increasing_brightness) {
        update_pwm_duty_cycle(led_slice_num, brightness);
        brightness += (100.0 * 100 / 1000.0);

        if (brightness >= 100.0) {
            increasing_brightness = false;
        }
    } else {
        brightness -= (100.0 * 100 / 1000.0);

        if (brightness <= 0.0) {
            increasing_brightness = true;
        }
    }

    return true;
}

bool reset_timer_callback(struct reset_timer *t) {
    variable_a = 0;
    variable_b = 0;
    cancel_breathing();
    reset_led_states();

    resetting = false;
    return false;
}

void cancel_breathing() {
    cancel_repeating_timer(&breathing_timer);
}

void start_breathing() {
    breathing_start_time = time_us_64();
    add_repeating_timer_ms(-100, breathing_timer_callback, NULL, &breathing_timer);
}

void trigger_resetting() {
    resetting = true;
    add_repeating_timer_ms(5000, reset_timer_callback, NULL, &reset_timer);
}

//#region CALCULATE

void display_result(int result) {
    int binary_count[6];
    int_to_binary_array(result, binary_count, 6);

    // GP10 to GP15
    int current_led = 10;
    for (int i = 5; i >= 0; --i) {
        gpio_put(current_led, binary_count[i]);
        current_led += 1;
    }
}

void handle_unsupported_result() {
    for (int i = 10; i >=15; ++i) {
        gpio_put(i, 1);
    }
    start_breathing();
}

void button_calculate_callback() {
    cancel_breathing();
    trigger_resetting();

    int result = variable_a * variable_b;
    printf("%d x %d = %d\n", variable_a, variable_b, result);

    if (result > MAX_SUPPORTED_VALUE) {
        handle_unsupported_result();
        return;
    }

    display_result(result);
    gpio_put(0, 0);
}

//#endregion

//#region BUTTON FOR B

void update_variable_b_led() {
    int binary_count[4];
    int_to_binary_array(variable_b, binary_count, 4);

    // GP6 to GP9
    int current_led = 6;
    for (int i = 3; i >= 0; --i) {
        gpio_put(current_led, binary_count[i]);
        current_led += 1;
    }
}

void button_b_callback() {
    variable_b += 1;
    printf("%d\n", variable_b);
    update_variable_b_led();
}

//#endregion

//#region BUTTON FOR A

void update_variable_a_led() {
    int binary_count[4];
    int_to_binary_array(variable_a, binary_count, 4);

    // GP2 to GP5
    int current_led = 2;
    for (int i = 3; i >= 0; --i) {
        gpio_put(current_led, binary_count[i]);
        current_led += 1;
    }
}

void button_a_callback() {
    variable_a += 1;
    printf("%d\n", variable_a);
    update_variable_a_led();
}

//#endregion

void button_callback(uint gpio, uint32_t events) {
    // NOTE: Ignore all inputs while it is resetting...
    if (resetting) {
        return;
    }

    uint64_t current_time = time_us_64();
    bool within_debouce_delay_threshold = ((current_time - last_button_press) <= DEBOUNCE_DELAY);
    if (within_debouce_delay_threshold) {
        return;
    }
    last_button_press = current_time;

    if (gpio == BUTTON_PIN_A) {
        button_a_callback();
    } else if (gpio == BUTTON_PIN_B) {
        button_b_callback();
    } else if (gpio == BUTTON_PIN_CALCULATE) {
        button_calculate_callback();
    }
}

void setup_pwm() {
    float divider = (float)SYSTEM_CLOCK_FREQ / (PWM_FREQUENCY * (PWM_WRAP + 1));
    gpio_set_function(BREATHING_PIN, GPIO_FUNC_PWM);

    led_slice_num = pwm_gpio_to_slice_num(BREATHING_PIN);
    pwm_set_wrap(led_slice_num, PWM_WRAP);
    pwm_set_clkdiv(led_slice_num, divider);

    pwm_set_enabled(led_slice_num, true);
}


int main() {
    stdio_init_all();

    // Enable interrupt on specified pin upon a button press,
    // (rising or falling edge)
    gpio_set_irq_enabled_with_callback(BUTTON_PIN_A, GPIO_IRQ_EDGE_FALL | GPIO_IRQ_EDGE_RISE, true, &button_callback);
    gpio_set_irq_enabled_with_callback(BUTTON_PIN_B, GPIO_IRQ_EDGE_FALL | GPIO_IRQ_EDGE_RISE, true, &button_callback);
    gpio_set_irq_enabled_with_callback(BUTTON_PIN_CALCULATE, GPIO_IRQ_EDGE_FALL | GPIO_IRQ_EDGE_RISE, true, &button_callback);

    // Init light ports from GP1 to GP15...
    for (int i = 1; i <= 15; i++) {
        gpio_init(i);
        gpio_set_dir(i, GPIO_OUT);
        gpio_put(i, 0);
    }

    setup_pwm();

    while (true) {}
}