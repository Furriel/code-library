# ESP32 SD File Server

Servidor HTTP local, somente leitura, para navegar e baixar arquivos de um cartao microSD conectado ao ESP32.

A feature e independente: ela nao depende de nenhuma outra pasta deste repositorio.

## O que ela faz

- cria uma pagina local para listar arquivos do SD;
- permite download pelo navegador;
- restringe os arquivos a uma raiz configurada;
- bloqueia caminhos com `..`;
- pode restringir downloads por extensao;
- possui endpoint `/health`;
- aceita um mutex externo para compartilhar o SD com um logger ou outra tarefa;
- mantem o mutex durante todo o `streamFile()`, evitando que outra tarefa use o cartao enquanto o arquivo esta sendo enviado.

## Ambiente

- MCU: ESP32 classico / ESP32 DevKit
- Framework: Arduino
- IDE recomendada: VS Code + PlatformIO
- Cartao: microSD via SPI
- Rede: Wi-Fi ja conectado pela aplicacao
- Servidor HTTP: `WebServer`, incluido no Arduino-ESP32

## Dependencias

Nao ha dependencia externa alem do framework Arduino-ESP32.

## Getting Started - 2 minutos

Entre na pasta:

```bash
cd features/esp32-sd-file-server
```

Compile:

```bash
pio run
```

Para testar em uma placa, altere no `src/main.cpp`:

```cpp
constexpr const char *WIFI_SSID = "YOUR_WIFI_SSID";
constexpr const char *WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";
constexpr uint8_t SD_CS_PIN = 5;
```

Depois:

```bash
pio run -t upload
pio device monitor -b 115200
```

Abra no navegador o IP mostrado no monitor serial.

## Exemplo minimo

```cpp
#include <SD.h>
#include "sd_file_server.h"

SdFileServerConfig config;
SdFileServer fileServer(config);

void setup() {
  SD.begin(5);
  fileServer.begin();
}

void loop() {
  fileServer.handleClient();
}
```

A aplicacao deve conectar o Wi-Fi e montar o SD antes de chamar `begin()`.

## Rotas

| Rota | Funcao |
|---|---|
| `/` | pagina inicial |
| `/files` | lista arquivos abaixo da raiz configurada |
| `/download?file=/caminho/arquivo.csv` | baixa um arquivo |
| `/health` | retorna estado simples em JSON |

## Compartilhando o SD com outra tarefa

Se outra tarefa tambem grava ou le o cartao, use o mesmo `SemaphoreHandle_t` nos dois lados.

```cpp
SemaphoreHandle_t sdMutex = xSemaphoreCreateMutex();

fileServer.begin(sdMutex);
```

O servidor segura esse mutex enquanto percorre diretorios e durante o download completo.

## Onde mexer no codigo

Abra `src/sd_file_server.cpp` e procure pelas secoes numeradas:

| Quero alterar... | Procure por |
|---|---|
| regras de seguranca do caminho | `[2] VALIDACAO DE CAMINHO` |
| tipos MIME | `[3] TIPO DE CONTEUDO` |
| pagina inicial e health | `[5] ROTAS SIMPLES` |
| listagem recursiva | `[6] LISTAGEM DE ARQUIVOS` |
| download | `[7] DOWNLOAD` |
| registro das rotas | `[8] INICIALIZACAO` |

No header `include/sd_file_server.h`, procure por `[1] CONFIGURACAO` para alterar os parametros expostos ao usuario.

## Fluxo

```text
navegador
   |
   +--> GET /files
   |       |
   |       +--> trava mutex
   |       +--> percorre SD
   |       +--> gera links
   |       +--> libera mutex
   |
   +--> GET /download?file=...
           |
           +--> valida caminho
           +--> trava mutex
           +--> abre arquivo
           +--> streamFile()
           +--> fecha arquivo
           +--> libera mutex
```

## Status de validacao

- software/estrutura: revisado para uso independente;
- compilacao: validada pelo CI do repositorio quando o PR correspondente passa;
- hardware: pendente de validacao fisica desta versao generalizada em ESP32 + microSD;
- download simultaneo com gravacao: arquitetura preparada por mutex compartilhado; requer teste fisico de concorrencia para ser declarado validado em hardware.

## Limitacoes

- o servidor e HTTP local, sem TLS e sem autenticacao;
- use somente em rede confiavel ou em um AP local controlado;
- a listagem recursiva e limitada por `max_depth` para evitar percursos acidentais muito profundos;
- a feature e somente leitura: nao apaga nem envia arquivos para o SD.
