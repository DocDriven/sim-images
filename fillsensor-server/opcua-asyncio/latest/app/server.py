import argparse
import asyncio
import logging

from asyncua import Server, ua, uamethod
# from asyncua.common.callback import CallbackType
from asyncua.common.xmlimporter import XmlImporter

logging.basicConfig(level=logging.INFO)
logger = logging.getLogger(__name__)


async def main(endpoint_uri: str):
    while True:
        try:
            server = Server()
            await server.init()
            # server.set_endpoint(endpoint_uri)
            # server.set_server_name("'watersensor-server")
            # await server.set_application_uri("urn:watersensor-server.local")
            # server.application_type = ua.ApplicationType.Server
            server.set_security_policy(
            [
                ua.SecurityPolicyType.NoSecurity,
                ua.SecurityPolicyType.Basic256Sha256_SignAndEncrypt,
                ua.SecurityPolicyType.Basic256Sha256_Sign
            ]
            )
            # await server.load_certificate('/pki/cert.der')
            # await server.load_private_key('/pki/key.pem')

            """
            Prepare the server by defining object types and initializing the instance
            """
            await server.import_xml("/app/WS.NodeSet2.xml")
            # tankSystemType = await server.nodes.base_object_type.add_object_type(1, "tankSystemType")
            # fillPercentageAttr = await tankSystemType.add_variable(1, "FillPercentage", 0., varianttype=ua.VariantType.Double)
            # await fillPercentageAttr.set_modelling_rule(True) # make attribute mandatory
            # valvePosAttr = await tankSystemType.add_variable(1, "ValvePosition", False, varianttype=ua.VariantType.Boolean)
            # await valvePosAttr.set_modelling_rule(True) # make attribute mandatory
            # thresholdAttr = await tankSystemType.add_variable(1, "Threshold", 0, varianttype=ua.VariantType.Int32)
            # await thresholdAttr.set_modelling_rule(True) # make attribute mandatory

            # tankSystem1 = await server.nodes.objects.add_object(1, "tankSystem1", tankSystemType.nodeid, True)

            """
            Add methods to server.
            Methods are placed here as the database and the server instances are made available to
            the callback functions via closures.
            """
            
            # @uamethod
            # async def setThresholdMethod(parent, newThreshold):
            #     parentNode = server.get_node(parent)
            #     thresholdNode = await parentNode.get_child("1:Threshold")

            #     """
            #     Prepare statements and execute for Threshold
            #     """
            #     try:
            #         await db.execute('INSERT INTO triggerthreshold (threshold) VALUES (?);',
            #                             (newThreshold,))
            #         await db.commit()
            #     except Exception as e:
            #         logger.error(f'Could not write threshold to database with error: {e}')
            #         return ua.UaStatusCodeError(ua.StatusCodes.BadInternalError)

            #     """
            #     Write the new value to the server
            #     """
            #     await thresholdNode.write_value(newThreshold, ua.VariantType.Int32)

            #     return ua.StatusCodes.Good

            # systemParamsOutput = ua.Argument()
            # systemParamsOutput.Name = "tankSystemParams"
            # systemParamsOutput.DataType = ua.NodeId(ua.ObjectIds.BaseDataType)
            # systemParamsOutput.ValueRank = ua.ValueRank.OneDimension
            # systemParamsOutput.ArrayDimensions = [3]
            # systemParamsOutput.Description = ua.LocalizedText("[FillPercentage: double, ValvePosition: bool, Threshold: int]")
            # systemParamsMethod = await tankSystem1.add_method(1, "getTankSystemParams", getTankSystemParamsMethod, [], [systemParamsOutput])
            # await systemParamsMethod.set_modelling_rule(True)

            # thresholdInput = ua.Argument()
            # thresholdInput.Name = "newThreshold"
            # thresholdInput.DataType = ua.NodeId(ua.ObjectIds.Int32)
            # thresholdInput.ValueRank = ua.ValueRank.Scalar
            # thresholdInput.ArrayDimensions = []
            # thresholdInput.Description = ua.LocalizedText("[NewThreshold: int]")
            # thresholdMethod = await tankSystem1.add_method(1, "setThreshold", setThresholdMethod, [thresholdInput], [])
            # await thresholdMethod.set_modelling_rule(True)

            try:
                async with server:
                    while True:
                        await asyncio.sleep(1)
            except Exception as e:
                logger.error(f'OPC UA server crashed: {e}')
                raise
        except Exception as e:
            logger.warning(f'Server crashed: {e} Restarting...')
            await asyncio.sleep(2)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='OPC UA water sensor relaying values from our internal sim')
    parser.add_argument('-u', '--uri', type=str,
                        default='opc.tcp://0.0.0.0:4840',
                        help='OPC UA server endpoint URI [default: opc.tcp://0.0.0.0:4840]')
    args = parser.parse_args()

    asyncio.run(main(args.uri))