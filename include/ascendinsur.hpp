#include <eosio/eosio.hpp>
#include <eosio/asset.hpp>
#include <delphioracle.hpp>
#include <ascendweathr.hpp>

#include <math.h> // pow() and trig functions

using namespace std;
using namespace eosio;

CONTRACT ascendinsur : public contract {
  public:
    using contract::contract;

    ACTION stationdefi( name station );

    ACTION mapstation( name station, name contract_n );
    ACTION mapbylatlon( name contract_n, name station_type, float distance_km );
    ACTION addsubscribe(name contract_n, name subscriber);
    ACTION newcontract( name contract_n, 
                        name checking_action,
                        name token_name,
                        float trigger_amount_usd,
                        float latitude_deg, 
                        float longitude_deg,
                        float threshold_1,
                        float threshold_2 );

    ACTION unmapstation( name station, name contract_n );
    ACTION rmcontract( name contract_n );

    ACTION payout(name contract_n); // Updates the balances of all subscribers
    ACTION claimbalance(name contract_n, name subscriber);

    ///////////  All checking functions should have same inputs:  ///////////////
    //            - station, contract_lat, contract_lon

    ACTION raincheck1(name station, name contract_n);
    //ACTION raincheck2(name station, name contract_n);
    ACTION heatcheck1(name station, name contract_n);

    /////////////////////////////////////////////////////////////////////////////

  private:

    float calcDistance( float lat1, float lon1, float lat2, float lon2 ); // Calculate distance between two points
    float degToRadians( float degrees );
    float calcDewPoint( float temperature, float humidity );

    TABLE contracts {
      name contract_name;
      name  checking_action;
      double latitude_deg;
      double longitude_deg;
      float threshold_1; // Used for the checking actions
      float threshold_2;

      name token_name;
      float trigger_amount_usd;
      bool if_enabled; // If payout already happened, then this will be false

      auto primary_key() const { return contract_name.value; }
      uint64_t by_checkaction() const { return checking_action.value; };
      double by_latitude() const { return latitude_deg; }
      double by_longitude() const { return longitude_deg; }
    };
    typedef multi_index<name("contracts"), 
                        contracts,
                        indexed_by<"latitude"_n, const_mem_fun<contracts, double, &contracts::by_latitude>>,
                        indexed_by<"longitude"_n, const_mem_fun<contracts, double, &contracts::by_longitude>>
    > contracts_table_t;

    // Scope will be contract name
    TABLE subscribers {
      name subscriber;
      checksum160 subscriber_evm;
      float balance;

      auto primary_key() const { return subscriber.value; }
    };
    typedef multi_index<name("subscribers"), subscribers> subscribers_table_t;

    // Scope will be station name
    TABLE stationmap {
      name contract;

      auto primary_key() const { return contract.value; }
    };
    typedef multi_index<name("stationmap"), stationmap> stationmap_table_t;
    

/*
    TABLE sensor2check {
      name checking_action;
      name station_type;

      auto primary_key() const { return checking_action.value; }
      uint64_t by_stationtype() const { return station_type.value; };
    };
    typedef multi_index<name("sensor2check"), 
                        sensor2check,
                        indexed_by<"stationtype"_n, const_mem_fun<sensor2check, uint64_t, &sensor2check::by_stationtype>>
    > sensor2check_table_t;
*/
};
