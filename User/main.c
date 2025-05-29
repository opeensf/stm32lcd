#include "stm32f10x.h"

#include "./lcd/bsp_ili9341_lcd.h"

#include "./font/fonts.h"
#include <stdio.h>
#include <string.h>

// --- 全局宏定义 ---
#define KEY1_PIN        GPIO_Pin_0
#define KEY1_PORT       GPIOA
#define KEY1_CLK        RCC_APB2Periph_GPIOA

#define KEY2_PIN        GPIO_Pin_13
#define KEY2_PORT       GPIOC
#define KEY2_CLK        RCC_APB2Periph_GPIOC

#define ADC_PIN         GPIO_Pin_1    // 例如 PA1
#define ADC_PORT        GPIOA
#define ADC_CLK         RCC_APB2Periph_GPIOA
#define ADC_CHANNEL     ADC_Channel_1 // 对应 PA1

#define UART_BUFFER_SIZE 128
#define CASE3_TOP_MARGIN 35 // 为案例3顶部留出更多空间，避免logo遮挡

// --- 全局变量 ---
// 显示控制
volatile uint8_t g_current_display_case = 1; 
const uint8_t MAX_DISPLAY_CASES = 5; 
volatile uint8_t g_display_operation_mode = 0; 

// 按键状态
volatile uint8_t g_key1_action_pending = 0; 
volatile uint8_t g_key2_action_pending = 0; 
uint8_t g_key1_last_state = 1; 
uint8_t g_key2_last_state = 1; 
uint32_t g_key_debounce_delay = 50; 

// ADC 相关
volatile uint16_t g_adc_value = 0;
volatile uint8_t g_animation_speed_level = 0; 
const uint32_t ANIMATION_SPEEDS[] = {100, 60, 30}; 

// UART 相关
char g_uart_tx_buffer[UART_BUFFER_SIZE];

// 案例2 ("I ? CSU") 动画相关
int16_t g_case2_text_x_pos;
int16_t g_case2_text_x_pos_old; 
const char CASE2_TEXT[] = "I <3 CSU"; 
uint16_t g_case2_text_pixel_width; 
uint32_t g_case2_last_move_time_ms = 0; 
uint16_t g_case2_text_y_pos = 0; 

// 案例3 (学术报告) 相关
const char* REPORT_TITLE = "Yuri Shardt 教授学术报告";
const char* REPORT_LINE1_L1 = "题目: Artificial";
const char* REPORT_LINE1_L2 = "  Intelligence and";
const char* REPORT_LINE1_L3 = "  Modelling: What Can Be";
const char* REPORT_LINE1_L4 = "  Done"; 
const char* REPORT_LINE2_L1 = "时间: 2025年2月28日(周二)";
const char* REPORT_LINE2_L2 = "  上午9:30开始";
const char* REPORT_LINE3_L1 = "地点: 中南大学岳麓山校区";
const char* REPORT_LINE3_L2 = "  升华后楼504会议室";
const char* REPORT_LINE4_L1 = "报告人: Yuri Shardt教授,";
const char* REPORT_LINE4_L2 = "  德国 Technical University";
const char* REPORT_LINE4_L3 = "  of Ilmenau";
const char* REPORT_ABSTRACT_TITLE = "报告摘要：";
const char* REPORT_ABSTRACT_P1_L1 = "As the ubiquity of";
const char* REPORT_ABSTRACT_P1_L2 = "artificial intelligence";
const char* REPORT_ABSTRACT_P1_L3 = "increases, there is a need";
const char* REPORT_ABSTRACT_P1_L4 = "to provide a sober and";
const char* REPORT_ABSTRACT_P1_L5 = "realistic assessment of its"; 
const char* REPORT_ABSTRACT_P2_L1 = "capabilities and future";
const char* REPORT_ABSTRACT_P2_L2 = "potential. In this";
const char* REPORT_ABSTRACT_P2_L3 = "presentation, the key";
const char* REPORT_ABSTRACT_P2_L4 = "terms will be examined,"; 
const char* REPORT_ABSTRACT_P3_L1 = "some applications";       
const char* REPORT_ABSTRACT_P3_L2 = "considered, and a";
const char* REPORT_ABSTRACT_P3_L3 = "perspective on the future";
const char* REPORT_ABSTRACT_P3_L4 = "of artificial"; 
const char* REPORT_ABSTRACT_P4_L1 = "intelligence and";
const char* REPORT_ABSTRACT_P4_L2 = "modelling presented. The";
const char* REPORT_ABSTRACT_P4_L3 = "primary focus will be on"; 
const char* REPORT_ABSTRACT_P5_L1 = "applications to chemical";
const char* REPORT_ABSTRACT_P5_L2 = "and process engineering.";

uint8_t g_report_page = 0;
const uint8_t MAX_REPORT_PAGES = 2; 
uint8_t g_case3_needs_redraw = 1; 

// 案例5 (收缩矩形) 相关
uint16_t g_case5_rect_x = 0;
uint16_t g_case5_rect_y = 0;
uint16_t g_case5_rect_width = 0;
uint16_t g_case5_rect_height = 0;
uint8_t  g_case5_color_index = 0;
uint32_t g_case5_last_draw_time_ms = 0;
uint8_t  g_case5_animation_done = 0;
const uint16_t CASE5_COLORS[] = {RED, GREEN, BLUE, YELLOW, MAGENTA, CYAN};
const uint8_t CASE5_NUM_COLORS = sizeof(CASE5_COLORS) / sizeof(CASE5_COLORS[0]);

volatile uint32_t g_systick_ms_count = 0;

// --- 函数声明 ---
void SysTick_Init(void);
uint32_t Get_System_ms(void);
void GPIO_Config(void);
void ADC_Config(void);
void UART_Config(void);
void UART_SendString(const char* str);
void UART_SendChar(char ch); 
void Process_Keys(void);
void Update_ADC_And_Speed(void);
void Init_Display_Case(uint8_t case_num);
void Display_Case1(void);
void Display_Case2_Update(void);
void Display_Case3(void);
void Display_Case4(void);
void Display_Case5_Update(void);

void SysTick_Init(void) {
    if (SysTick_Config(SystemCoreClock / 1000)) {
        while (1);
    }
}

void SysTick_Handler(void) {
    g_systick_ms_count++;
}

uint32_t Get_System_ms(void) {
    return g_systick_ms_count;
}

void GPIO_Config(void) {
    GPIO_InitTypeDef GPIO_InitStructure;
    RCC_APB2PeriphClockCmd(KEY1_CLK | KEY2_CLK | ADC_CLK | RCC_APB2Periph_AFIO, ENABLE);
    GPIO_InitStructure.GPIO_Pin = KEY1_PIN; GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU; GPIO_Init(KEY1_PORT, &GPIO_InitStructure);
    GPIO_InitStructure.GPIO_Pin = KEY2_PIN; GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU; GPIO_Init(KEY2_PORT, &GPIO_InitStructure);
    GPIO_InitStructure.GPIO_Pin = ADC_PIN; GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN; GPIO_Init(ADC_PORT, &GPIO_InitStructure);
}

void ADC_Config(void) {
    ADC_InitTypeDef ADC_InitStructure;
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE); RCC_ADCCLKConfig(RCC_PCLK2_Div6);
    ADC_DeInit(ADC1);
    ADC_InitStructure.ADC_Mode = ADC_Mode_Independent; ADC_InitStructure.ADC_ScanConvMode = DISABLE;
    ADC_InitStructure.ADC_ContinuousConvMode = DISABLE; ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None;
    ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right; ADC_InitStructure.ADC_NbrOfChannel = 1;
    ADC_Init(ADC1, &ADC_InitStructure);
    ADC_Cmd(ADC1, ENABLE);
    ADC_ResetCalibration(ADC1); while (ADC_GetResetCalibrationStatus(ADC1));
    ADC_StartCalibration(ADC1); while (ADC_GetCalibrationStatus(ADC1));
}

uint16_t Get_ADC_Value(uint8_t channel) {
    ADC_RegularChannelConfig(ADC1, channel, 1, ADC_SampleTime_55Cycles5);
    ADC_SoftwareStartConvCmd(ADC1, ENABLE);
    while (ADC_GetFlagStatus(ADC1, ADC_FLAG_EOC) == RESET);
    return ADC_GetConversionValue(ADC1);
}

void UART_Config(void) {
    GPIO_InitTypeDef GPIO_InitStructure; USART_InitTypeDef USART_InitStructure;
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_USART1, ENABLE); 
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9; GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP; GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz; GPIO_Init(GPIOA, &GPIO_InitStructure);
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10; GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING; GPIO_Init(GPIOA, &GPIO_InitStructure);
    USART_InitStructure.USART_BaudRate = 115200; USART_InitStructure.USART_WordLength = USART_WordLength_8b;
    USART_InitStructure.USART_StopBits = USART_StopBits_1; USART_InitStructure.USART_Parity = USART_Parity_No;
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None; USART_InitStructure.USART_Mode = USART_Mode_Tx | USART_Mode_Rx;
    USART_Init(USART1, &USART_InitStructure); USART_Cmd(USART1, ENABLE);
}

void UART_SendChar(char ch) {
    while (USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET);
    USART_SendData(USART1, (uint16_t)ch);
    while (USART_GetFlagStatus(USART1, USART_FLAG_TC) == RESET);
}

void UART_SendString(const char* str) {
    while (*str) UART_SendChar(*str++);
}

void Process_Keys(void) {
    uint8_t key1_current = GPIO_ReadInputDataBit(KEY1_PORT, KEY1_PIN);
    uint8_t key2_current = GPIO_ReadInputDataBit(KEY2_PORT, KEY2_PIN);
    static uint32_t key1_press_time = 0;
    static uint32_t key2_press_time = 0;

    if (key1_current == Bit_RESET && g_key1_last_state == Bit_SET) { 
        key1_press_time = Get_System_ms();
    } else if (key1_current == Bit_SET && g_key1_last_state == Bit_RESET) { 
        if ((Get_System_ms() - key1_press_time) > g_key_debounce_delay) { 
            g_key1_action_pending = 1; 
        }
    }
    g_key1_last_state = key1_current;

    if (key2_current == Bit_RESET && g_key2_last_state == Bit_SET) { 
        key2_press_time = Get_System_ms();
    } else if (key2_current == Bit_SET && g_key2_last_state == Bit_RESET) { 
        if ((Get_System_ms() - key2_press_time) > g_key_debounce_delay) {
            g_key2_action_pending = 1;
        }
    }
    g_key2_last_state = key2_current;
}

void Update_ADC_And_Speed(void) {
    float voltage; 
    g_adc_value = Get_ADC_Value(ADC_CHANNEL); 
    voltage = (g_adc_value / 4095.0f) * 3.3f; 
    if (voltage < 1.1f) g_animation_speed_level = 0; 
    else if (voltage < 2.2f) g_animation_speed_level = 1; 
    else g_animation_speed_level = 2; 
    if (g_current_display_case == 2) {
        sprintf(g_uart_tx_buffer, "ADC: %d (%.2fV), Speed Idx: %d (%lu ms)\r\n",
                g_adc_value, voltage, g_animation_speed_level, (unsigned long)ANIMATION_SPEEDS[g_animation_speed_level]);
        UART_SendString(g_uart_tx_buffer);
    }
}

void Init_Display_Case(uint8_t case_num) {
    int i; 
    ILI9341_Clear(0, 0, LCD_X_LENGTH, LCD_Y_LENGTH); 
    LCD_SetColors(BLACK, WHITE); 
    g_key1_action_pending = 0; 

    switch (case_num) {
        case 1:
            Display_Case1(); 
            break;
        case 2:
            LCD_SetFont(&Font24x32); 
            g_case2_text_pixel_width = 0; 
            for(i=0; CASE2_TEXT[i] != '\0'; ++i) { 
                g_case2_text_pixel_width += Font24x32.Width; 
            }
            g_case2_text_x_pos = LCD_X_LENGTH; 
            g_case2_text_x_pos_old = g_case2_text_x_pos; 
            g_case2_last_move_time_ms = Get_System_ms();
            g_case2_text_y_pos = LCD_Y_LENGTH / 2 - Font24x32.Height / 2; 
            break;
        case 3:
            g_report_page = 0; 
            g_case3_needs_redraw = 1; 
            break;
        case 4:
            Display_Case4(); 
            break;
        case 5:
            g_case5_rect_x = 0; g_case5_rect_y = 0;
            g_case5_rect_width = LCD_X_LENGTH; g_case5_rect_height = LCD_Y_LENGTH;
            g_case5_color_index = 0; g_case5_last_draw_time_ms = Get_System_ms();
            g_case5_animation_done = 0;
            break;
    }
}

void Display_Case1(void) {
    uint16_t y_pos = 30; 
    const uint16_t line_spacing_case1 = 20; 
    LCD_SetColors(BLACK, WHITE);
    LCD_SetFont(&Font24x32); 
    ILI9341_DispString_EN_CH(10, y_pos, (char *)"自动化学院");
    y_pos += Font24x32.Height + line_spacing_case1;
    LCD_SetFont(&Font16x24); 
    ILI9341_DispString_EN_CH(10, y_pos, (char *)"电子信息2301班"); 
    y_pos += Font16x24.Height + line_spacing_case1;
    ILI9341_DispString_EN(10, y_pos, "820723xxxx"); 
    y_pos += Font16x24.Height + line_spacing_case1;
    ILI9341_DispString_EN_CH(10, y_pos, (char *)"您的姓名"); 
}

void Display_Case2_Update(void) {
    uint16_t current_char_x_new, current_char_x_old;
    int i;
    uint16_t prev_text_color, prev_back_color; 

    if ((Get_System_ms() - g_case2_last_move_time_ms) >= ANIMATION_SPEEDS[g_animation_speed_level]) {
        g_case2_last_move_time_ms = Get_System_ms();

        LCD_GetColors(&prev_text_color, &prev_back_color); 
        LCD_SetFont(&Font24x32); 

        LCD_SetTextColor(prev_back_color); 
        current_char_x_old = g_case2_text_x_pos_old;
        for (i = 0; CASE2_TEXT[i] != '\0'; ++i) {
            if (current_char_x_old < LCD_X_LENGTH && (current_char_x_old + Font24x32.Width) > 0) {
                ILI9341_DispChar_EN(current_char_x_old, g_case2_text_y_pos, CASE2_TEXT[i]);
            }
            current_char_x_old += Font24x32.Width;
        }

        g_case2_text_x_pos_old = g_case2_text_x_pos; 
        g_case2_text_x_pos -= 5; 

        LCD_SetTextColor(RED); 
        current_char_x_new = g_case2_text_x_pos;
        for (i = 0; CASE2_TEXT[i] != '\0'; ++i) {
            if (current_char_x_new < LCD_X_LENGTH && (current_char_x_new + Font24x32.Width) > 0) {
                ILI9341_DispChar_EN(current_char_x_new, g_case2_text_y_pos, CASE2_TEXT[i]);
            }
            current_char_x_new += Font24x32.Width;
        }
        
        LCD_SetTextColor(prev_text_color); 

        if (g_case2_text_x_pos < -(int16_t)g_case2_text_pixel_width) {
            g_case2_text_x_pos = LCD_X_LENGTH;
            g_case2_text_x_pos_old = g_case2_text_x_pos; 
        }
    }
}

void Display_Case3(void) {
    uint16_t y_pos; 
    const uint16_t base_line_height = 16; 
    const uint16_t indent = 5;
    const uint16_t spacing = 2; 
    char page_info[35]; 

    if (g_key1_action_pending && g_current_display_case == 3) { 
        g_report_page = (g_report_page + 1) % MAX_REPORT_PAGES;
        g_case3_needs_redraw = 1; 
        g_key1_action_pending = 0; 
        sprintf(g_uart_tx_buffer, "Report Page: %d/%d\r\n", g_report_page + 1, MAX_REPORT_PAGES);
        UART_SendString(g_uart_tx_buffer);
    }

    if (!g_case3_needs_redraw && g_current_display_case == 3) { 
        return; 
    }
    
    if(g_current_display_case == 3) { 
        ILI9341_Clear(0, 0, LCD_X_LENGTH, LCD_Y_LENGTH); 
        LCD_SetColors(BLACK, WHITE);
        y_pos = CASE3_TOP_MARGIN; // 使用宏定义顶部边距

        if (g_report_page == 0) {
            LCD_SetFont(&Font16x24); 
            ILI9341_DispString_EN_CH(indent, y_pos, (char*)REPORT_TITLE);
            y_pos += Font16x24.Height + spacing * 2; 

            LCD_SetFont(&Font8x16); 
            ILI9341_DispString_EN_CH(indent, y_pos, (char*)REPORT_LINE1_L1);       y_pos += base_line_height + spacing;
            ILI9341_DispString_EN_CH(indent, y_pos, (char*)REPORT_LINE1_L2);       y_pos += base_line_height + spacing;
            ILI9341_DispString_EN_CH(indent, y_pos, (char*)REPORT_LINE1_L3);       y_pos += base_line_height + spacing;
            ILI9341_DispString_EN_CH(indent, y_pos, (char*)REPORT_LINE1_L4);       y_pos += base_line_height + spacing;

            ILI9341_DispString_EN_CH(indent, y_pos, (char*)REPORT_LINE2_L1);        y_pos += base_line_height + spacing;
            ILI9341_DispString_EN_CH(indent, y_pos, (char*)REPORT_LINE2_L2);        y_pos += base_line_height + spacing;

            ILI9341_DispString_EN_CH(indent, y_pos, (char*)REPORT_LINE3_L1);        y_pos += base_line_height + spacing;
            ILI9341_DispString_EN_CH(indent, y_pos, (char*)REPORT_LINE3_L2);        y_pos += base_line_height + spacing;

            ILI9341_DispString_EN_CH(indent, y_pos, (char*)REPORT_LINE4_L1);        y_pos += base_line_height + spacing;
            ILI9341_DispString_EN_CH(indent, y_pos, (char*)REPORT_LINE4_L2);        y_pos += base_line_height + spacing;
            ILI9341_DispString_EN_CH(indent, y_pos, (char*)REPORT_LINE4_L3);        y_pos += base_line_height + spacing * 2;

            ILI9341_DispString_EN_CH(indent, y_pos, (char*)REPORT_ABSTRACT_TITLE);  y_pos += base_line_height + spacing;
            ILI9341_DispString_EN_CH(indent, y_pos, (char*)REPORT_ABSTRACT_P1_L1);  y_pos += base_line_height + spacing;
            ILI9341_DispString_EN_CH(indent, y_pos, (char*)REPORT_ABSTRACT_P1_L2);  y_pos += base_line_height + spacing;
            ILI9341_DispString_EN_CH(indent, y_pos, (char*)REPORT_ABSTRACT_P1_L3);  y_pos += base_line_height + spacing;
            ILI9341_DispString_EN_CH(indent, y_pos, (char*)REPORT_ABSTRACT_P1_L4);  y_pos += base_line_height + spacing;
            ILI9341_DispString_EN_CH(indent, y_pos, (char*)REPORT_ABSTRACT_P1_L5);
            
            sprintf(page_info, "Page %d/%d (KEY1 Next->)", g_report_page + 1, MAX_REPORT_PAGES);
            ILI9341_DispString_EN_CH(indent, LCD_Y_LENGTH - base_line_height - spacing, page_info); 

        } else if (g_report_page == 1) {
            LCD_SetFont(&Font8x16); 
            y_pos = CASE3_TOP_MARGIN; // 确保第二页内容也从调整后的Y坐标开始
            ILI9341_DispString_EN_CH(indent, y_pos, (char*)REPORT_ABSTRACT_P2_L1);  y_pos += base_line_height + spacing;
            ILI9341_DispString_EN_CH(indent, y_pos, (char*)REPORT_ABSTRACT_P2_L2);  y_pos += base_line_height + spacing;
            ILI9341_DispString_EN_CH(indent, y_pos, (char*)REPORT_ABSTRACT_P2_L3);  y_pos += base_line_height + spacing;
            ILI9341_DispString_EN_CH(indent, y_pos, (char*)REPORT_ABSTRACT_P2_L4);  y_pos += base_line_height + spacing;
            ILI9341_DispString_EN_CH(indent, y_pos, (char*)REPORT_ABSTRACT_P3_L1);  y_pos += base_line_height + spacing;
            ILI9341_DispString_EN_CH(indent, y_pos, (char*)REPORT_ABSTRACT_P3_L2);  y_pos += base_line_height + spacing;
            ILI9341_DispString_EN_CH(indent, y_pos, (char*)REPORT_ABSTRACT_P3_L3);  y_pos += base_line_height + spacing;
            ILI9341_DispString_EN_CH(indent, y_pos, (char*)REPORT_ABSTRACT_P3_L4);  y_pos += base_line_height + spacing;
            ILI9341_DispString_EN_CH(indent, y_pos, (char*)REPORT_ABSTRACT_P4_L1);  y_pos += base_line_height + spacing;
            ILI9341_DispString_EN_CH(indent, y_pos, (char*)REPORT_ABSTRACT_P4_L2);  y_pos += base_line_height + spacing;
            ILI9341_DispString_EN_CH(indent, y_pos, (char*)REPORT_ABSTRACT_P4_L3);  y_pos += base_line_height + spacing; 
            ILI9341_DispString_EN_CH(indent, y_pos, (char*)REPORT_ABSTRACT_P5_L1);  y_pos += base_line_height + spacing;
            ILI9341_DispString_EN_CH(indent, y_pos, (char*)REPORT_ABSTRACT_P5_L2);  y_pos += base_line_height + spacing;
         //   ILI9341_DispString_EN_CH(indent, y_pos, (char*)REPORT_ABSTRACT_P6_L1);
            
            sprintf(page_info, "Page %d/%d (<-KEY1 Prev)", g_report_page + 1, MAX_REPORT_PAGES);
            ILI9341_DispString_EN_CH(indent, LCD_Y_LENGTH - base_line_height - spacing, page_info);
        }
        g_case3_needs_redraw = 0; 
    }
}

void Display_Case4(void) {
    LCD_SetFont(&Font16x24); LCD_SetColors(BLUE, WHITE);
    ILI9341_DispString_EN_CH(10, LCD_Y_LENGTH / 2 - 20, (char*)"案例4: 显示图片");
    LCD_SetFont(&Font8x16);
    ILI9341_DispString_EN_CH(10, LCD_Y_LENGTH / 2 + 10, (char*)"(此功能待实现)");
    ILI9341_DispString_EN_CH(10, LCD_Y_LENGTH / 2 + 30, (char*)"请准备图片数据并编写绘制函数");
}

void Display_Case5_Update(void) { 
    if (g_case5_animation_done) return;
    if ((Get_System_ms() - g_case5_last_draw_time_ms) >= 100) { 
        g_case5_last_draw_time_ms = Get_System_ms();
        if (g_case5_rect_width > 0 && g_case5_rect_height > 0) {
            LCD_SetTextColor(CASE5_COLORS[g_case5_color_index % CASE5_NUM_COLORS]);
            ILI9341_DrawRectangle(g_case5_rect_x, g_case5_rect_y, g_case5_rect_width, g_case5_rect_height, 1); 
             if (g_case5_rect_width <= 2 || g_case5_rect_height <= 2) { 
                 g_case5_animation_done = 1;
                 LCD_SetFont(&Font8x16); LCD_SetColors(BLACK, WHITE);
                 ILI9341_DispString_EN(10, LCD_Y_LENGTH - 20, (char*)"Case 5 Done. Press K2.");
                 return;
             }
            g_case5_rect_x += 1; g_case5_rect_y += 1;
            g_case5_rect_width -= 2; g_case5_rect_height -= 2;
            g_case5_color_index++;
        } else {
            g_case5_animation_done = 1;
            LCD_SetFont(&Font8x16); LCD_SetColors(BLACK, WHITE);
            ILI9341_DispString_EN(10, LCD_Y_LENGTH - 20, (char*)"Case 5 Finished.");
        }
    }
}

int main(void) {
    uint8_t prev_display_case = 0; 
    SysTick_Init(); GPIO_Config(); ADC_Config(); UART_Config(); ILI9341_Init(); __enable_irq(); 
    ILI9341_GramScan(6); 
    
    Init_Display_Case(g_current_display_case); 
    sprintf(g_uart_tx_buffer, "System Initialized. Mode: %d, Case: %d\r\n", g_display_operation_mode, g_current_display_case);
    UART_SendString(g_uart_tx_buffer);
    prev_display_case = g_current_display_case;

    while (1) {
        Process_Keys();

        if (g_key2_action_pending) { 
            g_current_display_case++; 
            if (g_current_display_case > MAX_DISPLAY_CASES) {
                g_current_display_case = 1;
            }
            Init_Display_Case(g_current_display_case); 
            sprintf(g_uart_tx_buffer, "Display Case Switched To: %d\r\n", g_current_display_case); 
            UART_SendString(g_uart_tx_buffer);
            prev_display_case = g_current_display_case; 
            g_key2_action_pending = 0; 
        }
        
        if (g_current_display_case == 2) {
             Update_ADC_And_Speed(); 
        }

        switch (g_current_display_case) {
            case 1:
                if (g_key1_action_pending) { 
                    Display_Case1(); 
                    g_key1_action_pending = 0;
                }
                break;
            case 2:
                Display_Case2_Update(); 
                break;
            case 3:
                Display_Case3(); 
                break;
            case 4:
                 if (g_key1_action_pending) { 
                    Display_Case4();
                    g_key1_action_pending = 0;
                }
                break;
            case 5:
                if (!g_case5_animation_done) {
                    Display_Case5_Update();
                }
                break;
            default:
                g_current_display_case = 1;
                Init_Display_Case(1);
                prev_display_case = 1;
                break;
        }
    }
}