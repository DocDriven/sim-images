from asyncua import Client
import asyncio
import time
from waterworks_components import tank, pump, well

time.sleep(5)  # Wait for the server to start

sim_step = .1

tank1 = tank(name= 'tank1', 
                   volume=1000, 
                   height=5000, 
                   max_fill_level=4500, 
                   min_fill_level=600, 
                   fill_level=3900, 
                   url='opc.tcp://fillsensor-server:4840',
                   sim_step=sim_step)

static_outflow = 40

pump = pump(name= 'pump_tank1',
              url='opc.tcp://fillsensor-server:4840',
              nominal_flow_rate= 60,
              flow_destination= tank1,
              sim_step=sim_step)

async def update_water_tank(client, fill_percentage):
    nsidx = 1 # Namespace index
    # Get the variable node for read / write
    var = await client.nodes.root.get_child(
        f"0:Objects/{nsidx}:tank1/{nsidx}:FillPercentage"
    )
    #write value
    await var.write_value(fill_percentage)
    #then read value again for verification  
    value = await var.read_value()
    print(f"new Value of fill_percentage: {fill_percentage}")
    print(f"new Value of MyVariable ({var}): {value}")

rising = True

async def main():
    async with Client(tank1.url) as client:
        while True:
            # Start timer for simulation step
            waiting4looptime = asyncio.create_task(asyncio.sleep(sim_step))

            if tank1.fill_level >= tank1.max_fill and pump.pump_status == True:
                pump.pump_status = False

            if tank1.fill_level <= tank1.min_fill and pump.pump_status == False:
                pump.pump_status = True

            meassured_flow = pump.get_flow_PT2()
            tank1.calculate_new_fill_level([meassured_flow], [static_outflow])

            await update_water_tank(client, tank1.fill_percentage)

            # Wait for simulation step to end
            await waiting4looptime

asyncio.run(main())
