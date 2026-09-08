// NMEA GGA parser
//
// This file has no project-specific dependencies. The parser is intentionally
// small so it can be copied into another Node.js application without bringing
// the rest of this repository with it.

function nmeaCoordinateToDecimal(value, direction) {
  if (!value || !direction) return null;

  const dot = value.indexOf('.');
  if (dot < 3) return null;

  // NMEA stores coordinates as degrees + minutes (ddmm.mmmm or dddmm.mmmm).
  // The two digits immediately before the decimal point always belong to minutes.
  const degrees = Number(value.slice(0, dot - 2));
  const minutes = Number(value.slice(dot - 2));

  if (!Number.isFinite(degrees) || !Number.isFinite(minutes)) return null;
  if (minutes < 0 || minutes >= 60) return null;

  let decimal = degrees + minutes / 60;
  if (direction === 'S' || direction === 'W') decimal *= -1;

  return Number(decimal.toFixed(7));
}

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

function parseGga(sentence) {
  if (!sentence) return null;

  const line = String(sentence).trim();
  if (!line.startsWith('$GPGGA') && !line.startsWith('$GNGGA')) return null;

  const fields = line.split(',');
  if (fields.length < 10) return null;

  const latitude = nmeaCoordinateToDecimal(fields[2], fields[3]);
  const longitude = nmeaCoordinateToDecimal(fields[4], fields[5]);
  if (latitude === null || longitude === null) return null;

  const fixQuality = Number(fields[6]);
  const satellites = Number(fields[7]);
  const hdop = Number(fields[8]);
  const altitude = Number(fields[9]);

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
