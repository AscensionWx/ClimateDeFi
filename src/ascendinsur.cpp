#include <ascendinsur.hpp>
#include <eosio/system.hpp>

ACTION ascendinsur::stationdefi( name station ) 
{

  require_auth(name("ascendweathr"));

  // First look at all the climate defi contracts relevant to this station
  stationmap_table_t _stationmap(get_self(), station.value);

  auto stationmap_itr = _stationmap.begin();
  while (stationmap_itr != _stationmap.end()) {
    name contract = stationmap_itr->contract;

    contracts_table_t _contracts( get_self(), get_self().value );
    auto contracts_itr = _contracts.find( contract.value );

    if ( contracts_itr != _contracts.end() ) {

      // Call inline action name while passing station as a parameter
      action(
        permission_level{ get_self(), "active"_n },
        get_self(), 
        contracts_itr->checking_action,
        std::make_tuple( station, contract )
      ).send();
    }

    stationmap_itr++;
  }

}

ACTION ascendinsur::mapstation( name station, name contract ) 
{

  require_auth(get_self());

  contracts_table_t _contracts( get_self(), get_self().value );
  auto contracts_itr = _contracts.find( contract.value );
  check( contracts_itr != _contracts.end() , "Climate DeFi contract doesn't exist." );

  stationmap_table_t _stations( get_self(), station.value );
  auto station_itr = _stations.find( contract.value );

  if ( station_itr == _stations.end() ) {
    _stations.emplace( get_self(), [&](auto& station) {
      station.contract = contract;
    });
  }

}

ACTION ascendinsur::mapbylatlon( name contract, name station_type, float distance_km ) 
{
  require_auth(get_self());

  contracts_table_t _contracts( get_self(), get_self().value );
  auto contracts_itr = _contracts.find( contract.value );

  check( contracts_itr != _contracts.end() , "Climate DeFi contract doesn't exist." );

  // First determine the length of one degree of lat and one degree of lon
  float one_lat_degree_km = 111.0; // Distance between latitudes is essentially constant
  float one_lon_degree_km = (M_PI/180)*6372.8*cos(degToRadians(contracts_itr->latitude_deg)); // meridian distance varies by lat

  float lat_lower = contracts_itr->latitude_deg - ( distance_km / one_lat_degree_km );
  float lat_upper = contracts_itr->latitude_deg + ( distance_km / one_lat_degree_km );
  float lon_lower = contracts_itr->longitude_deg - ( distance_km / one_lon_degree_km );
  float lon_upper = contracts_itr->longitude_deg + ( distance_km / one_lon_degree_km );

  ascendweathr::weather_table_t _weather( name("ascendweathr"), name("ascendweathr").value );
  ascendweathr::sensorsv3_table_t _sensors( name("ascendweathr"), name("ascendweathr").value );

    // Will return SORTED list of sensors to the "right" of the leftern-most station in the box.
  // I'm choosing longitude, because most stations are in Northern hemisphere,
  //      so stations are spread out more evenly by lon than lat
  auto lon_index = _weather.get_index<"longitude"_n>();
  auto lon_itr = lon_index.lower_bound( lon_lower );

  // Remember this lon_itr list is sorted... so we can start with the left-most
  //   station in the box, and stop at the first station that falls outside the box
  for ( auto itr = lon_itr ; itr->longitude_deg < lon_upper && itr != lon_index.end() ; itr++ )
  {
    // Check if latitude constraints are met as well.
    if ( itr->latitude_deg > lat_lower && itr->latitude_deg < lat_upper )
    {
    
      name station = itr->devname;

      // Check if station_type matches
      auto sensors_itr = _sensors.find( station.value );
      if ( sensors_itr->station_type == station_type ) {
      
        // Call inline action to map the station
        action(
          permission_level{ get_self(), "active"_n },
          get_self(), 
          "mapstation"_n,
          std::make_tuple( station, contract )
        ).send();
      
      } // station type check 
    } // end latitude loop
  } // end longtiude loop

}

ACTION ascendinsur::addsubscribe(name contract, name subscriber) 
{
  require_auth(get_self());

  subscribers_table_t _subscribers( get_self(), contract.value );
  auto subscriber_itr = _subscribers.find( subscriber.value );

  check( subscriber_itr != _subscribers.end(), "Already subscribed to this contract." );

  _subscribers.emplace( get_self(), [&](auto& sub) {
      sub.subscriber = subscriber;
      //sub.subscriber_evm = 
      sub.balance = 0;
  });


}

ACTION ascendinsur::newcontract( name contract_name, 
                    name checking_action,
                    name token_name,
                    float trigger_amount_usd,
                    float latitude_deg, 
                    float longitude_deg,
                    float threshold_1,
                    float threshold_2 ) 
{
  require_auth(get_self());

  contracts_table_t _contracts( get_self(), get_self().value );
  auto contract_itr = _contracts.find( contract_name.value );

  check( contract_itr != _contracts.end(),
         "Contract name already taken. Please use different name or remove current one." );

  _contracts.emplace( get_self(), [&](auto& ctrct) {
    ctrct.contract_name = contract_name;
    ctrct.checking_action = checking_action;
    ctrct.token_name = token_name;
    ctrct.trigger_amount_usd = trigger_amount_usd;
    ctrct.latitude_deg = latitude_deg;
    ctrct.longitude_deg = longitude_deg;
    ctrct.threshold_1 = threshold_1;
    ctrct.threshold_2 = threshold_2;
    ctrct.if_enabled = true;
  });
        
}

ACTION ascendinsur::unmapstation( name station, name contract ) 
{

  require_auth(get_self());

  stationmap_table_t _stations( get_self(), station.value );
  auto station_itr = _stations.find( contract.value );

  if ( station_itr != _stations.end() )
    _stations.erase(station_itr);

}

ACTION ascendinsur::rmcontract( name contract ) 
{

  require_auth(get_self());

  // First delete all subscribers
  subscribers_table_t _subscribers( get_self(), contract.value );
  auto subscriber_itr = _subscribers.begin();
  while (subscriber_itr != _subscribers.end()) {
    check( subscriber_itr->balance == 0, "Cannot delete contract. A subscriber holds non-zero balance." );
    subscriber_itr = _subscribers.erase( subscriber_itr );
  }

  contracts_table_t _contracts( get_self(), get_self().value );
  auto contracts_itr = _contracts.find( contract.value );

  if ( contracts_itr != _contracts.end() )
    _contracts.erase(contracts_itr);

}

ACTION ascendinsur::payout( name contract ) 
{

  // Very important
  require_auth(get_self());

  contracts_table_t _contracts( get_self(), get_self().value );
  auto contracts_itr = _contracts.find( contract.value );

  // Disable future payouts until it gets reset
  _contracts.modify( contracts_itr, get_self(), [&](auto& cntct) {
    cntct.if_enabled = false;
  });
  
  float balance_increment = contracts_itr->trigger_amount_usd;

  subscribers_table_t _subscribers( get_self(), contract.value );
  auto subscriber_itr = _subscribers.begin();

  while (subscriber_itr != _subscribers.end()) {
    // Keep the balance in USD. 
    _subscribers.modify( subscriber_itr, get_self(), [&](auto& subscriber) {
      subscriber.balance = subscriber_itr->balance + balance_increment;
    });
    subscriber_itr++;
  }

}

ACTION ascendinsur::claimbalance( name contract, name subscriber ) 
{

  // Anyone can call this action. But ideally subscriber will with web app
  //require_auth(get_self());

  contracts_table_t _contracts( get_self(), get_self().value );
  auto contracts_itr = _contracts.find( contract.value );

  subscribers_table_t _subscribers( get_self(), contract.value );
  auto subscriber_itr = _subscribers.find(subscriber.value);
  float balance = subscriber_itr->balance;

  uint8_t precision;
  string symbol_letters;

  check ( contracts_itr->token_name == name("eosio.token") ,
          "Contract token unsupported. Can only be eosio.token" );

  // Calculate how many tokens need to be withdrawn
  delphi_table_t _delphi_prices( name("delphioracle"), "tlosusd"_n.value );
  precision = 4;
  symbol_letters = "TLOS";
    
  auto delphi_itr = _delphi_prices.begin(); // gets the latest price
  float usd_price = delphi_itr->value / 10000.0;
  float token_amount  = balance / usd_price;

  // Check for null balance
  if ( token_amount == 0 )
    return;

  uint32_t amt_number = (uint32_t)(pow( 10, precision ) * 
                                        token_amount);
  eosio::asset reward = eosio::asset( 
                          amt_number,
                          symbol(symbol_code( symbol_letters ), precision));

  string memo = "Payout for contract " + contract.to_string() +
                " in the amount of " + to_string(balance) + " USD";

  bool evm_enabled = false;
  if( evm_enabled )
  {
    //payoutEVM( to, amt_number );

  } else {

    // Do token transfer using an inline function
    //   This works even with "iot" or another account's key being passed, because even though we're not passing
    //   the traditional "active" key, the "active" condition is still met with @eosio.code
    action(
      permission_level{ get_self(), "active"_n },
      contracts_itr->token_name , "transfer"_n,
      std::make_tuple( get_self(), subscriber, reward, memo)
    ).send();

  }

  // Set balance to 0
  _subscribers.modify( subscriber_itr, get_self(), [&](auto& subscriber) {
    subscriber.balance = 0.0;
  });

}

ACTION ascendinsur::raincheck1( name station, name contract ) 
{

  // Very important
  require_auth(get_self());

  bool passed_criteria = false;

  contracts_table_t _contracts( get_self(), get_self().value );
  auto contracts_itr = _contracts.find( contract.value );

  if ( contracts_itr->if_enabled == false )
    return;

  float rain_threshold_mm = contracts_itr->threshold_1;

  ascendweathr::raincumulate_table_t _rain( name("ascendweathr"), name("ascendweathr").value );
  auto rain_itr = _rain.find( station.value );

  if ( rain_itr->rain_24hr > rain_threshold_mm && rain_itr->flags == 0 )
  {
    // In the future we can use lon_index of raincumulate to check if other stations near the contract's
    //  location also received some precip in last 24 hours

    // We can also check if soil moisture is very very high (indicates flooding)

    passed_criteria = true;
  }

  // Have the following line at the end of every Checking action
  if ( passed_criteria ) {
    action(
        permission_level{ get_self(), "active"_n },
        get_self(), "payout"_n,
        std::make_tuple( contract )
    ).send();
  }

}

ACTION ascendinsur::heatcheck1( name station, name contract ) 
{

  // Very important
  require_auth(get_self());

  bool passed_criteria = false;

  contracts_table_t _contracts( get_self(), get_self().value );
  auto contracts_itr = _contracts.find( contract.value );

  if ( contracts_itr->if_enabled == false )
    return;

  float temperature_threshold = contracts_itr->threshold_1;

  ascendweathr::weather_table_t _weather( name("ascendweathr"), name("ascendweathr").value );
  auto weather_itr = _weather.find( station.value );

  if ( weather_itr->temperature_c > temperature_threshold && weather_itr->flags == 0 )
  {
    // In the future we can use lon_index of raincumulate to check if other stations near the contract's
    //  location also received some precip in last 24 hours

    // We can also check if soil moisture is very very high (indicates flooding)

    passed_criteria = true;
  }

  // Have the following line at the end of every Checking action
  if ( passed_criteria ) {
    action(
        permission_level{ get_self(), "active"_n },
        get_self(), "payout"_n,
        std::make_tuple( contract )
    ).send();
  }

}

float ascendinsur::calcDistance( float lat1, float lon1, float lat2, float lon2 )
{
  // This function uses the given lat/lon of the devices to deterimine
  //    the distance between them.
  // The calculation is made using the Haversine method and utilizes Math.h

  float R = 6372800; // Earth radius in meters

  float phi1 = degToRadians(lat1);
  float phi2 = degToRadians(lat2);
  float dphi = degToRadians( lat2 - lat1 );
  float dlambda = degToRadians( lon2 - lon1 );

  float a = pow(sin(dphi/2), 2) + pow( cos(phi1)*cos(phi2)*sin(dlambda/2.0) , 2);

  float distance_meters = 2*R*atan2( sqrt(a) , sqrt(1-a) );

  // return distance in km
  return distance_meters/1000.0;

}

float ascendinsur::degToRadians( float degrees )
{
  // M_PI comes from math.h
    return (degrees * M_PI) / 180.0;
}

float ascendinsur::calcDewPoint( float temperature_c, float humidity ) 
{
  
  const float c1 = 243.04;
  const float c2 = 17.625;
  float h = humidity / 100;
  if (h <= 0.01)
    h = 0.01;
  else if (h > 1.0)
    h = 1.0;

  const float lnh = log(h); // natural logarithm
  const float tpc1 = temperature_c + c1;
  const float txc2 = temperature_c * c2;
  
  float txc2_tpc1 = txc2 / tpc1;

  float tdew = c1 * (lnh + txc2_tpc1) / (c2 - lnh - txc2_tpc1);

  return tdew;
}

EOSIO_DISPATCH(ascendinsur, (stationdefi)\
                            (mapstation)(mapbylatlon)(addsubscribe)(newcontract)\
                            (unmapstation)(rmcontract)\
                            (payout)(claimbalance)\
                            (raincheck1)(heatcheck1))

