import argparse
import asyncio
import aiosqlite
import logging

from asyncua import Server, ua, uamethod
from asyncua.common.callback import CallbackType

logging.basicConfig(level=logging.INFO)
logger = logging.getLogger(__name__)


async def main(endpoint_uri: str, database: str):
    while True:
        try:
            async with aiosqlite.connect(database) as db:
                server = Server()
                await server.init()
                server.set_endpoint(endpoint_uri)
                server.set_server_name("opcua-server")
                await server.set_application_uri("urn:opcua-server.local")
                server.application_type = ua.ApplicationType.Server
                server.set_security_policy(
                [
                    # ua.SecurityPolicyType.NoSecurity,
                    ua.SecurityPolicyType.Basic256Sha256_SignAndEncrypt,
                    ua.SecurityPolicyType.Basic256Sha256_Sign
                ]
                )
                await server.load_certificate('/pki/cert.der')
                await server.load_private_key('/pki/key.pem')

                """
                Prepare the server by defining object types and initializing the instance
                """
                tankSystemType = await server.nodes.base_object_type.add_object_type(1, "tankSystemType")
                fillPercentageAttr = await tankSystemType.add_variable(1, "FillPercentage", 0., varianttype=ua.VariantType.Double)
                await fillPercentageAttr.set_modelling_rule(True) # make attribute mandatory
                valvePosAttr = await tankSystemType.add_variable(1, "ValvePosition", False, varianttype=ua.VariantType.Boolean)
                await valvePosAttr.set_modelling_rule(True) # make attribute mandatory
                thresholdAttr = await tankSystemType.add_variable(1, "Threshold", 0, varianttype=ua.VariantType.Int32)
                await thresholdAttr.set_modelling_rule(True) # make attribute mandatory

                tankSystem1 = await server.nodes.objects.add_object(1, "tankSystem1", tankSystemType.nodeid, True)

                """
                Add methods to server.
                Methods are placed here as the database and the server instances are made available to
                the callback functions via closures.
                """
                @uamethod
                async def getTankSystemParamsMethod(parent):
                    parentNode = server.get_node(parent)
                    fillPctNode = await parentNode.get_child("1:FillPercentage")
                    valvePosNode = await parentNode.get_child("1:ValvePosition")
                    thresholdNode = await parentNode.get_child("1:Threshold")

                    fillPct = 0.
                    valvePos = False
                    threshold = 0

                    """
                    Prepare statements and execute for FillPercentage
                    """
                    try:
                        fillPctCursor = await db.execute('SELECT level FROM waterlevel ORDER BY id DESC LIMIT 1;')
                        fillPct = await fillPctCursor.fetchone()
                        await fillPctCursor.close()
                    except Exception as e:
                        logger.error(f'Failed to prepare SQL statement for fill percentage with error: {e}')
                        return ua.UaStatusCodeError(ua.StatusCodes.BadInternalError)

                    """
                    Prepare statements and execute for ValvePosition
                    """
                    try:
                        valvePosCursor = await db.execute('SELECT position FROM valveposition ORDER BY id DESC LIMIT 1;')
                        valvePos = await valvePosCursor.fetchone()
                        await valvePosCursor.close()
                    except Exception as e:
                        logger.error(f'Failed to prepare SQL statement for valve position with error: {e}')
                        return ua.UaStatusCodeError(ua.StatusCodes.BadInternalError)

                    """
                    Prepare statements and execute for Threshold
                    """
                    try:
                        thresholdCursor = await db.execute('SELECT threshold FROM triggerthreshold ORDER BY id DESC LIMIT 1;')
                        threshold = await thresholdCursor.fetchone()
                        await thresholdCursor.close()
                    except Exception as e:
                        logger.error(f'Failed to prepare SQL statement for threshold with error: {e}')
                        return ua.UaStatusCodeError(ua.StatusCodes.BadInternalError)

                    """
                    Write the retrieved values to the server
                    """
                    if not (fillPct and valvePos and threshold):
                        logger.error(f'No data found in database')
                        return ua.UaStatusCodeError(ua.StatusCodes.BadOutOfRange)

                    """
                    Return the retrieved values to client
                    """
                    await fillPctNode.write_value(fillPct, ua.VariantType.Double)
                    await valvePosNode.write_value(valvePos, ua.VariantType.Boolean)
                    await thresholdNode.write_value(threshold, ua.VariantType.Int32)
                    return [fillPct, valvePos, threshold]

                @uamethod
                async def setThresholdMethod(parent, newThreshold):
                    parentNode = server.get_node(parent)
                    thresholdNode = await parentNode.get_child("1:Threshold")

                    """
                    Prepare statements and execute for Threshold
                    """
                    try:
                        await db.execute('INSERT INTO triggerthreshold (threshold) VALUES (?);',
                                         (newThreshold,))
                        await db.commit()
                    except Exception as e:
                        logger.error(f'Could not write threshold to database with error: {e}')
                        return ua.UaStatusCodeError(ua.StatusCodes.BadInternalError)

                    """
                    Write the new value to the server
                    """
                    await thresholdNode.write_value(newThreshold, ua.VariantType.Int32)

                    return ua.StatusCodes.Good

                systemParamsOutput = ua.Argument()
                systemParamsOutput.Name = "tankSystemParams"
                systemParamsOutput.DataType = ua.NodeId(ua.ObjectIds.BaseDataType)
                systemParamsOutput.ValueRank = ua.ValueRank.OneDimension
                systemParamsOutput.ArrayDimensions = [3]
                systemParamsOutput.Description = ua.LocalizedText("[FillPercentage: double, ValvePosition: bool, Threshold: int]")
                systemParamsMethod = await tankSystem1.add_method(1, "getTankSystemParams", getTankSystemParamsMethod, [], [systemParamsOutput])
                await systemParamsMethod.set_modelling_rule(True)

                thresholdInput = ua.Argument()
                thresholdInput.Name = "newThreshold"
                thresholdInput.DataType = ua.NodeId(ua.ObjectIds.Int32)
                thresholdInput.ValueRank = ua.ValueRank.Scalar
                thresholdInput.ArrayDimensions = []
                thresholdInput.Description = ua.LocalizedText("[NewThreshold: int]")
                thresholdMethod = await tankSystem1.add_method(1, "setThreshold", setThresholdMethod, [thresholdInput], [])
                await thresholdMethod.set_modelling_rule(True)

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
    parser = argparse.ArgumentParser(description='OPC UA server with encryption serving from a database.')
    parser.add_argument('-u', '--uri', type=str,
                        default='opc.tcp://0.0.0.0:4840',
                        help='OPC UA server endpoint URI [default: opc.tcp://0.0.0.0:4840]')
    parser.add_argument('-d', '--database', type=str,
                        default='/db.sqlite3',
                        help='SQLite database [default: /db.sqlite3]')
    args = parser.parse_args()

    asyncio.run(main(args.uri, args.database))

