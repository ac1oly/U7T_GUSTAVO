#include <stdio.h>
#include "pico/stdlib.h"
#include "inc/ssd1306.h"
#include "inc/font.h"
#include "hardware/pio.h"
#include "hardware/clocks.h"
#include "hardware/i2c.h"

// Definição dos pinos dos LEDs do semáforo de carros (LED RGB SMD5050)
#define LED_VERDE 11
#define LED_AZUL 12
#define LED_VERMELHO 13

// Definição do pino do sensor de proximidade (botão)
#define BOTAO 5

// Definição dos pinos I2C e endereço do display OLED
#define I2C_PORT i2c1
#define I2C_SDA 14
#define I2C_SCL 15
#define ENDERECO 0x3C

// Variáveis para ajustar tempos do semáforo
#define TEMPO_VERDE 8000    // Tempo do sinal verde para carros (normal)
#define TEMPO_AMARELO 3000  // Tempo do sinal amarelo para carros
#define TEMPO_VERMELHO 8000 // Tempo do vermelho para pedestres

const int TEMPO_EXTRA_VERDE = 1500; // Tempo extra do verde quando o pedestre é detectado

// ✅ Declaração da variável para o display OLED
ssd1306_t ssd;

// ✅ Declaração do protótipo da função mudar_para_pedestre()
void mudar_para_pedestre();

void configurar_gpio()
{
    gpio_init(LED_VERDE);
    gpio_init(LED_AZUL);
    gpio_init(LED_VERMELHO);
    gpio_set_dir(LED_VERDE, GPIO_OUT);
    gpio_set_dir(LED_AZUL, GPIO_OUT);
    gpio_set_dir(LED_VERMELHO, GPIO_OUT);

    gpio_init(BOTAO);
    gpio_set_dir(BOTAO, GPIO_IN);
    gpio_pull_up(BOTAO); // Altere para pull_up se o botão estiver conectado ao GND
}

volatile bool botao_pressionado = false;
uint32_t last_time = 0;

void gpio_irq_handler(uint gpio, uint32_t events)
{
    uint32_t current_time = to_us_since_boot(get_absolute_time());

    // Debounce: ignora leituras se for muito rápido
    if (current_time - last_time > 200000)
    {
        if (gpio == BOTAO)
        {
            botao_pressionado = true;
        }
    }
    last_time = current_time;
}

void atualizar_display_semaforo(const char *mensagem)
{
    ssd1306_fill(&ssd, false);
    ssd1306_send_data(&ssd);
    ssd1306_draw_string(&ssd, mensagem, 10, 30);
    ssd1306_send_data(&ssd);
}

void esperar_com_interrupcao(int tempo_total)
{
    int tempo_passado = 0;
    botao_pressionado = false;

    while (tempo_passado < tempo_total)
    {
        if (botao_pressionado)
        {                              // Se a interrupção detectou o botão
            botao_pressionado = false; // Reseta a flag para próxima leitura
            sleep_ms(TEMPO_EXTRA_VERDE);
            mudar_para_pedestre();
            return;
        }
        sleep_ms(100);
        tempo_passado += 100;
    }
}

void mudar_para_pedestre()
{
    atualizar_display_semaforo("Pedestre          Detectado");
    sleep_ms(3000);
}

int main()
{
    stdio_init_all();
    configurar_gpio();

    i2c_init(I2C_PORT, 400 * 1000);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);
    gpio_set_irq_enabled_with_callback(BOTAO, GPIO_IRQ_EDGE_RISE, true, &gpio_irq_handler);

    ssd1306_init(&ssd, WIDTH, HEIGHT, false, ENDERECO, I2C_PORT);
    ssd1306_config(&ssd);
    ssd1306_send_data(&ssd);

    ssd1306_fill(&ssd, false);
    ssd1306_send_data(&ssd);

    while (1)
    {
        gpio_put(LED_VERDE, 1);
        gpio_put(LED_VERMELHO, 0);
        atualizar_display_semaforo("Semaforo Verde");

        esperar_com_interrupcao(TEMPO_VERDE);

        gpio_put(LED_VERDE, 1);
        gpio_put(LED_VERMELHO, 1);
        atualizar_display_semaforo("Semaforo Amarelo");

        sleep_ms(TEMPO_AMARELO);

        gpio_put(LED_VERDE, 0);
        gpio_put(LED_VERMELHO, 1);
        atualizar_display_semaforo("Semaforo Vermelho");
        sleep_ms(TEMPO_VERMELHO);

        gpio_put(LED_VERMELHO, 0);
        gpio_put(LED_VERDE, 1);
    }
}
