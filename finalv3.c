#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "hardware/pwm.h"
#include "hardware/i2c.h"
#include "hardware/gpio.h"
#include "pico/time.h"
#include "ssd1306.h"
#include "fonte.h"
#include <stdlib.h>

// Declaração da função de borda do display OLED (caso não esteja definida em ssd1306.h)
void ssd1306_draw_border(ssd1306_t *dev, int thickness);

// =====================
// Definição dos pinos
// =====================
#define LED_VERDE 11      // LED verde: indica alerta fora do intervalo seguro
#define LED_AZUL 12       // LED azul: indica inatividade (pisca após 30 s)
#define LED_VERMELHO 13   // LED vermelho: alerta vermelho após 45 s de inatividade
#define JOYSTICK_X_ADC 1  // Canal ADC para o eixo X do joystick
#define JOYSTICK_Y_ADC 0  // Canal ADC para o eixo Y do joystick
#define BUTTON_A 5        // Botão A: reseta alertas e contadores
#define BUTTON_B 6        // Botão B: ativa/desativa a emergência
#define I2C_SDA_PIN 14    // Pino SDA para comunicação I2C com o display
#define I2C_SCL_PIN 15    // Pino SCL para comunicação I2C com o display
#define SSD1306_ADDR 0x3C // Endereço I2C do display OLED
#define BUZZER1_PIN 10    // Buzzer conectada no pino 10

// =====================
// Parâmetros do Sistema
// =====================
#define DEADZONE 100      // Zona morta para evitar leituras ruidosas do joystick
#define TIME_BLUE_LED 30  // Após 30 s de inatividade, o LED azul pisca 10 vezes
#define TIME_RED_ALERT 45 // Após 45 s de inatividade, ativa alerta vermelho (display + LED vermelho)
#define TIME_BUZZER 60    // Após 60 s de inatividade, a buzzer toca intermitente
#define TEXT_OFFSET 4     // Margem para posicionar o texto dentro do display

// =====================
// Função Inline para PWM
// =====================
static inline bool my_pwm_get_enabled(uint slice)
{
    return (pwm_hw->slice[slice].csr & (1u << 0)) != 0;
}

// =====================
// Variáveis Globais
// =====================
volatile absolute_time_t last_move_time;        // Tempo do último movimento do joystick
volatile uint16_t last_x = 2048, last_y = 2048; // Valores anteriores do joystick
volatile int stationary_count = 0;              // Contador de inatividade
volatile bool red_alert_active = false;         // Indica se o alerta vermelho está ativo
volatile bool buzzer_active = false;            // Indica se a buzzer está ativa

ssd1306_t display;    // Estrutura para o display OLED
uint slice_buzzer1;   // Slice do PWM para a buzzer
bool beep_on = false; // Variável para alternar o estado do beep

// Variáveis para a função emergência
volatile int buttonB_count = 0;
volatile bool emergency_active = false;
absolute_time_t last_buttonB_time;

// =====================
// Função: read_joystick
// =====================
void read_joystick(uint16_t *x, uint16_t *y)
{
    adc_select_input(JOYSTICK_X_ADC);
    *x = adc_read();
    adc_select_input(JOYSTICK_Y_ADC);
    *y = adc_read();
}

// =====================
// Função: gpio_callback
// =====================
// Trata as interrupções dos botões A e B
void gpio_callback(uint gpio, uint32_t events)
{
    if (gpio == BUTTON_A)
    {
        // Reseta os alertas e contadores
        red_alert_active = false;
        buzzer_active = false;
        pwm_set_enabled(slice_buzzer1, false); // Desliga a buzzer
        gpio_put(LED_VERMELHO, 0);
        ssd1306_clear(&display);
        ssd1306_draw_border(&display, 1);
        ssd1306_show(&display);
        last_move_time = get_absolute_time();
        stationary_count = 0;
    }
    else if (gpio == BUTTON_B)
    {
        // Lógica para acionar/desativar a emergência
        absolute_time_t now = get_absolute_time();
        if (absolute_time_diff_us(last_buttonB_time, now) < 500000)
        { // Intervalo < 500ms
            buttonB_count++;
        }
        else
        {
            buttonB_count = 1;
        }
        last_buttonB_time = now;

        // Se não estiver em emergência e ocorrer 2 pressões consecutivas, ativa a emergência
        if (!emergency_active && buttonB_count == 2)
        {
            emergency_active = true;
            printf("EMERGENCIA - X:%d Y:%d\n", last_x, last_y);
        }
        // Se a emergência estiver ativa e ocorrer 3 pressões consecutivas, desativa a emergência
        else if (emergency_active && buttonB_count == 3)
        {
            emergency_active = false;
            buttonB_count = 0;
            printf("Emergencia Desativada.\n");
        }
    }
}

// =====================
// Função: blink_led
// =====================
// Pisca um LED (definido por led_pin) por um número de vezes com um intervalo definido (ms)
void blink_led(uint led_pin, int times, int interval_ms)
{
    for (int i = 0; i < times; i++)
    {
        gpio_put(led_pin, 1);
        sleep_ms(interval_ms);
        gpio_put(led_pin, 0);
        sleep_ms(interval_ms);
    }
}

// =====================
// Função: display_alert
// =====================
// Exibe uma mensagem de alerta no display OLED, com os valores do joystick
void display_alert(uint16_t x, uint16_t y)
{
    char buffer[32];
    sprintf(buffer, "X:%d Y:%d", x, y);

    ssd1306_clear(&display);
    ssd1306_draw_border(&display, 1);
    ssd1306_draw_string(&display, TEXT_OFFSET, TEXT_OFFSET, "");
    ssd1306_draw_string(&display, TEXT_OFFSET, TEXT_OFFSET, "ATENCAO");
    ssd1306_draw_string(&display, TEXT_OFFSET, TEXT_OFFSET + 20, buffer);
    ssd1306_show(&display);
}

// =====================
// Função: main
// =====================
int main()
{
    stdio_init_all();
    printf("Inicializando...\n");

    // ---------- Configuração dos LEDs ----------
    gpio_init(LED_VERDE);
    gpio_set_dir(LED_VERDE, GPIO_OUT);
    gpio_put(LED_VERDE, 0);

    gpio_init(LED_AZUL);
    gpio_set_dir(LED_AZUL, GPIO_OUT);
    gpio_put(LED_AZUL, 0);

    gpio_init(LED_VERMELHO);
    gpio_set_dir(LED_VERMELHO, GPIO_OUT);
    gpio_put(LED_VERMELHO, 0);

    // ---------- Configuração da BUZZER (pino BUZZER1 - 10) ----------
    gpio_set_function(BUZZER1_PIN, GPIO_FUNC_PWM);
    slice_buzzer1 = pwm_gpio_to_slice_num(BUZZER1_PIN);
    pwm_set_wrap(slice_buzzer1, 1000);
    pwm_set_clkdiv(slice_buzzer1, 50.0f);
    pwm_set_chan_level(slice_buzzer1, PWM_CHAN_A, 900);
    pwm_set_enabled(slice_buzzer1, false);

    // ---------- Inicialização do ADC para o joystick ----------
    adc_init();
    adc_gpio_init(27); // Eixo Y
    adc_gpio_init(26); // Eixo X

    // ---------- Inicialização do I2C e do Display OLED ----------
    i2c_init(i2c1, 100 * 1000);
    gpio_set_function(I2C_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA_PIN);
    gpio_pull_up(I2C_SCL_PIN);
    ssd1306_init(&display, i2c1, SSD1306_ADDR, 128, 64);
    ssd1306_clear(&display);
    ssd1306_draw_border(&display, 1);
    ssd1306_show(&display);

    // ---------- Configuração do Botão A ----------
    gpio_init(BUTTON_A);
    gpio_set_dir(BUTTON_A, GPIO_IN);
    gpio_pull_up(BUTTON_A);
    gpio_set_irq_enabled_with_callback(BUTTON_A, GPIO_IRQ_EDGE_FALL, true, &gpio_callback);

    // ---------- Configuração do Botão B (Emergência) ----------
    gpio_init(BUTTON_B);
    gpio_set_dir(BUTTON_B, GPIO_IN);
    gpio_pull_up(BUTTON_B);
    gpio_set_irq_enabled(BUTTON_B, GPIO_IRQ_EDGE_FALL, true);

    last_move_time = get_absolute_time();

    // ---------- Loop Principal ----------
    while (true)
    {
        uint16_t x, y;
        read_joystick(&x, &y);

        // Verifica se houve movimento significativo comparando com os valores anteriores
        if ((abs(x - last_x) > DEADZONE) || (abs(y - last_y) > DEADZONE))
        {
            last_move_time = get_absolute_time();
            stationary_count = 0;
            red_alert_active = false; // Cancela o alerta vermelho se houver movimento
            if (buzzer_active)
            {
                buzzer_active = false;
                pwm_set_enabled(slice_buzzer1, false);
            }
            gpio_put(LED_AZUL, 0);
            gpio_put(LED_VERMELHO, 0);
            ssd1306_draw_border(&display, 1);
            ssd1306_show(&display);
            last_x = x;
            last_y = y;
        }
        else
        {
            stationary_count++;
        }

        // Se o modo emergência estiver ativo, envia a mensagem periodicamente
        if (emergency_active)
        {
            printf("EMERGENCIA - GPS - X:%d Y:%d\n", x, y);
            sleep_ms(1000);
            continue;
        }

        // 1. Checa se o joystick está fora do intervalo seguro [700, 3300]
        if (x < 700 || x > 3300 || y < 700 || y > 3300)
        {
            gpio_put(LED_VERDE, !gpio_get(LED_VERDE));
            gpio_put(LED_VERMELHO, !gpio_get(LED_VERMELHO));
            if (!buzzer_active)
            {
                buzzer_active = true;
                pwm_set_enabled(slice_buzzer1, true);
                beep_on = true;
            }
            else
            {
                beep_on = !beep_on;
                if (beep_on)
                    pwm_set_chan_level(slice_buzzer1, PWM_CHAN_A, 900);
                else
                    pwm_set_chan_level(slice_buzzer1, PWM_CHAN_A, 0);
            }
            sleep_ms(500);
            continue;
        }
        else
        {
            gpio_put(LED_VERDE, 0);
            if (!red_alert_active)
            {
                gpio_put(LED_VERMELHO, 0);
            }
            if (buzzer_active && stationary_count < TIME_BUZZER)
            {
                buzzer_active = false;
                pwm_set_enabled(slice_buzzer1, false);
            }
        }

        // Exibe os valores do joystick via serial
        printf("GPS - X:%d Y:%d\n", x, y);

        // 3. Alertas de inatividade
        if (stationary_count == TIME_BLUE_LED)
        {
            blink_led(LED_AZUL, 10, 500);
            gpio_put(LED_AZUL, 0);
        }

        
        if (stationary_count >= TIME_RED_ALERT)
        {
            red_alert_active = true;
           
            printf("ATENCAO - ");
            display_alert(x, y);
            gpio_put(LED_VERMELHO, !gpio_get(LED_VERMELHO));
        }

        if (stationary_count >= TIME_BUZZER)
        {
            if (!buzzer_active)
            {
                buzzer_active = true;
                pwm_set_enabled(slice_buzzer1, true);
                beep_on = true;
            }
        }

        int delay_ms = 2000;
        if (buzzer_active)
        {
            beep_on = !beep_on;
            if (beep_on)
                pwm_set_chan_level(slice_buzzer1, PWM_CHAN_A, 900);
            else
                pwm_set_chan_level(slice_buzzer1, PWM_CHAN_A, 0);
            delay_ms = 500;
        }

        if (red_alert_active && !buzzer_active)
        {
            delay_ms = 500;
        }

        sleep_ms(delay_ms);
    }

    return 0;
}
