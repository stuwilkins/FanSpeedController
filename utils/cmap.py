from matplotlib import cm

def main():
    print("static uint32_t colormap[] = {")
    for i in range(256):
        l = i / 255
        c = cm.hot(l, 1, True)
        print("  0x{0:02x}{1:02x}{2:02x}".format(c[0], c[1], c[2]), end='')
        if(i != 255):
            print(",")
    print("\n};")


if __name__ == "__main__":
    main()