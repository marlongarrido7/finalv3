
# Digital Badge (DB4.0)

## Simulador Utilizando a placa BitDogLab com o microcontrolador Raspberry Pico W 2040

---

## Contexto e Objetivos

O projeto **Digital Badge (DB4.0)** tem como objetivo desenvolver um crachá digital que ofereça as seguintes funcionalidades:

- **Localização em grandes áreas:** Monitoramento contínuo da posição.
- **Controles de acesso:** Verificação e alertas a areas restritas.
- **Mapas de movimentação:** Rastreamento dos trajetos percorridos.
- **Sistemas de emergência:** Alertas e mensagens imediatas em caso de inatividade ou situações críticas.

Esta solução foi idealizada para melhorar a segurança e o gerenciamento de espaços amplos, utilizando tecnologias embarcadas no microcontrolador Raspberry Pico W 2040 integrado à placa BitDogLab.

---

## Descrição do Projeto

Este simulador demonstra como um crachá digital pode ser implementado utilizando diversas funcionalidades do RP2040. Entre as principais tecnologias e conceitos explorados, estão:

- **Leitura Analógica:** Utiliza o ADC para captar os movimentos do joystick, que indicam a localização do dispositivo.
- **Controle PWM:** Ajusta a intensidade dos sinais enviados para LEDs e para o buzzer, gerando alertas visuais e sonoros.
- **Comunicação I2C:** Gerencia a interface com um display OLED (SSD1306) para exibir informações de status e alertas.
- **Interrupções e Tratamento de Botões:** Permite o reset de alertas e a ativação do modo emergência, melhorando o controle do sistema.

---

## Funcionalidades Principais

- **Monitoramento do Joystick:**
  - Leitura dos eixos X e Y utilizando o ADC.
  - Verificação de movimentação com base em uma *zona morta* definida (`DEADZONE`) para filtrar leituras ruidosas.
  - Atualização da posição registrada e reinicialização de contadores e alertas quando há movimento.

- **Alertas e Controles de Acesso:**
  - **Alertas Visuais:**
    - **LED Verde:** Indica que o dispositivo está fora do intervalo seguro.
    - **LED Azul:** Pisca após 30 segundos de inatividade.
    - **LED Vermelho:** Acionado após 45 segundos de inatividade, exibindo a mensagem “ATENCAO” no display OLED.
  - **Alerta Sonoro:**
    - **Buzzer:** Emite sinais intermitentes após 60 segundos de inatividade.
  - **Modo Emergência:**
    - Ativado/desativado via Botão B (duas pressões consecutivas para ativar; três para desativar).
    - Quando ativo, o sistema envia periodicamente a mensagem “EMERGENCIA - GPS - X:Y” enquanto o dispositivo permanecer inativo.

- **Exibição no Display OLED:**
  - Utiliza a biblioteca `ssd1306.h` para desenhar bordas e exibir mensagens.
  - Exibe alertas e a localização atual (valores dos eixos do joystick) no formato “ATENCAO” ou “GPS - X:Y”.

- **Controle do Tempo:**
  - O intervalo entre as leituras e os envios de mensagens é controlado dinamicamente via `sleep_ms(delay_ms)`.
  - O valor do delay varia de acordo com o estado do sistema (normal, alerta, ou emergência).

---

## Diagrama Simplificado de Conexões

```
+------------------------------------------------------------------+
|         Placa BitDogLab / Pico W 2040                            |
|                                                                  |
| GPIO 11  -----------------------> LED Verde (Alerta fora do      |
|                                     intervalo seguro)            |
| GPIO 12  -----------------------> LED Azul (Inatividade)         |
| GPIO 13  -----------------------> LED Vermelho (Alerta visual)   |
| ADC (canais 1 e 0) -----------> Leitura dos eixos X e Y do       |
|                                     Joystick                     |
| GPIO 5   -----------------------> Botão A (Reset de alertas)     |
| GPIO 6   -----------------------> Botão B (Modo Emergência)      |
| GPIO 14  -----------------------> SDA (Display OLED via I2C)     |
| GPIO 15  -----------------------> SCL (Display OLED via I2C)     |
| GPIO 10  -----------------------> BUZZER (Alerta Sonoro)         |
| ADC GPIO 26 ------------------> Inicialização para o eixo X      |
| ADC GPIO 27 ------------------> Inicialização para o eixo Y      |
+------------------------------------------------------------------+
```

---

## Bibliotecas Utilizadas

- **`<stdio.h>`**  
  Funções básicas de entrada e saída.

- **`pico/stdlib.h`**  
  Funções essenciais para o funcionamento do Raspberry Pico (inicialização, temporização, etc).

- **`hardware/adc.h`**  
  Configuração e leitura do conversor analógico-digital (ADC) para captar os valores do joystick.

- **`hardware/pwm.h`**  
  Controle PWM para modulação dos sinais enviados aos LEDs e ao buzzer.

- **`hardware/i2c.h`**  
  Gerenciamento da comunicação via I2C com o display OLED.

- **`hardware/gpio.h`**  
  Configuração dos pinos de entrada e saída, utilizados para LEDs, botões e buzzer.

- **`pico/time.h`**  
  Funções para medição e controle do tempo, essenciais para contadores e delays.

- **`ssd1306.h`**  
  Biblioteca para manipulação gráfica do display OLED, permitindo desenhar bordas, textos e formas.

- **`fonte.h` e `<stdlib.h>`**  
  Inclusões adicionais para manipulação de fontes e funções utilitárias padrão da linguagem C.

---

## Detalhamento do Código

### Inicialização e Configuração

- **Setup dos LEDs:**  
  Cada LED é configurado como saída e inicializado com estado baixo:
  - **LED Amarelo:** Indica alerta quando os valores do joystick estão fora do intervalo seguro.
  - **LED Azul:** Pisca após 30 segundos de inatividade.
  - **LED Vermelho:** Sinaliza alerta visual após 45 segundos de inatividade e se mantem acesa ate que o botão A seja precionado.

- **Configuração do Buzzer:**  
  O buzzer utiliza PWM para emitir sinais intermitentes, sendo configurado com wrap, divisor de clock e canal de saída, e inicia desabilitado.

- **Inicialização do ADC:**  
  Os canais ADC são configurados para os eixos X e Y do joystick:
  ```c
  adc_gpio_init(27); // Eixo Y
  adc_gpio_init(26); // Eixo X
  ```

- **Setup do Display OLED:**  
  A comunicação via I2C é configurada (pinos SDA e SCL) e o display é inicializado, limpo e preparado para exibição.

- **Configuração dos Botões:**  
  - **Botão A:** Reseta os alertas e contadores.
  - **Botão B:** Controla o modo emergência (duas pressões para ativar e três para desativar).

### Loop Principal

Dentro do loop principal, o sistema realiza as seguintes operações:

1. **Leitura do Joystick:**
   - Os valores dos eixos X e Y são lidos via ADC.
   - Se o movimento for significativo (considerando a zona morta), os contadores e alertas são resetados.

2. **Modo Emergência:**
   - Quando ativado, o sistema envia periodicamente a mensagem:
     ```c
     printf("EMERGENCIA - GPS - X:%d Y:%d\n", x, y);
     ```
     com um delay de 1000 ms, até que o modo seja desativado.

3. **Verificação dos Limites do Joystick:**
   - Se os valores estiverem fora do intervalo seguro ([700, 3300]), o sistema alterna os estados dos LEDs e aciona o buzzer com PWM, utilizando um delay de 500 ms.

4. **Exibição dos Dados "GPS":**
   - Em condições normais, os valores dos eixos X e Y são exibidos via serial:
     ```c
     printf("GPS - X:%d Y:%d\n", x, y);
     ```

5. **Alertas por Inatividade:**
   - **Após 30 segundos:** O LED azul pisca 10 vezes.
   - **Após 45 segundos:** O alerta vermelho é ativado e a mensagem “ATENCAO” é exibida no display OLED com os valores do joystick.
   - **Após 60 segundos:** O buzzer é ativado para emitir alertas sonoros.

6. **Controle do Tempo:**
   - O delay entre cada iteração do loop é ajustado dinamicamente com base nos estados dos alertas, através da função `sleep_ms(delay_ms)`.

---

## Como Executar o Projeto

### Pré-requisitos

- **Raspberry Pico SDK:** Certifique-se de que o SDK esteja instalado e configurado.
- **Dependências:** Instale as bibliotecas necessárias para I2C, ADC, PWM e para o display OLED.

### Passos para Compilação e Transferência

1. **Clone o Repositório:**
   ```bash
   git clone https://github.com/seu-usuario/DigitalBadgeDB4.0.git
   cd DigitalBadgeDB4.0
   ```

2. **Crie o Diretório de Build e Compile:**
   ```bash
   mkdir build
   cd build
   cmake ..
   make
   ```

3. **Transfira o Firmware:**
   - Conecte a placa Raspberry Pico W 2040 ao computador.
   - Copie o arquivo `.uf2` gerado para o volume USB da placa.

---

## Referências

- **Raspberry Pi Pico SDK:**  
  [Documentação oficial](https://www.raspberrypi.org/documentation/pico-sdk/)

- **Display OLED SSD1306:**  
  [Repositório da biblioteca SSD1306](https://github.com/adafruit/Adafruit_SSD1306)

---

## Contato

**Dr. Marlon da Silva Garrido**  
Professor Associado IV (CENAMB - PPGEA)  
Universidade Federal do Vale do São Francisco (UNIVASF)  
📧 Email: marlon.garrido@univasf.edu.br  
🌐 [UNIVASF](https://www.univasf.edu.br)

---

