#include <eosio/eosio.hpp>

using namespace std;
using namespace eosio;

namespace ascendweathr {

  bool check_bit( uint8_t device_flags, uint8_t target_flag );
  bool if_indoor_flagged( uint8_t flags, float temperature_c );
  bool if_physical_damage( uint8_t flags );
  
  TABLE parametersv1 {
  
      uint64_t period_start_time;
      uint64_t period_end_time;
      string latest_firmware_version;
  
      // Ony one row
      auto primary_key() const { return 0; }
  };
  typedef multi_index<name("parametersv1"), parametersv1> parameters_table_t;
  
  // Data Table List
  TABLE sensorsv3 {
      name devname;
      uint64_t devname_uint64;
      name station_type;
      uint64_t time_created;
      string message_type;
  
      string firmware_version;
  
      uint16_t voltage_mv;
      uint8_t num_uplinks_today;
      uint8_t num_uplinks_poor_today;
      uint8_t num_stations_nearby;
      float percent_uplinks_poor_previous;
      float sensor_pay_previous;
      float max_temp_today;
      float min_temp_today;
  
      bool if_indoor;
      bool permanently_damaged;
      bool in_transit;
      bool one_hotspot;
      bool active_this_period;
      bool active_last_period;
      bool has_helium_miner;
      bool allow_new_memo;
  
      auto  primary_key() const { return devname.value; }
  };
  typedef multi_index<name("sensorsv3"), sensorsv3> sensorsv3_table_t;
  
  TABLE weather {
    name devname;
    uint64_t unix_time_s;
    double latitude_deg;
    double longitude_deg;
    uint16_t elevation_gps_m;
    float pressure_hpa;
    float temperature_c;
    float humidity_percent;
    float dew_point;
    uint8_t flags;
    string loc_accuracy;
  
    auto primary_key() const { return devname.value; }
    uint64_t by_unixtime() const { return unix_time_s; }
    double by_latitude() const { return latitude_deg; }
    double by_longitude() const { return longitude_deg; }
    // TODO: custom index based on having / not having specific flag
  };
  //using observations_index = multi_index<"observations"_n, observations>;
  typedef multi_index<name("weather"), 
                      weather,
                      indexed_by<"unixtime"_n, const_mem_fun<weather, uint64_t, &weather::by_unixtime>>,
                      indexed_by<"latitude"_n, const_mem_fun<weather, double, &weather::by_latitude>>,
                      indexed_by<"longitude"_n, const_mem_fun<weather, double, &weather::by_longitude>>
  > weather_table_t;
  
  TABLE raincumulate {
    name devname;
    uint64_t unix_time_s;
    double latitude_deg;
    double longitude_deg;
    uint16_t elevation_gps_m;
    float rain_1hr;
    float rain_6hr;
    float rain_24hr;
    uint8_t flags;
  
    auto primary_key() const { return devname.value; }
    uint64_t by_unixtime() const { return unix_time_s; }
    double by_longitude() const { return longitude_deg; }
    // TODO: custom index based on having / not having specific flag
  };
  typedef multi_index<name("raincumulate"), 
                      raincumulate,
                      indexed_by<"unixtime"_n, const_mem_fun<raincumulate, uint64_t, &raincumulate::by_unixtime>>,
                      indexed_by<"longitude"_n, const_mem_fun<raincumulate, double, &raincumulate::by_longitude>>
  > raincumulate_table_t;
  
  TABLE rainraw {
  
    // Note, this table will use devname/station as the scope
    // Ideally should always have 96 rows present in this table for each of 96 data points
    uint64_t unix_time_s;
    float rain_1hr_mm;
    uint8_t flags;
  
    auto primary_key() const { return unix_time_s; }
    // TODO: custom index based on having / not having specific flag
  };
  typedef multi_index<name("rainraw"), rainraw> rainraw_table_t;
  
  TABLE wind {
    name devname;
    uint64_t unix_time_s;
    double latitude_deg;
    double longitude_deg;
    uint16_t elevation_gps_m;
    float wind_dir_deg;
    float wind_spd_ms;
    uint8_t flags;
  
    auto primary_key() const { return devname.value; }
    uint64_t by_unixtime() const { return unix_time_s; }
    double by_longitude() const { return longitude_deg; }
    // TODO: custom index based on having / not having specific flag
  };
  typedef multi_index<name("wind"), 
                      wind,
                      indexed_by<"unixtime"_n, const_mem_fun<wind, uint64_t, &wind::by_unixtime>>,
                      indexed_by<"longitude"_n, const_mem_fun<wind, double, &wind::by_longitude>>
  > wind_table_t;
  
  TABLE solar {
    name devname;
    uint64_t unix_time_s;
    double latitude_deg;
    double longitude_deg;
    uint16_t elevation_gps_m;
    uint32_t light_intensity_lux;
    uint8_t uv_index;
    uint8_t flags;
  
    auto primary_key() const { return devname.value; }
    uint64_t by_unixtime() const { return unix_time_s; }
    double by_longitude() const { return longitude_deg; }
    // TODO: custom index based on having / not having specific flag
  };
  typedef multi_index<name("solar"), 
                      solar,
                      indexed_by<"unixtime"_n, const_mem_fun<solar, uint64_t, &solar::by_unixtime>>,
                      indexed_by<"longitude"_n, const_mem_fun<solar, double, &solar::by_longitude>>
  > solar_table_t;
  
  TABLE stationtypes {
      name station_type;
      name token_contract;
  
      float miner_poor_rate;
      float miner_good_rate;
      float miner_great_rate;
  
      auto primary_key() const { return station_type.value; }
  };
  typedef multi_index<name("stationtypes"), stationtypes> stationtypes_table_t;
  
  // TODO: separate flags by scope ("weather_n", "rain", etc.)
  
  TABLE flags {
    uint64_t bit_value;
    string processing_step;
    string issue;
    string explanation;
  
    auto primary_key() const { return bit_value; }
  };
  typedef multi_index<name("flags"), flags> flags_table_t;

}
