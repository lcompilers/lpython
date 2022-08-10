for i in range(3, 0, -1):
    try:
        print()
    except IOError:
        if i == 1:
            raise
        print('retry')
    else:
        break

try:
   pass
except ValueError:
   pass
except TypeError:
   pass
except ZeroDivisionError:
    pass
except:
   pass

try:
    a = 1
    if a <= 0:
        raise ValueError("That is not a positive number!")
except ValueError as ve:
    print(ve)

try:
    raise KeyboardInterrupt
finally:
    print('Goodbye, world!')

try: ...
except Exception as e: ...

try: ...
except: ...
else: ...
finally: ...

try: shutil.rmtree(os.path.join(asv_build_cache, c))
except OSError: pass
