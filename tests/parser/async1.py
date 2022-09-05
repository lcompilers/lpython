async def func(param1, param2):
    await asyncio.sleep(1)
    do_something()

    async for x in y:
        do_something(x)

    async with open('examples/expr2.py', 'r') as file:
        x = file.read()

async def test_01():
    results = await tasks.gather(
        *[ag.aclose() for ag in closing_agens],
        return_exceptions=True)
    with await lock: pass
    data += await self.loop.sock_recv(sock, DATA_SIZE)
    return await self.run_in_executor(
            None, getaddr_func, host, port, family, type, proto, flags)

async def test_02():
    items = [await q.get() for _ in range(3)]
    {await c for c in [f(1), f(41)]}
    {i: await c for i, c in enumerate([f(1), f(41)])}
    [s for c in [f(''), f('abc'), f(''), f(['de', 'fg'])]
                            for s in await c]
    return (i * 2 for i in range(n) if await wrap(i))

async def t():
    results.append(await anext(g))
    self.assertIn('...', repr(await asyncio.wait_for(func(), timeout=10)))
    x = -await bar()
    return (await bar() + await wrap()() + await db['b']()()() +
                        await bar() * 1000 + await DB.b()())
