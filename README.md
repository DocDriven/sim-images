# sim-images

Contains a collection of OPC UA application implementations.

## TODOs and/or tbds:

- Fix node permissions and retest writing nodes
- PLC nodeset is lacking nodes for actuator (valve server) and upper/lower limits
- also, PLC nodeset indicator vs tank needs to be cleared up (what does what?)
- Writing only percentage values or absolute or both?
- Removing MyObject/MyVariable default node?
- Hardcode namespace/node pairs instead of requesting them at each upstart? Has to be tested
- add pki part to headunit


## Structure of the NodeSets

Devices directory does always exist, devices are added as needed.

```bash
Objects
|-- Devices (Folder)
|   |-- Tanks (Folder)
|   |   |-- T001
|   |   |-- T002
|   |-- Valves (Folder)
|   |   |-- V001
|   |   |-- V002
|   |-- [Other Device Types] (Folder)
Types
Views
```

### Structure for Tank

```bash
TXXX
|-- Info
|   |-- DeviceID
|   |-- Location
|   |-- Manufacturer
|   |-- Model
|-- Diagnostics
|   |-- Fault
|   |-- StatusOK
|-- Specifications
|   |-- Capacity
|-- Configuration
|   |-- MaxLevelPercent
|   |-- MinLevelPercent
|-- Measurement
|   |-- FillLevel (References to sensor value)
|   |   |-- Liter
|   |   |-- Percent
```

### Structure for Sensor (FillSensor in this case)

Omitted IO section as purpose is unclear for sim.
Tank measurements reference the corresponding sensors measurements of this class.

```bash
SXXX
|-- Info
|   |-- DeviceID
|   |-- Location
|   |-- Manufacturer
|   |-- Model
|-- Diagnostics
|   |-- Fault
|   |-- StatusOK
|-- Specifications (can be left empty, currently no runtime variables used)
|-- Alarms (can be used as is)
|-- Measurement
|   |-- FillLevel
|   |   |-- Liter
|   |   |-- Percent
```

### Structure for Valve

Omitted IO section as purpose is unclear for sim.

```bash
VXXX
|-- Info
|   |-- DeviceID
|   |-- Location
|   |-- Manufacturer
|   |-- Model
|-- Diagnostics
|   |-- Fault
|   |-- StatusOK
|-- Specifications (can be left empty, currently no runtime variables used)
|-- Alarms (can be used as is)
|-- Output (Could be analog/digital outputs numerated by 1 to 4)
|   |-- D1
|   |   |-- Active (Bool)
```

### Structure for PLC

Duplicate classes from before on the server, and just use PLC as an aggregate class.

```bash
PLCXXX
|-- Info
|   |-- DeviceID
|   |-- Location
|   |-- Manufacturer
|   |-- Model
|-- Diagnostics
|   |-- Fault
|   |-- StatusOK
|-- ManagedNodes
|   |-- Tanks
|   |   |-- TXXX (Reference to local copy)
|   |-- Sensors (can be omitted here as Tank abstracts it)
|   |   |-- SXXX (Reference to local copy)
|   |-- Valves
|   |   |-- VXXX (Reference to local copy)
```
