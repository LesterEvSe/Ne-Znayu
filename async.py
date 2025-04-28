import time
import asyncio

async def fact(num):
    res = 1
    while num > 2:
        res *= num
        num -= 1
    return res

#print((await smth))

async def say_hello():
    for i in range(3):
        print(f"[{time.strftime('%X')}] 👋 Привіт ({i + 1})")
        await asyncio.sleep(0.5)

async def count_numbers():
    for i in range(5):
        print(f"[{time.strftime('%X')}] 🔢 Число: {i}")
        await asyncio.sleep(0.5)

async def main():
    print(f"[{time.strftime('%X')}] 🚀 Старт")

    task1 = asyncio.create_task(say_hello())
    task2 = asyncio.create_task(count_numbers())

    await task1
    await task2

    print(f"[{time.strftime('%X')}] 🏁 Готово")

asyncio.run(main())
