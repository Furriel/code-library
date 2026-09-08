const assert = require('assert');
const {
  parseGga,
  nmeaCoordinateToDecimal,
  fixQualityName
} = require('../src/nmea_gga');

const sample = '$GNGGA,123519,1634.1234,S,04915.2345,W,4,18,0.8,721.5,M,0.0,M,,*52';
const parsed = parseGga(sample);

assert(parsed, 'GGA sentence should be parsed');
assert.strictEqual(parsed.fix, 'rtk_fixed');
assert.strictEqual(parsed.fix_quality, 4);
assert.strictEqual(parsed.satellites, 18);
assert(Math.abs(parsed.latitude + 16.5687233) < 0.000001);
assert(Math.abs(parsed.longitude + 49.2539083) < 0.000001);
assert.strictEqual(parsed.hdop, 0.8);
assert.strictEqual(parsed.altitude_m, 721.5);

assert.strictEqual(
  nmeaCoordinateToDecimal('4915.2345', 'W'),
  -49.2539083
);
assert.strictEqual(fixQualityName(5), 'rtk_float');
assert.strictEqual(parseGga('$GPRMC,1,2,3'), null);
assert.strictEqual(parseGga(''), null);

console.log('PASS nmea-gga-parser');
