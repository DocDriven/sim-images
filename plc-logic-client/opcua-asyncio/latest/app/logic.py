import argparse
import asyncio
import logging

from collections import namedtuple
from asyncua import Client
from asyncua.common.subscription import SubscriptionHandler

logging.basicConfig(level=logging.INFO)
logger = logging.getLogger(__name__)

# Collection of node IDs needed inside callback function
CallbackNodes = namedtuple('CallbackNodes',
                          ['vs_valve_pos',
                           'ps_upper_limit',
                           'ps_lower_limit',
                           'ps_fill_level',
                           'ps_valve_pos'])

# node paths on lss
LSS_FILL_LEVEL_PATH = f"2:TankV001/2:Measurement/2:FillLevel/2:Percent"

# node paths on vs
VS_VALVE_POS_PATH = f"2:ActuatorF001/2:Valve/2:Output"

# node paths on ps
PS_UPPER_LIMIT_PATH = f""
PS_LOWER_LIMIT_PATH = f""
PS_FILL_LEVEL_PATH = f"2:TankV001/2:Measurement/2:FillLevel/2:Percent"
PS_VALVE_POS_PATH = f""


# called when new value is written to levelsensor server
# also the logic resides here and pushes the results to
# the queues
class SubscriptionHandler:
    def __init__(self,
                 vs_client,
                 ps_client,
                 nodes):
        self.vs_client = vs_client
        self.ps_client = ps_client
        self.nodes = nodes

    async def datachange_notification(self, node, val, data):
        logger.info(f"VALUE CHANGE: {val}")
        # first update the fill level variable on the PLC server
        # await self.nodes.ps_fill_level.write_value(val)

        # get latest values needed for calculations from PLC and valve servers
        # upper_limit, lower_limit = await self.ps_client.read_values([
            # self.nodes.ps_upper_limit,
            # self.nodes.ps_lower_limit])
        valve_pos = await self.nodes.vs_valve_pos.read_value() # 1: open 0: closed
        logger.info(f"READ VALVE POS: {valve_pos}")
        # TODO: Do calculation here, maybe adjust
        # currently open valve if it is closed and enough water is in the tank
        # if not valve_pos and val >= upper_limit:
        #     await self.nodes.ps_valve_pos.write(1) # Maybe change to boolean
        #     await self.nodes.vs_valve_pos.write(1) # same here
        # # and close valve if it is open and not enough water is left in the tank
        # elif valve_pos and val <= lower_limit:
        #     await self.nodes.ps_valve_pos.write(0) # Maybe change to boolean
        #     await self.nodes.vs_valve_pos.write(0) # same here
        #
# task which subscribes to levelsensor server
async def main(lss_uri: str, vs_uri: str, ps_uri: str):
    while True:
        try:
            logger.info(f"Trying to connect to {lss_uri}, {vs_uri}, and {ps_uri}")
            async with Client(url=lss_uri) as lss_client, \
                       Client(url=vs_uri) as vs_client, \
                       Client(url=ps_uri) as ps_client:

                # get nodes from levelsensor server
                lss_fill_level = await lss_client.nodes.objects.get_child(
                    LSS_FILL_LEVEL_PATH)

                # get nodes from valve server
                vs_valve_pos = await vs_client.nodes.objects.get_child(
                    VS_VALVE_POS_PATH)

                # get nodes from PLC server
                ps_upper_limit = await ps_client.nodes.objects.get_child(
                    PS_UPPER_LIMIT_PATH)
                ps_lower_limit = await ps_client.nodes.objects.get_child(
                    PS_LOWER_LIMIT_PATH)
                ps_fill_level = await ps_client.nodes.objects.get_child(
                    PS_FILL_LEVEL_PATH)
                ps_valve_pos = await ps_client.nodes.objects.get_child(
                    PS_VALVE_POS_PATH)

                # create subscription
                handler = SubscriptionHandler(
                    vs_client,
                    ps_client,
                    CallbackNodes(vs_valve_pos, ps_upper_limit,
                                  ps_lower_limit, ps_fill_level,
                                  ps_valve_pos))
                subscription = await lss_client.create_subscription(1000, handler)
                await subscription.subscribe_data_change(lss_fill_level)

                logger.info(f"SUBSCRIPTION ADDED, WAITING...")

                # wait for value to change
                while True:
                    await asyncio.sleep(1)

        except Exception as e:
            logger.warning(f"[WARN] Connection failed: {e}. Retrying in 2 seconds...")
            await asyncio.sleep(2)



if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='OPC UA historian client')
    parser.add_argument('-l', '--lss', type=str,
                        default='opc.tcp://127.0.0.1:4840',
                        help='OPC UA level sensor server endpoint URI [default: opc.tcp://127.0.0.1:4840]')
    parser.add_argument('-v', '--vs', type=str,
                        default='opc.tcp://127.0.0.1:4840',
                        help='OPC UA valve server endpoint URI [default: opc.tcp://127.0.0.1:4840]')
    parser.add_argument('-p', '--ps', type=str,
                        default='opc.tcp://127.0.0.1:4840',
                        help='OPC UA PLC server endpoint URI [default: opc.tcp://127.0.0.1:4840]')
    args = parser.parse_args()

    asyncio.run(main(args.lss, args.vs, args.ps))
