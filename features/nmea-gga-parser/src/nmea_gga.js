// NMEA GGA PARSER
// -----------------------------------------------------------------------------
// Objetivo: receber uma linha NMEA GGA e devolver um objeto simples com
// latitude, longitude, qualidade do fix, satelites, HDOP e altitude.
//
// Exemplo de entrada:
//   $GNGGA,123519,1634.1234,S,04915.2345,W,4,18,0.8,721.5,M,0.0,M,,*52
//
// LEIA NESTA ORDEM:
//   [1] nmeaCoordinateToDecimal -> converte ddmm.mmmm para graus decimais
//   [2] fixQualityName          -> traduz o codigo numerico do fix
//   [3] parseGga                -> separa os campos e monta o resultado final
//
// Dica: se quiser alterar o que a funcao retorna, va direto para [3].

// [1] CONVERSAO DE COORDENADAS
// NMEA nao envia latitude/longitude diretamente em graus decimais.
// O formato e:
//   latitude:  ddmm.mmmm
//   longitude: dddmm.mmmm
//
// Exemplo:
//   1634.1234,S -> 16 graus + 34.1234 minutos -> valor negativo por ser Sul.
function nmeaCoordinateToDecimal(value, direction) {
  if (!value || !direction) return null;

  const dot = value.indexOf('.');
  if (dot < 3) return null;

  // Os dois digitos imediatamente antes do ponto decimal pertencem aos
  // minutos. Tudo que vem antes deles pertence aos graus.
  const degrees = Number(value.slice(0, dot - 2));
  const minutes = Number(value.slice(dot - 2));

  // Rejeita coordenadas que nao puderam ser convertidas para numero ou que
  // possuem minutos fora do intervalo valido de 0 a <60.
  if (!Number.isFinite(degrees) || !Number.isFinite(minutes)) return null;
  if (minutes < 0 || minutes >= 60) return null;

  let decimal = degrees + minutes / 60;

  // Sul e Oeste sao negativos no sistema de graus decimais convencional.
  if (direction === 'S' || direction === 'W') decimal *= -1;

  // Sete casas decimais sao suficientes para manter boa resolucao sem carregar
  // ruido desnecessario da representacao de ponto flutuante.
  return Number(decimal.toFixed(7));
}

// [2] QUALIDADE DO FIX
// O campo 6 da sentenca GGA e numerico. Esta funcao converte o numero para um
// nome mais facil de usar em logs, interfaces e regras da aplicacao.
function fixQualityName(value) {
  switch (value) {
    case 0: return 'no_fix';
    case 1: return 'gps';
    case 2: return 'dgps';
    case 4: return 'rtk_fixed';
    case 5: return 'rtk_float';
    case 6: return 'estimated';
    default: return 'unknown';
  }
}

// [3] PARSER PRINCIPAL
// Esta e a funcao que normalmente deve ser chamada pela aplicacao.
//
// Retorna:
//   objeto -> quando a linha e uma GGA valida para esta implementacao;
//   null   -> quando a linha nao e GGA ou esta incompleta/invalida.
function parseGga(sentence) {
  if (!sentence) return null;

  const line = String(sentence).trim();

  // Aceita os identificadores mais comuns de GGA:
  // GPGGA = GPS; GNGGA = GNSS combinado.
  if (!line.startsWith('$GPGGA') && !line.startsWith('$GNGGA')) return null;

  // NMEA usa virgula para separar os campos.
  const fields = line.split(',');
  if (fields.length < 10) return null;

  // Campos usados nesta feature:
  // fields[2] latitude      fields[3] N/S
  // fields[4] longitude     fields[5] E/W
  // fields[6] qualidade     fields[7] satelites
  // fields[8] HDOP          fields[9] altitude
  const latitude = nmeaCoordinateToDecimal(fields[2], fields[3]);
  const longitude = nmeaCoordinateToDecimal(fields[4], fields[5]);
  if (latitude === null || longitude === null) return null;

  const fixQuality = Number(fields[6]);
  const satellites = Number(fields[7]);
  const hdop = Number(fields[8]);
  const altitude = Number(fields[9]);

  // Aqui fica o formato publico retornado pela feature. Para acrescentar um
  // novo campo ao resultado, este e o ponto mais simples para comecar.
  return {
    latitude,
    longitude,
    fix_quality: Number.isFinite(fixQuality) ? fixQuality : 0,
    fix: fixQualityName(Number.isFinite(fixQuality) ? fixQuality : 0),
    satellites: Number.isFinite(satellites) ? satellites : 0,
    hdop: Number.isFinite(hdop) ? hdop : null,
    altitude_m: Number.isFinite(altitude) ? altitude : null
  };
}

module.exports = {
  parseGga,
  nmeaCoordinateToDecimal,
  fixQualityName
};
