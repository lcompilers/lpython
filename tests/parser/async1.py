async def func(param1, param2):
    await asyncio.sleep(1)
    do_something()

    async for x in y:
        do_something(x)

    async with open('examples/expr2.py', 'r') as file:
        x = file.read()
