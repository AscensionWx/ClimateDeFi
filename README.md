# ClimateDeFi
A "Climate Insurance" smart contract utilizing AscensionWx weather stations

This antelope contract uses live weather and Quality data from all AscensionWx weather stations as inputs to determine if severe weather has occured within the contract's domain. When extreme weather is reported by the weather station (ie. hourly rainfall above a threshold), the smart contract automatically disperses a payment to all subscribers.

Supports:
- Live weather data
- Multiple domains
- Multiple subscribers
- Ability to add new "checking actions" for criteria such as heat waves (temperature), hurricanes (atmospheric pressure, wind, rain), etc.
- EVM-compatibile (In Progress)

The contract is deployed live on the Telos blockchain as "ascendinsur1" and can be explored here: https://explorer.telos.net/account/ascendinsur1
