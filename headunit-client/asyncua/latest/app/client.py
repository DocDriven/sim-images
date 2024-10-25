import aiosqlite
import argparse
import asyncio
import logging

from asyncua import Client, ua
from asyncua.crypto.security_policies import SecurityPolicyBasic256Sha256, MessageSecurityMode
from contextlib import AsyncExitStack

logging.basicConfig(level=logging.INFO)
logger = logging.getLogger(__name__)


async def main(opc_server_uri: str, database: str, interval: int):
    while True:
        client = Client(opc_server_uri)
        await client.set_security(SecurityPolicyBasic256Sha256,
                                  certificate='/pki/cert.der',
                                  private_key='/pki/key.pem',
                                  mode=MessageSecurityMode.SignAndEncrypt)
        try:
            async with AsyncExitStack() as stack:
                await stack.enter_async_context(client)
                db = await stack.enter_async_context(aiosqlite.connect(database))

                while True:
                    try:
                        tanksystemobj = await client.nodes.objects.get_child(['1:tankSystem1'])
                        qn = ua.QualifiedName('getTankSystemParams', 1)
                        resp = await tanksystemobj.call_method(qn)
                        try:
                            await db.execute('INSERT INTO tanksystemdata (level, position, threshold) VALUES (?, ?, ?);',
                                             (resp[0], resp[1], resp[2]))
                            await db.commit()
                        except aiosqlite.OperationalError as db_err:
                            logger.error(f'Database error: {db_err}')
                    except Exception as e:
                        logger.error(f'Failed to invoke getTankSystemParams method with error: {e}')
                    await asyncio.sleep(interval)

        except Exception:
            logger.warning('Connection lost. Reconnecting...')
            await asyncio.sleep(2)

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='Retrieve data from OPC UA server with encryption.')
    parser.add_argument('-u', '--uri', type=str,
                        default='opc.tcp://127.0.0.1:4840',
                        help='OPC UA server URI [default: opc.tcp://127.0.0.1:4840]')
    parser.add_argument('-d', '--database', type=str,
                        default='/db.sqlite3',
                        help='SQLite database [default: /db.sqlite3]')
    parser.add_argument('-i', '--interval', type=int,
                        default=5,
                        help='Interval between server requests [default: 5]')
    args = parser.parse_args()

    asyncio.run(main(args.uri, args.database, args.interval))

