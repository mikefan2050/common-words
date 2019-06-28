import os
import random


def main():

    file_sz = 400000000

    wlist = ["make", "the", "edge", "you", "speak", "phone",
             "guest", "solution", "home", "type"]

    try:
        with open(os.getcwd()+"/sample1.txt", "w+") as f:
            for i in range(file_sz):
                f.write(wlist[random.randint(0, 9)] + " ")
                if i > 0 and i % 25 == 0:
                    f.write("\n")
            f.close()
    except IOError as e:
        print("open error (%s)" % e)

    return 0


if __name__ == "__main__":
    main()
