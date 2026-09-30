/* USER CODE BEGIN Header */

/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  ******************************************************************************
  */

/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <string.h>
#include <stdio.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/*
 * GANTI TOKEN DI BAWAH INI DENGAN ACCESS TOKEN BARU
 * DARI DEVICE THINGSBOARD KAMU.
 */
#define THINGSBOARD_TOKEN "oUWS683HLBbvLNnQUtIn"

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */
/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

uint8_t engine;
uint8_t ac1;
uint8_t ac2;
uint8_t dc;

uint8_t rx_data;

uint8_t last_engine = 0;
uint8_t last_ac1 = 0;
uint8_t last_ac2 = 0;
uint8_t last_dc = 0;

uint8_t first_reading = 1;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */


/* ============================================================
   FUNGSI UNTUK MENGIRIM COMMAND AT KE SIM900A
   ============================================================ */
void SIM900A_SendCommand(char *command)
{
    uint8_t rx_data;

    /* Kirim command ke SIM900A */
    HAL_UART_Transmit(
        &huart3,
        (uint8_t *)command,
        strlen(command),
        HAL_MAX_DELAY
    );

    /* Tampilkan command ke PuTTY */
    HAL_UART_Transmit(
        &huart2,
        (uint8_t *)"\r\nKIRIM: ",
        sizeof("\r\nKIRIM: ") - 1,
        HAL_MAX_DELAY
    );

    HAL_UART_Transmit(
        &huart2,
        (uint8_t *)command,
        strlen(command),
        HAL_MAX_DELAY
    );

    /* Header respons */
    HAL_UART_Transmit(
        &huart2,
        (uint8_t *)"RESPON:\r\n",
        sizeof("RESPON:\r\n") - 1,
        HAL_MAX_DELAY
    );

    /*
     * Baca respons SIM900A.
     *
     * Timeout 10 detik digunakan untuk mengakhiri
     * pembacaan ketika tidak ada data lagi.
     */
    while (1)
    {
        if (HAL_UART_Receive(
                &huart3,
                &rx_data,
                1,
                10000
            ) == HAL_OK)
        {
            HAL_UART_Transmit(
                &huart2,
                &rx_data,
                1,
                HAL_MAX_DELAY
            );
        }
        else
        {
            break;
        }
    }

    HAL_UART_Transmit(
        &huart2,
        (uint8_t *)"\r\n----------------------------------------\r\n",
        sizeof("\r\n----------------------------------------\r\n") - 1,
        HAL_MAX_DELAY
    );
}


/* ============================================================
   FUNGSI KIRIM DATA KE THINGSBOARD
   ============================================================ */
uint8_t SIM900A_SendThingsBoard(void)
{
    char http_request[600];

    uint8_t rx_data;
    uint8_t ctrl_z = 0x1A;

    uint8_t connect_ok = 0;
    uint8_t prompt_ok = 0;
    uint8_t http_ok = 0;

    uint32_t start_time;

    char response[1200];
    uint16_t response_index = 0;

    memset(response, 0, sizeof(response));


    /* ========================================================
       HEADER
       ======================================================== */

    HAL_UART_Transmit(
        &huart2,
        (uint8_t *)"\r\n========================================\r\n",
        sizeof("\r\n========================================\r\n") - 1,
        HAL_MAX_DELAY
    );

    HAL_UART_Transmit(
        &huart2,
        (uint8_t *)"MENGIRIM DATA KE THINGSBOARD...\r\n",
        sizeof("MENGIRIM DATA KE THINGSBOARD...\r\n") - 1,
        HAL_MAX_DELAY
    );


    /* ========================================================
       BERSIHKAN RX UART
       ======================================================== */

    while (
        HAL_UART_Receive(
            &huart3,
            &rx_data,
            1,
            10
        ) == HAL_OK
    )
    {
        /* Buang data lama */
    }


    /* ========================================================
       CIPSTART

       GPRS TIDAK DI-RESET LAGI.

       GPRS sudah diaktifkan satu kali saat startup.
       ======================================================== */

    char cipstart[] =
        "AT+CIPSTART=\"TCP\",\"thingsboard.cloud\",\"80\"\r\n";


    HAL_UART_Transmit(
        &huart2,
        (uint8_t *)"\r\nKIRIM: ",
        sizeof("\r\nKIRIM: ") - 1,
        HAL_MAX_DELAY
    );

    HAL_UART_Transmit(
        &huart2,
        (uint8_t *)cipstart,
        strlen(cipstart),
        HAL_MAX_DELAY
    );


    HAL_UART_Transmit(
        &huart3,
        (uint8_t *)cipstart,
        strlen(cipstart),
        HAL_MAX_DELAY
    );


    /* ========================================================
       TUNGGU RESPON CIPSTART
       ======================================================== */

    HAL_UART_Transmit(
        &huart2,
        (uint8_t *)"RESPON CIPSTART:\r\n",
        sizeof("RESPON CIPSTART:\r\n") - 1,
        HAL_MAX_DELAY
    );


    start_time = HAL_GetTick();

    while ((HAL_GetTick() - start_time) < 60000)
    {
        if (
            HAL_UART_Receive(
                &huart3,
                &rx_data,
                1,
                200
            ) == HAL_OK
        )
        {
            /* Tampilkan semua karakter */
            HAL_UART_Transmit(
                &huart2,
                &rx_data,
                1,
                HAL_MAX_DELAY
            );


            /* Simpan response */
            if (response_index < sizeof(response) - 1)
            {
                response[response_index++] = rx_data;
                response[response_index] = '\0';
            }


            /* CONNECT OK */
            if (
                strstr(
                    response,
                    "CONNECT OK"
                ) != NULL
            )
            {
                connect_ok = 1;

                HAL_UART_Transmit(
                    &huart2,
                    (uint8_t *)
                    "\r\n>>> CONNECT OK TERDETEKSI <<<\r\n",
                    sizeof("\r\n>>> CONNECT OK TERDETEKSI <<<\r\n") - 1,
                    HAL_MAX_DELAY
                );

                break;
            }


            /* ALREADY CONNECT */
            if (
                strstr(
                    response,
                    "ALREADY CONNECT"
                ) != NULL
            )
            {
                connect_ok = 1;

                HAL_UART_Transmit(
                    &huart2,
                    (uint8_t *)
                    "\r\n>>> ALREADY CONNECT <<<\r\n",
                    sizeof("\r\n>>> ALREADY CONNECT <<<\r\n") - 1,
                    HAL_MAX_DELAY
                );

                break;
            }


            /* CONNECT FAIL */
            if (
                strstr(
                    response,
                    "CONNECT FAIL"
                ) != NULL
            )
            {
                HAL_UART_Transmit(
                    &huart2,
                    (uint8_t *)
                    "\r\n>>> CONNECT FAIL TERDETEKSI <<<\r\n",
                    sizeof("\r\n>>> CONNECT FAIL TERDETEKSI <<<\r\n") - 1,
                    HAL_MAX_DELAY
                );

                break;
            }


            /* ERROR */
            if (
                strstr(
                    response,
                    "ERROR"
                ) != NULL
            )
            {
                HAL_UART_Transmit(
                    &huart2,
                    (uint8_t *)
                    "\r\n>>> ERROR TERDETEKSI <<<\r\n",
                    sizeof("\r\n>>> ERROR TERDETEKSI <<<\r\n") - 1,
                    HAL_MAX_DELAY
                );

                break;
            }
        }
    }


    HAL_UART_Transmit(
        &huart2,
        (uint8_t *)
        "\r\n--- SELESAI MENUNGGU CIPSTART ---\r\n",
        sizeof("\r\n--- SELESAI MENUNGGU CIPSTART ---\r\n") - 1,
        HAL_MAX_DELAY
    );


    /* ========================================================
       CEK HASIL CIPSTART
       ======================================================== */

    if (!connect_ok)
    {
        HAL_UART_Transmit(
            &huart2,
            (uint8_t *)
            "\r\n=== CIPSTART GAGAL ===\r\n",
            sizeof("\r\n=== CIPSTART GAGAL ===\r\n") - 1,
            HAL_MAX_DELAY
        );

        /*
         * Cek status socket.
         */
        SIM900A_SendCommand("AT+CIPSTATUS\r\n");

        HAL_UART_Transmit(
            &huart2,
            (uint8_t *)
            "[GAGAL] CONNECT TCP THINGSBOARD\r\n",
            sizeof("[GAGAL] CONNECT TCP THINGSBOARD\r\n") - 1,
            HAL_MAX_DELAY
        );

        return 0;
    }


    HAL_UART_Transmit(
        &huart2,
        (uint8_t *)"[CONNECT OK]\r\n",
        sizeof("[CONNECT OK]\r\n") - 1,
        HAL_MAX_DELAY
    );


    /* ========================================================
       BUAT HTTP POST
       ======================================================== */

    sprintf(
        http_request,

        "POST /api/v1/%s/telemetry HTTP/1.1\r\n"

        "Host: thingsboard.cloud\r\n"

        "Content-Type: application/json\r\n"

        "Content-Length: 35\r\n"

        "Connection: close\r\n"

        "\r\n"

        "{\"Engine\":%d,\"AC1\":%d,\"AC2\":%d,\"DC\":%d}",

        THINGSBOARD_TOKEN,

        engine,
        ac1,
        ac2,
        dc
    );


    /* ========================================================
       CIPSEND
       ======================================================== */

    HAL_UART_Transmit(
        &huart2,
        (uint8_t *)"\r\nKIRIM: AT+CIPSEND\r\n",
        sizeof("\r\nKIRIM: AT+CIPSEND\r\n") - 1,
        HAL_MAX_DELAY
    );


    HAL_UART_Transmit(
        &huart3,
        (uint8_t *)"AT+CIPSEND\r\n",
        strlen("AT+CIPSEND\r\n"),
        HAL_MAX_DELAY
    );


    HAL_UART_Transmit(
        &huart2,
        (uint8_t *)"Menunggu prompt > ...\r\n",
        sizeof("Menunggu prompt > ...\r\n") - 1,
        HAL_MAX_DELAY
    );


    /* ========================================================
       TUNGGU PROMPT >
       ======================================================== */

    start_time = HAL_GetTick();

    while ((HAL_GetTick() - start_time) < 15000)
    {
        if (
            HAL_UART_Receive(
                &huart3,
                &rx_data,
                1,
                100
            ) == HAL_OK
        )
        {
            HAL_UART_Transmit(
                &huart2,
                &rx_data,
                1,
                HAL_MAX_DELAY
            );


            if (rx_data == '>')
            {
                prompt_ok = 1;
                break;
            }
        }
    }


    HAL_UART_Transmit(
        &huart2,
        (uint8_t *)"\r\n",
        2,
        HAL_MAX_DELAY
    );


    /* ========================================================
       JIKA TIDAK DAPAT PROMPT >
       ======================================================== */

    if (!prompt_ok)
    {
        HAL_UART_Transmit(
            &huart2,
            (uint8_t *)
            "[GAGAL] Tidak mendapat prompt >\r\n",
            sizeof("[GAGAL] Tidak mendapat prompt >\r\n") - 1,
            HAL_MAX_DELAY
        );


        HAL_UART_Transmit(
            &huart3,
            (uint8_t *)"AT+CIPCLOSE\r\n",
            strlen("AT+CIPCLOSE\r\n"),
            HAL_MAX_DELAY
        );


        HAL_Delay(1000);

        return 0;
    }


    /* ========================================================
       TAMPILKAN DATA
       ======================================================== */

    HAL_UART_Transmit(
        &huart2,
        (uint8_t *)
        "\r\nDATA YANG DIKIRIM KE THINGSBOARD:\r\n",
        sizeof("\r\nDATA YANG DIKIRIM KE THINGSBOARD:\r\n") - 1,
        HAL_MAX_DELAY
    );


    HAL_UART_Transmit(
        &huart2,
        (uint8_t *)http_request,
        strlen(http_request),
        HAL_MAX_DELAY
    );


    /* ========================================================
       KIRIM HTTP REQUEST
       ======================================================== */

    HAL_UART_Transmit(
        &huart3,
        (uint8_t *)http_request,
        strlen(http_request),
        HAL_MAX_DELAY
    );


    /* ========================================================
       CTRL + Z
       ======================================================== */

    HAL_UART_Transmit(
        &huart3,
        &ctrl_z,
        1,
        HAL_MAX_DELAY
    );


    /* ========================================================
       TUNGGU RESPON THINGSBOARD
       ======================================================== */

    HAL_UART_Transmit(
        &huart2,
        (uint8_t *)
        "\r\nMENUNGGU RESPON THINGSBOARD:\r\n",
        sizeof("\r\nMENUNGGU RESPON THINGSBOARD:\r\n") - 1,
        HAL_MAX_DELAY
    );


    start_time = HAL_GetTick();

    memset(response, 0, sizeof(response));
    response_index = 0;


    while ((HAL_GetTick() - start_time) < 20000)
    {
        if (
            HAL_UART_Receive(
                &huart3,
                &rx_data,
                1,
                100
            ) == HAL_OK
        )
        {
            /* Tampilkan respons */
            HAL_UART_Transmit(
                &huart2,
                &rx_data,
                1,
                HAL_MAX_DELAY
            );


            /* Simpan respons */
            if (response_index < sizeof(response) - 1)
            {
                response[response_index++] = rx_data;
                response[response_index] = '\0';
            }


            /* HTTP 200 */
            if (
                strstr(
                    response,
                    "HTTP/1.1 200"
                ) != NULL
            )
            {
                http_ok = 1;
            }


            /* CLOSED */
            if (
                strstr(
                    response,
                    "CLOSED"
                ) != NULL
            )
            {
                break;
            }
        }
    }


    HAL_UART_Transmit(
        &huart2,
        (uint8_t *)"\r\n",
        2,
        HAL_MAX_DELAY
    );


    /* ========================================================
       HASIL PENGIRIMAN
       ======================================================== */

    if (http_ok)
    {
        HAL_UART_Transmit(
            &huart2,
            (uint8_t *)
            "[BERHASIL] Data diterima ThingsBoard\r\n",
            sizeof("[BERHASIL] Data diterima ThingsBoard\r\n") - 1,
            HAL_MAX_DELAY
        );


        HAL_UART_Transmit(
            &huart2,
            (uint8_t *)
            "========================================\r\n",
            sizeof("========================================\r\n") - 1,
            HAL_MAX_DELAY
        );


        return 1;
    }
    else
    {
        HAL_UART_Transmit(
            &huart2,
            (uint8_t *)
            "[GAGAL] ThingsBoard tidak memberikan HTTP 200\r\n",
            sizeof("[GAGAL] ThingsBoard tidak memberikan HTTP 200\r\n") - 1,
            HAL_MAX_DELAY
        );


        HAL_UART_Transmit(
            &huart2,
            (uint8_t *)
            "========================================\r\n",
            sizeof("========================================\r\n") - 1,
            HAL_MAX_DELAY
        );


        return 0;
    }
}


/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */
  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */
  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */
  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_USART2_UART_Init();
  MX_USART3_UART_Init();
  /* USER CODE BEGIN 2 */


    /* ========================================================
       HEADER
       ======================================================== */

    HAL_UART_Transmit(
        &huart2,
        (uint8_t *)
        "\r\n========================================\r\n",
        sizeof("\r\n========================================\r\n") - 1,
        HAL_MAX_DELAY
    );


    HAL_UART_Transmit(
        &huart2,
        (uint8_t *)
        " STM32 NUCLEO-G0B1RE + SIM900A\r\n",
        sizeof(" STM32 NUCLEO-G0B1RE + SIM900A\r\n") - 1,
        HAL_MAX_DELAY
    );


    HAL_UART_Transmit(
        &huart2,
        (uint8_t *)
        " 4 GPIO + THINGSBOARD\r\n",
        sizeof(" 4 GPIO + THINGSBOARD\r\n") - 1,
        HAL_MAX_DELAY
    );


    HAL_UART_Transmit(
        &huart2,
        (uint8_t *)
        "========================================\r\n",
        sizeof("========================================\r\n") - 1,
        HAL_MAX_DELAY
    );


    HAL_UART_Transmit(
        &huart2,
        (uint8_t *)"Engine = PB1\r\n",
        sizeof("Engine = PB1\r\n") - 1,
        HAL_MAX_DELAY
    );


    HAL_UART_Transmit(
        &huart2,
        (uint8_t *)"AC1    = PA4\r\n",
        sizeof("AC1    = PA4\r\n") - 1,
        HAL_MAX_DELAY
    );


    HAL_UART_Transmit(
        &huart2,
        (uint8_t *)"AC2    = PA1\r\n",
        sizeof("AC2    = PA1\r\n") - 1,
        HAL_MAX_DELAY
    );


    HAL_UART_Transmit(
        &huart2,
        (uint8_t *)"DC     = PA0\r\n",
        sizeof("DC     = PA0\r\n") - 1,
        HAL_MAX_DELAY
    );


    HAL_UART_Transmit(
        &huart2,
        (uint8_t *)
        "========================================\r\n",
        sizeof("========================================\r\n") - 1,
        HAL_MAX_DELAY
    );


    /* ========================================================
       TEST AT
       ======================================================== */

    SIM900A_SendCommand("AT\r\n");

    HAL_Delay(2000);


    /* ========================================================
       AKTIFKAN GPRS

       BAGIAN INI HANYA DILAKUKAN SEKALI SAAT STARTUP.
       ======================================================== */

    HAL_UART_Transmit(
        &huart2,
        (uint8_t *)
        "\r\n========================================\r\n",
        sizeof("\r\n========================================\r\n") - 1,
        HAL_MAX_DELAY
    );


    HAL_UART_Transmit(
        &huart2,
        (uint8_t *)
        " AKTIVASI GPRS AWAL\r\n",
        sizeof(" AKTIVASI GPRS AWAL\r\n") - 1,
        HAL_MAX_DELAY
    );


    HAL_UART_Transmit(
        &huart2,
        (uint8_t *)
        "========================================\r\n",
        sizeof("========================================\r\n") - 1,
        HAL_MAX_DELAY
    );


    /* --------------------------------------------------------
       Reset koneksi GPRS
       -------------------------------------------------------- */

    SIM900A_SendCommand("AT+CIPSHUT\r\n");

    HAL_Delay(5000);


    /* --------------------------------------------------------
       Set APN
       -------------------------------------------------------- */

    SIM900A_SendCommand(
        "AT+CSTT=\"internet\",\"\",\"\"\r\n"
    );

    HAL_Delay(3000);


    /* --------------------------------------------------------
       Aktifkan GPRS
       -------------------------------------------------------- */

    SIM900A_SendCommand("AT+CIICR\r\n");

    HAL_Delay(10000);


    /* --------------------------------------------------------
       Cek status
       -------------------------------------------------------- */

    SIM900A_SendCommand("AT+CIPSTATUS\r\n");

    HAL_Delay(2000);


    /* --------------------------------------------------------
       Ambil IP
       -------------------------------------------------------- */

    SIM900A_SendCommand("AT+CIFSR\r\n");

    HAL_Delay(2000);


    /* ========================================================
       TEST DNS
       ======================================================== */

    HAL_UART_Transmit(
        &huart2,
        (uint8_t *)
        "\r\n=== TEST DNS THINGSBOARD ===\r\n",
        sizeof("\r\n=== TEST DNS THINGSBOARD ===\r\n") - 1,
        HAL_MAX_DELAY
    );


    SIM900A_SendCommand(
        "AT+CDNSGIP=\"thingsboard.cloud\"\r\n"
    );

    HAL_Delay(5000);


    /* ========================================================
       BACA KONDISI AWAL GPIO
       ======================================================== */

    engine = HAL_GPIO_ReadPin(
        ENGINE_INPUT_GPIO_Port,
        ENGINE_INPUT_Pin
    );


    ac1 = HAL_GPIO_ReadPin(
        AC1_INPUT_GPIO_Port,
        AC1_INPUT_Pin
    );


    ac2 = HAL_GPIO_ReadPin(
        AC2_INPUT_GPIO_Port,
        AC2_INPUT_Pin
    );


    dc = HAL_GPIO_ReadPin(
        DC_INPUT_GPIO_Port,
        DC_INPUT_Pin
    );


    /* Simpan kondisi awal */

    last_engine = engine;
    last_ac1 = ac1;
    last_ac2 = ac2;
    last_dc = dc;


    /* ========================================================
       KIRIM KONDISI AWAL
       ======================================================== */

    HAL_UART_Transmit(
        &huart2,
        (uint8_t *)
        "\r\n=== KIRIM KONDISI AWAL ===\r\n",
        sizeof("\r\n=== KIRIM KONDISI AWAL ===\r\n") - 1,
        HAL_MAX_DELAY
    );


    if (SIM900A_SendThingsBoard())
    {
        HAL_UART_Transmit(
            &huart2,
            (uint8_t *)
            "STATUS: PENGIRIMAN BERHASIL\r\n",
            sizeof("STATUS: PENGIRIMAN BERHASIL\r\n") - 1,
            HAL_MAX_DELAY
        );
    }
    else
    {
        HAL_UART_Transmit(
            &huart2,
            (uint8_t *)
            "STATUS: PENGIRIMAN GAGAL\r\n",
            sizeof("STATUS: PENGIRIMAN GAGAL\r\n") - 1,
            HAL_MAX_DELAY
        );
    }


    HAL_Delay(2000);


  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */

    while (1)
    {
        /* ====================================================
           BACA 4 GPIO
           ==================================================== */

        engine = HAL_GPIO_ReadPin(
            ENGINE_INPUT_GPIO_Port,
            ENGINE_INPUT_Pin
        );


        ac1 = HAL_GPIO_ReadPin(
            AC1_INPUT_GPIO_Port,
            AC1_INPUT_Pin
        );


        ac2 = HAL_GPIO_ReadPin(
            AC2_INPUT_GPIO_Port,
            AC2_INPUT_Pin
        );


        dc = HAL_GPIO_ReadPin(
            DC_INPUT_GPIO_Port,
            DC_INPUT_Pin
        );


        /* ====================================================
           TAMPILKAN KONDISI GPIO
           ==================================================== */

        char msg[100];

        int len = sprintf(
            msg,
            "Engine = %d | AC1 = %d | AC2 = %d | DC = %d\r\n",
            engine,
            ac1,
            ac2,
            dc
        );


        HAL_UART_Transmit(
            &huart2,
            (uint8_t *)msg,
            len,
            HAL_MAX_DELAY
        );


        /* ====================================================
           CEK PERUBAHAN
           ==================================================== */

        if (
            (engine != last_engine) ||
            (ac1 != last_ac1) ||
            (ac2 != last_ac2) ||
            (dc != last_dc)
        )
        {
            HAL_UART_Transmit(
                &huart2,
                (uint8_t *)
                "\r\n*** ADA PERUBAHAN INPUT ***\r\n",
                sizeof("\r\n*** ADA PERUBAHAN INPUT ***\r\n") - 1,
                HAL_MAX_DELAY
            );


            /* ================================================
               KIRIM DATA BARU
               ================================================ */

            if (SIM900A_SendThingsBoard())
            {
                HAL_UART_Transmit(
                    &huart2,
                    (uint8_t *)
                    "STATUS: PENGIRIMAN BERHASIL\r\n",
                    sizeof("STATUS: PENGIRIMAN BERHASIL\r\n") - 1,
                    HAL_MAX_DELAY
                );
            }
            else
            {
                HAL_UART_Transmit(
                    &huart2,
                    (uint8_t *)
                    "STATUS: PENGIRIMAN GAGAL\r\n",
                    sizeof("STATUS: PENGIRIMAN GAGAL\r\n") - 1,
                    HAL_MAX_DELAY
                );
            }


            /* ================================================
               SIMPAN KONDISI TERBARU
               ================================================ */

            last_engine = engine;
            last_ac1 = ac1;
            last_ac2 = ac2;
            last_dc = dc;
        }


        /* Baca input setiap 1 detik */

        HAL_Delay(1000);
    }

    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSIDiv = RCC_HSI_DIV1;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */
/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */

    __disable_irq();

    while (1)
    {
    }

  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */

  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
