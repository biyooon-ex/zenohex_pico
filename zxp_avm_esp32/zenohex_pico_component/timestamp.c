#include <stdint.h>
#include <zenoh-pico.h>

#include "timestamp.h"

static int zxp_decimal_digit(uint8_t value)
{
  return value >= '0' && value <= '9' ? value - '0' : -1;
}

static int zxp_hex_digit(uint8_t value)
{
  if (value >= '0' && value <= '9') return value - '0';
  if (value >= 'a' && value <= 'f') return value - 'a' + 10;
  return -1;
}

static bool zxp_parse_decimal(const uint8_t *data, size_t size, uint32_t *value)
{
  uint32_t parsed = 0;
  for (size_t index = 0; index < size; index++) {
    int digit = zxp_decimal_digit(data[index]);
    if (digit < 0) return false;
    parsed = parsed * 10 + (uint32_t) digit;
  }
  *value = parsed;
  return true;
}

static bool zxp_is_leap_year(uint32_t year)
{
  return year % 4 == 0 && (year % 100 != 0 || year % 400 == 0);
}

static uint32_t zxp_days_in_month(uint32_t year, uint32_t month)
{
  static const uint32_t days[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
  return month == 2 && zxp_is_leap_year(year) ? 29 : days[month - 1];
}

static int64_t zxp_days_from_civil(uint32_t year, uint32_t month, uint32_t day)
{
  int64_t adjusted_year = (int64_t) year - (month <= 2);
  int64_t era = (adjusted_year >= 0 ? adjusted_year : adjusted_year - 399) / 400;
  uint32_t year_of_era = (uint32_t) (adjusted_year - era * 400);
  uint32_t adjusted_month = month > 2 ? month - 3 : month + 9;
  uint32_t day_of_year = (153 * adjusted_month + 2) / 5 + day - 1;
  uint32_t day_of_era = year_of_era * 365 + year_of_era / 4 - year_of_era / 100 + day_of_year;
  return era * 146097 + (int64_t) day_of_era - 719468;
}

bool zxp_timestamp_from_binary(const char *binary_data, size_t size, z_timestamp_t *timestamp)
{
  const uint8_t *data = (const uint8_t *) binary_data;
  if (size != 63 || data[4] != '-' || data[7] != '-' || data[10] != 'T' || data[13] != ':' ||
      data[16] != ':' || data[19] != '.' || data[29] != 'Z' || data[30] != '/') return false;

  uint32_t year, month, day, hour, minute, second, nanos = 0;
  if (!zxp_parse_decimal(data, 4, &year) || !zxp_parse_decimal(data + 5, 2, &month) ||
      !zxp_parse_decimal(data + 8, 2, &day) || !zxp_parse_decimal(data + 11, 2, &hour) ||
      !zxp_parse_decimal(data + 14, 2, &minute) || !zxp_parse_decimal(data + 17, 2, &second) ||
      !zxp_parse_decimal(data + 20, 9, &nanos) || month == 0 || month > 12 || day == 0 ||
      day > zxp_days_in_month(year, month) || hour > 23 || minute > 59 || second > 59) return false;

  _z_id_t id = _z_id_empty();
  for (size_t index = 0; index < ZENOH_ID_SIZE; index++) {
    int high = zxp_hex_digit(data[31 + index * 2]);
    int low = zxp_hex_digit(data[32 + index * 2]);
    if (high < 0 || low < 0) return false;
    id.id[index] = (uint8_t) ((high << 4) | low);
  }

  int64_t days = zxp_days_from_civil(year, month, day);
  if (days < 0 || (uint64_t) days > UINT64_MAX / 86400) return false;
  uint64_t seconds = (uint64_t) days * 86400 + hour * 3600 + minute * 60 + second;
  if (seconds > UINT32_MAX) return false;

  timestamp->valid = true;
  timestamp->id = id;
  timestamp->time = _z_timestamp_ntp64_from_time((uint32_t) seconds, nanos);
  return true;
}
