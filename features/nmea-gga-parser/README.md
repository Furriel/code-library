# NMEA GGA Parser

Parser pequeno e independente para sentencas NMEA GGA (`$GPGGA` e `$GNGGA`).

## Getting Started - 2 minutos

Pre-requisito: Node.js instalado.

Na raiz do repositorio:

```bash
cd features/nmea-gga-parser
node tests/test.js
```

Resultado esperado:

```text
PASS nmea-gga-parser
```

Para testar uma leitura real, crie `example.js` nesta pasta:

```javascript
const { parseGga } = require('./src/nmea_gga');

const line = '$GNGGA,123519,1634.1234,S,04915.2345,W,4,18,0.8,721.5,M,0.0,M,,*52';
console.log(parseGga(line));
```

Execute:

```bash
node example.js
```

Se o objeto retornar latitude, longitude, `rtk_fix`, 18 satelites, HDOP 0.8 e altitude 721.5 m, a feature esta funcionando.

## O que faz

Converte uma sentenca GGA em um objeto com:

- latitude e longitude em graus decimais;
- qualidade do fix;
- numero de satelites;
- HDOP;
- altitude.

Tambem identifica GPS, DGPS, RTK fixed, RTK float e posicionamento estimado.

## Ambiente

- Microcontrolador: nao e necessario.
- Runtime: Node.js.
- IDE: qualquer editor; VS Code e suficiente.
- Dependencias externas: nenhuma.

O codigo e JavaScript puro e pode ser adaptado para outras plataformas com facilidade.

## Arquivos

```text
src/nmea_gga.js   parser
tests/test.js     teste funcional
```

## Uso

```javascript
const { parseGga } = require('./src/nmea_gga');

const result = parseGga(
  '$GNGGA,123519,1634.1234,S,04915.2345,W,4,18,0.8,721.5,M,0.0,M,,*52'
);

console.log(result);
```

## Teste

```bash
node tests/test.js
```

Resultado esperado:

```text
PASS nmea-gga-parser
```

## Status de validacao

Teste funcional de conversao de coordenadas, RTK fix, satelites, HDOP, altitude e rejeicao de sentencas que nao sao GGA.

Observacao: esta implementacao nao valida o checksum NMEA. Isso e intencional para manter a feature simples; a verificacao pode ser adicionada quando a aplicacao exigir esse nivel de integridade.
