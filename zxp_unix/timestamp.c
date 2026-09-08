#include <erl_nif.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <zenoh-pico.h>

#include "helper/helper.h"
#include "term.h"
#include "timestamp.h"

static int zxp_decimal_digit(uint8_t value)
{
  return value >= '0' && value <= '9' ? value - '0' : -1;
}

static int zxp_hex_digit(uint8_t value)
{
  if (value >= '0' && value <= '9')
  {
    return value - '0';
  }
  if (value >= 'a' && value <= 'f')
  {
    return value - 'a' + 10;
  }
  return -1;
}

static bool zxp_parse_decimal(const uint8_t *data, size_t length, uint32_t *value)
{
  uint32_t parsed = 0;
  for (size_t index = 0; index < length; index++)
  {
    int digit = zxp_decimal_digit(data[index]);
    if (digit < 0)
    {
      return false;
    }
    parsed = parsed * 10 + (uint32_t)digit;
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
  if (month == 2 && zxp_is_leap_year(year))
  {
    return 29;
  }
  return days[month - 1];
}

static int64_t zxp_days_from_civil(uint32_t year, uint32_t month, uint32_t day)
{
  int64_t adjusted_year = (int64_t)year - (month <= 2);
  int64_t era = (adjusted_year >= 0 ? adjusted_year : adjusted_year - 399) / 400;
  uint32_t year_of_era = (uint32_t)(adjusted_year - era * 400);
  uint32_t adjusted_month = month > 2 ? month - 3 : month + 9;
  uint32_t day_of_year = (153 * adjusted_month + 2) / 5 + day - 1;
  uint32_t day_of_era = year_of_era * 365 + year_of_era / 4 - year_of_era / 100 + day_of_year;
  return era * 146097 + (int64_t)day_of_era - 719468;
}

static void zxp_civil_from_days(int64_t days, uint32_t *year, uint32_t *month, uint32_t *day)
{
  days += 719468;
  int64_t era = (days >= 0 ? days : days - 146096) / 146097;
  uint32_t day_of_era = (uint32_t)(days - era * 146097);
  uint32_t year_of_era =
      (day_of_era - day_of_era / 1460 + day_of_era / 36524 - day_of_era / 146096) / 365;
  int64_t calculated_year = (int64_t)year_of_era + era * 400;
  uint32_t day_of_year = day_of_era - (365 * year_of_era + year_of_era / 4 - year_of_era / 100);
  uint32_t month_prime = (5 * day_of_year + 2) / 153;

  *day = day_of_year - (153 * month_prime + 2) / 5 + 1;
  *month = month_prime < 10 ? month_prime + 3 : month_prime - 9;
  *year = (uint32_t)(calculated_year + (*month <= 2));
}

bool zxp_timestamp_from_binary(const ErlNifBinary *binary, z_timestamp_t *timestamp)
{
  const uint8_t *data = binary->data;
  size_t length = binary->size;
  if (length != 63 || data[4] != '-' || data[7] != '-' || data[10] != 'T' || data[13] != ':' ||
      data[16] != ':' || data[19] != '.' || data[29] != 'Z' || data[30] != '/')
  {
    return false;
  }

  uint32_t year;
  uint32_t month;
  uint32_t day;
  uint32_t hour;
  uint32_t minute;
  uint32_t second;
  if (!zxp_parse_decimal(data, 4, &year) || !zxp_parse_decimal(data + 5, 2, &month) ||
      !zxp_parse_decimal(data + 8, 2, &day) || !zxp_parse_decimal(data + 11, 2, &hour) ||
      !zxp_parse_decimal(data + 14, 2, &minute) || !zxp_parse_decimal(data + 17, 2, &second) ||
      month == 0 || month > 12 || day == 0 || day > zxp_days_in_month(year, month) || hour > 23 ||
      minute > 59 || second > 59)
  {
    return false;
  }

  uint32_t nanos = 0;
  if (!zxp_parse_decimal(data + 20, 9, &nanos))
  {
    return false;
  }

  _z_id_t id = _z_id_empty();
  for (size_t id_index = 0; id_index < ZENOH_ID_SIZE; id_index++)
  {
    int high = zxp_hex_digit(data[31 + id_index * 2]);
    int low = zxp_hex_digit(data[32 + id_index * 2]);
    if (high < 0 || low < 0)
    {
      return false;
    }
    id.id[id_index] = (uint8_t)((high << 4) | low);
  }

  int64_t days = zxp_days_from_civil(year, month, day);
  if (days < 0 || (uint64_t)days > UINT64_MAX / 86400)
  {
    return false;
  }
  uint64_t seconds = (uint64_t)days * 86400 + hour * 3600 + minute * 60 + second;
  if (seconds > UINT32_MAX)
  {
    return false;
  }

  timestamp->valid = true;
  timestamp->id = id;
  timestamp->time = _z_timestamp_ntp64_from_time((uint32_t)seconds, nanos);
  return true;
}

ERL_NIF_TERM zxp_binary_from_zp_timestamp(ErlNifEnv *env, const z_timestamp_t *timestamp)
{
  if (timestamp == NULL)
  {
    return nil_atom;
  }
  uint64_t ntp64 = z_timestamp_ntp64_time(timestamp);
  uint64_t seconds = ntp64 >> 32;
  uint32_t fraction = (uint32_t)ntp64;
  uint32_t nanos = (uint32_t)(((uint64_t)fraction * 1000000000 + ((uint64_t)1 << 31)) >> 32);
  if (nanos == 1000000000)
  {
    seconds++;
    nanos = 0;
  }

  uint32_t year;
  uint32_t month;
  uint32_t day;
  zxp_civil_from_days((int64_t)(seconds / 86400), &year, &month, &day);
  if (year > 9999)
  {
    return zxp_raise(env, __FILE__, __LINE__, "timestamp year out of range");
  }

  uint64_t seconds_of_day = seconds % 86400;
  z_id_t id = z_timestamp_id(timestamp);
  char timestamp_string[64];
  int length =
      snprintf(timestamp_string,
               sizeof(timestamp_string),
               "%04" PRIu32 "-%02" PRIu32 "-%02" PRIu32 "T%02llu:%02llu:%02llu.%09" PRIu32 "Z/",
               year,
               month,
               day,
               (unsigned long long)(seconds_of_day / 3600),
               (unsigned long long)((seconds_of_day / 60) % 60),
               (unsigned long long)(seconds_of_day % 60),
               nanos);
  if (length != 31)
  {
    return zxp_raise(env, __FILE__, __LINE__, "timestamp formatting failed");
  }

  for (size_t index = 0; index < ZENOH_ID_SIZE; index++)
  {
    snprintf(timestamp_string + 31 + index * 2, 3, "%02x", id.id[index]);
  }
  return zxp_binary_from_bytes(
      env, (const uint8_t *)timestamp_string, sizeof(timestamp_string) - 1);
}
