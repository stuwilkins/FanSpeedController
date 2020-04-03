Import("env")

#print(env.Dump())

def cpplint_callback(*args, **kwargs):
    env.Execute(
        "cpplint --filter=-build/include_subdir,-runtime/int --recursive src/* include/*")

env.AddPreAction("bluetooth.cpp.o", cpplint_callback)
