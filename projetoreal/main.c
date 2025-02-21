#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "pico/stdlib.h"
#include "hardware/uart.h"
#include "hardware/i2c.h"
#include "hardware/gpio.h"
#include "hardware/pwm.h"
#include "pico/time.h"

#include "ssd1306.h"
#include "fonte.h"
#include "gps.h"
#include "lora.h"
#include "mpu6050.h"

// Definições de pinos (ajuste conforme sua montagem)
#define LED_BLUE    16
#define LED_RED     17
#define LED_GREEN   18
#define BUZZER_PIN  19
#define BUTTON_A    20
#define BUTTON_B    21

// Pinos para I2C (Display SSD1306 e MPU6050)
#define I2C_SDA     22
#define I2C_SCL     23

// Configuração da UART para o módulo GPS (UART0)
#define GPS_UART    uart0
#define GPS_TX_PIN  0
#define GPS_RX_PIN  1
#define GPS_BAUD    9600

// Configuração da UART para o módulo LoRa (UART1)
#define LORA_UART    uart1
#define LORA_TX_PIN  2
#define LORA_RX_PIN  3
#define LORA_BAUD    9600

// Tempos (em milissegundos)
#define NO_MOVEMENT_5_MIN   (5 * 60 * 1000)
#define NO_MOVEMENT_10_MIN  (10 * 60 * 1000)
#define NO_MOVEMENT_15_MIN  (15 * 60 * 1000)
#define LORA_TX_INTERVAL    (2 * 60 * 1000)

volatile absolute_time_t last_movement_time;
volatile absolute_time_t last_lora_tx_time;
volatile bool buzzer_active = false;

// Prototipação das funções de tratamento dos botões
void button_a_handler(uint gpio, uint32_t events);
void button_b_handler(uint gpio, uint32_t events);

int main() {
    stdio_init_all();
    
    // Configuração dos LEDs e Buzzer
    gpio_init(LED_BLUE);   gpio_set_dir(LED_BLUE, GPIO_OUT);   gpio_put(LED_BLUE, 0);
    gpio_init(LED_RED);    gpio_set_dir(LED_RED, GPIO_OUT);    gpio_put(LED_RED, 0);
    gpio_init(LED_GREEN);  gpio_set_dir(LED_GREEN, GPIO_OUT);  gpio_put(LED_GREEN, 0);
    gpio_init(BUZZER_PIN); gpio_set_dir(BUZZER_PIN, GPIO_OUT); gpio_put(BUZZER_PIN, 0);
    
    // Configuração dos botões (com pull-up)
    gpio_init(BUTTON_A);   gpio_set_dir(BUTTON_A, GPIO_IN);   gpio_pull_up(BUTTON_A);
    gpio_init(BUTTON_B);   gpio_set_dir(BUTTON_B, GPIO_IN);   gpio_pull_up(BUTTON_B);
    
    // Habilita interrupção para o Botão A (usando callback)
    gpio_set_irq_enabled_with_callback(BUTTON_A, GPIO_IRQ_EDGE_FALL, true, &button_a_handler);
    // Para o Botão B usaremos polling neste exemplo (poderia ser via IRQ também)
    
    // Inicialização do I2C (para SSD1306 e MPU6050)
    i2c_init(i2c0, 100 * 1000);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);
    
    // Inicializa o display OLED
    ssd1306_t display;
    ssd1306_init(&display, i2c0, 0x3C, 128, 64);
    ssd1306_clear(&display);
    ssd1306_show(&display);
    
    // Inicializa o MPU6050
    mpu6050_t mpu;
    if (!mpu6050_init(&mpu, i2c0, 0x68)) {
        printf("Erro ao inicializar MPU6050!\n");
    }
    
    // Inicializa a UART para o módulo GPS
    uart_init(GPS_UART, GPS_BAUD);
    gpio_set_function(GPS_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(GPS_RX_PIN, GPIO_FUNC_UART);
    
    // Inicializa a UART para o módulo LoRa
    uart_init(LORA_UART, LORA_BAUD);
    gpio_set_function(LORA_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(LORA_RX_PIN, GPIO_FUNC_UART);
    
    // Inicializa variáveis de tempo
    last_movement_time = get_absolute_time();
    last_lora_tx_time = get_absolute_time();
    
    char gps_data[100] = {0};
    char lora_message[150] = {0};
    
    while (true) {
        // --- Leitura do módulo GPS ---
        if(gps_read(GPS_UART, gps_data, sizeof(gps_data))) {
            // gps_data contém dados recebidos (pode ser uma sentença NMEA)
        }
        
        // --- Leitura do acelerômetro (MPU6050) ---
        float ax, ay, az;
        if (mpu6050_read_accel(&mpu, &ax, &ay, &az)) {
            // Considera movimento se qualquer aceleração ultrapassar um limiar
            float movement_threshold = 0.1f; // ajuste conforme necessário
            bool movement_detected = (fabs(ax) > movement_threshold || fabs(ay) > movement_threshold || fabs(az) > movement_threshold);
            if (movement_detected) {
                last_movement_time = get_absolute_time();
                // Desliga alertas visuais e sonoros
                gpio_put(LED_BLUE, 0);
                gpio_put(LED_RED, 0);
                gpio_put(BUZZER_PIN, 0);
                buzzer_active = false;
                // Limpa mensagem de alerta no display
                ssd1306_clear(&display);
                ssd1306_show(&display);
            }
        }
        
        // --- Verificação do tempo de inatividade ---
        int64_t elapsed = absolute_time_diff_us(last_movement_time, get_absolute_time()) / 1000; // em ms
        
        // Se inativo por 5 minutos, pisca LED azul por 30 s
        if (elapsed >= NO_MOVEMENT_5_MIN && elapsed < NO_MOVEMENT_5_MIN + 30000) {
            gpio_put(LED_BLUE, 1);
            sleep_ms(500);
            gpio_put(LED_BLUE, 0);
            sleep_ms(500);
        }
        
        // Se inativo por 10 minutos, pisca LED vermelho e exibe "ATENCAO" por 30 s
        if (elapsed >= NO_MOVEMENT_10_MIN && elapsed < NO_MOVEMENT_10_MIN + 30000) {
            gpio_put(LED_RED, 1);
            ssd1306_draw_string(&display, 0, 0, "ATENCAO");
            ssd1306_show(&display);
            sleep_ms(500);
            gpio_put(LED_RED, 0);
            sleep_ms(500);
        }
        
        // Se inativo por 15 minutos, ativa buzzer até o Botão A ser pressionado
        if (elapsed >= NO_MOVEMENT_15_MIN && !buzzer_active) {
            buzzer_active = true;
            gpio_put(BUZZER_PIN, 1);
        }
        
        // --- Transmissão via LoRa a cada 2 minutos ---
        int64_t lora_elapsed = absolute_time_diff_us(last_lora_tx_time, get_absolute_time()) / 1000;
        if (lora_elapsed >= LORA_TX_INTERVAL) {
            snprintf(lora_message, sizeof(lora_message), "GPS: %s", gps_data);
            lora_send(LORA_UART, lora_message);
            last_lora_tx_time = get_absolute_time();
        }
        
        // --- Verificação do Botão B para EMERGENCIA ---
        if (!gpio_get(BUTTON_B)) {
            sleep_ms(50); // debounce
            if (!gpio_get(BUTTON_B)) {
                printf("EMERGENCIA: %s\n", gps_data);
                snprintf(lora_message, sizeof(lora_message), "EMERGENCIA: %s", gps_data);
                lora_send(LORA_UART, lora_message);
                // Aguarda liberação do botão para evitar múltiplos disparos
                while (!gpio_get(BUTTON_B)) { sleep_ms(50); }
            }
        }
        
        sleep_ms(200); // Delay do loop principal
    }
    
    return 0;
}

void button_a_handler(uint gpio, uint32_t events) {
    // Ao pressionar o Botão A, desativa o buzzer e reseta os alertas
    gpio_put(BUZZER_PIN, 0);
    buzzer_active = false;
    last_movement_time = get_absolute_time();
    printf("Botao A pressionado: alertas reiniciados.\n");
}
